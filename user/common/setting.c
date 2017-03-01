/*
 * setting.c
 *
 *  Created on: 2015/6/24
 *      Author: jk
 */
#include <string.h>
#include <stdio.h>

#include <eat_fs_type.h>
#include <eat_fs.h>
#include <eat_fs_errcode.h>
#include <eat_modem.h>
#include <eat_interface.h>
#include <eat_uart.h>

#include "setting.h"
#include "version.h"
#include "debug.h"
#include "log.h"
#include "fs.h"
#include "cJSON.h"
#include "mem.h"
#include "utils.h"

typedef struct
{
    //Server configuration
    ADDR_TYPE addr_type;
    union
    {
        char domain[MAX_DOMAIN_NAME_LEN];
        u8 ipaddr[4];
    }addr;
    u16 port;

    //Timer configuration
    u32 gps_send_timer_period;

}STORAGE;


SETTING setting;

//for debug command
#define CMD_STRING_SERVER   "server"

//for JSON tag
#define TAG_SERVER  "SERVER"
#define TAG_ADDR_TYPE   "ADDR_TYPE"
#define TAG_ADDR   "ADDR"
#define TAG_PORT    "PORT"

#define TAG_AUTOLOCK    "autolock"
#define TAG_LOCK    "lock"
#define TAG_PERIOD  "period"

#define TAG_BATTERY  "battery"
#define TAG_ISUSERTYPE "isusertype"
#define TAG_BATTERYTYPE "batterytype"
#define TAG_BATTERYTYPE_JUDGING "batterytype_judging"
#define TAG_IS_BATTERYTYPE_JUDGING "isbatterytype_judging"

#define TAG_IS_VIBRATEFIXED "isVibrateFixed"
#define TAG_VIBRATE "defendstate"

#define TAG_BLUETOOTH "bluetooth"
#define TAG_BLUETOOTH_ID "id"
#define TAG_BLUETOOTH_SWICTH  "sw"


int setting_changeServer(const unsigned char* cmdString, unsigned short length)
{
    char address[MAX_DOMAIN_NAME_LEN] = {0};
    int ip[4] = {0};
    int port = 0;
    int count = 0;

    const char *serverString = string_bypass(cmdString, CMD_STRING_SERVER);
    serverString = string_trimLeft(serverString);

    count = sscanf(serverString, "%[^:]:%d", address, &port);
    if (count != 2)
    {
        LOG_DEBUG("format not correct, should be like 'server 10.11.12.23:9876' or 'server server.xiaoan.com:9876'");
        return -1;
    }

    count = sscanf(address, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]);
    if (4 == count)   // ip
    {
        if (ip[0] <= 255 && ip[1] <= 255 && ip[2] <= 255 && ip[3] <= 255)//validity check
        {
            setting.addr_type = ADDR_TYPE_IP;
            setting.ipaddr[0] = (u8) ip[0];
            setting.ipaddr[1] = (u8) ip[1];
            setting.ipaddr[2] = (u8) ip[2];
            setting.ipaddr[3] = (u8) ip[3];
            setting.port = (u16) port;

            setting_save();
        }
        else
        {
            LOG_DEBUG("ip addrss format not correct!");
            return -1;
        }
    }
    else            // domain
    {
        setting.addr_type = ADDR_TYPE_DOMAIN;
        strncpy(setting.domain, address, MAX_DOMAIN_NAME_LEN);
        setting.port = (u16) port;

        setting_save();
    }
    return 0;
}

