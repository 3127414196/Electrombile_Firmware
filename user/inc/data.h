/*
 * data.h
 *
 *  Created on: 2015??7??9??
 *      Author: jk
 */

#ifndef USER_ELECTROMBILE_DATA_H_
#define USER_ELECTROMBILE_DATA_H_

#include <eat_type.h>
#include "protocol.h"
#include "thread_msg.h"

eat_bool gps_isQueueFull(void);
eat_bool gps_isQueueEmpty(void);


eat_bool gps_enqueue(GPS* gps);
eat_bool gps_dequeue(GPS* gps);

int gps_size(void);

LOCAL_GPS* gps_get_last(void);
int gps_save_last(LOCAL_GPS* gps);

int getVibrationTime(void);
int VibrationTimeAdd(void);
int ResetVibrationTime(void);

void set_manager_ATcmd_state(char state);
eat_bool get_manager_ATcmd_state(void);

void set_manager_seq(int seq);
int get_manager_seq(void);

void Vibration_setMoved(eat_bool state);
eat_bool Vibration_isMoved(void);

void battery_setVoltage(u8 voltage);
u8 battery_getVoltage(void);
void battery_setPercent(u8 percent);
u8 battery_getPercent(void);


#define MAX_GPS_COUNT 10


#endif /* USER_ELECTROMBILE_DATA_H_ */
