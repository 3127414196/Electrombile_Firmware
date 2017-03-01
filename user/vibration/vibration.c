/*
 * vibration.c
 *
 *  Created on: 2015??7??1??
 *      Author: jk
 */

#include <stdlib.h>
#include <eat_interface.h>
#include <eat_periphery.h>

#include "vibration.h"
#include "thread.h"
#include "log.h"
#include "timer.h"
#include "thread_msg.h"
#include "setting.h"
#include "mma8652.h"
#include "led.h"
#include "data.h"
#include "telecontrol.h"

#define MAX_MOVE_DATA_LEN   500
#define MOVE_TIMER_PERIOD    10

#define MOVE_THRESHOLD 5


void DigitalIntegrate(float * sour, float * dest,int len,float cycle)
{
	int i;
	if (len==0)
		return ;
	dest[0]=0;
	for (i=1;i<len;i++)
	{
		dest[i]=dest[i-1]+(sour[i-1]+sour[i])*cycle/2;
	}
}


static eat_bool vivration_AutolockStateSend(eat_bool state)
{
    eat_bool ret;
    u8 msgLen = sizeof(MSG_THREAD) + sizeof(AUTOLOCK_INFO);
    MSG_THREAD* msg = allocMsg(msgLen);
    AUTOLOCK_INFO* msg_state = 0;

    if (!msg)
    {
        LOG_ERROR("alloc msg failed!");
        return EAT_FALSE;
    }

    msg->cmd = CMD_THREAD_AUTOLOCK;
    msg->length = sizeof(AUTOLOCK_INFO);

    msg_state = (AUTOLOCK_INFO*)msg->data;
    msg_state->state = state;

    LOG_DEBUG("send autolock state msg to main thread!");
    ret = sendMsg(THREAD_MAIN, msg, msgLen);

    return ret;
}

static eat_bool vibration_sendAlarm(int cmd)
{
    u8 msgLen = sizeof(MSG_THREAD) + sizeof(ALARM_INFO);
    MSG_THREAD *msg = NULL;
    ALARM_INFO *msg_data = NULL;

    msg = allocMsg(msgLen);
    if(!msg)
    {
        LOG_ERROR("alloc memory error");
        return EAT_FALSE;
    }
    msg_data = (ALARM_INFO*)msg->data;

    msg->cmd = CMD_THREAD_ALARM;
    msg->length = sizeof(ALARM_INFO);
    msg_data->alarm_type = cmd;

    LOG_DEBUG("vibration alarm:cmd(%d),length(%d),data(%d)", msg->cmd, msg->length, msg_data->alarm_type);
    return sendMsg(THREAD_MAIN, msg, msgLen);
}

static eat_bool vibration_alarm_move(void)
{
    u8 msgLen = sizeof(MSG_THREAD) + sizeof(ALARM_INFO);
    MSG_THREAD *msg = NULL;
    ALARM_INFO *msg_data = NULL;

    Add_AlarmCount();
    if(Get_AlarmCount() < 2)
    {
        set_vibration_state(EAT_FALSE);// if alarm, set the vibration state as defend off
        return vibration_sendAlarm(ALARM_VIBRATE);
    }

    return EAT_FALSE;
}

static eat_bool vibration_alarm_cutoff(void)
{
    return vibration_sendAlarm(ALARM_BAT_CUT);
}

static eat_bool vibration_alarm_switchOpen(void)
{
    set_vibration_state(EAT_FALSE);// if alarm, set the vibration state as defend off
    return vibration_sendAlarm(ALARM_SWITCH_CHANGE);
}

