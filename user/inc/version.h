/*
 * version.h
 *
 *  Created on: 2015/6/27
 *      Author: jk
 */

#ifndef USER_ELECTROMBILE_VERSION_H_
#define USER_ELECTROMBILE_VERSION_H_


/*
 * Changelog
 * 1.0.0: initial framework
 * 1.1.1: add diagnosis function
 * 1.2.0: add battery, for RC1
 * 1.2.1: add message re-send mechanism
 * 1.2.2: add nmealib, and fix the battery bug
 * 1.2.3: add diagnostic function
 * 1.2.4: change SMS ack to socket message instead of real SMS
 * 1.2.5: allow user to set the battery type manually
 * 1.2.6: remove battery's 50% lower alarm
 * 1.3.1: add outage alarm
 * 1.4.0: add switch on alarm
 */

#define VERSION_MAJOR   1
#define VERSION_MINOR   3
#define VERSION_MICRO   3


#define VERSION_INT(a, b, c)    (a << 16 | b << 8 | c)
#define VERSION_DOT(a, b, c)    a##.##b##.##c
#define VERSION(a, b, c)        VERSION_DOT(a, b, c)

#define STRINGIFY(s)         TOSTRING(s)
#define TOSTRING(s) #s

#define VERSION_STR STRINGIFY(VERSION(VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO))

#define VERSION_NUM VERSION_INT(VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO)

#endif /* USER_ELECTROMBILE_VERSION_H_ */
