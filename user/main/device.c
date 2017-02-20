/*
 * device.c
 *
 *  Created on: 2016/12/15
 *      Author: lc
 */

#include <stdio.h>
#include <string.h>

#include <eat_interface.h>
#include <eat_fs.h>
#include "cJSON.h"

#include "client.h"
#include "socket.h"
#include "fs.h"
#include "ftp.h"
#include "msg.h"
#include "log.h"
#include "uart.h"
#include "data.h"
#include "setting.h"
#include "thread.h"
#include "thread_msg.h"
#include "timer.h"
#include "msg_queue.h"
#include "device.h"
#include "record.h"
#include "protocol.h"
#include "telecontrol.h"
#include "audio_source.h"

enum
{
    DEVICE_GET_DEVICEINFO    = 0,
    DEVICE_GET_LOCATE        = 1,
    DEVICE_SET_AUTOLOCK      = 2,
    DEVICE_GET_AUTOLOCK      = 3,
    DEVICE_SET_DEFENDSTATE   = 4,
    DEVICE_GET_DEFENDSTATE   = 5,
    DEVICE_GET_BATTERY       = 6,
    DEVICE_SET_BATTERYTYPE   = 7,
    DEVICE_START_RECORD      = 8,
    DEVICE_STOP_RECORD       = 9,
    DEVICE_SET_BLUETOOTHID   = 10,
    DEVICE_DOWNLOAD_AUDIOFILE= 12,
    DEVICE_SET_BLUETOOTHSW   = 13,
    DEVICE_START_ALARM       = 14,
    DEVICE_CONTROL_LOCK      = 15,
}DEVICE_CMD_NAME;

typedef int (*DEVICE_PROC)(const void*, cJSON*);
typedef struct
{
    char cmd;
    DEVICE_PROC pfn;
}DEVICE_MSG_PROC;

static int device_responseOK(const void* req)
{
    char *buf = "{\"code\":0}";
    int length = sizeof(MSG_DEVICE_RSP) + strlen(buf);

    MSG_DEVICE_RSP *msg = alloc_device_msg(req, length);
    if(!msg)
    {
        LOG_ERROR("device inner error");
        return -1;
    }
    strncpy(msg->data, buf, strlen(buf));

    socket_sendDataDirectly(msg, length);
    return 0;
}

static int device_responseERROR(const void* req)
{
    char *buf = "{\"code\":110}";
    int length = sizeof(MSG_DEVICE_RSP) + strlen(buf);

    MSG_DEVICE_RSP *msg = alloc_device_msg(req, length);
    if(!msg)
    {
        LOG_ERROR("device inner error");
        return -1;
    }
    strncpy(msg->data, buf, strlen(buf));

    socket_sendDataDirectly(msg, length);
    return 0;
}


static int device_GetDeviceInfo(const void* req, cJSON *param)
{
    cJSON *autolock = NULL;
    cJSON *battery= NULL;
    cJSON *gps = NULL;
    cJSON *root = NULL;
    cJSON *json_root = NULL;

    char *buffer = NULL;
    MSG_DEVICE_RSP *rsp = NULL;
    int msgLen = 0;
    LOCAL_GPS* last_gps = gps_get_last();

    autolock = cJSON_CreateObject();
    if(!autolock)
    {
        device_responseERROR(req);
        return EAT_FALSE;
    }

    battery = cJSON_CreateObject();
    if(!battery)
    {
        cJSON_Delete(autolock);
        device_responseERROR(req);
        return EAT_FALSE;
    }

    gps = cJSON_CreateObject();
    if(!gps)
    {
        cJSON_Delete(autolock);
        cJSON_Delete(battery);
        device_responseERROR(req);
        return EAT_FALSE;
    }

    root = cJSON_CreateObject();
    if(!root)
    {
        cJSON_Delete(autolock);
        cJSON_Delete(battery);
        cJSON_Delete(gps);
        device_responseERROR(req);
        return EAT_FALSE;
    }

    json_root = cJSON_CreateObject();
    if(!json_root)
    {
        cJSON_Delete(autolock);
        cJSON_Delete(battery);
        cJSON_Delete(gps);
        cJSON_Delete(json_root);
        device_responseERROR(req);
        return EAT_FALSE;
    }

    cJSON_AddNumberToObject(json_root, "code", 0);

    cJSON_AddNumberToObject(autolock, "sw", setting.isAutodefendFixed);
    cJSON_AddNumberToObject(autolock, "period", setting.autodefendPeriod);
    cJSON_AddItemToObject(root, "autolock", autolock);

    cJSON_AddNumberToObject(battery, "percent", battery_getPercent());
    cJSON_AddNumberToObject(battery, "type", setting.BatteryType);
    cJSON_AddItemToObject(root, "battery", battery);

    cJSON_AddNumberToObject(root, "defend", setting.isVibrateFixed);

    cJSON_AddNumberToObject(gps, "timestamp", rtc_getTimestamp());
    cJSON_AddNumberToObject(gps, "lat", last_gps->gps.latitude);
    cJSON_AddNumberToObject(gps, "lng", last_gps->gps.longitude);
    cJSON_AddNumberToObject(gps, "speed", last_gps->gps.speed);
    cJSON_AddNumberToObject(gps, "course", last_gps->gps.course);
    cJSON_AddItemToObject(root, "gps", gps);

    cJSON_AddItemToObject(json_root, "result", root);

    buffer = cJSON_PrintUnformatted(json_root);
    if(!buffer)
    {
        cJSON_Delete(json_root);
        device_responseERROR(req);
        return EAT_FALSE;
    }

    cJSON_Delete(json_root);
    msgLen = sizeof(MSG_DEVICE_RSP) + strlen(buffer);
    rsp = alloc_device_msg((MSG_HEADER*)req, msgLen);

    if(!rsp)
    {
        device_responseERROR(req);
        return EAT_FALSE;
    }

    strncpy(rsp->data, buffer, strlen(buffer));
    free(buffer);

    socket_sendDataDirectly(rsp, msgLen);
    return 0;
}

