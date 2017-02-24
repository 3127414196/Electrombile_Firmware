/*
 * record.h
 *
 *  Created on: 2017/01/04
 *      Author: lc
 */
#ifndef __USER_RECORD_H__
#define __USER_RECORD_H__


eat_bool record_start(void);
eat_bool record_stop(void);
void record_modem_run(u8 * buf);


#endif/*__USER_RECORD_H__*/
