/*
 * http.c
 *
 *  Created on: 2017/03/04
 *      Author: xq
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#include "modem.h"
#include "http.h"
#include "record.h"
#include "log.h"
#include "thread_msg.h"
#include "thread.h"
#include "setting.h"
#include "fs.h"

#define MAX_LOCALFILENAME_LEN 32
#define MAX_HTTPCMD_LEN 256
#define MAX_DATA_LEN 1024

enum{
    HTTP_UPLOAD,
    HTTP_DOWNLOAD,
}HTTPTYPE;

typedef void(*HTTP_PROC)(u8 *, u16 len);
typedef struct
{
	char cmd[16];
	HTTP_PROC pfn;
}MC_HTTP_PROC;

static eat_bool isGPRSOpening = EAT_FALSE;
static int http_type = HTTP_UPLOAD;
static eat_bool flag_wait_download = EAT_FALSE;
static unsigned char http_localFileName[MAX_LOCALFILENAME_LEN] = {0};
static unsigned char http_serverFileName[MAX_LOCALFILENAME_LEN] = {0};

static void http_openGPRS(void)
{
    isGPRSOpening = EAT_TRUE;
    modem_AT("AT+SAPBR=1,1" CR);
}

static void http_closeGPRS(u8 * buf, u16 len)
{
    modem_AT("AT+SAPBR=0,1" CR);
}

static void http_end(void)
{
    modem_AT("AT+HTTPTERM" CR);
}

static unsigned int http_get_DataLen(void)
{
    int rc = 0;
    UINT filesize = 0;
    FS_HANDLE fh = NULL;

    fh = eat_fs_Open(RECORDE_FILE_NAME, FS_READ_ONLY);
    if(EAT_FS_FILE_NOT_FOUND == fh || fh < EAT_FS_NO_ERROR)
    {
        LOG_ERROR("read record.amr file fail, rc: %d", fh);
        return EAT_FALSE;
    }
    rc = eat_fs_GetFileSize(fh, &filesize);
    if(EAT_FS_NO_ERROR != rc)
    {
        LOG_ERROR("get file size error, return %d",rc);
        eat_fs_Close(fh);
        return EAT_FALSE;
    }
    eat_fs_Close(fh);

    return filesize;
}

void http_upload_file(const unsigned char *localFileName)
{
    memset(http_localFileName, 0, MAX_LOCALFILENAME_LEN);
    strncpy(http_localFileName, localFileName, MAX_LOCALFILENAME_LEN);
    http_type = HTTP_UPLOAD;

    http_openGPRS();
}

void http_download_file(const unsigned char *localFileName, unsigned char * serverFileName)
{
    strncpy(http_localFileName, localFileName, MAX_LOCALFILENAME_LEN);
    strncpy(http_serverFileName, serverFileName, MAX_LOCALFILENAME_LEN);
    http_type = HTTP_DOWNLOAD;
    http_openGPRS();
}

static void http_GPRScheck(u8 * buf, u16 len)
{
    if((strstr((const char *)buf, "OK") && isGPRSOpening) || (strstr((const char *)buf, "ERROR") && strstr((const char *)buf, "AT+SAPBR=1,1")))
    {
        isGPRSOpening = EAT_FALSE;
        modem_AT("AT+HTTPINIT" CR);
    }
    else if(flag_wait_download == EAT_TRUE)
    {
        modem_AT("AT+HTTPACTION=1" CR);
        flag_wait_download = EAT_FALSE;
    }
}

static void http_set_server_1(u8 * buf, u16 len)
{
    if((strstr((const char *)buf, "OK")) || ((strstr((const char *)buf, "ERROR") && strstr((const char *)buf, "AT+HTTPINIT"))))
    {
        modem_AT("AT+HTTPPARA=\"CID\",1" CR);
    }
}

static void http_set_server_2(u8 * buf, u16 len)
{
    char cmd[MAX_HTTPCMD_LEN] = {0};
    unsigned int data_len = 0;

    if(strstr((const char *)buf, "OK") && strstr((const char *)buf, "CID"))
    {
        if(http_type == HTTP_UPLOAD)
            snprintf(cmd, MAX_HTTPCMD_LEN, "AT+HTTPPARA=\"URL\",\"http://test.xiaoan110.com:8083/v1/uploadFile\"" CR);
        else
            snprintf(cmd, MAX_HTTPCMD_LEN, "AT+HTTPPARA=\"URL\",\"http://test.xiaoan110.com:8083/v1/uploadFile/%s\"" CR, http_serverFileName);
        modem_AT((u8 *)cmd);
    }
    else if(strstr((const char *)buf, "OK") && strstr((const char *)buf, "URL"))
    {
        snprintf(cmd, MAX_HTTPCMD_LEN, "AT+HTTPPARA=\"CONTENT\",\"multipart/form-data\"" CR);
        modem_AT((u8 *)cmd);
    }
    else if(strstr((const char *)buf, "OK") || strstr((const char *)buf, "CONTENT"))
    {
        if(http_type == HTTP_DOWNLOAD)
        {
            modem_AT("AT+HTTPACTION=0" CR);
        }
        else if(http_type == HTTP_UPLOAD)
        {
            data_len = http_get_DataLen();
            if(data_len == EAT_FALSE)
            {
                LOG_DEBUG("get file size error");
                return ;
            }
            snprintf(cmd, MAX_HTTPCMD_LEN, "AT+HTTPDATA=%d,100000" CR, data_len + MAX_LOCALFILENAME_LEN);
            modem_AT((u8 *)cmd);
        }
    }
    else
    {
        http_end();
    }
}

static void http_post_start(u8 * buf, u16 len)
{
    FS_HANDLE fh;
    int rc = 0;
    char data[MAX_DATA_LEN] = {0};
    UINT readLen = 0;
    UINT filesize = 0;
    int file_offset = 0;
    int write_len = 0;
    if(strstr((const char *)buf, "DOWNLOAD"))     //until all the data post
    {
        eat_modem_write(http_localFileName, MAX_LOCALFILENAME_LEN);

        fh = eat_fs_Open(RECORDE_FILE_NAME, FS_READ_ONLY);
        if(EAT_FS_FILE_NOT_FOUND == fh)
        {
            LOG_DEBUG("file not exists.");
            file_offset = 0;
            return ;
        }
        if (fh < EAT_FS_NO_ERROR)
        {
            LOG_DEBUG("open file failed, eat_fs_Open return %d!", fh);
            file_offset = 0;
            return ;
        }

        rc = eat_fs_GetFileSize(fh, &filesize);
        if (rc < EAT_FS_NO_ERROR)
        {
            LOG_DEBUG("seek file pointer failed:%d", rc);
            eat_fs_Close(fh);
            return ;
        }

        while(1)
        {
            rc = eat_fs_Seek(fh, file_offset, EAT_FS_FILE_BEGIN);
            if (rc < EAT_FS_NO_ERROR)
            {
                LOG_DEBUG("seek file pointer failed:%d", rc);
                eat_fs_Close(fh);
                return ;
            }

            rc = eat_fs_Read(fh, data, MAX_DATA_LEN, &readLen);
            if (rc < EAT_FS_NO_ERROR)
            {
                LOG_DEBUG("read file failed:%d", rc);
                eat_fs_Close(fh);
                return ;
            }

            write_len = eat_modem_write((const unsigned char *)data, readLen);
            file_offset += write_len;
            if(file_offset >= filesize)
                break;
        }

        eat_fs_Close(fh);

        LOG_DEBUG("write data ok is poat_len = %d", file_offset + MAX_LOCALFILENAME_LEN);
        flag_wait_download = EAT_TRUE;
    }
    else
    {
        LOG_DEBUG("data not write all");
        http_end();
    }
}

static void http_judge_sucess(u8 * buf, u16 len)
{
    int return_code = 0;

    sscanf(buf, "%*[^,],%d", &return_code);
    if(return_code != 200)
    {
        LOG_DEBUG("http error %d", return_code);
        http_end();
        return ;
    }

    if(http_type == HTTP_DOWNLOAD)
    {
        modem_AT("AT+HTTPREAD" CR);
    }
    else if(http_type == HTTP_UPLOAD)
    {
        modem_AT("AT+HTTPREAD" CR);
    }
}

static void http_save_data(u8 * buf, u16 len)
{
    u16 data_len = 0;
    UINT write_len = 0;
    UINT write_whole_len = 0;
	FS_HANDLE fh, rc;
    WCHAR filename_w[MAX_LOCALFILENAME_LEN];
	u8 data[MAX_DATA_LEN] = {0};
    u8 *pcpos = NULL;

	if(strstr((const char *)buf, "+HTTPREAD:") && http_type == HTTP_DOWNLOAD)
	{
		sscanf(buf, "%*[^:]: %d\n%s", &data_len, data);
        LOG_DEBUG("dataLen:%d data:%s ", data_len, data);

        pcpos = strstr((const char *)buf, "#!AMR");
        LOG_DEBUG("%d", *pcpos);
        data_len = len - (pcpos- buf);
        LOG_DEBUG("%d", data_len);

        ascii2unicode(filename_w, http_localFileName);      //FIXME: overflow bug: the filename length may exceed MAX_FILENAME_LEN

        fh = eat_fs_Open(filename_w, FS_READ_WRITE|FS_CREATE);
		if(EAT_FS_NO_ERROR <= fh)
    	{
        	LOG_DEBUG("open file %s success, fh=%d.", http_localFileName, fh);

            do
            {
                rc = eat_fs_Write(fh, pcpos, data_len, &write_len);
                LOG_DEBUG("%d", write_len);
                write_whole_len += write_len;
            	if(EAT_FS_NO_ERROR != rc)
            	{
                	LOG_ERROR("write file failed, and Return Error is %d", rc);
            	}
                data_len = eat_modem_read(data, MAX_DATA_LEN);
            }while(data_len == MAX_DATA_LEN);

            rc = eat_fs_Write(fh, data, data_len, &write_len);
            write_whole_len += write_len;
            LOG_DEBUG("write whole len %d", write_whole_len);
            if(EAT_FS_NO_ERROR != rc)
        	{
            	LOG_ERROR("write file failed, and Return Error is %d", rc);
        	}
    	}
        eat_fs_Close(fh);
    	http_end();
	}
	else if(strstr((const char *)buf, "OK"))
	{
		LOG_DEBUG("post success");
		http_end();
	}
}

MC_HTTP_PROC http_procs[] =
{
    {"OK",          http_GPRScheck},
    {"ERROR",       http_GPRScheck},
    {"HTTPINIT",    http_set_server_1},
    {"HTTPPARA",    http_set_server_2},
    {"HTTPACTION:", http_judge_sucess},
    {"+HTTPREAD:",  http_save_data},
    {"HTTPTERM",    http_closeGPRS},
    {"HTTPDATA",    http_post_start},
};

void http_modem_run(u8 * buf, u16 len)
{
    int i = 0;
    for (i = 0; i < sizeof(http_procs) / sizeof(http_procs[0]); i++)
    {
        if (strstr((const char *)buf, http_procs[i].cmd))
        {
            http_procs[i].pfn(buf, len);
        }
    }
}

