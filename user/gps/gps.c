/*
 * gps.c
 *
 *  Created on: 2015/6/25
 *      Author: jk
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <eat_interface.h>
#include <eat_modem.h>
#include <eat_gps.h>

#include "gps.h"
#include "timer.h"
#include "thread.h"
#include "thread_msg.h"
#include "log.h"
#include "setting.h"
#include "modem.h"
#include "vibration.h"
#include "rtc.h"
#include "data.h"
#include "utils.h"
#include "mem.h"

typedef struct
{
   short year;
   short mon;
   short day;
   short hour;
   short min;
   short sec;
}__attribute__((__packed__)) GPSTIME;

#define TIMER_GPS_PERIOD (5 * 1000)
#define EARTH_RADIUS 6378137 //radius of our earth unit :  m
#define PI 3.141592653
#define DEG2RAD(d) (d * PI / 180.f)//degree to radian

static eat_bool isGpsFixed = EAT_FALSE;
static float latitude = 0.0;
static float longitude = 0.0;
static float altitude = 0.0;
static float speed = 0.0;
static float course = 0.0;
static double iGpstime = 0.0;
static int satellite = 0;


static long double getdistance(LOCAL_GPS *pre_gps, LOCAL_GPS *gps)  //get distance of new gps and the last gps
{
    long double radLat1 = DEG2RAD(pre_gps->gps.latitude);
    long double radLat2 = DEG2RAD(gps->gps.latitude);
    long double a = radLat1 - radLat2;
    long double b = DEG2RAD(pre_gps->gps.longitude) - DEG2RAD(gps->gps.longitude);

    long double s = 2 * asin(sqrt(sin(a/2)*sin(a/2)+cos(radLat1)*cos(radLat2)*sin(b/2)*sin(b/2)));
    LOG_DEBUG("Lat1:%lf,Lat2:%lf,Lon1:%lf,Lon2:%lf",pre_gps->gps.latitude,gps->gps.latitude,pre_gps->gps.longitude,gps->gps.longitude);

    s = s * EARTH_RADIUS;

    return s;
}

static eat_bool gps_DuplicateCheck(LOCAL_GPS *pre_gps, LOCAL_GPS *gps)
{
    double distance = 0;
    static int timerCount =0;

    if(pre_gps->isGps != gps->isGps)
    {
        LOG_DEBUG("gps_type is different.");
        return EAT_FALSE;
    }
    else
    {
        if(pre_gps->isGps == EAT_TRUE)
        {

            distance = getdistance(pre_gps,gps);
            //if the distance change 10m but not float,push the information of GPS
            if(distance <= 10 || !Vibration_isMoved())
            {
                LOG_DEBUG("GPS is the same. %f, %f.", pre_gps->gps.latitude, gps->gps.latitude);
                return EAT_TRUE;
            }
            else
            {
                //avoid appearing the situation that distance always beyond 70
                if(distance >= 70 && timerCount < 5)
                {
                    timerCount++;
                    LOG_DEBUG("GPS is floating. %f, %f.", pre_gps->gps.latitude, gps->gps.latitude);
                    return EAT_TRUE;
                }
                else
                {
                    timerCount = 0;
                    LOG_DEBUG("GPS is different. %f, %f.", pre_gps->gps.latitude, gps->gps.latitude);
                    return EAT_FALSE;
                }
            }
        }
    }
}

static eat_bool gps_saveGps(void)
{
    LOCAL_GPS gps;
    LOCAL_GPS * last_gps = NULL;

    gps.isGps = EAT_TRUE;
    gps.gps.timestamp = rtc_getTimestamp();
    gps.gps.latitude = latitude;
    gps.gps.longitude = longitude;
    gps.gps.speed = speed;
    gps.gps.course = course;

    memcpy(last_gps, &gps, sizeof(LOCAL_GPS));

    gps_save_last(&gps);//save the last gps in data

    return EAT_TRUE;

}

/*
*fun: send dpop information to main thread
*/
static void gps_GPSSignalSend(const MSG_THREAD* thread_msg)
{
#define GPS_NOT_FIX_HDOP 99.99
    unsigned char buf[1024] = {0};
    unsigned char* buf_p1 = NULL;
    int rc;
    u8 msgLen = 0;
    MSG_THREAD *msg = NULL;
    GPS_HDOP_INFO *data = NULL;
    MANAGERSEQ_INFO *seq = NULL;
    float longtitude = 0.0;
    float latitude = 0.0;
    float hdop = 0.0;
    char satellites = 0;

    /*
    *$GPGGA,083316.000,0030.5131,N,00114.4249,E,1,3,6.00,163.8,M,-13.5,M,,
    */
    rc = eat_gps_nmea_info_output(EAT_NMEA_OUTPUT_GPGGA,buf,1024);
    if(rc == EAT_FALSE)
    {
        LOG_ERROR("get gps error ,and erturn is %d",rc);
        return;
    }

    msgLen = sizeof(MSG_THREAD)+sizeof(GPS_HDOP_INFO);
    msg = allocMsg(msgLen);
    if (!msg)
    {
        LOG_ERROR("alloc msg failed!");
        return ;
    }
    seq = (MANAGERSEQ_INFO*)thread_msg->data;
    msg->cmd = thread_msg->cmd;
    msg->length = sizeof(GPS_HDOP_INFO);
    data = (GPS_HDOP_INFO*)(msg->data);
    data->managerSeq = seq->managerSeq;

    buf_p1 = string_bypass(buf, "$GPGGA,");
    rc = sscanf(buf_p1,"%*f,%f,%*c,%f,%*c,%*d,%d,%f,%*s",&latitude,&longtitude,&satellites,&hdop);
    if(rc == 4 && latitude > 0 && longtitude > 0)
    {
        data->hdop = hdop;
        data->satellites = satellites;
    }
    else
    {
        data->hdop = GPS_NOT_FIX_HDOP;
        data->satellites = satellites;
    }

    LOG_DEBUG("send hdop to MainThread");
    sendMsg(THREAD_MAIN, msg, msgLen);

    return;
}

