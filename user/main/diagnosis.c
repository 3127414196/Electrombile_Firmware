/*
 * diagnosis.c
 *
 *  Created on: 2016年3月4日
 *      Author: jk
 */

#include <eat_type.h>
#include <eat_periphery.h>
#include <eat_network.h>

#include "diagnosis.h"
#include "adc.h"
#include "led.h"
#include "log.h"

#define Realvalue_2_ADvalue(x) x*1000*3/103 //unit mV, 3K & 100k divider

/*
 * 检测输入电压范围
 */
static eat_bool diag_batterCheck(void)
{
    eat_bool rc;
    u32 voltage;

    rc = eat_get_adc_sync(ADC_VOLTAGE, &voltage);
    if (!rc)
    {
        LOG_ERROR("Get battery voltage failed");
        return EAT_FALSE;
    }

    //电池电压介于[36v, 66v]之间
    //FIXME: 根据分压计算区间
    if (voltage < Realvalue_2_ADvalue(36) || voltage > Realvalue_2_ADvalue(66))
    {
        LOG_ERROR("battery voltage check failed: %d", voltage);
        return EAT_FALSE;
    }
    else
    {
        LOG_DEBUG("battery voltage check success: %d", voltage);
    }

    return EAT_TRUE;
}


/*
 * 检测GSM的信号强度是否 > 13
 */
static eat_bool diag_gsmSignalCheck(void)
{
    int csq = eat_network_get_csq();
    if (csq < 13)
    {
        LOG_ERROR("GSM signal quality not enough: %d", csq);
        return EAT_FALSE;
    }
    else
    {
        LOG_DEBUG("GSM signal quality enough: %d", csq);
    }

    return EAT_TRUE;
}

/*
 * 检测433的信号强度
 */
static eat_bool diag_433Check(void)
{
    eat_bool rc;
    u32 voltage;

    rc = eat_get_adc_sync(ADC_433, &voltage);
    if (!rc)
    {
        LOG_ERROR("Get 433 signal quality failed");
        return EAT_FALSE;
    }

    //检查433信号强度是否在[100mv, 300mv]之间
    if (voltage < 100 || voltage > 300)
    {
        LOG_ERROR("433 signal quality not enough: %d", voltage);
        return EAT_FALSE;
    }
    else
    {
        LOG_DEBUG("433 signal quality enough: %d", voltage);
    }

    return EAT_TRUE;
}

/*
 * 自检总入口
 */
eat_bool diag_check(void)
{
    LED_on();

    if (!diag_batterCheck())
    {
        LOG_ERROR("battery check failed!");
        //TODO: light the led
        LED_off();
        return EAT_FALSE;
    }

    if (!diag_gsmSignalCheck())
    {
        LOG_ERROR("GSM check failed!");
        //TODO: light the led
        LED_off();

        return EAT_FALSE;
    }

    if (!diag_433Check())
    {
        LOG_ERROR("433 check failed!");
        //TODO: light the led
        LED_off();

        return EAT_FALSE;
    }

    LOG_DEBUG("System check ok");
    return EAT_TRUE;
}
