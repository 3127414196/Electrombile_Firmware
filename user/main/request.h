/*
 * request.h
 *
 *  Created on: 2016年2月4日
 *      Author: jk
 */

#ifndef USER_MAIN_REQUEST_H_
#define USER_MAIN_REQUEST_H_

#include "protocol.h"

int cmd_Login(void);
int cmd_SMS(const void* msg);
void cmd_Heartbeat(void);
int cmd_Seek(unsigned int value);
int cmd_GPS(GPS* gps);

int cmd_GPSPack(void);
int cmd_Itinerary_check(void);
int cmd_alarm(char alarm_type);




#endif /* USER_MAIN_REQUEST_H_ */
