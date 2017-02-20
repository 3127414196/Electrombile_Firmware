/*
 * log.c
 *
 *  Created on: 2015/10/16
 *      Author: jk
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "uart.h"
#include "debug.h"
#include "log.h"


int log_catlog(void)
{
#define READ_BUFFER_LENGTH  512
    FS_HANDLE fh;
    int rc = 0;
    char buf[READ_BUFFER_LENGTH] = {0};
    UINT readLen = 0;
    int printlen = 0;

    eat_bool uart_buffer_full = EAT_FALSE;
    eat_bool end_of_file = EAT_FALSE;

    static int file_offset = 0;

    fh = eat_fs_Open(LOG_FILE_NAME, FS_READ_ONLY);

    //the log file is not found
    if(EAT_FS_FILE_NOT_FOUND == fh)
    {
        print("log file not exists.");
        file_offset = 0;
        uart_setWrite(0);
        return -1;
    }

    if (fh < EAT_FS_NO_ERROR)
    {
        print("open file failed, eat_fs_Open return %d!", fh);
        file_offset = 0;
        uart_setWrite(0);
        return -1;
    }

    rc = eat_fs_Seek(fh, file_offset, EAT_FS_FILE_BEGIN);
    if (rc < EAT_FS_NO_ERROR)
    {
        print("seek file pointer failed:%d", rc);
        eat_fs_Close(fh);
        file_offset = 0;
        uart_setWrite(0);
        return -1;
    }

    do
    {
        rc = eat_fs_Read(fh, buf, READ_BUFFER_LENGTH, &readLen);
        if (rc < EAT_FS_NO_ERROR)
        {
            print("read file failed:%d", rc);
            file_offset = 0;
            uart_setWrite(0);
            eat_fs_Close(fh);

            return -1;
        }

        if (readLen < READ_BUFFER_LENGTH)   //read the end of file
        {
            end_of_file = EAT_TRUE;
        }

        printlen = print("%s", buf);
        file_offset += printlen;
        if (printlen < readLen) //UART driver's receive buffer is full
        {
            uart_buffer_full = EAT_TRUE;
            uart_setWrite(log_catlog);
        }

//        LOG_INFO("read %d bytes, print %d bytes", readLen, printlen);

        if (!uart_buffer_full && end_of_file)
        {

            file_offset = 0;
            uart_setWrite(0);
        }

    }while (!uart_buffer_full && !end_of_file);

    eat_fs_Close(fh);

    return 0;
}

int log_GetLog(char buf[], s32 len)
{
    FS_HANDLE fh;
    int rc = 0;
    UINT readLen = 0;
    UINT filesize = 0;
    char *pbuf = NULL;

    fh = eat_fs_Open(LOG_FILE_NAME, FS_READ_ONLY);
    if(EAT_FS_FILE_NOT_FOUND == fh)
    {
        strncpy(buf,"no log file!",len);
        return 0;
    }

    if(EAT_FS_NO_ERROR > fh)
    {
        LOG_ERROR("open file error, fh=%d.", fh);
        return -1;
    }

    rc = eat_fs_GetFileSize(fh,&filesize);
    if(EAT_FS_NO_ERROR > rc)
    {
        LOG_ERROR("get filesize error, rc = %d.", rc);
        eat_fs_Close(fh);
        return -1;
    }

    if(filesize > len)
    {
        filesize = len;
    }

    rc = eat_fs_Seek(fh,-filesize,EAT_FS_FILE_END);
    if(EAT_FS_NO_ERROR > rc)
    {
        LOG_ERROR("seek file error, rc = %d.", rc);
        eat_fs_Close(fh);
        return -1;
    }

    eat_fs_Read(fh,buf,filesize,&readLen);
    if (EAT_FS_NO_ERROR > rc)
    {
        LOG_ERROR("read file failed:%d", rc);
        eat_fs_Close(fh);
        return -1;
    }

    pbuf = strstr(buf,"\r\n");
    if(!pbuf)
    {
        strncpy(buf,"no log file!",len);
        eat_fs_Close(fh);
        return 0;
    }

    snprintf(buf, len, "%s",pbuf);
    eat_fs_Close(fh);
    return 0;
}

int cmd_catlog(const unsigned char* cmdString, unsigned short length)
{
    print("cat log file begin:");
    return log_catlog();
}

void log_initial(void)
{
    regist_cmd("catlog", cmd_catlog);
}

/*
 * The hex log is in the following format:
 *
 *     0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F      0123456789ABCDEF
 * 01  aa 55 01 00 00 00 25 00 38 36 35 30 36 37 30 32     .U....%.86506702
 * 02  30 34 39 30 31 36 38 30 00 00 00 00 00 00 00 00     04901680........
 * 03  00 00 00 00 00 00 00 00 00 00 00 00                 ............
 *
 */
