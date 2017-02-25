/*
 * modem.h
 *
 *  Created on: 2016年2月4日
 *      Author: jk
 */

#ifndef USER_INC_MODEM_H_
#define USER_INC_MODEM_H_
#include <eat_type.h>

eat_bool modem_IsCallReady(char* modem_rsp);

#if 0
eat_bool modem_ReadGPRSStatus(void);
eat_bool modem_IsGPRSAttached(char* modem_rsp);
#else
eat_bool modem_GPRSAttach(void);
#endif

eat_bool modem_switchEngineeringMode(int mode, int Ncell);

eat_bool modem_readCellInfo(void);

eat_bool modem_readCCIDInfo(void);
eat_bool modem_IsCCIDOK(char* modem_rsp);

eat_bool modem_GNSS(void);

eat_bool modem_AT(unsigned char *cmd);

#define CR  "\r"    //CR (carriage return)
#define LF  "\n"    //LF (line feed - new line)

#define READ_BUFF_SIZE 1024
#endif /* USER_INC_MODEM_H_ */
