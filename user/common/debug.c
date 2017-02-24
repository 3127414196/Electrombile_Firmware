/*
 * debug.c
 *
 *  Created on: 2016年2月4日
 *      Author: jk
 */
#include <stdio.h>
#include <string.h>

#include <eat_interface.h>

#include "debug.h"
#include "log.h"
#include "rtc.h"
#include "utils.h"
#include "version.h"
#include "modem.h"
#include "ftp.h"

#define MAX_CMD_LENGTH (16)
#define MAX_CMD_NUMBER  (32)

typedef struct
{
    unsigned char cmd[MAX_CMD_LENGTH];
    CMD_ACTION  action;
}CMD_MAP;

#define DBG_OUT(fmt, ...) eat_trace("[DEBUG]>"fmt, ##__VA_ARGS__)

static int cmd_debug(const unsigned char* cmdString, unsigned short length);
static int cmd_version(const unsigned char* cmdString, unsigned short length);
static int cmd_imei(const unsigned char* cmdString, unsigned short length);
static int cmd_imsi(const unsigned char* cmdString, unsigned short length);
static int cmd_chipid(const unsigned char* cmdString, unsigned short length);
static int cmd_AT(const unsigned char* cmdString, unsigned short length);
static int cmd_startRecord(const unsigned char* cmdString, unsigned short length);
static int cmd_stopRecord(const unsigned char* cmdString, unsigned short length);
static int cmd_PlayOut(const unsigned char* cmdString, unsigned short length);

static int cmd_upload(const unsigned char* cmdString, unsigned short length);
static int cmd_download(const unsigned char* cmdString, unsigned short length);

#ifdef APP_DEBUG
static int cmd_reboot(const unsigned char* cmdString, unsigned short length);
static int cmd_halt(const unsigned char* cmdString, unsigned short length);
static int cmd_rtc(const unsigned char* cmdString, unsigned short length);
#endif

static CMD_MAP cmd_map[MAX_CMD_NUMBER] =
{
        {"debug",       cmd_debug},
        {"version",     cmd_version},
        {"imei",        cmd_imei},
        {"imsi",        cmd_imsi},
        {"chipid",      cmd_chipid},
        {"AT",          cmd_AT},
        {"at",          cmd_AT},
        {"start",       cmd_startRecord},
        {"stop",        cmd_stopRecord},
        {"play",        cmd_PlayOut},
        {"upload",      cmd_upload},
        {"download",    cmd_download},

#ifdef APP_DEBUG
        {"reboot",      cmd_reboot},
        {"halt",        cmd_halt},
        {"rtc",         cmd_rtc},
#endif
};


static int cmd_debug(const unsigned char* cmdString, unsigned short length)
{
    int i = 0;
    DBG_OUT("support cmd:");
    for (i = 0; i < MAX_CMD_NUMBER && cmd_map[i].action; i++)
    {
        DBG_OUT("\t%s\t%p", cmd_map[i].cmd, cmd_map[i].action);
    }

    return 0;
}

static int cmd_version(const unsigned char* cmdString, unsigned short length)
{
    DBG_OUT("version:%s(build:%s %s) core:%s(buildNo:%s@%s)", VERSION_STR, __DATE__, __TIME__, eat_get_version(), eat_get_buildno(), eat_get_buildtime());
    return 0;
}

static int cmd_imei(const unsigned char* cmdString, unsigned short length)
{
    u8 imei[32] = {0};
    eat_get_imei(imei, 31);
    DBG_OUT("IMEI = %s", imei);
    return 0;
 }

static int cmd_imsi(const unsigned char* cmdString, unsigned short length)
{
    u8 imsi[32] = {0};
    eat_get_imsi(imsi, 31);
    DBG_OUT("IMSI = %s", imsi);
    return 0;

}

static int cmd_chipid(const unsigned char* cmdString, unsigned short length)
{
#define MAX_CHIPID_LEN  16
    char chipid[MAX_CHIPID_LEN + 1] = {0};
    char chipid_desc[MAX_CHIPID_LEN * 2 + 1] = {0}; //hex character
    int i = 0; //loop var

    eat_get_chipid(chipid, MAX_CHIPID_LEN);
    for (i = 0; i < MAX_CHIPID_LEN; i++)
    {
        snprintf(chipid_desc + i * 2, 2, "%02X", chipid[i]);
    }
    DBG_OUT("chipd = %s", chipid_desc);
    return 0;
}

