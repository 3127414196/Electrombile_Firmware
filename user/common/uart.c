/*
 * uart.c
 *
 *  Created on: 2015��7��8��
 *      Author: jk
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <eat_interface.h>
#include <eat_uart.h>
#include <eat_modem.h>

#include "uart.h"
#include "log.h"
#include "setting.h"


#define MAX_CMD_LENGTH (16)
#define MAX_CMD_NUMBER  (32)

typedef struct
{
    char cmd[MAX_CMD_LENGTH];
    CMD_ACTION  action;
}CMD_MAP;

int cmd_version(const char* cmdString, unsigned short length)
{
    LOG_INFO("version = %s build time = %s, build No. = %s", eat_get_version(), eat_get_buildtime(), eat_get_buildno());
    return 0;
}

int cmd_imei(const char* cmdString, unsigned short length)
{
    u8 imei[32] = {0};
    eat_get_imei(imei, 31);
    LOG_INFO("IMEI = %s", imei);
    return 0;
 }

int cmd_imsi(const char* cmdString, unsigned short length)
{
    u8 imsi[32] = {0};
    eat_get_imsi(imsi, 31);
    LOG_INFO("IMSI = %s", imsi);
    return 0;

}

int cmd_chipid(const char* cmdString, unsigned short length)
{
#define MAX_CHIPID_LEN  16
    u8 chipid[MAX_CHIPID_LEN + 1] = {0};
    u8 chipid_desc[MAX_CHIPID_LEN * 2 + 1] = {0}; //hex character
    int i = 0; //loop var

    eat_get_chipid(chipid, MAX_CHIPID_LEN);
    for (i = 0; i < MAX_CHIPID_LEN; i++)
    {
        sprintf(chipid_desc + i * 2, "%02X", chipid[i]);
    }
    LOG_INFO("chipd = %s", chipid_desc);
    return 0;
}

int cmd_reboot(const char* cmdString, unsigned short length)
{
    eat_reset_module();
    return 0;
}

int cmd_halt(const char* cmdString, unsigned short length)
{
    eat_power_down();
    return 0;
}

int cmd_rtc(const char* cmdString, unsigned short length)
{
    EatRtc_st rtc = {0};
    eat_bool result = eat_get_rtc(&rtc);
    if (result)
    {
        LOG_INFO("RTC timer: %d-%02d-%02d %02d:%02d:%02d", rtc.year, rtc.mon, rtc.day, rtc.hour, rtc.min, rtc.sec);
    }
    else
    {
        LOG_ERROR("Get rtc time failed:%d", result);
    }

    return 0;
}

int cmd_AT(const char* cmdString, unsigned short length)
{
    //forward AT command to modem
    eat_modem_write(cmdString, length);
    return 0;
}

//TODO: the following command should be in FS module
#if 0
    if(strstr(buf, "deletesetting"))
    {
        LOG_DEBUG("setting.txt deleted.");
        eat_fs_Delete(SETITINGFILE_NAME);//TODO, for debug
        return 0;
    }
    if(strstr(buf, "deletemileage"))
    {
        LOG_DEBUG("mileage.txt deleted.");
        eat_fs_Delete(MILEAGEFILE_NAME);//TODO, for debug
        return 0;
    }
    if(strstr(buf, "deletelog"))
    {
        LOG_DEBUG("log.txt deleted.");
        eat_fs_Delete(LOGFILE_NAME);//TODO, for debug
        return 0;
    }
#endif


static CMD_MAP cmd_map[MAX_CMD_NUMBER] =
{
        {"version",     cmd_version},
        {"imei",        cmd_imei},
        {"imsi",        cmd_imsi},
        {"chipid",      cmd_chipid},
#ifdef LOG_DEBUG_FLAG
        {"reboot",      cmd_reboot},
        {"halt",        cmd_halt},
        {"rtc",         cmd_rtc},
        {"AT",          cmd_AT},
#endif
};

int event_uart_ready_rd(const EatEvent_st* event)
{
	u16 length = 0;
	EatUart_enum uart = event->data.uart.uart;
	char buf[1024] = {0};	//TODO: use macro
	int i = 0;

	length = eat_uart_read(uart, buf, 1024);
	if (length)
	{
		buf[length] = 0;
		LOG_DEBUG("uart recv: %s", buf);
	}
	else
	{
		return 0;
	}

	for (i = 0; i < MAX_CMD_NUMBER; i++)
	{
	    if (cmd_map[i].cmd != NULL && strstr(buf, cmd_map[i].cmd))
	    {
	        return cmd_map[i].action(buf, length);
	    }
	}

	return 0;
}

int regist_cmd(const char* cmd, CMD_ACTION action)
{
    int i = 0;

    //寻找第一个空位命令
    while (i < MAX_CMD_NUMBER && cmd_map[i++].cmd);

    if ( i >= MAX_CMD_NUMBER)
    {
        LOG_ERROR("exceed MAX command number: %d", MAX_CMD_NUMBER);
        return -1;
    }

    strncpy(cmd_map[i].cmd, cmd, MAX_CMD_LENGTH);
    cmd_map[i].action = action;

    return 0;
}

void print(const char* fmt, ...)
{
    char buf[1024] = {0};
    int length = 0;

    va_list arg;
    va_start (arg, fmt);
    vsnprintf(buf, 1024, fmt, arg);
    va_end (arg);

    length = strlen(buf);

    eat_uart_write(EAT_UART_1, buf, length);
}
