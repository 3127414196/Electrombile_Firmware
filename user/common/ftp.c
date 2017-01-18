/*
 * ftp.c
 *
 *  Created on: 2017/01/04
 *      Author: lc
 */
#include <string.h>

#include "modem.h"
#include "ftp.h"
#include "record.h"
#include "log.h"
#include "thread_msg.h"
#include "thread.h"
#include "setting.h"

#define MAX_LOCALFILENAME_LEN 32
#define MAX_FTPCMD_LEN 64

enum{
    FTP_UPLOAD,
    FTP_DOWNLOAD,
}FTPTYPE;

typedef void(*FTP_PROC)(u8 *);
typedef struct
{
	u8 cmd[16];
	FTP_PROC pfn;
}MC_FTP_PROC;

static eat_bool isGPRSOpening = EAT_FALSE;

static int ftp_type = FTP_UPLOAD;
static char ftp_localFileName[MAX_LOCALFILENAME_LEN] = {0};
static char ftp_serverFileName[MAX_SERVERFILENAME_LEN] = {0};

static int ftp_sendMsg2Event(u8 code)
{
    u8 msgLen = sizeof(MSG_THREAD) + sizeof(FTP_PUTFILE_INFO) + strlen(ftp_serverFileName) + 1;
    MSG_THREAD *msg = NULL;
    FTP_PUTFILE_INFO *msg_data = NULL;

    msg = allocMsg(msgLen);
    if(!msg)
    {
        LOG_ERROR("device inner error");
        return -1;
    }
    msg->length = sizeof(FTP_PUTFILE_INFO) + strlen(ftp_serverFileName) + 1;
    msg->cmd = CMD_THREAD_PUTEND;

    msg_data = (FTP_PUTFILE_INFO *)msg->data;
    msg_data->code = code;
    if(0 == code)//put file successful
    {
        strncpy(msg_data->fileName, ftp_serverFileName, strlen(ftp_serverFileName));
        msg_data->fileName[strlen(ftp_serverFileName)] = 0;
    }

    return sendMsg(THREAD_MAIN, msg, msgLen);
}

static void ftp_openGPRS(void)
{
    isGPRSOpening = EAT_TRUE;
    modem_AT("AT+SAPBR=1,1" CR);
}

static void ftp_closeGPRS(void)
{
    modem_AT("AT+SAPBR=0,1" CR);
}

void ftp_upload_file(char *localFileName, char *serverFileName)
{
    strncpy(ftp_localFileName, localFileName, MAX_LOCALFILENAME_LEN);
    strncpy(ftp_serverFileName, serverFileName, MAX_SERVERFILENAME_LEN);
    ftp_type = FTP_UPLOAD;
    ftp_openGPRS();
}

void ftp_download_file(char *localFileName, char *serverFileName)
{
    strncpy(ftp_localFileName, localFileName, MAX_LOCALFILENAME_LEN);
    strncpy(ftp_serverFileName, serverFileName, MAX_SERVERFILENAME_LEN);
    ftp_type = FTP_DOWNLOAD;
    ftp_openGPRS();
}

static void ftp_GPRScheck(u8 * buf)
{
    if((strstr(buf, "OK") && isGPRSOpening) || (strstr(buf, "ERROR") && strstr(buf, "AT+SAPBR=1,1")))
    {
        isGPRSOpening = EAT_FALSE;
        modem_AT("AT+FTPCID=1" CR);
    }
}

static void ftp_set_server(u8 * buf)
{
    unsigned char cmd[MAX_FTPCMD_LEN] = {0};
    if(strstr(buf, "OK"))
    {
        snprintf(cmd, MAX_FTPCMD_LEN, "AT+FTPSERV=\"%s\"\r", setting.ftp_domain);
        modem_AT(cmd);
    }
    else
    {
        LOG_ERROR("FTPCID error");
        ftp_closeGPRS();
        ftp_sendMsg2Event(FTP_FTPCID_ERROR);
    }
}

static void ftp_set_name(u8 * buf)
{
    if(strstr(buf, "OK"))
    {
        modem_AT("AT+FTPUN=\"anonymous\"" CR);
    }
    else
    {
        LOG_ERROR("FTPSERV error");
        ftp_closeGPRS();
        ftp_sendMsg2Event(FTP_FTPSERV_ERROR);
    }
}

