/*
 * protocol.h
 *
 *  Created on: 2015/6/29
 *      Author: jk
 *
 *  Copyright (c) 2015 Wuhan Xiaoan Technology Co., Ltd. All rights reserved.
 *
 *  Change log:
 *      2.15    去掉CMD_LOGIN中的CCID字段
 *              增加CMD_SIM_INFO命令字
 *
 */

#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

enum
{
    PROTOCOL_VESION_251 = 215,
};

#define PROTOCOL_VERSION    PROTOCOL_VESION_251

#define START_FLAG (0xAA55)
#define MAX_IMEI_LENGTH 15
#define MAX_CCID_LENGTH 20
#define MAX_IMSI_LENGTH 15
#define MAX_CELL_NUM 7
#define TEL_NO_LENGTH 11


enum
{
    CMD_SERVER          = -9,
    CMD_GET_AT          = -8,
    CMD_REBOOT          = -7,
    CMD_GET_LOG         = -6,
    CMD_GET_433         = -5,
    CMD_GET_GSM         = -4,
    CMD_GET_GPS         = -3,
    CMD_GET_SETTING     = -2,
    CMD_GET_BATTERY     = -1,
    CMD_WILD            =  0,
    CMD_LOGIN           =  1,
    CMD_PING            =  2,
    CMD_GPS             =  3,
    CMD_CELL            =  4,
    CMD_ALARM           =  5,
    CMD_SMS             =  6,
    CMD_433             =  7,
    CMD_DEFEND          =  8,
    CMD_SEEK            =  9,
    CMD_LOCATE          = 10,
    CMD_SET_SERVER      = 11,
    CMD_SET_TIMER       = 12,
    CMD_SET_AUTOSWITCH  = 13,
    CMD_GET_AUTOSWITCH  = 14,
    CMD_SET_PERIOD      = 15,
    CMD_GET_PERIOD      = 16,
    CMD_ITINERARY       = 17,
    CMD_BATTERY         = 18,
    CMD_DEFEND_ON       = 19,
    CMD_DEFEND_OFF      = 20,
    CMD_DEFEND_GET      = 21,
    CMD_DEFEND_NOTIFY   = 22,
    CMD_UPGRADE_START   = 23,
    CMD_UPGRADE_DATA    = 24,
    CMD_UPGRADE_END     = 25,
    CMD_SIM_INFO        = 26,
    //CMD_REBOOT          = 27,
    CMD_DEVICE_INFO_GET = 28,
    CMD_GPS_PACK        = 29,
    CMD_SET_BATTERY_TYPE= 30,
    CMD_DEVICE          = 31,
    CMD_FTPPUT          = 32,
};

enum
{
    MSG_SUCCESS = 0,
    MSG_VERSION_NOT_SUPPORTED = -1,
    MSG_DISK_NO_SPACE = -2,
    MSG_UPGRADE_CHECKSUM_FAILED = -3,
};

#pragma pack(push, 1)

/*
 * Message header definition
 */
typedef struct
{
    short signature;
    char cmd;
    char seq;
    short length;
}__attribute__((__packed__)) MSG_HEADER;

#define MSG_HEADER_LEN sizeof(MSG_HEADER)

typedef struct
{
    MSG_HEADER header;
    int managerSeq;
}__attribute__((__packed__)) MSG_GET_HEADER;

/*
 * set server message structure
 */
typedef struct
{
    MSG_HEADER header;
    int managerSeq;
    char data[];
}__attribute__((__packed__)) MSG_SET_SERVER_REQ;

typedef struct
{
    MSG_HEADER header;
    int managerSeq;
}__attribute__((__packed__)) MSG_SET_SERVER_RSP;

/*
 * get log message structure
 */
typedef struct
{
    MSG_HEADER header;
    int managerSeq;
    char data[];
}__attribute__((__packed__)) MSG_GET_AT_REQ;

typedef struct
{
    MSG_HEADER header;
    int managerSeq;
    char data[];
}__attribute__((__packed__)) MSG_GET_AT_RSP;

/*
 * reboot message structure
 */
typedef MSG_HEADER MSG_REBOOT_REQ;

/*
 * get log message structure
 */
typedef MSG_GET_HEADER MSG_GET_LOG_REQ;

