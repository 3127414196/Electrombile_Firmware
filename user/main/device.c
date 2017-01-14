/*
 * device.c
 *
 *  Created on: 2016/12/15
 *      Author: lc
 */

#include <stdio.h>
#include <string.h>

#include <eat_interface.h>
#include "cJSON.h"

#include "client.h"
#include "socket.h"
#include "msg.h"
#include "log.h"
#include "uart.h"
#include "setting.h"
#include "thread.h"
#include "thread_msg.h"
#include "timer.h"
#include "msg_queue.h"
#include "device.h"
#include "record.h"
#include "protocol.h"
#include "data.h"
#include "response.h"

enum
{
    DEVICE_GET_DEVICEINFO   = 0,
    DEVICE_GET_LOCATE       = 1,
    DEVICE_SET_AUTOLOCK     = 2,
    DEVICE_GET_AUTOLOCK     = 3,
    DEVICE_SET_DEFENDSTATE  = 4,
    DEVICE_GET_DEFENDSTATE  = 5,
    DEVICE_GET_BATTERY      = 6,
    DEVICE_SET_BATTERYTYPE  = 7,
    DEVICE_START_RECORD     = 8,
    DEVICE_STOP_RECORD      = 9,
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
    u8 msgLen = sizeof(MSG_THREAD) + sizeof(DEVICE_LOCATION_SEQ);
    MSG_THREAD* msg = allocMsg(msgLen);
    DEVICE_LOCATION_SEQ *seq;
    seq->seq = *((char *)msg->data);

    msg->cmd = CMD_THREAD_DEVICE_LOCATION;
    msg->length = 0;

    seq->seq = ((MSG_DEVICE_REQ *)req)->header.seq;

    LOG_DEBUG("send CMD_THREAD_DEVICE_LOCATION to THREAD_GPS.");
    sendMsg(THREAD_GPS, msg, msgLen);

    return 0;
}

static int device_SetAutolock(const void* req, cJSON *param)
{
    if(!param)
    {
        device_responseERROR(req);
        return EAT_FALSE;
    }
    setting.isAutodefendFixed = cJSON_GetObjectItem(param, "sw")->valueint ? EAT_TRUE : EAT_FALSE;
    setting.autodefendPeriod = cJSON_GetObjectItem(param, "period")->valueint;

    setting_save();

    device_responseOK(req);

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
    if(!param)
    {
        device_responseERROR(req);
        return EAT_FALSE;
    }

    setting.isVibrateFixed = cJSON_GetObjectItem(param, "defend")->valueint ? EAT_TRUE : EAT_FALSE;
    setting_save();

    device_responseOK(req);

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
    if(!param)
    {
        device_responseERROR(req);
        return EAT_FALSE;
    }

    set_battery_type(cJSON_GetObjectItem(param, "batterytype")->valueint);

    device_responseOK(req);

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

static DEVICE_MSG_PROC deviceProcs[] =
{
    {DEVICE_GET_DEVICEINFO, device_GetDeviceInfo},
    {DEVICE_GET_LOCATE,     device_GetLocation},
    {DEVICE_SET_AUTOLOCK,   device_SetAutolock},
    {DEVICE_GET_AUTOLOCK,   device_GetAutolock},
    {DEVICE_SET_DEFENDSTATE,device_SetDeffend},
    {DEVICE_GET_DEFENDSTATE,device_GetDeffend},
    {DEVICE_GET_BATTERY,    device_GetBattery},
    {DEVICE_SET_BATTERYTYPE,device_SetBatteryType},
    {DEVICE_START_RECORD,   device_StartRecord},
    {DEVICE_STOP_RECORD,    device_StopRecord},
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

int device_sendGPS(char seq)
{
    cJSON *result = NULL;
    cJSON *json_root = NULL;
    cJSON *gps = NULL;
    MSG_DEVICE_REQ *req = NULL;
    MSG_DEVICE_RSP *rsp = NULL;
    int msgLen = 0;
    char *buffer = NULL;
    LOCAL_GPS* last_gps = gps_get_last();

    req = (MSG_DEVICE_REQ *)malloc(sizeof(MSG_DEVICE_REQ)+msgLen);
    req->header.seq = seq;
    req->header.signature = htons(START_FLAG);
    req->header.length = msgLen;
    req->header.cmd = CMD_DEVICE_INFO_GET;

    gps = cJSON_CreateObject();
    if(!gps)
    {
        device_responseERROR(req);
        return EAT_FALSE;
    }

    result = cJSON_CreateObject();
    if(!result)
    {
        cJSON_Delete(gps);
        device_responseERROR(req);
        return EAT_FALSE;
    }

    json_root = cJSON_CreateObject();
    if(!json_root)
    {
        cJSON_Delete(gps);
        cJSON_Delete(result);
        cJSON_Delete(json_root);
        device_responseERROR(req);
        return EAT_FALSE;
    }

    cJSON_AddNumberToObject(gps, "timestamp", rtc_getTimestamp());
    cJSON_AddNumberToObject(gps, "lat", last_gps->gps.latitude);
    cJSON_AddNumberToObject(gps, "lng", last_gps->gps.longitude);
    cJSON_AddNumberToObject(gps, "speed", last_gps->gps.speed);
    cJSON_AddNumberToObject(gps, "course", last_gps->gps.course);
    cJSON_AddItemToObject(result, "gps", gps);

    cJSON_AddNumberToObject(json_root, "number", 0);
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
}