static void ftp_set_file(u8 * buf)
{
    unsigned char cmd[MAX_FTPCMD_LEN] = {0};
    if(strstr(buf, "OK"))
    {
        if(ftp_type == FTP_UPLOAD)
        {
            snprintf(cmd, MAX_FTPCMD_LEN, "AT+FTPPUTNAME=\"%s\"\r", ftp_serverFileName);
        }
        else
        {
            snprintf(cmd, MAX_FTPCMD_LEN, "AT+FTPGETNAME=\"%s\"\r", ftp_serverFileName);
        }
        modem_AT(cmd);
    }
    else
    {
        LOG_ERROR("FTPUN error");
        ftp_closeGPRS();
        ftp_sendMsg2Event(FTP_FTPUN_ERROR);
    }
}

static void ftp_set_path(u8 * buf)
{
    if(strstr(buf, "OK"))
    {
        if(ftp_type == FTP_UPLOAD)
        {
            modem_AT("AT+FTPPUTPATH=\"/home/\"" CR);
        }
        else
        {
            modem_AT("AT+FTPGETPATH=\"/home/\"" CR);
        }
    }
    else
    {
        LOG_ERROR("FTPSERVNAME error");
        ftp_closeGPRS();
        ftp_sendMsg2Event(FTP_FTPSERVNAME_ERROR);
    }
}

static void ftp_put_file(u8 * buf)
{
    unsigned char cmd[MAX_FTPCMD_LEN] = {0};
    if(strstr(buf, "OK"))
    {
        snprintf(cmd, MAX_FTPCMD_LEN, "AT+FTPPUTFRMFS=\"%s\"\r", ftp_localFileName);
        modem_AT(cmd);
    }
    else
    {
        LOG_ERROR("FTPPUTPATH error");
        ftp_closeGPRS();
        ftp_sendMsg2Event(FTP_FTPPUTPATH_ERROR);
    }
}

static void ftp_get_file(u8 * buf)
{
    unsigned char cmd[MAX_FTPCMD_LEN] = {0};
    if(strstr(buf, "OK"))
    {
        snprintf(cmd, MAX_FTPCMD_LEN, "AT+FTPGETTOFS=0,\"%s\"\r", ftp_localFileName);
        modem_AT(cmd);
    }
    else
    {
        LOG_ERROR("FTPGETPATH error");
        ftp_closeGPRS();
        ftp_sendMsg2Event(FTP_FTPGETPATH_ERROR);
    }
}

static void ftp_put_end(u8 * buf)
{
    if(strstr(buf, "FTPPUTFRMFS: 0"))
    {
        LOG_DEBUG("put file OK");
        ftp_sendMsg2Event(FTP_SUCCESS);
    }
    else
    {
        LOG_DEBUG("put file ERROR");
        ftp_closeGPRS();
        ftp_sendMsg2Event(FTP_FTPPUTFILE_ERROR);
    }
    ftp_closeGPRS();
}

static void ftp_get_end(u8 * buf)
{
    if(strstr(buf, "FTPGETTOFS: 0"))
    {
        LOG_DEBUG("get file OK");
        ftp_sendMsg2Event(FTP_SUCCESS);
    }
    else
    {
        LOG_ERROR("get file ERROR");
        ftp_closeGPRS();
        ftp_sendMsg2Event(FTP_FTPGETFILE_ERROR);
    }
    ftp_closeGPRS();
}

MC_FTP_PROC ftp_procs[] =
{
    {"OK",          ftp_GPRScheck},
    {"ERROR",       ftp_GPRScheck},
    {"FTPCID",      ftp_set_server},
    {"FTPSERV",     ftp_set_name},
    {"FTPUN",       ftp_set_file},
    {"FTPPUTNAME",  ftp_set_path},
    {"FTPGETNAME",  ftp_set_path},
    {"FTPPUTPATH",  ftp_put_file},
    {"FTPGETPATH",  ftp_get_file},
    {"FTPPUTFRMFS:",ftp_put_end},
    {"FTPGETTOFS:", ftp_get_end}
};

void ftp_modem_run(u8 * buf)
{
    int i = 0;
    for (i = 0; i < sizeof(ftp_procs) / sizeof(ftp_procs[0]); i++)
    {
        if (strstr(buf, ftp_procs[i].cmd))
        {
            ftp_procs[i].pfn(buf);
        }
    }
}

