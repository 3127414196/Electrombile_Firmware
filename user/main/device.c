/*
 * device.c
 *
 *  Created on: 2016/12/15
 *      Author: lc
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <eat_interface.h>
#include <eat_fs.h>
#include "cJSON.h"

#include "client.h"
#include "socket.h"
#include "fs.h"
#include "http.h"
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
#include "response.h"
#include "eat_modem.h"
#include "modem.h"

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
    DEVICCE_GET_SERVER       = 16,
    DEVICE_GET_GPS_SIGNAL    = 17,
    DEVICE_GET_GSM_SIGNAL    = 18,
    DEVICE_AT                = 19,
    DEVICE_GET_LOG           = 20,
    DEVICE_REBOOT            = 21,
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
    DEVICE_LOCATION_SEQ *seq = NULL;
    u8 msgLen = sizeof(MSG_THREAD) + sizeof(DEVICE_LOCATION_SEQ);

    MSG_THREAD* msg = allocMsg(msgLen);
    if(msg)
    {
        seq = (DEVICE_LOCATION_SEQ *)msg->data;

        msg->cmd = CMD_THREAD_DEVICE_LOCATION;
        msg->length = sizeof(DEVICE_LOCATION_SEQ);

        seq->seq = ((MSG_DEVICE_REQ *)req)->header.seq;

        LOG_DEBUG("send CMD_THREAD_DEVICE_LOCATION to THREAD_GPS.");
        sendMsg(THREAD_GPS, msg, msgLen);
    }
    else
    {
        device_responseERROR(req);
    }

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
        if(defend->valueint)
        {
            set_vibration_state(EAT_TRUE);
            telecontrol_switch_off();
        }
        else
        {
            set_vibration_state(EAT_FALSE);
            telecontrol_switch_on();
        }
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

    if(!param)
    {
        return device_responseERROR(req);
    }

    bluetoothId = cJSON_GetObjectItem(param, "bluetoothId");
    if(bluetoothId)
    {
        set_bluetooth_id(bluetoothId->valuestring);
        bluetooth_resetState();
        device_responseOK(req);
    }
    else
    {
        device_responseERROR(req);
    }

    return 0;
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
        http_download_file("close_audio.amr", fileNameString);
    }
    else if(use->valueint == 1)
    {
        fileName = cJSON_GetObjectItem(param, "fileName");
        strncpy(fileNameString, fileName->valuestring,32);
        http_download_file("far_audio.amr", fileNameString);
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

int device_sendGPS(const MSG_THREAD* msg)
{
    int msgLen = 0;
    char *buffer = NULL;
    MSG_DEVICE_REQ req = {0};

    cJSON *gps = NULL;
    cJSON *result = NULL;
    cJSON *json_root = NULL;
    MSG_DEVICE_RSP *rsp = NULL;

    LOCAL_GPS* last_gps = gps_get_last();
    DEVICE_LOCATION_SEQ *seq = (DEVICE_LOCATION_SEQ *)msg->data;

    req.header.seq = seq->seq;
    req.header.signature = htons(START_FLAG);
    req.header.length = 0;
    req.header.cmd = CMD_DEVICE;

    gps = cJSON_CreateObject();
    if(!gps)
    {
        device_responseERROR(&req);
        return -1;
    }

    result = cJSON_CreateObject();
    if(!result)
    {
        cJSON_Delete(gps);
        device_responseERROR(&req);
        return -1;
    }

    json_root = cJSON_CreateObject();
    if(!json_root)
    {
        cJSON_Delete(gps);
        cJSON_Delete(result);
        cJSON_Delete(json_root);
        device_responseERROR(&req);
        return -1;
    }

    cJSON_AddNumberToObject(gps, "timestamp", rtc_getTimestamp());
    cJSON_AddNumberToObject(gps, "lat", last_gps->gps.latitude);
    cJSON_AddNumberToObject(gps, "lng", last_gps->gps.longitude);
    cJSON_AddNumberToObject(gps, "speed", last_gps->gps.speed);
    cJSON_AddNumberToObject(gps, "course", last_gps->gps.course);
    cJSON_AddItemToObject(result, "gps", gps);

    cJSON_AddItemToObject(json_root, "result", result);

    buffer = cJSON_PrintUnformatted(json_root);
    if(!buffer)
    {
        cJSON_Delete(json_root);
        device_responseERROR(&req);
        return -1;
    }
    cJSON_Delete(json_root);

    msgLen = sizeof(MSG_DEVICE_RSP) + strlen(buffer);
    rsp = alloc_device_msg((MSG_HEADER*)&req, msgLen);
    if(!rsp)
    {
        device_responseERROR(&req);
        return -1;
    }

    strncpy(rsp->data, buffer, strlen(buffer));
    free(buffer);

    socket_sendDataDirectly(rsp, msgLen);

    return 0;
}

static int device_GetServer(const void* req, cJSON *param)
{
    char *buffer = NULL;
    char server[MAX_DOMAIN_NAME_LEN] = {0};
    MSG_DEVICE_RSP *rsp = NULL;
    int msgLen = 0;

    cJSON *result = NULL;
    cJSON *autodefend = NULL;
    cJSON *json_root = NULL;

    result = cJSON_CreateObject();
    if(!result)
    {
        device_responseERROR(req);
        return -1;
    }

    autodefend = cJSON_CreateObject();
    if(!autodefend)
    {
        device_responseERROR(req);
        cJSON_Delete(result);
    }

    json_root = cJSON_CreateObject();
    if(!json_root)
    {
        cJSON_Delete(result);
        cJSON_Delete(autodefend);
        device_responseERROR(req);
        return -1;
    }

    if(setting.addr_type == ADDR_TYPE_DOMAIN)
    {
        snprintf(server, MAX_DOMAIN_NAME_LEN, "%s:%d", setting.ftp_domain, setting.port);
    }
    else if(setting.addr_type == ADDR_TYPE_IP)
    {
        snprintf(server, MAX_DOMAIN_NAME_LEN, "%u.%u.%u.%u:%d", setting.ipaddr[0], setting.ipaddr[1], setting.ipaddr[2], setting.ipaddr[3], setting.port);
    }


    cJSON_AddNumberToObject(json_root, "code", 0);
    cJSON_AddStringToObject(result, "server", server);
    cJSON_AddNumberToObject(autodefend, "sw", setting.isAutodefendFixed);
    cJSON_AddNumberToObject(autodefend, "period", setting.autodefendPeriod);
    cJSON_AddItemToObject(result, "autodefend", autodefend);
    cJSON_AddNumberToObject(result, "defend", setting.isVibrateFixed);
    cJSON_AddStringToObject(result, "BTAddress", setting.BluetoothId);

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

static int device_GetGpsSignal(const void* req, cJSON *param)
{
    DEVICE_LOCATION_SEQ *seq = NULL;
    u8 msgLen = sizeof(MSG_THREAD) + sizeof(DEVICE_LOCATION_SEQ);

    MSG_THREAD* msg = allocMsg(msgLen);
    if(msg)
    {
        seq = (DEVICE_LOCATION_SEQ *)msg->data;

        msg->cmd = CMD_THREAD_DEVICE_GPSHODP;
        msg->length = sizeof(DEVICE_LOCATION_SEQ);

        seq->seq = ((MSG_DEVICE_REQ *)req)->header.seq;

        LOG_DEBUG("send CMD_THREAD_DEVICE_GPSHODP to THREAD_GPS.");
        sendMsg(THREAD_GPS, msg, msgLen);
    }
    else
    {
        device_responseERROR(req);
    }
    return 0;
}

int device_sendGPSSignal(const MSG_THREAD* msg)
{
    int msgLen = 0;
    char *buffer = NULL;
    MSG_DEVICE_REQ req = {0};

    cJSON *result = NULL;
    cJSON *json_root = NULL;
    MSG_DEVICE_RSP *rsp = NULL;
    GPS_HDOP_INFO *data = (GPS_HDOP_INFO *)msg->data;

    req.header.seq = data->managerSeq;
    req.header.signature = htons(START_FLAG);
    req.header.length = 0;
    req.header.cmd = CMD_DEVICE;

    result = cJSON_CreateObject();
    if(!result)
    {
        device_responseERROR(&req);
        return -1;
    }

    json_root = cJSON_CreateObject();
    if(!json_root)
    {
        cJSON_Delete(result);
        cJSON_Delete(json_root);
        device_responseERROR(&req);
        return -1;
    }

    cJSON_AddNumberToObject(result, "GPSSignal", data->hdop);
    cJSON_AddNumberToObject(json_root, "code", 0);
    cJSON_AddItemToObject(json_root, "result", result);

    buffer = cJSON_PrintUnformatted(json_root);
    if(!buffer)
    {
        cJSON_Delete(json_root);
        device_responseERROR(&req);
        return -1;
    }
    cJSON_Delete(json_root);

    msgLen = sizeof(MSG_DEVICE_RSP) + strlen(buffer);
    rsp = alloc_device_msg((MSG_HEADER*)&req, msgLen);
    if(!rsp)
    {
        device_responseERROR(&req);
        return -1;
    }

    strncpy(rsp->data, buffer, strlen(buffer));
    free(buffer);

    socket_sendDataDirectly(rsp, msgLen);

    return 0;
}


static int device_GetGsmSignal(const void* req, cJSON *param)
{
    cJSON *root = NULL;
    cJSON *json_root = NULL;

    char *buffer = NULL;
    MSG_DEVICE_RSP *rsp = NULL;
    int msgLen = 0;

    root = cJSON_CreateObject();
    if(!root)
    {
        device_responseERROR(req);
        return -1;
    }

    json_root = cJSON_CreateObject();
    if(!json_root)
    {
        cJSON_Delete(root);
        device_responseERROR(req);
        return -1;
    }

    cJSON_AddNumberToObject(root, "GSMSignal", diag_gsm_get());

    cJSON_AddNumberToObject(json_root, "code", 0);
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

static int device_AT(const void* req, cJSON *param)
{
    DEVICE_LOCATION_SEQ *seq = NULL;
    u8 msgLen = sizeof(MSG_THREAD) + sizeof(DEVICE_LOCATION_SEQ);
    u8 buf[READ_BUFF_SIZE] = {0};
    unsigned short len = 0;
    unsigned short rc = 0;
    char *buf_AT = NULL;

    MSG_THREAD* msg = allocMsg(msgLen);
    if(msg)
    {
        seq = (DEVICE_LOCATION_SEQ *)msg->data;

        msg->cmd = CMD_THREAD_DEVICE_GET_AT;
        msg->length = sizeof(DEVICE_LOCATION_SEQ);

        seq->seq = ((MSG_DEVICE_REQ *)req)->header.seq;
        buf_AT = cJSON_GetObjectItem(param, "AT")->valuestring;

        len = strlen(buf_AT);
        LOG_DEBUG("%s %d", buf_AT, len);
        rc = eat_modem_write(buf_AT, rc);
        debug_proc(buf_AT, len);

        LOG_DEBUG("send CMD_THREAD_DEVICE_AT to THREAD_MAIN.");
        sendMsg(THREAD_MAIN, msg, msgLen);
    }
    else
    {
        device_responseERROR(req);
    }
}

int device_sendAT(const MSG_THREAD* msg)
{
    int msgLen = 0;
    char *buffer = NULL;
    MSG_DEVICE_REQ req = {0};

    cJSON *root = NULL;
    cJSON *json_root = NULL;
    DEVICE_AT_RD *data = NULL;
    MSG_DEVICE_RSP *rsp = NULL;
    data = (DEVICE_AT_RD *)msg->data;
    if(!data)
    {
        LOG_DEBUG("ERROR!");
        return -1;
    }
    req.header.seq = data->seq - 48;    //subtract 0 ASCLL
    req.header.signature = htons(START_FLAG);
    req.header.length = 0;
    req.header.cmd = CMD_DEVICE;

    root = cJSON_CreateObject();
    if(!root)
    {
        device_responseERROR(&req);
        return -1;
    }

    json_root = cJSON_CreateObject();
    if(!json_root)
    {
        cJSON_Delete(root);
        device_responseERROR(&req);
        return -1;
    }

    cJSON_AddStringToObject(root, "response", data->AT_RD);
    cJSON_AddNumberToObject(json_root, "code", 0);
    cJSON_AddItemToObject(json_root, "result", root);
    buffer = cJSON_PrintUnformatted(json_root);
    buffer = cJSON_PrintUnformatted(json_root);
    if(!buffer)
    {
        cJSON_Delete(json_root);
        device_responseERROR(&req);
        return EAT_FALSE;
    }

    cJSON_Delete(json_root);
    msgLen = sizeof(MSG_DEVICE_RSP) + strlen(buffer);
    rsp = alloc_device_msg((MSG_HEADER*)&req, msgLen);

    if(!rsp)
    {
        device_responseERROR(&req);
        return EAT_FALSE;
    }

    strncpy(rsp->data, buffer, strlen(buffer));
    free(buffer);

    socket_sendDataDirectly(rsp, msgLen);
    return 0;
}

static int device_GetLog(const void* req, cJSON *param)
{
    cJSON *root = NULL;
    cJSON *json_root = NULL;

    char *buffer = NULL;
    MSG_DEVICE_RSP *rsp = NULL;
    int msgLen = 0;
    int rc = 0;
    char buf_log[MAX_DEBUG_BUF_LEN] = {0};

    rc = log_GetLog(buf_log,MAX_DEBUG_BUF_LEN);
    if(strlen(buf_log) > 220)   //256-36(other massage len)
    {
         LOG_DEBUG("data is to large to send");
         buf_log[218] = '\0';
    }
    if(MSG_SUCCESS > rc)
    {
        LOG_ERROR("get log file error");
        device_responseERROR(req);
        return -1;
    }

    root = cJSON_CreateObject();
    if(!root)
    {
        device_responseERROR(req);
        return -1;
    }

    json_root = cJSON_CreateObject();
    if(!json_root)
    {
        cJSON_Delete(root);
        device_responseERROR(req);
        return -1;
    }

    cJSON_AddStringToObject(root, "log", buf_log);
    cJSON_AddNumberToObject(json_root, "code", 0);
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

static int device_reboot(const void* req, cJSON *param)
{
    LOG_DEBUG("reboot.");

    eat_reset_module();

    device_responseOK(req);
    return 0;
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
    {DEVICE_CONTROL_LOCK,      device_ControlCarLocked},
    {DEVICCE_GET_SERVER,       device_GetServer},
    {DEVICE_GET_GPS_SIGNAL,    device_GetGpsSignal},
    {DEVICE_GET_GSM_SIGNAL,    device_GetGsmSignal},
    {DEVICE_AT,                device_AT},
    {DEVICE_GET_LOG,           device_GetLog},
    {DEVICE_REBOOT,            device_reboot},
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

