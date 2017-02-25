/*
 * device.h
 *
 *  Created on: 2016/12/15
 *      Author: lc
 */
#ifndef USER_DEVICE_H_
#define USER_DEVICE_H_

int cmd_device_handler(const void* msg);
int device_sendGPS(const MSG_THREAD* msg);
int device_sendGPSSignal(const MSG_THREAD* msg);

#endif