typedef struct
{
    MSG_HEADER header;
    int managerSeq;
    char data[];
}__attribute__((__packed__)) MSG_GET_LOG_RSP;

/*
 * get 433 message structure
 */
typedef MSG_GET_HEADER MSG_GET_433_REQ;

typedef struct
{
    MSG_HEADER header;
    int managerSeq;
    char data[];
}__attribute__((__packed__)) MSG_GET_433_RSP;

/*
 * get GSM message structure
 */
typedef MSG_GET_HEADER MSG_GET_GSM_REQ;

typedef struct
{
    MSG_HEADER header;
    int managerSeq;
    char data[];
}__attribute__((__packed__)) MSG_GET_GSM_RSP;

/*
 * get GPS message structure
 */
typedef MSG_GET_HEADER MSG_GET_GPS_REQ;

typedef struct
{
    MSG_HEADER header;
    int managerSeq;
    char data[];
}__attribute__((__packed__)) MSG_GET_GPS_RSP;

/*
 * get setting message structure
 */
typedef MSG_GET_HEADER MSG_GET_SETTING_REQ;

typedef struct
{
    MSG_HEADER header;
    int managerSeq;
    char data[];
}__attribute__((__packed__)) MSG_GET_SETTING_RSP;

/*
 * get battery message structure
 */
typedef MSG_GET_HEADER MSG_GET_BATTERY_REQ;

typedef struct
{
    MSG_HEADER header;
    int managerSeq;
    char data[];
}__attribute__((__packed__)) MSG_GET_BATTERY_RSP;

/*
 * Login message structure
 */
typedef struct
{
    MSG_HEADER header;
    int version;
    char deciveType;
    char IMEI[MAX_IMEI_LENGTH];
}__attribute__((__packed__)) MSG_LOGIN_REQ;

typedef struct
{
    MSG_HEADER header;
    int version;
    char deciveType;
    char IMEI[MAX_IMEI_LENGTH];
    char voltage;
}__attribute__((__packed__)) MSG_LOGIN_REQ_NEW;


enum DeviceType{
    XiaoAnBao1 = 1,
    XiaoAnBao2 = 2,
    XiaoAnBao3 = 3,
    XiaoAnBao4 = 4
};

typedef MSG_HEADER MSG_LOGIN_RSP;

/*
 * ping message structure
 */
typedef struct
{
    MSG_HEADER header;
    char gsm;
    char voltage;
}__attribute__((__packed__)) MSG_PING_REQ;

typedef MSG_HEADER MSG_PING_RSP;

/*
 * GPS structure
 */
typedef struct
{
    int timestamp;
    float longitude;
    float latitude;
    char speed;
    short course;
}__attribute__((__packed__)) GPS;

typedef struct
{
    MSG_HEADER header;
    GPS gps;
}__attribute__((__packed__)) MSG_GPS;

/*
 * CELL structure
 */
typedef struct
{
   short lac;       //local area code
   short cellid;    //cell id
   short rxl;       //receive level
}__attribute__((__packed__)) CELL;

typedef struct
{
    short mcc;  //mobile country code
    short mnc;  //mobile network code
    char  cellNo;// cell count
    //CELL cell[];
}__attribute__((__packed__)) CGI;       //Cell Global Identifier

typedef struct
{
    MSG_HEADER header;
    CGI cgi;
}__attribute__((__packed__)) MSG_CGI;

/*
 * alarm message structure
 */
enum ALARM_TYPE
{
    ALARM_FENCE_OUT = 1,
    ALARM_FENCE_IN  = 2,
    ALARM_VIBRATE   = 3,
    ALARM_BATTERY50 = 4,
    ALARM_BATTERY30 = 5,
    ALARM_BAT_CUT   = 6,
    ALARM_SWITCH_CHANGE = 7,
};

typedef struct
{
    MSG_HEADER header;
    char alarmType;
}__attribute__((__packed__)) MSG_ALARM_REQ;

typedef MSG_HEADER MSG_ALARM_RSP;

/*
 * SMS message structure
 */
 enum SMS_TYPE
{
    SMS_SEND_DIRECT = 0,
    SMS_SEND_PROCED = 1,
    SMS_SEND_SERVER = 2,
};

typedef struct
{
    MSG_HEADER header;
    char telphone[TEL_NO_LENGTH + 1];
    char type;
    char smsLen;
    char sms[];
}__attribute__((__packed__)) MSG_SMS_REQ;

