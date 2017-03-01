/*
 * telecontrol.h
 *
 *  Created on: 2017/1/10
 *      Author: lc
 */
#ifndef __TELECONTROL_H__
#define __TELECONTROL_H__

void telecontrol_initail(void);

void telecontrol_lock(void);
void telecontrol_unlock(void);

void telecontrol_switch_on(void);
void telecontrol_switch_off(void);

void telecontrol_break_on(void);
void telecontrol_break_off(void);

EatGpioLevel_enum telecontrol_getSwitchState(void);

#endif/*__TELECONTROL_H__*/