static void setting_initial(void)
{
    cJSON_Hooks mem_hooks;

    mem_hooks.malloc_fn = malloc;
    mem_hooks.free_fn = free;

    LOG_DEBUG("setting initial to default value.");

    cJSON_InitHooks(&mem_hooks);//initial the cJSON memory hook

    regist_cmd(CMD_STRING_SERVER, setting_changeServer);//register the debug command

    /* Server configuration */
#if 1
    setting.addr_type = ADDR_TYPE_DOMAIN;
    strncpy(setting.domain, "test.xiaoan110.com",MAX_DOMAIN_NAME_LEN);
#else
    setting.addr_type = ADDR_TYPE_IP;
    setting.ipaddr[0] = 118;
    setting.ipaddr[1] = 89;
    setting.ipaddr[2] = 104;
    setting.ipaddr[3] = 130;
#endif

    strncpy(setting.ftp_domain, "test.xiaoan110.com", MAX_DOMAIN_NAME_LEN);
    setting.port = 9880;

    /* Timer configuration */
    setting.main_loop_timer_period = 5000;
    setting.vibration_timer_period = 1000;
    setting.seek_timer_period = 2000;
    setting.timeupdate_timer_peroid = 24 * 60 * 60 * 1000;      //24h * 60m * 60s * 1000ms
    setting.gps_send_period = 30 * 1000;
    setting.bluetooth_timer_period = 1 * 1000;

    /* Switch configuration */
    setting.isVibrateFixed = EAT_FALSE;

    //autolock configuration
    setting.isAutodefendFixed = EAT_TRUE;
    setting.autodefendPeriod = 5;

    //Baterry Type
    setting.isUserType = EAT_FALSE;
    setting.BatteryType = NULL;    //Initial default NULL,wait for certainly judge type
    setting.BaterryType_Judging = NULL;
    setting.isBatteryJudging = EAT_FALSE;

    setting.BluetoothSwitch = EAT_TRUE;
    strncpy(setting.BluetoothId, "00:00:00:00:00:00", BLUETOOTH_ID_LEN);

    return;
}

eat_bool vibration_fixed(void)
{
    return setting.isVibrateFixed;
}

void set_vibration_state(eat_bool fixed)
{
    setting.isVibrateFixed = fixed;
    setting_save();
}

eat_bool get_autodefend_state(void)
{
    return setting.isAutodefendFixed;
}

void set_autodefend_state(eat_bool fixed)
{
    setting.isAutodefendFixed = fixed;
    setting_save();
}

unsigned char get_autodefend_period(void)
{
    return setting.autodefendPeriod;
}

void set_autodefend_period(unsigned char period)
{
    setting.autodefendPeriod = period;
    setting_save();
}

void set_UserBatteryTpye(u8 batteryType)
{
    setting.isUserType = EAT_TRUE;
    setting.BatteryType = batteryType;
    setting_save();
}

eat_bool isUserBatteryTpye(void)
{
    return setting.isUserType;
}

unsigned char get_battery_type(void)
{
    return setting.BatteryType;
}

void set_battery_type(unsigned char batterytype)
{
    setting.BatteryType = batterytype;
    setting_save();
}

unsigned char get_batterytype_Judging(void)
{
    return setting.BaterryType_Judging;
}

eat_bool get_battery_isJudging(void)
{
    return setting.isBatteryJudging;
}

void set_battery_isJudging(eat_bool isBatteryJudging, u8 baterrytype)
{
    setting.BaterryType_Judging = baterrytype;
    setting.isBatteryJudging = isBatteryJudging;
    setting_save();
}

void set_bluetooth_id(const char* BluetoothIdString)
{
    strncpy(setting.BluetoothId, BluetoothIdString, BLUETOOTH_ID_LEN);
    setting_save();
    LOG_DEBUG("set bluetooth ID: %s", setting.BluetoothId);
}

void set_bluetooth_switch(eat_bool sw)
{
    setting.BluetoothSwitch = sw;
    setting_save();
    LOG_DEBUG("set bluetooth switch: %d", sw);
}

eat_bool is_bluetoothOn(void)
{
    return setting.BluetoothSwitch;
}

