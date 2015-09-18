//
// Created by jk on 2015/7/1.
//

#ifndef ELECTROMBILE_FIRMWARE_THREAD_MSG_H
#define ELECTROMBILE_FIRMWARE_THREAD_MSG_H

#include <eat_type.h>
#include <eat_interface.h>

#include "protocol.h"


enum CMD
{
    CMD_THREAD_GPS,
    CMD_THREAD_SMS,
    CMD_THREAD_VIBRATE,
    CMD_THREAD_SEEK,
};

#pragma pack(push,1)

typedef struct
{
    u8 cmd;
    u8 length;
    u8 data[];
}MSG_THREAD;


typedef struct
{
    short mcc;  //mobile country code
    short mnc;  //mobile network code
    char  cellNo;// cell count
    CELL cell[MAX_CELL_NUM];
}CELL_INFO;       //Cell Global Identifier

typedef struct
{
    float value;
}SEEK_INFO;

#pragma anon_unions
typedef struct
{
    eat_bool isGpsFixed;    //TURE: ���͵���GPS��Ϣ��FALSE�����͵��ǻ�վ��Ϣ
    union
    {
        GPS gps;
        CELL_INFO cellInfo;
    };
}LOCAL_GPS;
#pragma pack(pop)

MSG_THREAD* allocMsg(u8 len);
void freeMsg(MSG_THREAD* msg);
eat_bool sendMsg(EatUser_enum from, EatUser_enum to, void* msg, u8 len);

#endif //ELECTROMBILE_FIRMWARE_MSG_H