static eat_bool gps_GetGps(void)
{
    unsigned char buf[1024] = {0};
    unsigned char* buf_p1 = NULL;
    int rc;

    /*
     * the output format of eat_gps_nmea_info_output
     * $GPSIM,<latitude>,<longitude>,<altitude>,<UTCtime>,<TTFF>,<num>,<speed>,<course>
     * note:
            <TTFF>:time to first fix(in seconds)
            <num> :satellites in view for fix
     * example:$GPSIM,114.5,30.15,28.5,1461235600.123,3355,7,2.16,179.36
     */
    rc = eat_gps_nmea_info_output(EAT_NMEA_OUTPUT_SIMCOM,buf,1024);
    if(rc == EAT_FALSE)
    {
        LOG_ERROR("get gps error ,and erturn is %d",rc);
    }

    LOG_DEBUG("%s",buf);

    buf_p1 = string_bypass(buf, "$GPSIM,");

    rc = sscanf(buf_p1,"%f,%f,%f,%lf,%*d,%d,%f,%f",\
        &latitude,&longitude,&altitude,&iGpstime,&satellite,&speed,&course);

    if(!rtc_synced())
    {
        rtc_update((long long)iGpstime);
    }

    if(longitude > 0 && latitude > 0)//get GPS
    {
        isGpsFixed = EAT_TRUE;
    }
    else
    {
        isGpsFixed = EAT_FALSE;
    }

    return isGpsFixed;
}

