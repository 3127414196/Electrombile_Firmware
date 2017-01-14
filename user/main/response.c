/*
 * response.c
 *
 *  Created on: 2016年2月4日
 *      Author: jk
 */
#include <string.h>
#include <stdint.h>


#include <eat_interface.h>

#include "response.h"
#include "msg.h"
#include "log.h"
#include "thread_msg.h"
#include "thread.h"
#include "socket.h"
#include "fsm.h"
#include "version.h"
#include "upgrade.h"
#include "adler32.h"
#include "data.h"
#include "fs.h"
#include "setting.h"
#include "battery.h"
#include "request.h"
#include "timer.h"
#include "msg_queue.h"
#include "diagnosis.h"
#include "mem.h"
#include "modem.h"
#include "data.h"

int cmd_Login_rsp(const void* msg)
{
    LOG_DEBUG("get login respond.");

    fsm_run(EVT_LOGINED);

    return 0;
}

int cmd_Ping_rsp(const void* msg)
{
    LOG_DEBUG("get ping respond.");

    return 0;
}

int cmd_SimInfo_rsp(const void* msg)
{
    LOG_DEBUG("get siminfo respond.");

    msg_ack(msg);

    return 0;
}

int cmd_Alarm_rsp(const void* msg)
{
    LOG_DEBUG("get alarm respond");

    msg_ack(msg);

    return 0;
}

int cmd_Sms_rsp(const void* msg)
{
    return 0;
}


int cmd_Reboot_rsp(const void* msg)
{
    MSG_HEADER* req = (MSG_HEADER*)msg;
    MSG_HEADER* rsp = NULL;

    LOG_DEBUG("reboot.");
    rsp = alloc_rspMsg(req);
    if (!rsp)
    {
        LOG_ERROR("alloc reboot rsp message failed!");
        return -1;
    }

    socket_sendDataDirectly(rsp, sizeof(MSG_HEADER));

    eat_reset_module();
    return 0;
}

int cmd_Location_rsp(const void* msgLocation)
{
    u8 msgLen = sizeof(MSG_THREAD);
    MSG_THREAD* msg = allocMsg(msgLen);

    msg->cmd = CMD_THREAD_LOCATION;
    msg->length = 0;

    LOG_DEBUG("send CMD_THREAD_LOCATION to THREAD_GPS.");
    sendMsg(THREAD_GPS, msg, msgLen);

    return 0;
}

int cmd_AutodefendSwitchSet_rsp(const void* msg)
{
    MSG_AUTODEFEND_SWITCH_SET_REQ* req = (MSG_AUTODEFEND_SWITCH_SET_REQ*)msg;
    MSG_AUTODEFEND_SWITCH_SET_RSP* rsp = NULL;

    if(req->onOff == AUTO_DEFEND_ON)
    {
        set_autodefend_state(EAT_TRUE);
        LOG_DEBUG("set autodefend swtich on.");
    }
    else if(req->onOff == AUTO_DEFEND_OFF)
    {
        set_autodefend_state(EAT_FALSE);
        LOG_DEBUG("set autodefend swtich off.");
    }

    rsp = alloc_rspMsg(&req->header);
    if (!rsp)
    {
        LOG_ERROR("alloc autodefend_swtich_set rsp message failed!");
        return -1;
    }

    rsp->token = req->token;
    rsp->result = MSG_SUCCESS;

    socket_sendDataDirectly(rsp, sizeof(MSG_AUTODEFEND_SWITCH_SET_RSP));

    return 0;
}

int cmd_AutodefendSwitchGet_rsp(const void* msg)
{
    MSG_AUTODEFEND_SWITCH_GET_REQ* req = (MSG_AUTODEFEND_SWITCH_GET_REQ*)msg;
    MSG_AUTODEFEND_SWITCH_GET_RSP* rsp = NULL;

    rsp = alloc_rspMsg(&req->header);
    if (!rsp)
    {
        LOG_ERROR("alloc autodefend_swtich_get rsp message failed!");
        return -1;
    }

    rsp->token = req->token;
    rsp->result = get_autodefend_state() ? AUTO_DEFEND_ON : AUTO_DEFEND_OFF;

    socket_sendDataDirectly(rsp, sizeof(MSG_AUTODEFEND_SWITCH_GET_RSP));

    return 0;
}