static int cmd_AT(const unsigned char* cmdString, unsigned short length)
{
    //forward AT command to modem
    eat_modem_write(cmdString, length);
    eat_modem_write("\n", 1);
    return 0;
}

static int cmd_upload(const unsigned char* cmdString, unsigned short length)
{
    ftp_upload_file("NOTHING", "NOTHING");
    return 0;
}

static int cmd_download(const unsigned char* cmdString, unsigned short length)
{
    ftp_download_file("NOTHING", "NOTHING");
    return 0;
}

static int cmd_startRecord(const unsigned char* cmdString, unsigned short length)
{
    record_start();
    return 0;
}
static int cmd_stopRecord(const unsigned char* cmdString, unsigned short length)
{
    record_stop();
    return 0;
}

static int cmd_PlayOut(const unsigned char* cmdString, unsigned short length)
{
#define READ_BUFFER_LENGTH  512
#define MUSIC_NAME L"C:\\record.amr"
    FS_HANDLE fh;
    int rc = 0;
    char buf[READ_BUFFER_LENGTH] = {0};
    UINT readLen = 0;
    int printlen = 0;
    UINT filesize = 0;
    int file_offset = 0;

    fh = eat_fs_Open(MUSIC_NAME, FS_READ_ONLY);

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

    rc = eat_fs_GetFileSize(fh, &filesize);
    if (rc < EAT_FS_NO_ERROR)
    {
        print("seek file pointer failed:%d", rc);
        eat_fs_Close(fh);
        return -1;
    }

    while(1)
    {
        rc = eat_fs_Seek(fh, file_offset, EAT_FS_FILE_BEGIN);
        if (rc < EAT_FS_NO_ERROR)
        {
            print("seek file pointer failed:%d", rc);
            eat_fs_Close(fh);
            return -1;
        }

        rc = eat_fs_Read(fh, buf, READ_BUFFER_LENGTH, &readLen);
        if (rc < EAT_FS_NO_ERROR)
        {
            print("read file failed:%d", rc);
            eat_fs_Close(fh);
            return -1;
        }

        printlen = eat_uart_write(EAT_UART_1,(const unsigned char *)buf, readLen);
        file_offset += printlen;
        if(file_offset >= filesize)break;
    }

    eat_fs_Close(fh);

    return 0;
}


#ifdef APP_DEBUG
static int cmd_reboot(const unsigned char* cmdString, unsigned short length)
{
    eat_reset_module();
    return 0;
}

static int cmd_halt(const unsigned char* cmdString, unsigned short length)
{
    eat_power_down();
    return 0;
}

static int cmd_rtc(const unsigned char* cmdString, unsigned short length)
{
    const int RTC_BASE = 1954;
    EatRtc_st rtc = {0};
    eat_bool result = eat_get_rtc(&rtc);
    if (result)
    {
        DBG_OUT("RTC:%d-%02d-%02d %02d:%02d:%02d UTC, timestamp:%d", rtc.year + RTC_BASE, rtc.mon, rtc.day, rtc.hour, rtc.min, rtc.sec, rtc_getTimestamp());
    }
    else
    {
        LOG_ERROR("get rtc time failed:%d", result);
    }

    return 0;
}

#endif

int debug_proc(const unsigned char* cmdString, unsigned short length)
{
    int i = 0;

    const unsigned char* cmd = string_trimLeft(cmdString);

    for (i = 0; i < MAX_CMD_NUMBER && cmd_map[i].action; i++)
    {
        if (strncmp(cmd, cmd_map[i].cmd, strlen(cmd_map[i].cmd)) == 0)
        {
            return cmd_map[i].action(cmdString, length);
        }
    }

    LOG_INFO("CMD not processed");

    return 0;
}


int regist_cmd(const unsigned char* cmd, CMD_ACTION action)
{
    int i = 0;

    //寻找第一个空位命令
    while (i < MAX_CMD_NUMBER && cmd_map[i].action) i++;

    if ( i >= MAX_CMD_NUMBER)
    {
        LOG_ERROR("exceed MAX command number: %d", MAX_CMD_NUMBER);
        return -1;
    }

    strncpy(cmd_map[i].cmd, cmd, MAX_CMD_LENGTH);
    cmd_map[i].action = action;

    LOG_DEBUG("register cmd %s(%p) at position %d success", cmd, action, i);
    return 0;
}
