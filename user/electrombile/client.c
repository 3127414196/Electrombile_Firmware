/*
 * client.c
 *
 *  Created on: 2015��7��9��
 *      Author: jk
 */
#include <stdio.h>

#include <eat_interface.h>

#include "client.h"
#include "msg.h"
#include "log.h"
#include "uart.h"


typedef int (*MSG_PROC)(const void* msg);
typedef struct
{
    char cmd;
    MSG_PROC pfn;
}MC_MSG_PROC;


static int mc_login_rsp(const void* msg);
static int mc_ping_rsp(const void* msg);
static int mc_alarm_rsp(const void* msg);
static int mc_sms(const void* msg);


static MC_MSG_PROC msgProcs[] =
{
        {CMD_LOGIN, mc_login_rsp},
        {CMD_PING,  mc_ping_rsp},
        {CMD_ALARM, mc_alarm_rsp},
        {CMD_SMS,   mc_sms},
};

static void print_hex(const char* data, int length)
{
    int i = 0, j = 0;

    for (i  = 0; i < 16; i++)
    {
        print("    %X  ", i);
    }
    print("    ");
    for (i = 0; i < 16; i++)
    {
        print("%X", i);
    }

    print("\n");

    for (i = 0; i < length; i += 16)
    {
        print("%02d  ", i % 16 + 1);
        for (j = i; j < 16 && j < length; j++)
        {
            print("%02x ", data[j] & 0xff);
        }
        print("    ");
        for (j = i; j < 16 && j < length; j++)
        {
            print("%c ", data[j] & 0xff);
        }

        print("\n");
    }
}

int client_proc(const void* m, int msgLen)
{
    MSG_HEADER* msg = (MSG_HEADER*)m;
    size_t i = 0;

    print_hex(m, msgLen);

    if (msgLen < sizeof(MSG_HEADER))
    {
        LOG_ERROR("receive message length not enough: %zu(at least(%zu)", msgLen, sizeof(MSG_HEADER));

        return -1;
    }

    if (msg->signature != START_FLAG)
    {
        LOG_ERROR("receive message head signature error:%d", msg->signature);
        return -1;
    }

    for (i = 0; i < sizeof(msgProcs) / sizeof(msgProcs[0]); i++)
    {
        if (msgProcs[i].cmd == msg->cmd)
        {
            MSG_PROC pfn = msgProcs[i].pfn;
            if (pfn)
            {
                return pfn(msg);
            }
            else
            {
                LOG_ERROR("Message %d not processed", msg->cmd);
                return -1;
            }
        }
    }

    LOG_ERROR("unknown message %d", msg->cmd);
    return -1;
}

static int mc_login_rsp(const void* msg)
{
    return 0;
}

static int mc_ping_rsp(const void* msg)
{
    return 0;
}

static int mc_alarm_rsp(const void* msg)
{
    return 0;
}

static int mc_sms(const void* msg)
{
    return 0;
}