static int device_GetLocation(const void* req, cJSON *param)
{
    //TODO SEND DEVICE INFO MSG TO SERVER
    return 0;
}

static int device_SetAutolock(const void* req, cJSON *param)
{
    cJSON *sw = NULL;
    cJSON *period = NULL;

    if(!param)
    {
        device_responseERROR(req);
        return EAT_FALSE;
    }

    sw = cJSON_GetObjectItem(param, "sw");
    period = cJSON_GetObjectItem(param, "period");
    if(sw && period)
    {
        setting.isAutodefendFixed = sw->valueint ? EAT_TRUE : EAT_FALSE;
        setting.autodefendPeriod = period->valueint;
        setting_save();
        device_responseOK(req);
    }
    else
    {
        device_responseERROR(req);
    }

    return 0;
}

static int device_GetAutolock(const void* req, cJSON *param)
{
    cJSON *result = NULL;
    cJSON *json_root = NULL;

    char *buffer = NULL;
    int msgLen = 0;
    MSG_DEVICE_RSP *rsp = NULL;

    result = cJSON_CreateObject();
    if(!result)
    {
        device_responseERROR(req);
        return EAT_FALSE;
    }

    json_root = cJSON_CreateObject();
    if(!json_root)
    {
        cJSON_Delete(result);
        device_responseERROR(req);
        return EAT_FALSE;
    }

    cJSON_AddNumberToObject(json_root, "code", 0);

    cJSON_AddNumberToObject(result, "sw", setting.isAutodefendFixed);
    cJSON_AddNumberToObject(result, "period", setting.autodefendPeriod);
    cJSON_AddItemToObject(json_root, "result", result);

    buffer = cJSON_PrintUnformatted(json_root);
    if(!buffer)
    {
        cJSON_Delete(json_root);
        device_responseERROR(req);
        return EAT_FALSE;
    }

    cJSON_Delete(json_root);
    msgLen = sizeof(MSG_DEVICE_RSP) + strlen(buffer);
    rsp = alloc_device_msg((MSG_HEADER*)req, msgLen);

    if(!rsp)
    {
        device_responseERROR(req);
        return EAT_FALSE;
    }

    strncpy(rsp->data, buffer, strlen(buffer));
    free(buffer);

    socket_sendDataDirectly(rsp, msgLen);
    return 0;
}

static int device_SetDeffend(const void* req, cJSON *param)
{
    cJSON *defend = NULL;

    if(!param)
    {
        device_responseERROR(req);
        return EAT_FALSE;
    }

    defend = cJSON_GetObjectItem(param, "defend");
    if(defend)
    {
        set_vibration_state(defend->valueint ? EAT_TRUE : EAT_FALSE);
        device_responseOK(req);
    }
    else
    {
        device_responseERROR(req);
    }

    return 0;
}

