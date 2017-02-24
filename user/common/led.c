/*
 * led.c
 *
 *  Created on: 2016年3月7日
 *      Author: jk
 */
#include <eat_periphery.h>
#include <eat_timer.h>

#include "log.h"

#define ONE_SECOND_GPT_TIME 16384
void LED_on(void)
{
    eat_bool rc;

    rc = eat_gpio_setup(EAT_PIN55_ROW3, EAT_GPIO_DIR_OUTPUT, EAT_GPIO_LEVEL_HIGH);

    if(EAT_FALSE == rc)
    {
        LOG_ERROR("LED_ON error");
    }
}

void LED_off(void)
{
    eat_bool rc;

    rc = eat_gpio_setup(EAT_PIN55_ROW3, EAT_GPIO_DIR_OUTPUT, EAT_GPIO_LEVEL_LOW);

    if(EAT_FALSE == rc)
    {
        LOG_ERROR("LED_OFF error");
    }
}

static void LED_fastBlink(void)
{
    static eat_bool state = EAT_TRUE;

    eat_bool rc;
    state = !state;
    rc = eat_gpio_setup(EAT_PIN55_ROW3, EAT_GPIO_DIR_OUTPUT, state);
    if(EAT_FALSE == rc)
    {
        LOG_ERROR("LED_OFF error");
    }
}

void LED_startFastBlink(void)
{
    eat_gpt_start(ONE_SECOND_GPT_TIME, EAT_TRUE, LED_fastBlink);
}

void LED_slowBlink(void)
{

}