int cmd_AutodefendPeriodSet_rsp(const void* msg)
{
    MSG_AUTODEFEND_PERIOD_SET_REQ* req = (MSG_AUTODEFEND_PERIOD_SET_REQ*)msg;
    MSG_AUTODEFEND_PERIOD_SET_RSP* rsp = NULL;

    LOG_DEBUG("set autodefend period as %dmins.", req->period);
    set_autodefend_period(req->period);

    rsp = alloc_rspMsg(&req->header);
    if (!rsp)
    {
        LOG_ERROR("alloc autodefend_period_set rsp message failed!");
        return -1;
    }

    rsp->token = req->token;
    rsp->result = MSG_SUCCESS;

    socket_sendDataDirectly(rsp, sizeof(MSG_AUTODEFEND_PERIOD_SET_RSP));

    return 0;
}

int cmd_AutodefendPeriodGet_rsp(const void* msg)
{
    MSG_AUTODEFEND_PERIOD_GET_REQ* req = (MSG_AUTODEFEND_PERIOD_GET_REQ*)msg;
    MSG_AUTODEFEND_PERIOD_GET_RSP* rsp = NULL;

    rsp = alloc_rspMsg(&req->header);
    if (!rsp)
    {
        LOG_ERROR("alloc autodefend_period_get rsp message failed!");
        return -1;
    }

    rsp->token = req->token;
    rsp->period = get_autodefend_period();
    LOG_DEBUG("send autodefend period: %d minutes",rsp->period);
    socket_sendDataDirectly(rsp, sizeof(MSG_AUTODEFEND_PERIOD_GET_RSP));

    return 0;
}

int cmd_SetServer_rsp(const void* msg)
{
    MSG_SET_SERVER_REQ *req = (MSG_SET_SERVER_REQ*)msg;
    MSG_SET_SERVER_RSP*rsp = NULL;
    u32 ip[4] = {0};
    u32 port = 0;

    int count = sscanf(req->data,"%u.%u.%u.%u:%u",&ip[0],&ip[1],&ip[2],&ip[3],&port);
    if(5 == count)
    {
        setting.addr_type = ADDR_TYPE_IP;
        setting.ipaddr[0] = (u8)ip[0];
        setting.ipaddr[1] = (u8)ip[1];
        setting.ipaddr[2] = (u8)ip[2];
        setting.ipaddr[3] = (u8)ip[3];
        setting.port = (u16)port;

        setting_save();

        rsp = alloc_msg(req->header.cmd,sizeof(MSG_SET_SERVER_RSP));
        if (!rsp)
        {
            LOG_ERROR("alloc rsp message failed!");
            return -1;
        }
        rsp->managerSeq = req->managerSeq;
        socket_sendDataDirectly(rsp, sizeof(MSG_SET_SERVER_RSP));
    }
}

int cmd_Battery_rsp(const void* msg)
{
    MSG_BATTERY_RSP* rsp = alloc_msg(CMD_BATTERY, sizeof(MSG_BATTERY_RSP));
    if (!rsp)
    {
        LOG_ERROR("alloc battery_msg rsp message failed!");
        return -1;
    }
    rsp->percent = battery_getPercent();
    rsp->miles = 0;

    LOG_DEBUG("send battery msg: %d",battery_getPercent());

    socket_sendDataDirectly(rsp, sizeof(MSG_BATTERY_RSP));

    return 0;
}

int cmd_GetBattery_rsp(const void* msg)
{
    u8 msgLen = 0;
    char buf[MAX_DEBUG_BUF_LEN] = {0};
    MSG_GET_GPS_RSP* battery_msg = NULL;
    MSG_GET_HEADER *server_msg = (MSG_GET_HEADER*)msg;

    snprintf(buf,MAX_DEBUG_BUF_LEN,"Volatge: %d, percent: %d, miles: %d",battery_getVoltage(),battery_getPercent(),0);

    msgLen = sizeof(MSG_GET_HEADER) + strlen(buf) + 1;
    battery_msg = alloc_msg(CMD_GET_BATTERY,msgLen);
    if (!battery_msg)
    {
        LOG_ERROR("alloc LogInfo rsp message failed!");
        return -1;
    }
    battery_msg->managerSeq = server_msg->managerSeq;
    strncpy(battery_msg->data,buf,strlen(buf)+1);
    socket_sendDataDirectly(battery_msg, msgLen);

    return 0;
}

