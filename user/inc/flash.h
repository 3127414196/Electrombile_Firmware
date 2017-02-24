/*
 * fs.h
 *
 *  Created on: 2017/1/11
 *      Author: lc
 */
#ifndef USER_INC_FLASH_H_
#define USER_INC_FLASH_H_

/*flash.h头文件规划写入flash数据首地址地址偏移量
 *偏移量基准获取: eat_get_app_base_addr()     (0x10309000)
 *总的app space 大小为: 588 * 1024
 */

#define UPGRADE_DATA_OFFSET (150 * 1024)                        // start: 150K, size: 150K
#define BTAUDIO_NEAR_OFFSET (UPGRADE_DATA_OFFSET + 150 * 1024)  // start: 300K, size: 25K
#define BTAUDIO_AWAY_OFFSET (BTAUDIO_NEAR_OFFSET + 25 * 1024)   // start: 325K, size: 25K
#define FREE_OFFSET         (BTAUDIO_AWAY_OFFSET + 25 * 1024)   // start: 350K, size: 238K

#endif /*USER_INC_FLASH_H_*/
