/*
 * fs.h
 *
 *  Created on: 2016年2月19日
 *      Author: jk
 */

#ifndef USER_INC_FS_H_
#define USER_INC_FS_H_
#include "eat_fs_type.h"

#define SETTINGFILE_NAME  L"C:\\setting.conf"
#define UPGRADE_FILE_NAME  L"C:\\app.bin"
#define RECORDE_FILE_NAME L"C:\\record.amr"
#define RECORD_FILE_NAME_CHAR "C:\\record.amr"

void fs_initial(void);
SINT64 fs_getDiskFreeSize(void);

int fs_factory(void);
int fs_delete_file(const WCHAR * FileName);



#endif /* USER_INC_FS_H_ */
