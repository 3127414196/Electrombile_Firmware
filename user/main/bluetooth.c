/*
 * bluetooth.h
 *
 *  Created on: 2017/01/15
 *      Author: kky
 */
#include <stdio.h>
#include <string.h>
#include <eat_interface.h>

#include "log.h"
#include "timer.h"
#include "modem.h"
#include "setting.h"
#include "bluetooth.h"
#include "audio_source.h"

#define BLUETOOTH_TIMER_PERIOD (1 * 1000) // 1s for once

static eat_bool isHostAtNear = EAT_FALSE;
static eat_bool isBluetoothFound = EAT_FALSE;

/*
*fun:reset the bluetooth state
*/
void bluetooth_resetState(void)
{
    isHostAtNear = EAT_FALSE;
}

/*
*fun:check the bluetooth id
*/
int bluetooth_check_run(u8* buf)
{
    if(strstr((const char *)buf, "+BTSCAN: 0") && strstr((const char *)buf,setting.BluetoothId))
    {
        if(!isHostAtNear)
        {
            isHostAtNear = EAT_TRUE;
            set_vibration_state(EAT_FALSE);
            telecontrol_switch_on();
            audio_bluetoothFoundSound();
        }
        isBluetoothFound = EAT_TRUE;
    }
    return 0;
}

static void bluetooth_ScanProc(int time)
{
    static int count = 0;
    static eat_bool isScaning = EAT_FALSE;

    if(!isScaning)
    {
        isScaning = EAT_TRUE;
        isBluetoothFound = EAT_FALSE;
        modem_AT("AT+BTSCAN=1,10" CR);// start scan
    }
    else if(time % 10 == 0)
    {
        isScaning = EAT_FALSE;
        modem_AT("AT+BTSCAN=0" CR);// stop scan
    }

    if(isBluetoothFound)
    {
        count = 0;
    }
    else if(count++ > 60)// if last 60s not find host, as far away
    {
        isHostAtNear = EAT_FALSE;
    }
}

void bluetooth_startLoop(void)
{
    modem_AT("AT+BTPOWER=1" CR);
    eat_timer_start(TIMER_BLUETOOTH, setting.bluetooth_timer_period);
}

void bluetooth_onesecondLoop(void)
{
    static int time = 0;

    if(is_bluetoothOn() && !Vibration_isMoved())
    {
        bluetooth_ScanProc(time++);
    }
}