eat_bool setting_restore(void)
{
    FS_HANDLE fh;
    int rc;
    UINT filesize = 0;
    char *buf = 0;
    cJSON *conf = 0;
    cJSON *addr = 0;
    cJSON *autolock = 0;
    cJSON *battery = 0;
    cJSON *defend_state = 0;
    cJSON *bluetooth = 0;

    setting_initial();

    LOG_DEBUG("restore setting from file");

    //open the file
    fh = eat_fs_Open(SETTINGFILE_NAME, FS_READ_ONLY);
    if(EAT_FS_FILE_NOT_FOUND == fh)
    {
        LOG_INFO("setting file not exists.");
        return EAT_TRUE;
    }

    if (fh < EAT_FS_NO_ERROR)
    {
        LOG_ERROR("read setting file fail, rc: %d", fh);
        return EAT_FALSE;
    }

    //get the file length
    rc = eat_fs_GetFileSize(fh, &filesize);
    if(EAT_FS_NO_ERROR != rc)
    {
        LOG_ERROR("get file size error, return %d",rc);
        eat_fs_Close(fh);
        return EAT_FALSE;
    }
    else
    {
        LOG_DEBUG("file size %d",filesize);
    }

    //malloc memory to read the file
    buf = malloc(filesize);
    if (!buf)
    {
        LOG_ERROR("malloc file content buffer failed");
        eat_fs_Close(fh);
        return EAT_FALSE;
    }
    else
    {
        LOG_DEBUG("malloc %d bytes for read setting", filesize);
    }


    //read the file
    rc = eat_fs_Read(fh, buf, filesize, NULL);
    if (rc != EAT_FS_NO_ERROR)
    {
        LOG_ERROR("read file fail, and return error: %d", fh);
        eat_fs_Close(fh);
        free(buf);
        return EAT_FALSE;
    }

    //parse the JSON data
    conf = cJSON_Parse(buf);
    if (!conf)
    {
        LOG_ERROR("setting config file format error!");
        eat_fs_Close(fh);
        free(buf);
        cJSON_Delete(conf);
        return EAT_FALSE;
    }

    addr = cJSON_GetObjectItem(conf, TAG_SERVER);
    if (addr)
    {
        setting.addr_type = cJSON_GetObjectItem(addr, TAG_ADDR_TYPE)->valueint;
        if (setting.addr_type == ADDR_TYPE_DOMAIN)
        {
            char *domain = cJSON_GetObjectItem(addr, TAG_ADDR)->valuestring;

            LOG_DEBUG("restore domain name");

            strncpy(setting.domain, domain, MAX_DOMAIN_NAME_LEN);
        }
        else
        {
            char *ipaddr = cJSON_GetObjectItem(addr, TAG_ADDR)->valuestring;
            int ip[4] = {0};
            int count = sscanf(ipaddr, "%u.%u.%u.%u", ip, ip + 1, ip + 2, ip + 3);

            LOG_DEBUG("restore ip address");
            if (count != 4) //4 means got four number of ip
            {
                LOG_ERROR("restore ip address failed");
                eat_fs_Close(fh);
                free(buf);
                cJSON_Delete(conf);
                return EAT_FALSE;
            }
            setting.ipaddr[0] = ip[0];
            setting.ipaddr[1] = ip[1];
            setting.ipaddr[2] = ip[2];
            setting.ipaddr[3] = ip[3];

        }

        setting.port = cJSON_GetObjectItem(addr, TAG_PORT)->valueint;
    }

    autolock = cJSON_GetObjectItem(conf, TAG_AUTOLOCK);
    if (autolock)
    {
        LOG_DEBUG("restore autolock conf");
        setting.isAutodefendFixed = cJSON_GetObjectItem(autolock, TAG_LOCK)->valueint ? EAT_TRUE : EAT_FALSE;
        setting.autodefendPeriod = cJSON_GetObjectItem(autolock, TAG_PERIOD)->valueint;
    }

    battery = cJSON_GetObjectItem(conf, TAG_BATTERY);
    if(battery)
    {
        LOG_DEBUG("restore battery conf");
        setting.isUserType = cJSON_GetObjectItem(battery, TAG_ISUSERTYPE)->valueint ? EAT_TRUE : EAT_FALSE;
        setting.BatteryType = cJSON_GetObjectItem(battery, TAG_BATTERYTYPE)->valueint;
        setting.BaterryType_Judging = cJSON_GetObjectItem(battery, TAG_BATTERYTYPE_JUDGING)->valueint;
        setting.isBatteryJudging = cJSON_GetObjectItem(battery, TAG_IS_BATTERYTYPE_JUDGING)->valueint ? EAT_TRUE : EAT_FALSE;
        LOG_DEBUG("BATTERY TYPE IS %d", setting.BatteryType);
    }

    defend_state = cJSON_GetObjectItem(conf, TAG_VIBRATE);
    if(defend_state)
    {
        LOG_DEBUG("restore defend_state conf");
        setting.isVibrateFixed = cJSON_GetObjectItem(defend_state, TAG_IS_VIBRATEFIXED)->valueint ? EAT_TRUE : EAT_FALSE;
    }

    bluetooth = cJSON_GetObjectItem(conf, TAG_BLUETOOTH);
    if(bluetooth)
    {
        char *BluetoothId = cJSON_GetObjectItem(bluetooth, TAG_BLUETOOTH_ID)->valuestring;
        LOG_DEBUG("restore bluetooth conf");
        strncpy(setting.BluetoothId, BluetoothId, BLUETOOTH_ID_LEN);
        setting.BluetoothSwitch = cJSON_GetObjectItem(bluetooth, TAG_BLUETOOTH_SWICTH)->valueint ? EAT_TRUE : EAT_FALSE;
    }

    free(buf);
    eat_fs_Close(fh);
    cJSON_Delete(conf);

    return EAT_TRUE;
}


