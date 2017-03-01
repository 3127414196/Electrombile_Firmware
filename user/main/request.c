/*
 * request.c
 *
 *  Created on: 2016年2月4日
 *      Author: jk
 */

#include <string.h>

#include <eat_interface.h>
#include <eat_other.h>
#include <eat_fs.h>

#include "request.h"
#include "setting.h"
#include "msg.h"
#include "log.h"
#include "socket.h"
#include "version.h"
#include "data.h"
#include "modem.h"
#include "response.h"
#include "data.h"

int cmd_Login(void)
{
    MSG_LOGIN_REQ_NEW* msg = alloc_msg(CMD_LOGIN, sizeof(MSG_LOGIN_REQ_NEW));
    u8 imei[MAX_IMEI_LENGTH] = {0};

    if (!msg)
    {
        LOG_ERROR("alloc login message failed!");
        return -1;
    }

    msg->version = htonl(VERSION_NUM);

    eat_get_imei(imei, MAX_IMEI_LENGTH);
    memcpy(msg->IMEI, imei, MAX_IMEI_LENGTH);

    msg->voltage = battery_getVoltage();

    LOG_DEBUG("send login message.");

    socket_sendDataDirectly(msg, sizeof(MSG_LOGIN_REQ_NEW));

    return 0;
}

int cmd_SimInfo(char* buf)
{
    MSG_SIM_INFO* msg = alloc_msg(CMD_SIM_INFO, sizeof(MSG_SIM_INFO));
    u8 imsi[MAX_IMSI_LENGTH + 2] = {'\0'};//len: len >=17[length of imsi(15 or 16)+ 1('\0')]
    if (!msg)
    {
        LOG_ERROR("alloc login message failed!");
        return -1;
    }

    eat_get_imsi(imsi,MAX_IMSI_LENGTH + 2);

    sscanf(buf,"%20s",msg->CCID);
    LOG_DEBUG("CCID: %20s",msg->CCID);

    sscanf(imsi,"%15s",msg->IMSI);
    LOG_DEBUG("IMSI: %15s",msg->IMSI);

    LOG_DEBUG("send SIM_info message.");


    socket_sendDataWaitAck(msg, sizeof(MSG_SIM_INFO),NULL,NULL);
    return 0;
}


void cmd_Heartbeat(void)
{
    u8 msgLen = sizeof(MSG_HEADER) + sizeof(short);
    MSG_PING_REQ* msg = alloc_msg(CMD_PING, msgLen);

    msg->gsm = (u8)eat_network_get_csq();
    msg->voltage = battery_getVoltage();

    socket_sendDataDirectly(msg, msgLen);
}

int cmd_alarm(char alarm_type)
{
    MSG_ALARM_REQ* socket_msg = alloc_msg(CMD_ALARM, sizeof(MSG_ALARM_REQ));
    if (!socket_msg)
    {
        LOG_ERROR("alloc message failed!");
        return -1;
    }

    LOG_DEBUG("send alarm vibrate message.");
    socket_msg->alarmType = alarm_type;

    //TODO: should provide the message send failed handler
    socket_sendDataWaitAck(socket_msg, sizeof(MSG_ALARM_REQ), NULL, NULL);

    return 0;
}


int cmd_PutEnd(char code, char *fileName)
{
    int msgLen = sizeof(MSG_FTPPUT_REQ) + strlen(fileName) + 1;
    MSG_FTPPUT_REQ *msg = alloc_msg(CMD_FTPPUT, msgLen);
    if (!msg)
    {
        LOG_ERROR("alloc message failed!");
        return -1;
    }

    LOG_DEBUG("send put end message.");
    msg->code = code;
    strncpy(msg->fileName, fileName, strlen(fileName));

    socket_sendDataWaitAck(msg, msgLen, NULL, NULL);

    return 0;
}


int cmd_SMS(char number[], char type, char smsLen, char content[])
{
    u8 msgLen = sizeof(MSG_SMS_REQ) + smsLen;
    MSG_SMS_REQ *msg = alloc_msg(CMD_SMS, msgLen);

    msg->type = type;
    msg->smsLen = smsLen;
    strncpy(msg->telphone, number, TEL_NO_LENGTH);
    msg->telphone[TEL_NO_LENGTH] = 0;
    strncpy(msg->sms, content, smsLen);

    socket_sendDataDirectly(msg, msgLen);
    return 0;
}


void cmd_Wild(const void* m, int len)
{
    u8 msgLen = sizeof(MSG_HEADER) + len;
    MSG_HEADER* msg = alloc_msg(CMD_WILD, msgLen);

    memcpy(msg + 1, m, len);

    socket_sendDataDirectly(msg, msgLen);
}

int cmd_Seek(unsigned int value)
{
    MSG_433* seek_msg = alloc_msg(CMD_433, sizeof(MSG_433));
    if (!seek_msg)
    {
        LOG_ERROR("alloc seek message failed!");
        return -1;
    }

    LOG_DEBUG("send seek value message:%d", value);
    seek_msg->intensity = htonl(value);
    socket_sendDataDirectly(seek_msg, sizeof(MSG_433));

    return 0;
}

int cmd_GPS(GPS* gps)
{
    MSG_GPS* msg = alloc_msg(CMD_GPS, sizeof(MSG_GPS));
    if (!msg)
    {
        LOG_ERROR("alloc GPS message failed!");
        return -1;
    }

    memcpy(&msg->gps, gps, sizeof(GPS));
    msg->gps.timestamp = htonl(gps->timestamp);
    msg->gps.course = htons(gps->course);

    LOG_DEBUG("send GPS message.");

    socket_sendDataDirectly(msg, sizeof(MSG_GPS));

    return 0;
}

int cmd_GPSPack(void)
{
    //TODO: send all the gps data in the queue
    u8 msgLen = sizeof(MSG_HEADER) + gps_size()*sizeof(GPS);
    MSG_GPS_PACK* msg;
    int count = 0;

    if(gps_isQueueEmpty())//if queue is empty , do not send gps msg
    {
        return 0;
    }

    LOG_DEBUG("msgLen %d, sizeof(GPS) %d", msgLen, sizeof(GPS));

    msg = alloc_msg(CMD_GPS_PACK, msgLen);
    if (!msg)
    {
        LOG_ERROR("alloc GPSPack message failed!");
        return -1;
    }

    for(count = 0;count < MAX_GPS_COUNT;count++)
    {
        if(gps_isQueueEmpty())
            break;

        gps_dequeue(&(msg->gps[count]));
        msg->gps[count].timestamp = htonl(msg->gps[count].timestamp);
        msg->gps[count].course = htons(msg->gps[count].course);
    }

    socket_sendDataDirectly(msg, msgLen);

    return 0;

}