int cmd_LogInfo_rsp(const void * msg)
{
    MSG_GET_LOG_REQ*req = (MSG_GET_LOG_REQ*)msg;
    MSG_GET_LOG_RSP*rsp = NULL;
    int msgLen;
    int rc = 0;
    char buf[MAX_DEBUG_BUF_LEN] = {0};

    rc = log_GetLog(buf,MAX_DEBUG_BUF_LEN);
    if(MSG_SUCCESS > rc)
    {
        LOG_ERROR("get log file error");
        return -1;
    }

    msgLen = sizeof(MSG_GET_HEADER) + strlen(buf) + 1;
    rsp = alloc_msg(req->header.cmd,msgLen);
    if (!rsp)
    {
        LOG_ERROR("alloc LogInfo rsp message failed!");
        return -1;
    }

    strncpy(rsp->data,buf,strlen(buf)+1);
    rsp->managerSeq = req->managerSeq;

    socket_sendDataDirectly(rsp, msgLen);
    return 0;
}

int cmd_GetSetting_rsp(const void * msg)
{
    MSG_GET_SETTING_REQ*req = (MSG_GET_SETTING_REQ*)msg;
    MSG_GET_SETTING_RSP*rsp = NULL;
    int msgLen;
    char buf[MAX_DEBUG_BUF_LEN] = {0};
    if(setting.addr_type == ADDR_TYPE_DOMAIN)
    {
        snprintf(buf,MAX_DEBUG_BUF_LEN,"server(%s:%d),isAutodefendFixed(%d),autodefendPeriod(%d),isVibrateFixed(%d)",\
            setting.domain,setting.port,setting.isAutodefendFixed,setting.autodefendPeriod,setting.isVibrateFixed);
    }
    else
    {
        snprintf(buf,MAX_DEBUG_BUF_LEN,"server(%d.%d.%d.%d:%d),isAutodefendFixed(%d),autodefendPeriod(%d),isVibrateFixed(%d)",\
            setting.ipaddr[0],setting.ipaddr[1],setting.ipaddr[2],setting.ipaddr[3],setting.port,setting.isAutodefendFixed,setting.autodefendPeriod,setting.isVibrateFixed);
    }

    msgLen = sizeof(MSG_GET_HEADER) + strlen(buf) + 1;
    rsp = alloc_msg(req->header.cmd,msgLen);
    if (!rsp)
    {
        LOG_ERROR("alloc LogInfo rsp message failed!");
        return -1;
    }

    strncpy(rsp->data,buf,strlen(buf)+1);
    rsp->managerSeq = req->managerSeq;

    socket_sendDataDirectly(rsp, msgLen);
    return 0;
}


int cmd_GSMSignal_rsp(const void * msg)
{
    MSG_GET_GSM_REQ*req = (MSG_GET_GSM_REQ*)msg;
    MSG_GET_GSM_RSP *rsp = NULL;
    char csq = diag_gsm_get();
    int msgLen;
    char buf[MAX_DEBUG_BUF_LEN] = {0};

    if(!buf)
    {
        LOG_ERROR("alloc buf space failed!");
        return -1;
    }

    snprintf(buf,MAX_DEBUG_BUF_LEN,"GSM signal %d",csq);

    msgLen = sizeof(MSG_GET_HEADER) + strlen(buf) + 1;
    rsp = alloc_msg(req->header.cmd,msgLen);
    if (!rsp)
    {
        LOG_ERROR("alloc LogInfo rsp message failed!");
        return -1;
    }

    strncpy(rsp->data,buf,strlen(buf)+1);
    rsp->managerSeq = req->managerSeq;

    socket_sendDataDirectly(rsp, msgLen);
    return 0;
}

int cmd_GPSSignal_rsp(const void * msg)
{
    u8 msgLen = sizeof(MSG_THREAD) + sizeof(MANAGERSEQ_INFO);
    MSG_GET_HEADER *server_msg = (MSG_GET_HEADER*)msg;
    MANAGERSEQ_INFO *data = NULL;
    MSG_THREAD* m = allocMsg(msgLen);

    m->cmd = CMD_THREAD_GPSHDOP;
    m->length = sizeof(MANAGERSEQ_INFO);
    data = (MANAGERSEQ_INFO*)m->data;
    data->managerSeq = server_msg->managerSeq;

    LOG_DEBUG("send CMD_THREAD_GPSHDOP to THREAD_GPS.");
    sendMsg(THREAD_GPS, m, msgLen);
    return 0;
}