static eat_bool gps_sendGps(u8 cmd)
{
    u8 msgLen = sizeof(MSG_THREAD) + sizeof(LOCAL_GPS);
    MSG_THREAD* msg = allocMsg(msgLen);
    LOCAL_GPS* gps = 0;
    eat_bool cmp = EAT_FALSE;
    eat_bool ret = EAT_FALSE;
    LOCAL_GPS* last_gps = gps_get_last();

    if (!msg)
    {
        LOG_ERROR("alloc msg failed!");
        return EAT_FALSE;
    }
    msg->cmd = cmd;             //CMD_THREAD_GPS or CMD_THREAD_LOCATION
    msg->length = sizeof(LOCAL_GPS);

    gps = (LOCAL_GPS*)msg->data;

    gps->isGps = EAT_TRUE;
    gps->gps.timestamp = rtc_getTimestamp();
    gps->gps.latitude = latitude;
    gps->gps.longitude = longitude;
    gps->gps.speed = speed;
    gps->gps.course = course;

    if(msg->cmd == CMD_THREAD_LOCATION)
    {
        LOG_DEBUG("active acquisition:location!");

        cmp = EAT_FALSE;        // 0 express send , 1 express do not send
    }
    else
    {
        cmp = gps_DuplicateCheck(last_gps, gps);
    }

    if(EAT_TRUE == cmp)//GPS is the same as before, do not send this msg
    {

        ret = EAT_TRUE;
        freeMsg(msg);
    }

    else//GPS is different from before, send this msg, update the last_gps
    {
        memcpy(last_gps, gps, sizeof(LOCAL_GPS));
        gps_save_last(gps);//save the last gps in data

        LOG_DEBUG("send gps to THREAD_MAIN");
        ret = sendMsg(THREAD_MAIN, msg, msgLen);
    }

    return ret;
}

static eat_bool gps_sendCell(u8 cmd)
{

    static eat_bool isCellGet = EAT_FALSE;
    static short mcc = 0;//mobile country code
    static short mnc = 0;//mobile network code
    static char  cellNo = 0;//cell count
    static CELL  cells[7] = {0};

    u8 msgLen = sizeof(MSG_THREAD) + sizeof(LOCAL_GPS);
    MSG_THREAD *msg = allocMsg(msgLen);
    LOCAL_GPS *gps = 0;
    int i = 0;
    eat_bool cmp = EAT_FALSE;
    eat_bool ret = EAT_FALSE;
    LOCAL_GPS* last_gps = gps_get_last();

    if (!msg)
    {
        LOG_ERROR("alloc msg failed!");
        return EAT_FALSE;
    }
    msg->cmd = cmd;//CMD_THREAD_GPS or CMD_THREAD_LOCATION
    msg->length = sizeof(LOCAL_GPS);

    gps = (LOCAL_GPS*)msg->data;
    gps->isGps = EAT_FALSE;

    gps->cellInfo.mcc = 0;
    gps->cellInfo.mnc = 0;
    gps->cellInfo.cellNo = 0;

    for (i = 0; i < cellNo; i++)
    {
        gps->cellInfo.cell[i].lac = cells[i].lac;
        gps->cellInfo.cell[i].cellid = cells[i].cellid;
        gps->cellInfo.cell[i].rxl = cells[i].rxl;
    }

    if(last_gps == 0 || msg->cmd == CMD_THREAD_LOCATION)
    {
        LOG_DEBUG("the first cell or active acquisition");

        cmp = EAT_FALSE; // 0 express send , 1 express do not send
    }

    if(EAT_TRUE == cmp)//GPS is the same as before, do not send this msg
    {
        ret = EAT_TRUE;
        freeMsg(msg);
    }
    else
    {
        memcpy(last_gps, gps, sizeof(LOCAL_GPS));


        gps_save_last(gps);//save the last gps in data

        //GPS is different from before, send this msg, update the last_gps
        LOG_DEBUG("send cell to THREAD_MAIN: mcc(%d), mnc(%d), cellNo(%d).", mcc, mnc, cellNo);
        ret = sendMsg(THREAD_MAIN, msg, msgLen);
    }

    return ret;
}

static void gps_timer_handler(u8 cmd)
{
    if(gps_GetGps())
    {
        LOG_DEBUG("send gps.");
        gps_sendGps(cmd);
    }
    else
    {
        LOG_INFO("GPS is not fixed.");
    }

    return;
}

