/*
 * telecontrol.c
 *
 *  Created on: 2016/11/29
 *      Author: lc
 */
#include <eat_interface.h>

#include "telecontrol.h"
/*
*function: 初始化电门开关状态检测模块，初始化下拉
*/
static void telecontrol_initSwitchState(void)
{
    eat_gpio_setup(EAT_PIN60_COL2, EAT_GPIO_DIR_INPUT, EAT_GPIO_LEVEL_LOW); // electric switch flag default low
}

/*
*function: 初始化电机锁模块，初始化默认电机锁关闭
*/
static void telecontrol_break_initial(void)
{
    eat_gpio_setup(EAT_PIN59_COL3, EAT_GPIO_DIR_OUTPUT, EAT_GPIO_LEVEL_LOW);// break state default off
}

/*
*function: 初始化门开关状态控制模块，初始化默认电门关闭
*/
static void telecontrol_switch_initial(void)
{
    eat_gpio_setup(EAT_PIN61_COL1, EAT_GPIO_DIR_OUTPUT, EAT_GPIO_LEVEL_LOW);// switch state default off
}

/*
*function: 打开电机锁
*/
static void telecontrol_break_on(void)
{
    eat_gpio_setup(EAT_PIN59_COL3, EAT_GPIO_DIR_OUTPUT, EAT_GPIO_LEVEL_HIGH);// break on
}

/*
*function: 关闭电机锁
*/
static void telecontrol_break_off(void)
{
    eat_gpio_setup(EAT_PIN59_COL3, EAT_GPIO_DIR_OUTPUT, EAT_GPIO_LEVEL_LOW);// break off
}

/*
*function: 打开电门
*/
void telecontrol_switch_on(void)
{
    eat_gpio_setup(EAT_PIN61_COL1, EAT_GPIO_DIR_OUTPUT, EAT_GPIO_LEVEL_HIGH);// switch on
}

/*
*function: 关闭电门
*/
void telecontrol_switch_off(void)
{
    eat_gpio_setup(EAT_PIN61_COL1, EAT_GPIO_DIR_OUTPUT, EAT_GPIO_LEVEL_LOW);// switch off
}

/*
*function: 远程控制模块初始化
*/
void telecontrol_initail(void)
{
    telecontrol_initSwitchState();
    telecontrol_break_initial();
    telecontrol_switch_initial();
}

/*
*function: 锁死电动车，即电门打开，电机打开
*/
void telecontrol_lock(void)
{
    telecontrol_break_on();
    telecontrol_switch_on();
}

/*
*function: 解锁电动车，即电门关闭，电机关闭
*/
void telecontrol_unlock(void)
{
    telecontrol_break_off();
    telecontrol_switch_off();
}

/*
*function: 获取电门开关状态
*/
EatGpioLevel_enum telecontrol_getSwitchState(void)
{
    return eat_gpio_read(EAT_PIN60_COL2);
}

