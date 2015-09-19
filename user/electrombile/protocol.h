/*
 * protocol.h
 *
 *  Created on: 2015骞�6鏈�29鏃�
 *      Author: jk
 */

#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#define START_FLAG (0xAA55)
#define IMEI_LENGTH 15
#define MAX_CELL_NUM 7
#define TEL_NO_LENGTH 11

enum
{
    CMD_LOGIN   = 0x01,		//��¼
    CMD_GPS     = 0x02,		//GPS�����ϱ�
    CMD_CELL    = 0x03,		//��վ��Ϣ�ϱ�
    CMD_PING    = 0x04,		//������
    CMD_ALARM   = 0x05,		//�澯
    CMD_SMS     = 0x06,		//�������·��Ķ��Ż�Ӧ
    CMD_433     = 0x07,		//�ҳ�ģʽ�£��ɼ�����433ģ���ź�ǿ��
	CMD_DEFEND  = 0x08,		//�򿪲�������
    CMD_SEEK    = 0x09,		//���ҳ����أ������ҳ�ģʽ
};

enum ALARM_TYPE
{
	ALARM_FENCE_OUT,
	ALARM_FENCE_IN,
	ALARM_VIBRATE,
};

#pragma pack(push, 1)

/*
 * GPS structure
 */
typedef struct
{
    float longitude;
    float latitude;
}GPS;

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
//    CELL cell[];
}__attribute__((__packed__)) CGI;       //Cell Global Identifier





/*
 * Message header definition
 */
typedef struct
{
    short signature;
    char cmd;
    unsigned short seq;
    unsigned short length;
}__attribute__((__packed__)) MSG_HEADER;

#define MSG_HEADER_LEN sizeof(MSG_HEADER)

/*
 * Login message structure
 */
typedef struct
{
    MSG_HEADER header;
    char IMEI[IMEI_LENGTH + 1];
}MSG_LOGIN_REQ;

typedef MSG_HEADER MSG_LOGIN_RSP;

/*
 * GPS message structure
 * this message has no response
 */
typedef struct
{
    MSG_HEADER header;
    GPS gps;
}MSG_GPS;

/*
 * heart-beat message structure
 */
typedef struct
{
    MSG_HEADER header;
    short status;   //TODO: to define the status bits
}MSG_PING_REQ;

typedef MSG_HEADER MSG_PING_RSP;

/*
 * alarm message structure
 */
typedef struct
{
    MSG_HEADER header;
    unsigned char alarmType;
}MSG_ALARM_REQ;

typedef MSG_HEADER MSG_ALARM_RSP;

/*
 * SMS message structure
 */
typedef struct
{
    MSG_HEADER header;
    char telphone[TEL_NO_LENGTH + 1];
    char smsLen;
    char sms[];
}MSG_SMS_REQ;

typedef MSG_SMS_REQ MSG_SMS_RSP;

/*
 * seek message structure
 * the message has no response
 */
typedef struct
{
    MSG_HEADER header;
    int intensity;
}MSG_433;


/*
 * defend message structure
 */
typedef MSG_HEADER MSG_DEFEND;

/*
 * switch on the seek mode
 */
typedef MSG_HEADER MSG_SEEK;


#pragma pack(pop)

#endif /* _PROTOCOL_H_ */
