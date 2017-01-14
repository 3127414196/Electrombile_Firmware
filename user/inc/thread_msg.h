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
    CMD_THREAD_GPSHDOP,
    CMD_THREAD_SMS,
    CMD_THREAD_ALARM,
    CMD_THREAD_LOCATION,
    CMD_THREAD_AUTOLOCK,
    CMD_THREAD_PUTEND,
    CMD_THREAD_DEVICE_LOCATION
};


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

#pragma anon_unions
typedef struct
{
    eat_bool isGps;    //TURE: GPS; FALSE: 基站信息
    union
    {
        GPS gps;
        CELL_INFO cellInfo;
    };
}LOCAL_GPS;


typedef struct
{
    char state;

}AUTOLOCK_INFO;

typedef struct
{
    char alarm_type;
}__attribute__((__packed__))ALARM_INFO;

typedef struct
{
    float hdop;
    char satellites;
    int managerSeq;
}__attribute__((__packed__))GPS_HDOP_INFO;

typedef struct
{
    int managerSeq;
}__attribute__((__packed__))MANAGERSEQ_INFO;

typedef struct
{
    char percent;
    char miles;
    int managerSeq;
}__attribute__((__packed__))BATTERY_GET_INFO;

typedef struct
{
    char number[TEL_NO_LENGTH];
    char type;
    char smsLen;
    char content[];
}__attribute__((__packed__))SMS_SEND_INFO;

typedef struct
{
    char code;
    char fileName[];
}__attribute__((__packed__))FTP_PUTFILE_INFO;

typedef struct
{
    char seq;
}__attribute__((__packed__))DEVICE_LOCATION_SEQ;


#define allocMsg(len) eat_mem_alloc(len)
#define freeMsg(msg) eat_mem_free(msg)

eat_bool sendMsg(EatUser_enum peer, void* msg, u8 len);

#endif //ELECTROMBILE_FIRMWARE_MSG_H
