/*
 * data.c
 *
 *  Created on: 2015??7??9??
 *      Author: jk
 */


#include "data.h"
#include "math.h"

typedef struct queue
{
    int front;
    int rear;
    GPS gps[MAX_GPS_COUNT];
}QUEUE;
static QUEUE gps_queue = {0, 0};
static LOCAL_GPS last_gps_info;
static LOCAL_GPS* last_gps = &last_gps_info;

static int VibrationTime = 0;
static int AlarmCount = 0;

static int manager_seq = 0;
static eat_bool manager_ATcmd_flag = EAT_FALSE;

static eat_bool isMoved = EAT_FALSE;

static u8 batteryVoltage = 0;
static u8 batteryPercent = 0;

/*
 * to judge whether the queue is full
 */
eat_bool gps_isQueueFull(void)
{
    return gps_queue.front == (gps_queue.rear + 1) % MAX_GPS_COUNT;
}

/*
 * to judge whether the queue is empty
 */
eat_bool gps_isQueueEmpty(void)
{
    return gps_queue.front == gps_queue.rear;
}

/*
 * put a GPS in the queue
 */
eat_bool gps_enqueue(GPS* gps)
{
    if (gps_isQueueFull())
    {
        return EAT_FALSE;
    }

    /*memcpy is not useful, dont know why*/
    //memcpy(gps_queue.gps[gps_queue.rear], gps, sizeof(GPS));

    gps_queue.gps[gps_queue.rear].timestamp= gps->timestamp;
    gps_queue.gps[gps_queue.rear].longitude = gps->longitude;
    gps_queue.gps[gps_queue.rear].latitude = gps->latitude;
    gps_queue.gps[gps_queue.rear].speed = gps->speed;
    gps_queue.gps[gps_queue.rear].course = gps->course;

    gps_queue.rear = (gps_queue.rear + 1) % MAX_GPS_COUNT;

    return EAT_TRUE;
}


/*
 * get a GPS in the queue
 */
eat_bool gps_dequeue(GPS* gps)
{
    if (!gps)
    {
        return EAT_FALSE;
    }

    if (gps_isQueueEmpty())
    {
        return EAT_FALSE;
    }

    /*memcpy is not useful, dont know why*/
    //memcpy(gps, gps_queue.gps[gps_queue.front], sizeof(GPS));

    gps->timestamp = gps_queue.gps[gps_queue.front].timestamp;
    gps->longitude = gps_queue.gps[gps_queue.front].longitude;
    gps->latitude = gps_queue.gps[gps_queue.front].latitude;
    gps->speed = gps_queue.gps[gps_queue.front].speed;
    gps->course = gps_queue.gps[gps_queue.front].course;

    gps_queue.front = (gps_queue.front + 1) % MAX_GPS_COUNT;

    return EAT_TRUE;
}

/*
 * get the queue's actual size
 */
int gps_size(void)
{
    return (gps_queue.rear - gps_queue.front + MAX_GPS_COUNT) % MAX_GPS_COUNT;
}

/*
*get the last gps info
*/
LOCAL_GPS* gps_get_last(void)
{
    return last_gps;
}

/*
*save the last gps info
*/
int gps_save_last(LOCAL_GPS* gps)
{
    last_gps_info.isGps = gps->isGps;
    if(gps->isGps)
    {
        last_gps_info.gps.timestamp = gps->gps.timestamp;
        last_gps_info.gps.latitude = gps->gps.latitude;
        last_gps_info.gps.longitude = gps->gps.longitude;
        last_gps_info.gps.speed = gps->gps.speed;
        last_gps_info.gps.course = gps->gps.course;
    }
    else
    {
        last_gps_info.cellInfo = gps->cellInfo;
    }

    return EAT_TRUE;
}


/*
*vibration time ,for autolock & initerary
*/
int getVibrationTime(void)
{
    return VibrationTime;
}
/*
*Add once a sec
*/
int VibrationTimeAdd(void)
{
    return VibrationTime++;
}
/*
*if move or set defendoff ,reset the time
*/
int ResetVibrationTime(void)
{
    return VibrationTime = 0;
}

/*
* manager msg proc
*/
void set_manager_ATcmd_state(char state)
{
    manager_ATcmd_flag = state;
}
eat_bool get_manager_ATcmd_state(void)
{
    return manager_ATcmd_flag;
}

void set_manager_seq(int seq)
{
    manager_seq= seq;
}
int get_manager_seq(void)
{
    return manager_seq;
}

/*
*data of isMove
*/
void Vibration_setMoved(eat_bool state)
{
    isMoved = state;
}
eat_bool Vibration_isMoved(void)
{
    return isMoved;
}

/*
*batteryVoltage
*/
void battery_setVoltage(u8 voltage)
{
    batteryVoltage = voltage;
}

u8 battery_getVoltage(void)
{
    return batteryVoltage;
}

void battery_setPercent(u8 percent)
{
    batteryPercent = percent;
}

u8 battery_getPercent(void)
{
    return batteryPercent;
}

