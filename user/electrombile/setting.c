/*
 * setting.c
 *
 *  Created on: 2015��6��24��
 *      Author: jk
 */
#include "setting.h"

static SETTING setting =
{
		ADDR_TYPE_DOMAIN,
		{0},
		"server.xiaoan110.com"
};

/*
 * ��FLASH�ж�ȡ����
 */
eat_bool SETTING_initial(void)
{

}

/*
 * �������õ�FLASH
 */
eat_bool SETTING_save(void)
{

}
