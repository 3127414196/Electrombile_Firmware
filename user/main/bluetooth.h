/*
 * bluetooth.h
 *
 *  Created on: 2017/01/15
 *      Author: kky
 */

#ifndef _USER_BLUETOOTH_H_
#define _USER_BLUETOOTH_H_

void bluetooth_startLoop(void);
void bluetooth_onesecondLoop(void);
int bluetooth_check_run(u8* buf);
void bluetooth_resetState(void);

#endif/*_USER_BLUETOOTH_H_*/