void log_hex(const char* data, int length)
{
    int i = 0, j = 0;

    print("    ");
    for (i  = 0; i < 16; i++)
    {
        print("%X  ", i);
    }
    print("    ");
    for (i = 0; i < 16; i++)
    {
        print("%X", i);
    }

    print("\r\n");

    for (i = 0; i < length; i += 16)
    {
        print("%02d  ", i / 16 + 1);
        for (j = i; j < i + 16 && j < length; j++)
        {
            print("%02x ", data[j] & 0xff);
        }
        if (j == length && length % 16)
        {
            for (j = 0; j < (16 - length % 16); j++)
            {
                print("   ");
            }
        }
        print("    ");
        for (j = i; j < i + 16 && j < length; j++)
        {
            if (data[j] < 32 || data[j] >= 127)
            {
                print(".");
            }
            else
            {
                print("%c", data[j] & 0xff);
            }
        }

        print("\r\n");
    }
}

static eat_bool log_checkLogFileSize(void)
{
    FS_HANDLE fh;
    int rc = 0;
    UINT filesize = 0;

    fh = eat_fs_Open(LOG_FILE_NAME, FS_READ_WRITE);
    if (fh == EAT_FS_FILE_NOT_FOUND)
    {
        return EAT_TRUE;
    }

    if(fh < EAT_FS_NO_ERROR)
    {
        return EAT_FALSE;
    }

    rc = eat_fs_GetFileSize(fh, &filesize);
    if(rc < EAT_FS_NO_ERROR)
    {
        LOG_INFO("get file size error return:%d", rc);
        eat_fs_Close(fh);
        return EAT_FALSE;
    }

    if(filesize < MAX_LOGFILE_SIZE)
    {
        eat_fs_Close(fh);

        return EAT_TRUE;
    }

    eat_fs_Close(fh);

    //first delete the backup log file
    rc = eat_fs_Delete(LOG_FILE_BAK);
    if(EAT_FS_FILE_NOT_FOUND != rc && EAT_FS_NO_ERROR != rc)
    {
        LOG_INFO("delete old_log_file failed.rc= %d", rc);
        return EAT_FALSE;
    }

    //rename the log file to backup log file
    rc = eat_fs_Rename(LOG_FILE_NAME, LOG_FILE_BAK);
    if(rc < EAT_FS_NO_ERROR)
    {
        LOG_ERROR("rename failed, rc= %d",rc);
        return EAT_FALSE;
    }

    return EAT_TRUE;
}

void log_file(const char* fmt, ...)
{
#define MAX_LOG_BUFFER_SIZE 1024
    char buf[MAX_LOG_BUFFER_SIZE + 2] = "\0";   // the additional 2 space is for the CR+LF appended to the end
    FS_HANDLE fh_open;
    int rc = 0;

    va_list arg;
    va_start(arg, fmt);
    vsnprintf(buf, MAX_LOG_BUFFER_SIZE, fmt, arg);
    va_end(arg);


    strcpy(buf + strlen(buf),"\r\n");

    if (!log_checkLogFileSize())
    {
        LOG_INFO("log file size check failed");
        return;
    }

    fh_open = eat_fs_Open(LOG_FILE_NAME, FS_READ_WRITE | FS_CREATE);

    if(fh_open < EAT_FS_NO_ERROR)
    {
        LOG_INFO("open file failed, fh=%d!", fh_open);
        return;
    }

    rc = eat_fs_Seek(fh_open, 0, EAT_FS_FILE_END);

    if(rc < 0)
    {
        /*log_file == LOG_ERROR,there should not be LOG_ERROR*/
        LOG_INFO("Seek File Pointer Fail");

        eat_fs_Close(fh_open);

        return;
    }

    rc = eat_fs_Write(fh_open, buf, strlen(buf), NULL);
    if(EAT_FS_NO_ERROR > rc)
    {
        LOG_INFO("write file failed,Error %d", rc);
    }
    eat_fs_Close(fh_open);
    return;
}