static void move_alarm_timer_handler()
{
    unsigned char readbuf[3];
    int i;
    float tmp[3]={0};
    short temp;
    static float x_data[MAX_MOVE_DATA_LEN], y_data[MAX_MOVE_DATA_LEN], z_data[MAX_MOVE_DATA_LEN];
    float temp_data[MAX_MOVE_DATA_LEN];
    static int timerCount = 0;
    bool ret = mma8652_i2c_register_read(MMA8652_REG_OUT_X_MSB, readbuf, 3);
    temp = readbuf[0]<<8;
    x_data[timerCount] = temp/256;
     temp = readbuf[1]<<8;
    y_data[timerCount] = temp/256;
     temp = readbuf[2]<<8;
    z_data[timerCount] = temp/256;
    timerCount++;

    if(timerCount<MAX_MOVE_DATA_LEN)
    {
        eat_timer_start(TIMER_MOVE_ALARM, MOVE_TIMER_PERIOD);
    }
    else
    {
        timerCount = 0;
        for(i=0;i<MAX_MOVE_DATA_LEN;i++)
        {
            tmp[0] += (x_data[i]/MAX_MOVE_DATA_LEN);
            tmp[1] += (y_data[i]/MAX_MOVE_DATA_LEN);
            tmp[2] += (z_data[i]/MAX_MOVE_DATA_LEN);
        }
        for(i=0;i<MAX_MOVE_DATA_LEN;i++)
        {
            x_data[i] = x_data[i] - tmp[0];
            y_data[i] = y_data[i] - tmp[1];
            z_data[i] = z_data[i] - tmp[2];
        }
        DigitalIntegrate(x_data, temp_data, MAX_MOVE_DATA_LEN,MOVE_TIMER_PERIOD/1000.0);
        DigitalIntegrate(temp_data, x_data, MAX_MOVE_DATA_LEN,MOVE_TIMER_PERIOD/1000.0);
        for(i=0;i<MAX_MOVE_DATA_LEN;i++)
        {
            if(x_data[0]<abs(x_data[i]))
            {
                x_data[0] = abs(x_data[i]);
            }
            if(x_data[i] > MOVE_THRESHOLD)
            {
                if(EAT_TRUE == vibration_fixed())
                {
                    vibration_alarm_move();
                    LOG_DEBUG("MOVE_TRESHOLD_Z[%d]   = %f",i, x_data[i]);
                }
                return;
            }

        }
        LOG_DEBUG("MAX_X  = %f", x_data[0]);
        DigitalIntegrate(y_data, temp_data, MAX_MOVE_DATA_LEN,MOVE_TIMER_PERIOD/1000.0);

        DigitalIntegrate(temp_data, y_data, MAX_MOVE_DATA_LEN,MOVE_TIMER_PERIOD/1000.0);
        for(i=0;i<MAX_MOVE_DATA_LEN;i++)
        {
            if(y_data[i] > MOVE_THRESHOLD)
            {
                if(EAT_TRUE == vibration_fixed())
                {
                    vibration_alarm_move();
                    LOG_DEBUG("MOVE_TRESHOLD_Z[%d]   = %f",i, y_data[i]);
                }
                return;
            }
            if(y_data[0]<abs(y_data[i]))
            {
                y_data[0] = abs(y_data[i]);
            }

        }
        LOG_DEBUG("MAX_Y  = %f", y_data[0]);
        DigitalIntegrate(z_data, temp_data, MAX_MOVE_DATA_LEN,MOVE_TIMER_PERIOD/1000.0);
        DigitalIntegrate(temp_data, z_data, MAX_MOVE_DATA_LEN,MOVE_TIMER_PERIOD/1000.0);
        for(i=0;i<MAX_MOVE_DATA_LEN;i++)
        {
            if(z_data[i] > MOVE_THRESHOLD)
            {
                if(EAT_TRUE == vibration_fixed())
                {
                    vibration_alarm_move();
                    LOG_DEBUG("MOVE_TRESHOLD_Z[%d]   = %f",i, z_data[i]);
                }
                return;
            }
            if(z_data[0]<abs(z_data[i]))
            {
                z_data[0] = abs(z_data[i]);
            }
        }
        LOG_DEBUG("MAX_z  = %f", z_data[0]);
    }


}