static void location_handler(u8 cmd)
{
    if(gps_GetGps())
    {
        LOG_DEBUG("gps isFix, send location gps.");
        gps_sendGps(cmd);
    }
    else
    {
        LOG_DEBUG("gps isUnfix, send location cell.");
        gps_sendCell(cmd);
    }
}

static void device_location_handler(const MSG_THREAD* thread_msg)
{
    u8 msgLen = (u8)(sizeof(MSG_THREAD) + sizeof(DEVICE_LOCATION_SEQ));
    MSG_THREAD* msg = allocMsg(msgLen);
    DEVICE_LOCATION_SEQ *seq = (DEVICE_LOCATION_SEQ *)thread_msg->data;
    DEVICE_LOCATION_SEQ *data = (DEVICE_LOCATION_SEQ *)msg->data;

    if(gps_GetGps())
    {
        gps_saveGps();
    }

    msg->cmd = thread_msg->cmd;
    msg->length = sizeof(DEVICE_LOCATION_SEQ);

    data->seq = seq->seq;

    LOG_DEBUG("send CMD_THREAD_DEVICE_LOCATION to THREAD_MAIN.");
    sendMsg(THREAD_MAIN, msg, msgLen);
}

void app_gps_thread(void *data)
{
    EatEvent_st event;
	MSG_THREAD* msg = 0;
    u8 msgLen = 0;

    LOG_INFO("gps thread start.");

    eat_gps_power_req(EAT_TRUE);    //turn on GNSS power supply, equal to AT+CGNSPWR=1
    LOG_INFO("gps sleep mode %d", eat_gps_sleep_read());

    modem_switchEngineeringMode(3, 1);  //set cell on, AT+CENG=3,1\r

    eat_timer_start(TIMER_GPS, TIMER_GPS_PERIOD);

    while(EAT_TRUE)
    {
        eat_get_event_for_user(THREAD_GPS, &event);
        switch(event.event)
        {
            case EAT_EVENT_TIMER :
                switch (event.data.timer.timer_id)
                {
                    case TIMER_GPS:
                        LOG_DEBUG("TIMER_GPS expire.");
                        gps_timer_handler(CMD_THREAD_GPS);
                        eat_timer_start(TIMER_GPS, TIMER_GPS_PERIOD);
                        break;

                    case TIMER_UPDATE_RTC:
                        LOG_DEBUG("TIMER_UPDATE_RTC expire.");
                        rtc_setSyncFlag(EAT_FALSE);//at next time , updata RTC_time
                        break;


                    default:
                    	LOG_ERROR("timer[%d] expire!", event.data.timer.timer_id);
                        break;
                }
                break;

            case EAT_EVENT_MDM_READY_RD:
                LOG_DEBUG("EAT_EVENT_MDM_READY_RD happen");
                break;

            case EAT_EVENT_USER_MSG:
                msg = (MSG_THREAD*) event.data.user_msg.data_p;
                msgLen = event.data.user_msg.len;

                switch (msg->cmd)
                {
                    case CMD_THREAD_LOCATION:
                        LOG_DEBUG("gps get CMD_THREAD_LOCATION.");
                        location_handler(msg->cmd);
                        break;

                    case CMD_THREAD_DEVICE_LOCATION:
                        LOG_DEBUG("gps get CMD_THREAD_DEVICE_LOCATION.");
                        device_location_handler(msg);
                        break;

                    case CMD_THREAD_GPSHDOP:
                        LOG_DEBUG("gps get CMD_THREAD_GPSHDOP.");
                        gps_GPSSignalSend(msg);
                        break;

                    default:
                        LOG_ERROR("cmd(%d) not processed!", msg->cmd);
                        break;
                }
                freeMsg(msg);
                break;

            default:
            	LOG_ERROR("event(%d) not processed!", event.event);
                break;

        }
    }
}