int cmd_433Signal_rsp(const void * msg)
{
    MSG_GET_433_REQ*req = (MSG_GET_433_REQ*)msg;
    MSG_GET_433_RSP*rsp = NULL;
    short signal_433 = diag_433_get();
    char buf[MAX_DEBUG_BUF_LEN] = {0};
    int msgLen;

    snprintf(buf,MAX_DEBUG_BUF_LEN,"433 signal: %d",signal_433);

    msgLen = sizeof(MSG_GET_HEADER) + strlen(buf) + 1;
    rsp = alloc_msg(req->header.cmd,msgLen);
    if (!rsp)
    {
        LOG_ERROR("alloc LogInfo rsp message failed!");
        return -1;
    }

    strncpy(rsp->data,buf,strlen(buf)+1);
    rsp->managerSeq = req->managerSeq;

    socket_sendDataDirectly(rsp, msgLen);
    return 0;

}

int cmd_GetAT_rsp(const void* msg)
{
    MSG_GET_AT_REQ *req = (MSG_GET_AT_REQ*)msg;
    MSG_GET_AT_RSP*rsp = NULL;
    char buf[MAX_DEBUG_BUF_LEN] = {0};
    int msgLen;
    eat_bool rc = EAT_FALSE;

    set_manager_ATcmd_state(EAT_TRUE);
    set_manager_seq(req->managerSeq);

    rc = modem_AT(req->data);
    if(EAT_TRUE != rc)
    {
        snprintf(buf,MAX_DEBUG_BUF_LEN,"AT error!!");
        msgLen = sizeof(MSG_GET_HEADER) + strlen(buf) + 1;
        rsp = alloc_msg(req->header.cmd,msgLen);
        if (!rsp)
        {
            LOG_ERROR("alloc LogInfo rsp message failed!");
            return -1;
        }
        strncpy(rsp->data,buf,strlen(buf)+1);
        rsp->managerSeq = req->managerSeq;
        socket_sendDataDirectly(rsp, msgLen);
    }

    return 0;
}


int cmd_DefendOn_rsp(const void* msg)
{
    MSG_HEADER* req = (MSG_HEADER*)msg;
    MSG_DEFEND_ON_RSP* rsp = NULL;
    int result = MSG_SUCCESS;

    LOG_DEBUG("set defend switch on.");

    Reset_AlarmCount();
    set_vibration_state(EAT_TRUE);
    if(EAT_TRUE != vibration_fixed())
    {
        result = -1;
    }

    rsp = alloc_rspMsg(req);
    if (!rsp)
    {
        LOG_ERROR("alloc defend rsp message failed!");
        return -1;
    }

    rsp->result = result;

    socket_sendDataDirectly(rsp, sizeof(MSG_DEFEND_ON_RSP));

    return 0;
}

int cmd_DefendOff_rsp(const void* msg)
{
    MSG_HEADER* req = (MSG_HEADER*)msg;
    MSG_DEFEND_OFF_RSP* rsp = NULL;
    int result = MSG_SUCCESS;

    LOG_DEBUG("set defend switch off.");

    ResetVibrationTime();
    set_vibration_state(EAT_FALSE);
    if(EAT_FALSE != vibration_fixed())
    {
        result = -1;
    }

    rsp = alloc_rspMsg(req);
    if (!rsp)
    {
        LOG_ERROR("alloc defend rsp message failed!");
        return -1;
    }

    rsp->result = result;

    socket_sendDataDirectly(rsp, sizeof(MSG_DEFEND_OFF_RSP));

    return 0;
}
int cmd_DefendGet_rsp(const void* msg)
{
    MSG_HEADER* req = (MSG_HEADER*)msg;
    MSG_DEFEND_GET_RSP* rsp = NULL;
    unsigned char result = MSG_SUCCESS;

    result = vibration_fixed() ? DEFEND_ON : DEFEND_OFF;
    LOG_DEBUG("get defend switch state(%d).", result);

    rsp = alloc_rspMsg(req);
    if (!rsp)
    {
        LOG_ERROR("alloc defend rsp message failed!");
        return -1;
    }
    rsp->status= result;

    socket_sendDataDirectly(rsp, sizeof(MSG_DEFEND_GET_RSP));

    return 0;
}