static void vibration_move_handler(void)
{
    static eat_bool isFirstTime = EAT_TRUE;

    uint8_t transient_src = 0;

    mma8652_i2c_register_read(MMA8652_REG_TRANSIENT_SRC, &transient_src, sizeof(transient_src));
    if(transient_src & MMA8652_TRANSIENT_SRC_EA)
    {
        /* At the first time, the value of MMA8X5X_TRANSIENT_SRC is strangely 0x60.
         * Do not send alarm at the first time.
         */
        if(isFirstTime)
        {
            isFirstTime = EAT_FALSE;

            Vibration_setMoved(EAT_FALSE);
        }
        else
        {
            Vibration_setMoved(EAT_TRUE);
        }
    }
    else
    {
        Vibration_setMoved(EAT_FALSE);
    }

    if(Vibration_isMoved())
    {
        ResetVibrationTime();
        LOG_DEBUG("shake!");
        eat_timer_start(TIMER_MOVE_ALARM, MOVE_TIMER_PERIOD);
    }
    else
    {
        VibrationTimeAdd();

        if(getVibrationTime() * setting.vibration_timer_period >= (get_autodefend_period() * 60000))
        {
            if(get_autodefend_state())
            {
                if(EAT_FALSE == vibration_fixed())
                {
                    vivration_AutolockStateSend(EAT_TRUE);    //TODO:send autolock_msg to main thread
                    set_vibration_state(EAT_TRUE);
                }
            }
            telecontrol_switch_off();
            Reset_AlarmCount();
        }
    }

    return;
}

static void vibration_switchState_handler(void)
{
    static EatGpioLevel_enum last_switchState = EAT_GPIO_LEVEL_LOW;

    EatGpioLevel_enum switchState = telecontrol_getSwitchState();
    if(vibration_fixed())
    {
        if(EAT_GPIO_LEVEL_LOW == last_switchState && EAT_GPIO_LEVEL_HIGH == switchState)
        {
            vibration_alarm_switchOpen();
        }
    }
    last_switchState = switchState;
}

static void vibration_cutoff_handler(void)
{
    static EatGpioLevel_enum last_level = EAT_GPIO_LEVEL_LOW;

    EatGpioLevel_enum pin_level = eat_gpio_read(EAT_PIN62_COL0);
    if(EAT_GPIO_LEVEL_HIGH == last_level && EAT_GPIO_LEVEL_LOW == pin_level)
    {
        vibration_alarm_cutoff();
    }
    last_level = pin_level;
}

static void vibration_switchState_handler(void)
{
    static EatGpioLevel_enum last_switchState = EAT_GPIO_LEVEL_LOW;

    EatGpioLevel_enum switchState = telecontrol_getSwitchState();
    if(vibration_fixed())
    {
        if(EAT_GPIO_LEVEL_LOW == last_switchState && EAT_GPIO_LEVEL_HIGH == switchState)
        {
            vibration_alarm_switchOpen();
        }
    }
    last_switchState = switchState;
}

static void vibration_oneSecond_Loop(void)
{
    vibration_move_handler();
    vibration_cutoff_handler();
    vibration_switchState_handler();
}

void app_vibration_thread(void *data)
{
	EatEvent_st event;
	bool ret;

	LOG_INFO("vibration thread start.");

	ret = mma8652_init();
	if (!ret)
	{
        LED_startFastBlink();
        LOG_ERROR("mma8652 init failed");
	}
	else
	{
	    mma8652_config();
	}

	eat_timer_start(TIMER_VIBRATION, setting.vibration_timer_period);

	while(EAT_TRUE)
	{
        eat_get_event_for_user(THREAD_VIBRATION, &event);
        switch(event.event)
        {
            case EAT_EVENT_TIMER:
                switch (event.data.timer.timer_id)
                {
                    case TIMER_VIBRATION:
                        vibration_oneSecond_Loop();
                        eat_timer_start(TIMER_VIBRATION, setting.vibration_timer_period);
                        break;
                    case TIMER_MOVE_ALARM:
                        move_alarm_timer_handler();
                        break;

                    default:
                        LOG_ERROR("timer(%d) expire!", event.data.timer.timer_id);
                        break;
                }
                break;

            default:
            	LOG_ERROR("event(%d) not processed!", event.event);
                break;
        }
    }
}