typedef MSG_SMS_REQ MSG_SMS_RSP;

/*
 * seek message structure
 * the message has no response
 */
typedef struct
{
    MSG_HEADER header;
    int intensity;
}__attribute__((__packed__)) MSG_433;

/*
 * defend message structure
 */
enum DEFEND_TYPE
{
    DEFEND_ON   = 0x01,
    DEFEND_OFF  = 0x02,
    DEFEND_GET  = 0x03,
};

typedef struct
{
    MSG_HEADER header;
    int token;
    char operator;     // refer to DEFEND_TYPE
}__attribute__((__packed__)) MSG_DEFEND_REQ;

typedef struct
{
    MSG_HEADER header;
    int token;
    char result;
}__attribute__((__packed__)) MSG_DEFEND_RSP;

/*
 * switch on the seek mode
 */
 enum SEEK_TYPE
{
    SEEK_OFF    = 0x01,
    SEEK_ON     = 0x02,
};

typedef struct
{
    MSG_HEADER header;
    int token;
    char operator;     //refer to SEEK_TYPE
}__attribute__((__packed__)) MSG_SEEK_REQ;

typedef struct
{
    MSG_HEADER header;
    int token;
    char result;
}__attribute__((__packed__)) MSG_SEEK_RSP;

/*
*server set_ip/domain message structure
*this message has no response
*/
typedef struct
{
    MSG_HEADER header;
    int port;
    char server[];
}__attribute__((__packed__)) MSG_SET_SERVER;

/*
 * GPS set_time message structure
 */
typedef struct
{
    MSG_HEADER header;
    int timer;
}__attribute__((__packed__)) MSG_SET_TIMER_REQ;

typedef struct
{
    MSG_HEADER header;
    int result;
}__attribute__((__packed__)) MSG_SET_TIMER_RSP;

/*
 * autolock set message structure
 */
enum AUTODEFEND_SWITCH
{
    AUTO_DEFEND_OFF,
    AUTO_DEFEND_ON,
};

typedef struct
{
    MSG_HEADER header;
    int token;
    unsigned char onOff; //refer to AUTODEFEND_SWITCH
}__attribute__((__packed__)) MSG_AUTODEFEND_SWITCH_SET_REQ;

typedef struct
{
    MSG_HEADER header;
    int token;
    unsigned char result;
}__attribute__((__packed__)) MSG_AUTODEFEND_SWITCH_SET_RSP;

/*
 * autolock get message structure
 */
typedef struct
{
    MSG_HEADER header;
    int token;
}__attribute__((__packed__)) MSG_AUTODEFEND_SWITCH_GET_REQ;

typedef struct
{
    MSG_HEADER header;
    int token;
    unsigned char result;
}__attribute__((__packed__)) MSG_AUTODEFEND_SWITCH_GET_RSP;

/*
 * autoperiod set message structure
 */
typedef struct
{
    MSG_HEADER header;
    int token;
    unsigned char period;   //time unit: minutes
}__attribute__((__packed__)) MSG_AUTODEFEND_PERIOD_SET_REQ;

typedef struct
{
    MSG_HEADER header;
    int token;
    unsigned char result;
}__attribute__((__packed__)) MSG_AUTODEFEND_PERIOD_SET_RSP;

/*
 * autoperiod get message structure
 */
typedef struct
{
    MSG_HEADER header;
    int token;
}__attribute__((__packed__)) MSG_AUTODEFEND_PERIOD_GET_REQ;

typedef struct
{
    MSG_HEADER header;
    int token;
    unsigned char period;   //time unit: minutes
}__attribute__((__packed__)) MSG_AUTODEFEND_PERIOD_GET_RSP;

/*
 * itinerary message structure
 */
typedef struct
{
    MSG_HEADER header;
    int starttime;
    int endtime;
    int mileage;
}__attribute__((__packed__)) MSG_ITINERARY_REQ;

typedef MSG_HEADER MSG_ITINERARY_RSP;

/*
 * battery message structure
 */
typedef struct
{
    MSG_HEADER header;
    char percent;
    char miles;
}__attribute__((__packed__)) MSG_BATTERY_RSP;

/*
 * defend switch message structure
 */
typedef struct
{
    MSG_HEADER header;
    char result;
}__attribute__((__packed__)) MSG_DEFEND_ON_RSP;