int cmd_Timer_rsp(const void* msg)
{
    MSG_SET_TIMER_REQ* req = (MSG_SET_TIMER_REQ*)msg;
    MSG_SET_TIMER_RSP* rsp = NULL;
    if(0 >= ntohl(req->timer))
    {
        ;//rsp at the end
    }
    else if(10 >= ntohl(req->timer))
    {
        setting_save();

    }
    else if(21600 <= ntohl(req->timer))
    {
        setting_save();

    }
    else if((10 < ntohl(req->timer))&&(21600 > ntohl(req->timer)))
    {

        setting_save();

    }
    rsp = alloc_rspMsg(&req->header);

    //TODO: fix the gps upload time
    rsp->result = htonl(30);
    socket_sendDataDirectly(rsp,sizeof(MSG_SET_TIMER_RSP));

    return 0;
}

int cmd_Server_rsp(const void* msg)
{
    MSG_SET_SERVER* msg_server = (MSG_SET_SERVER*)msg;
    u32 ip[4] = {0};
    int count;
    signed char domain[MAX_DOMAIN_NAME_LEN] = {0};

    count = sscanf(msg_server->server,"%u.%u.%u.%u",&ip[0],&ip[1],&ip[2],&ip[3]);
    if(4 == count)
    {
        setting.addr_type = ADDR_TYPE_IP;
        setting.ipaddr[0] = (u8)ip[0];
        setting.ipaddr[1] = (u8)ip[1];
        setting.ipaddr[2] = (u8)ip[2];
        setting.ipaddr[3] = (u8)ip[3];
        setting.port = (u16)ntohl(msg_server->port);

        setting_save();
        LOG_DEBUG("server proc %s:%d successful!",msg_server->server,msg_server->port);

        eat_reset_module();
    }
    else
    {
        count = sscanf(msg_server->server, "%[abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.]", domain);
        if(1 == count)
        {
            setting.addr_type = ADDR_TYPE_DOMAIN;
            strncpy(setting.domain, msg_server->server,MAX_DOMAIN_NAME_LEN);
            setting.port = (u16)ntohl(msg_server->port);

            setting_save();
            LOG_DEBUG("server proc %s:%d successful!",msg_server->server,msg_server->port);

            eat_reset_module();
        }
        else
        {
            LOG_DEBUG("server proc %s:%d error!",msg_server->server,msg_server->port);
        }
    }

    return 0;
}

int cmd_UpgradeStart_rsp(const void* msg)
{
    MSG_UPGRADE_START* req = (MSG_UPGRADE_START*)msg;
    MSG_UPGRADE_START_RSP* rsp = NULL;
    int rc = 0;
    SINT64 freeDiskSize = 0;

    LOG_DEBUG("version: %#x -> %#x, file size: %d(%dkb)", VERSION_NUM, ntohl(req->version), ntohl(req->size), ntohl(req->size) / 1024);

    if (ntohl(req->version) <= VERSION_NUM)    //No need to upgrade, normally not happened
    {
        rc = MSG_VERSION_NOT_SUPPORTED;
        LOG_DEBUG("No need to upgrade%d:%d",ntohl(req->version),VERSION_NUM);
    }
    else
    {
        upgrade_DeleteOldApp();
        freeDiskSize = fs_getDiskFreeSize();
        if (ntohl(req->size) >= freeDiskSize)      //TODO: equal is not enough, maybe should reserve some disk space
        {
            LOG_ERROR("Free disk is not enough , can not start app_upgrade!");
            rc = MSG_DISK_NO_SPACE;
        }
        else
        {
            LOG_DEBUG("Free disk is enough , create file to start app_upgrade!");
            //创建升级包文件
            rc = upgrade_createFile();
        }
    }

    //回应是否可以升级
    rsp = alloc_rspMsg(msg);
    if (!rsp)
    {
        LOG_ERROR("alloc rsp msg failed:cmd =%d", req->header.cmd);
        return -1;
    }
    rsp->code = rc;
    socket_sendDataDirectly(rsp,sizeof(MSG_UPGRADE_START_RSP));

    return 0;
}