static int device_GetDeffend(const void* req, cJSON *param)
{
    cJSON *result = NULL;
    cJSON *json_root = NULL;

    char *buffer = NULL;
    int msgLen = 0;
    MSG_DEVICE_RSP *rsp = NULL;

    result = cJSON_CreateObject();
    if(!result)
    {
        device_responseERROR(req);
        return EAT_FALSE;
    }

    json_root = cJSON_CreateObject();
    if(!json_root)
    {
        cJSON_Delete(result);
        device_responseERROR(req);
        return EAT_FALSE;
    }

    cJSON_AddNumberToObject(json_root, "code", 0);

    cJSON_AddNumberToObject(result, "defend", setting.isVibrateFixed);
    cJSON_AddItemToObject(json_root, "result", result);

    buffer = cJSON_PrintUnformatted(json_root);
    if(!buffer)
    {
        cJSON_Delete(json_root);
        device_responseERROR(req);
        return EAT_FALSE;
    }

    cJSON_Delete(json_root);
    msgLen = sizeof(MSG_DEVICE_RSP) + strlen(buffer);
    rsp = alloc_device_msg((MSG_HEADER*)req, msgLen);

    if(!rsp)
    {
        device_responseERROR(req);
        return EAT_FALSE;
    }

    strncpy(rsp->data, buffer, strlen(buffer));
    free(buffer);

    socket_sendDataDirectly(rsp, msgLen);
    return 0;
}

static int device_GetBattery(const void* req, cJSON *param)
{
    cJSON *result = NULL;
    cJSON *json_root = NULL;

    char *buffer = NULL;
    int msgLen = 0;
    MSG_DEVICE_RSP *rsp = NULL;

    result = cJSON_CreateObject();
    if(!result)
    {
        device_responseERROR(req);
        return EAT_FALSE;
    }

    json_root = cJSON_CreateObject();
    if(!json_root)
    {
        cJSON_Delete(result);
        device_responseERROR(req);
        return EAT_FALSE;
    }

    cJSON_AddNumberToObject(json_root, "code", 0);

    cJSON_AddNumberToObject(result, "percent", battery_getPercent());
    cJSON_AddItemToObject(json_root, "result", result);

    buffer = cJSON_PrintUnformatted(json_root);
    if(!buffer)
    {
        cJSON_Delete(json_root);
        device_responseERROR(req);
        return EAT_FALSE;
    }

    cJSON_Delete(json_root);
    msgLen = sizeof(MSG_DEVICE_RSP) + strlen(buffer);
    rsp = alloc_device_msg((MSG_HEADER*)req, msgLen);

    if(!rsp)
    {
        device_responseERROR(req);
        return EAT_FALSE;
    }

    strncpy(rsp->data, buffer, strlen(buffer));
    free(buffer);

    socket_sendDataDirectly(rsp, msgLen);
    return 0;
}

static int device_SetBatteryType(const void* req, cJSON *param)
{
    cJSON *batterytype = NULL;

    if(!param)
    {
        device_responseERROR(req);
        return EAT_FALSE;
    }

    batterytype = cJSON_GetObjectItem(param, "batterytype");
    if(batterytype)
    {
        set_battery_type(batterytype->valueint);
        device_responseOK(req);
    }
    else
    {
        device_responseERROR(req);
    }

    return 0;
}

static int device_StartRecord(const void* req, cJSON *param)
{
    record_start();

    return device_responseOK(req);
}

static int device_StopRecord(const void* req, cJSON *param)
{
    record_stop();

    return device_responseOK(req);
}

static int device_SetBluetoothId(const void* req, cJSON *param)
{
    cJSON *bluetoothId = NULL;
    u8 msgLen = sizeof(MSG_THREAD);
    MSG_THREAD *msg = NULL;

    msg = allocMsg(msgLen);
    if(!msg)
    {
        return device_responseERROR(req);
    }

    msg->cmd = CMD_THREAD_BLUETOOTHRESET;
    msg->length = 0;

    if(!param)
    {
        return device_responseERROR(req);
    }
    bluetoothId = cJSON_GetObjectItem(param, "bluetoothId");
    set_bluetooth_id(bluetoothId->valuestring);

    sendMsg(THREAD_BLUETOOTH, msg, msgLen);

    return device_responseOK(req);
}

