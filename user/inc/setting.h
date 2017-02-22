/*
 * setting.h
 *
 *  Created on: 2015/6/24
 *      Author: jk
 */

#ifndef USER_ELECTROMBILE_SETTING_H_
#define USER_ELECTROMBILE_SETTING_H_

#include <eat_type.h>

#define MAX_DOMAIN_NAME_LEN 32
#define BLUETOOTH_ID_LEN 18
typedef enum
{
	ADDR_TYPE_IP,
	ADDR_TYPE_DOMAIN
}ADDR_TYPE;



#pragma anon_unions
typedef struct
{
	//Server configuration
	ADDR_TYPE addr_type;
	union
	{
		char domain[MAX_DOMAIN_NAME_LEN];
		u8 ipaddr[4];
	};
	u16 port;
    char ftp_domain[MAX_DOMAIN_NAME_LEN];

	//Timer configuration
    struct
    {
        u32 main_loop_timer_period;
        u32 vibration_timer_period;
        u32 seek_timer_period;
        u32 timeupdate_timer_peroid;
        u32 gps_send_period;
        u32 bluetooth_timer_period;
    };

    //Switch configuration
    eat_bool isVibrateFixed;

    //autolock configration
    struct
    {
        eat_bool isAutodefendFixed;
        unsigned char autodefendPeriod;
    };

    //battery type
    eat_bool isUserType;
    u8 BatteryType;
    u8 BaterryType_Judging;
    eat_bool isBatteryJudging;

    //bluetooth id
    eat_bool BluetoothSwitch;
    char BluetoothId[BLUETOOTH_ID_LEN];

}SETTING;



extern SETTING setting;

eat_bool vibration_fixed(void) __attribute__((always_inline));
void set_vibration_state(eat_bool fixed) __attribute__((always_inline));

void set_autodefend_state(eat_bool fixed);
unsigned char get_autodefend_period(void);

void set_autodefend_period(unsigned char period);
eat_bool get_autodefend_state(void);

eat_bool isUserBatteryTpye(void);
void set_UserBatteryTpye(u8 batteryType);
unsigned char get_battery_type(void);
void set_battery_type(unsigned char batterytype);
unsigned char get_batterytype_Judging(void);
eat_bool get_battery_isJudging(void);
void set_battery_isJudging(eat_bool isBatteryJudging, u8 baterrytype);
void set_bluetooth_id(const char* BluetoothIdString);
void set_bluetooth_switch(eat_bool sw);
eat_bool is_bluetoothOn(void);


eat_bool setting_restore(void);
eat_bool setting_save(void);

#endif /* USER_ELECTROMBILE_SETTING_H_ */
