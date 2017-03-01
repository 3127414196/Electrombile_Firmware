/*
 * timer.h
 *
 *  Created on: 2015/6/25
 *      Author: jk
 */

#ifndef USER_ELECTROMBILE_TIMER_H_
#define USER_ELECTROMBILE_TIMER_H_

#include <eat_timer.h>

#define TIMER_GPS  	        EAT_TIMER_1     //GPS现场中获取定时获取GPS位置
#define TIMER_VIBRATION     EAT_TIMER_2     //震动线程中检测车辆震动状况
#define TIMER_MSG_RESEND    EAT_TIMER_3     //消息重传
#define TIMER_LOOP          EAT_TIMER_4     //主线程事件循环
#define TIMER_GPS_SEND      EAT_TIMER_5     //GPS定时发送定时器，主线程中

#define TIMER_SEEKAUTOOFF   EAT_TIMER_6
#define TIMER_UPDATE_RTC    EAT_TIMER_7
#define TIMER_MOVE_ALARM    EAT_TIMER_8
#define TIMER_VOLTAGE_GET   EAT_TIMER_9
#define TIMER_BATTERY_CHECK EAT_TIMER_10

#define TIMER_BLUETOOTH EAT_TIMER_11


#endif /* USER_ELECTROMBILE_TIMER_H_ */