eat_bool setting_save(void)
{
    FS_HANDLE fh, rc;
    eat_bool ret = EAT_FALSE;

    cJSON *root = cJSON_CreateObject();
    cJSON *address = cJSON_CreateObject();
    cJSON *autolock = cJSON_CreateObject();
    cJSON *battery = cJSON_CreateObject();
    cJSON *defend_state = cJSON_CreateObject();
    cJSON *bluetooth = cJSON_CreateObject();

    char *content = 0;


    cJSON_AddNumberToObject(address, TAG_ADDR_TYPE, setting.addr_type);
    if (setting.addr_type == ADDR_TYPE_DOMAIN)
    {
        cJSON_AddStringToObject(address, "ADDR", setting.domain);
    }
    else
    {
        char server[MAX_DOMAIN_NAME_LEN] = {0};
        snprintf(server, MAX_DOMAIN_NAME_LEN, "%d.%d.%d.%d", setting.ipaddr[0], setting.ipaddr[1], setting.ipaddr[2], setting.ipaddr[3]);
        cJSON_AddStringToObject(address, TAG_ADDR, server);
    }
    cJSON_AddNumberToObject(address, TAG_PORT, setting.port);

    cJSON_AddItemToObject(root, TAG_SERVER, address);

    cJSON_AddBoolToObject(autolock, TAG_LOCK, setting.isAutodefendFixed);
    cJSON_AddNumberToObject(autolock, TAG_PERIOD, setting.autodefendPeriod);

    cJSON_AddItemToObject(root, TAG_AUTOLOCK, autolock);

    cJSON_AddNumberToObject(battery, TAG_ISUSERTYPE, setting.isUserType);
    cJSON_AddNumberToObject(battery, TAG_BATTERYTYPE, setting.BatteryType);
    cJSON_AddNumberToObject(battery, TAG_BATTERYTYPE_JUDGING, setting.BaterryType_Judging);
    cJSON_AddNumberToObject(battery, TAG_IS_BATTERYTYPE_JUDGING, setting.isBatteryJudging);
    cJSON_AddItemToObject(root, TAG_BATTERY, battery);

    cJSON_AddNumberToObject(defend_state, TAG_IS_VIBRATEFIXED, setting.isVibrateFixed);
    cJSON_AddItemToObject(root, TAG_VIBRATE, defend_state);

    cJSON_AddStringToObject(bluetooth, TAG_BLUETOOTH_ID, setting.BluetoothId);
    cJSON_AddNumberToObject(bluetooth, TAG_BLUETOOTH_SWICTH, setting.BluetoothSwitch);
    cJSON_AddItemToObject(root, TAG_BLUETOOTH, bluetooth);

    content = cJSON_PrintUnformatted(root);// PrintUnformatted use space less

    LOG_DEBUG("save setting...");

    fh = eat_fs_Open(SETTINGFILE_NAME, FS_READ_WRITE|FS_CREATE);
    if(EAT_FS_NO_ERROR <= fh)
    {
        LOG_DEBUG("open file success, fh=%d.", fh);

        rc = eat_fs_Write(fh, content, strlen(content), 0);
        if(EAT_FS_NO_ERROR == rc)
        {
            LOG_DEBUG("write file success.");
        }
        else
        {
            LOG_ERROR("write file failed, and Return Error is %d", rc);
        }
    }
    else
    {
        LOG_ERROR("open file failed, fh=%d.", fh);
    }

    free(content);
    cJSON_Delete(root);
    eat_fs_Close(fh);

    return ret;
}