int cmd_UpgradeData_rsp(const void* msg)
{
    MSG_UPGRADE_DATA* req = (MSG_UPGRADE_DATA*)msg;
    MSG_UPGRADE_DATA_RSP* rsp = NULL;
    int rc = 0;
    int expectLength = 0;
    short data_length ;
    int data_offset = ntohl(req->offset);

    data_length = htons(req->header.length) - sizeof(req->offset);

    LOG_DEBUG("data length:%d , data offset:%d",data_length,data_offset);

    rc = upgrade_appendFile(data_offset, req->data, data_length);

    if (rc >= 0)
    {
        expectLength = data_offset + data_length;
        LOG_DEBUG("success this time, and expect length is:%d",expectLength);
    }
    else
    {
        expectLength = data_offset;
        LOG_DEBUG("error this time, and expect length is:%d",expectLength);
    }

    rsp = alloc_rspMsg(msg);
    if (!rsp)
    {
        LOG_ERROR("alloc rsp msg failed:cmd=%d", req->header.cmd);
        return -1;
    }

    rsp->offset = htonl(expectLength);
    socket_sendDataDirectly(rsp,sizeof(MSG_UPGRADE_DATA_RSP));

    return rc;
}

int cmd_UpgradeEnd_rsp(const void* msg)
{
    MSG_UPGRADE_END* req = (MSG_UPGRADE_END*)msg;
    MSG_UPGRADE_END_RSP* rsp = NULL;

    int rc =  upgrade_CheckAppfile(ntohl(req->size),ntohl(req->checksum));

    rsp = alloc_rspMsg(msg);
    if (!rsp)
    {
        LOG_ERROR("alloc rsp msg failed:cmd=%d", req->header.cmd);
        return -1;
    }
    rsp->code = rc;
    socket_sendDataDirectly(rsp,sizeof(MSG_UPGRADE_END_RSP));

    //启动升级
    if(0 <= rc)
    {
        rc = upgrade_do();
    }

    return rc;
}

int cmd_DeviceInfo_rsp(const void* msg)
{
    int msgLen = 0;
    MSG_DEVICE_INFO_GET_RSP* rsp = NULL;
    LOCAL_GPS* local_gps = gps_get_last();

    if(local_gps->isGps)    //becaus of auto malloc ram,cant use alloc_rspMsg
    {
        msgLen = sizeof(MSG_DEVICE_INFO_GET_RSP);
    }
    else
    {
        msgLen = sizeof(MSG_DEVICE_INFO_GET_RSP)-sizeof(GPS) + 4*sizeof(short);
    }

    rsp = alloc_msg(CMD_DEVICE_INFO_GET, msgLen);
    if (!rsp)
    {
        LOG_ERROR("alloc defend rsp message failed!");
        return -1;
    }

    rsp->isGps = local_gps->isGps;
    rsp->autolock = setting.isAutodefendFixed;
    rsp->autoperiod = setting.autodefendPeriod;
    rsp->defend = setting.isVibrateFixed;
    rsp->percent = battery_getPercent();
    rsp->miles = 0;

    if(rsp->isGps)
    {
        rsp->gps.timestamp = htonl(local_gps->gps.timestamp);
        rsp->gps.latitude = local_gps->gps.latitude;
        rsp->gps.longitude = local_gps->gps.longitude;
        rsp->gps.speed = local_gps->gps.speed;
        rsp->gps.course = htons(local_gps->gps.course);
    }
    else
    {
        rsp->mcc = htons(local_gps->cellInfo.mcc);
        rsp->mnc = htons(local_gps->cellInfo.mnc);
        rsp->lac = htons(local_gps->cellInfo.cell[0].lac);
        rsp->cid = htons(local_gps->cellInfo.cell[0].cellid);
    }

    socket_sendDataDirectly(rsp,msgLen);
    return 0;
}

int cmd_SetBatteryType_rsp(const void* msg)
{
    MSG_SET_BATTERY_TYPE* req = (MSG_SET_BATTERY_TYPE*)msg;
    MSG_SET_BATTERY_TYPE_RSP* rsp = NULL;

    LOG_DEBUG("set battery type :%d",req->type);
    rsp = alloc_rspMsg(msg);
    if (!rsp)
    {
        LOG_ERROR("alloc reboot rsp message failed!");
        return -1;
    }

    set_UserBatteryTpye(req->type);

    socket_sendDataDirectly(rsp, sizeof(MSG_SET_BATTERY_TYPE_RSP));
    return 0;
}