static int device_DownloadAudioFile(const void* req, cJSON *param)
{
    cJSON *use = NULL;
    cJSON *fileName = NULL;
    char fileNameString [32]= {0};
    if(!param)
    {
        return device_responseERROR(req);
    }
    use = cJSON_GetObjectItem(param, "use");
    if(use->valueint == 0)
    {
        fileName = cJSON_GetObjectItem(param, "fileName");
        strncpy(fileNameString, fileName->valuestring,32);
        ftp_download_file("close_audio.amr", fileNameString);
    }
    else if(use->valueint == 1)
    {
        fileName = cJSON_GetObjectItem(param, "fileName");
        strncpy(fileNameString, fileName->valuestring,32);
        ftp_download_file("far_audio.amr", fileNameString);
    }
    else if(use->valueint == 3)
    {
        eat_fs_Delete(AUDIO_FILE_NAME_FOUND);
        eat_fs_Delete(AUDIO_FILE_NAME_LOST);
    }
    return device_responseOK(req);
}

static int device_SetBlutoothSwitch(const void* req, cJSON *param)
{
    cJSON *bluetoothSwitch = NULL;

    if(!param)
    {
        return device_responseERROR(req);
    }

    bluetoothSwitch = cJSON_GetObjectItem(param, "sw");
    set_bluetooth_switch(bluetoothSwitch->valueint?EAT_TRUE:EAT_FALSE);

    return device_responseOK(req);
}

static int device_StartAlarm(const void* req, cJSON *param)
{
    audio_StartAlarmSound();

    return device_responseOK(req);
}

static int device_ControlCarLocked(const void* req, cJSON *param)
{
    cJSON *Switch = NULL;

    if(!param)
    {
        return device_responseERROR(req);
    }

    Switch = cJSON_GetObjectItem(param, "sw");
    if(1 == Switch->valueint)
    {
        telecontrol_lock();
    }
    else
    {
        telecontrol_unlock();
    }

    return device_responseOK(req);
}

static DEVICE_MSG_PROC deviceProcs[] =
{
    {DEVICE_GET_DEVICEINFO,    device_GetDeviceInfo},
    {DEVICE_GET_LOCATE,        device_GetLocation},
    {DEVICE_SET_AUTOLOCK,      device_SetAutolock},
    {DEVICE_GET_AUTOLOCK,      device_GetAutolock},
    {DEVICE_SET_DEFENDSTATE,   device_SetDeffend},
    {DEVICE_GET_DEFENDSTATE,   device_GetDeffend},
    {DEVICE_GET_BATTERY,       device_GetBattery},
    {DEVICE_SET_BATTERYTYPE,   device_SetBatteryType},
    {DEVICE_START_RECORD,      device_StartRecord},
    {DEVICE_STOP_RECORD,       device_StopRecord},
    {DEVICE_SET_BLUETOOTHID,   device_SetBluetoothId},
    {DEVICE_DOWNLOAD_AUDIOFILE,device_DownloadAudioFile},
    {DEVICE_SET_BLUETOOTHSW,   device_SetBlutoothSwitch},
    {DEVICE_START_ALARM,       device_StartAlarm},
    {DEVICE_CONTROL_LOCK,      device_ControlCarLocked}
};

int cmd_device_handler(const void* msg)
{
    int i =0;
    char cmd = 0;
    cJSON *json_cmd = NULL;
    cJSON *json_root = NULL;
    cJSON *json_param = NULL;
    MSG_DEVICE_REQ *req = (MSG_DEVICE_REQ *)msg;

    json_root = cJSON_Parse(req->data);
    if(!json_root)
    {
        LOG_ERROR("content is not json type");
        return -1;
    }

    json_cmd = cJSON_GetObjectItem(json_root, "c");
    if(!json_cmd)
    {
        cJSON_Delete(json_root);
        LOG_ERROR("no cmd in content");
        return -1;
    }
    cmd = json_cmd->valueint;

    json_param = cJSON_GetObjectItem(json_root, "param");

    for (i = 0; i < sizeof(deviceProcs) / sizeof(deviceProcs[0]); i++)
    {
        if (deviceProcs[i].cmd == json_cmd->valueint)
        {
            DEVICE_PROC pfn = deviceProcs[i].pfn;
            if (pfn)
            {
                pfn(msg, json_param);
                return 0;
            }
            else
            {
                LOG_ERROR("Message %d not processed!", cmd);
                return -1;
            }
        }
    }

    LOG_ERROR("unknown device type %d!", cmd);
    return -1;
}