typedef struct
{
    MSG_HEADER header;
    char result;
}__attribute__((__packed__)) MSG_DEFEND_OFF_RSP;

typedef struct
{
    MSG_HEADER header;
    char status;             //0: OFF,1: ON
}__attribute__((__packed__)) MSG_DEFEND_GET_RSP;

typedef struct
{
    MSG_HEADER header;
    char state;             //0 express OFF,1 express ON
}__attribute__((__packed__)) MSG_AUTODEFEND_STATE_REQ;

/*
 * upgrade message structure
 */
typedef struct
{
    MSG_HEADER header;
    int version;
    int size;
}__attribute__((__packed__)) MSG_UPGRADE_START;

typedef struct
{
    MSG_HEADER header;
    char code;      //MSG_SUCCESS means OK to upgrade
}__attribute__((__packed__)) MSG_UPGRADE_START_RSP;

typedef struct
{
    MSG_HEADER header;
    int offset;
    char data[];
}__attribute__((__packed__)) MSG_UPGRADE_DATA;

typedef struct
{
    MSG_HEADER header;
    int offset;
}__attribute__((__packed__)) MSG_UPGRADE_DATA_RSP;

typedef struct
{
    MSG_HEADER header;
    int checksum;
    int size;
}__attribute__((__packed__)) MSG_UPGRADE_END;

typedef struct
{
    MSG_HEADER header;
    char code;
}__attribute__((__packed__)) MSG_UPGRADE_END_RSP;

/*
 * sim info message structure
 */
typedef struct
{
    MSG_HEADER header;
    char CCID[MAX_CCID_LENGTH];
    char IMSI[MAX_IMSI_LENGTH];
}__attribute__((__packed__)) MSG_SIM_INFO;

typedef MSG_HEADER MSG_DEVICE_INFO_GET_REQ;

/*
 * packed GPS message structure
 */
typedef struct
{
    MSG_HEADER header;
    GPS gps[];
}__attribute__((__packed__)) MSG_GPS_PACK;

/*
 * CMD_SET_BATTERY_TYPE message structure
 */
typedef struct
{
    MSG_HEADER header;
    char type;
}__attribute__((__packed__)) MSG_SET_BATTERY_TYPE;

typedef MSG_HEADER MSG_SET_BATTERY_TYPE_RSP;

/*
 * CMD_DEVICE message structure
 */
typedef struct
{
    MSG_HEADER header;
    char data[];
}__attribute__((__packed__)) MSG_DEVICE_REQ;

typedef struct
{
    MSG_HEADER header;
    char data[];
}__attribute__((__packed__)) MSG_DEVICE_RSP;

/*
 * CMD_FTPPUTOK message structure
 */
typedef struct
{
    MSG_HEADER header;
    char code;
    char fileName[];
}__attribute__((__packed__)) MSG_FTPPUT_REQ;

typedef struct
{
    MSG_HEADER header;
}__attribute__((__packed__)) MSG_FTPPUT_RSP;


typedef MSG_HEADER MSG_DEBUG_REQ;

typedef struct
{
    MSG_HEADER header;
    char data[];
}__attribute__((__packed__)) MSG_DEBUG_RSP;

typedef struct
{
    MSG_HEADER header;
    char isGps;
    GPS gps;
}__attribute__((__packed__)) MSG_GPSLOCATION_RSP;   //FIXME: change the name

typedef struct
{
    MSG_HEADER header;
    char isGps;
}__attribute__((__packed__)) MSG_CELLLOCATION_HEADER;   //FIXME: change the name

#pragma anon_unions
typedef struct
{
    MSG_HEADER header;

    //auto lock related configuration
    struct
    {
        char autolock;
        char autoperiod;
    }__attribute__((__packed__));

    //battery configuration
    struct
    {
        char percent;
        char miles;
    }__attribute__((__packed__));

    //defend switch status
    char defend;

    char isGps;
    union
    {
        GPS gps;        //GPS
        struct          //CELL
        {
            short mcc;
            short mnc;
            short lac;
            short cid;
        }__attribute__((__packed__));
    }__attribute__((__packed__));

}__attribute__((__packed__)) MSG_DEVICE_INFO_GET_RSP;

#pragma pack(pop)

#endif /* _PROTOCOL_H_ */
