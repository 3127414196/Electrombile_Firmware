/*
 * upgrade.c
 *
 *  Created on: 2016年2月18日
 *      Author: jk
 */

#include <stdint.h>
#include <string.h>

#include <eat_fs.h>
#include <eat_interface.h>


#include "upgrade.h"
#include "adler32.h"
#include "setting.h"
#include "log.h"
#include "error.h"
#include "utils.h"
#include "minilzo.h"
#include "mem.h"
#include "fs.h"
#include "flash.h"


void upgrade_DeleteOldApp(void)
{
    fs_delete_file(UPGRADE_FILE_NAME);
    fs_delete_file(RECORDE_FILE_NAME);
}

static UINT upgrade_getAppsize(void)
{
    FS_HANDLE FileHandle;
    UINT filesize = 0;
    int rc = 0;

    FileHandle  = eat_fs_Open(UPGRADE_FILE_NAME, FS_READ_ONLY);
    if(EAT_FS_NO_ERROR <= FileHandle)
    {
        LOG_DEBUG("open file success, fh=%d.", FileHandle);
    }
    else
    {
        LOG_ERROR("open file failed, fh=%d.", FileHandle);
        return 0;
    }

    rc = eat_fs_GetFileSize(FileHandle,&filesize);
    if(EAT_FS_NO_ERROR != rc)
    {
        LOG_ERROR("get file size error , and return error is :%d",rc);
        eat_fs_Close(FileHandle);
        return 0;
    }
    else
    {
        LOG_DEBUG("get file size success , file size is:%d",filesize);
    }

    eat_fs_Close(FileHandle);

    return filesize;
}



static int upgrade_getAppContent(char * app_buf,UINT filesize)
{
    FS_HANDLE FileHandle;
    int rc = 0;

    FileHandle  = eat_fs_Open(UPGRADE_FILE_NAME, FS_READ_ONLY);
    if(EAT_FS_NO_ERROR <= FileHandle)
    {
        LOG_DEBUG("open file success, fh=%d.", FileHandle);

        rc = eat_fs_Read(FileHandle,app_buf,filesize, NULL);
        if (EAT_FS_NO_ERROR == rc)
        {
            LOG_DEBUG("read app file success.");
        }
        else
        {
            LOG_ERROR("read app file fail, and return error: %d", rc);
            return -1;
        }
    }
    else
    {
        LOG_ERROR("open file failed, fh=%d.", FileHandle);
        return -1;
    }

    eat_fs_Close(FileHandle);

    return SUCCESS;
}


/*
*fun:check app file
*return:0 express OK , !0 express not OK
*/
int upgrade_CheckAppfile(int req_size,int req_checksum)
{
    int filesize = 0;
    int checksum = 0;
    char* app_buf = NULL;
    int rc = 0;

    LOG_DEBUG("req->size: %d , req->checksum: %u",req_size,req_checksum);

    filesize = (int)upgrade_getAppsize();

    if(filesize >= SUCCESS)
    {
        LOG_DEBUG("get appLen success:%d",filesize);
    }
    else
    {
        LOG_ERROR("get appLen error!");
        return -1;
    }


    if(filesize != req_size)
    {
        LOG_ERROR("file size is not equal,req->size:filesize = %d:%d",req_size,filesize);
        rc = -1;
    }
    else
    {
        app_buf = malloc(filesize);
        if (!app_buf)
        {
            LOG_ERROR("alloc app_buf error!");
            return -1;
        }

        rc = upgrade_getAppContent(app_buf,filesize);

        if(SUCCESS < rc)
        {
            LOG_DEBUG("get app data failed , and return is %d",app_buf);
        }
        else
        {
            LOG_DEBUG("get app data success.");

            checksum = adler32(app_buf, filesize);
            LOG_DEBUG("appLen:%d,checksum:%u",filesize,checksum);

            if(req_checksum != checksum)
            {
                LOG_ERROR("checksum error,req->checksum:file->checksum = %d:%d",req_checksum,checksum);
                rc = -1;
            }
        }

        free(app_buf);
    }

    return rc;
}


/*
 * Function:    upgrade_createFile
 * Description: create the upgrade file, if the file already exits, truncate it zero size.
 * Parameters :
 *      None.
 *
 * Returns:
 *      0:  success
 *      other: failed
 *
 */
int upgrade_createFile(void)
{
    FS_HANDLE fh;

    LOG_DEBUG("create upgrade file...");

    fh = eat_fs_Delete(UPGRADE_FILE_NAME);
    if(EAT_FS_FILE_NOT_FOUND != fh && EAT_FS_NO_ERROR != fh)
    {
        LOG_ERROR("upgrade file exists , but can't delete it:%d",fh);
        return -1;  //TODO: error code return
    }

    fh = eat_fs_Open(UPGRADE_FILE_NAME,FS_CREATE);
    if(EAT_FS_NO_ERROR <= fh)
    {
        LOG_DEBUG("create file success, fh=%d.", fh);
    }
    else
    {
        LOG_ERROR("create file failed, fh=%d.", fh);
        return -1;  //TODO: error code return
    }

    eat_fs_Close(fh);

    return SUCCESS;
}

/*
 * Function :upgrade_appendFile
 * Description:append the received data to the end of file.
 * Parameters :
 *      offset: [IN]    the offset of the file to be append, now just used to verify
 *      data:   [IN]    the data buffer pointer
 *      length: [IN]    the buffer length
 *
 * Returns:
 *      the length of data written to the file, < 0 if error happened
 *
 * NOTE
 *      The parameter offset is not used currently, just seek to the end of file
 */
int upgrade_appendFile(int offset, char* data,  unsigned int length)
{
    FS_HANDLE fh_open, fh_write, fh_commit,seekRet;
    UINT writedLen;
    int rc = 0;

    fh_open = eat_fs_Open(UPGRADE_FILE_NAME, FS_READ_WRITE);

    if(EAT_FS_NO_ERROR <= fh_open)
    {
        LOG_DEBUG("open app file success, fh=%d.", fh_open);

        seekRet = eat_fs_Seek(fh_open,0,EAT_FS_FILE_END);

        if(seekRet < 0)
        {
            LOG_ERROR("Seek File Pointer Fail");
            rc = -1;
        }
        else
        {
            LOG_DEBUG("Seek File Pointer Success");

            fh_write = eat_fs_Write(fh_open, data, length, &writedLen);
            if((EAT_FS_NO_ERROR == fh_write) && length == writedLen)
            {
                //don't know how soon the next block will come , commit it
                fh_commit = eat_fs_Commit(fh_open);

                if(EAT_FS_NO_ERROR == fh_commit)
                {
                    LOG_DEBUG("commit file success.");
                }
                else
                {
                    LOG_ERROR("commit file failed, and Return Error is %d.", fh_commit);
                    rc = -1;
                }
            }
            else
            {
                LOG_ERROR("write file failed,Error is %d",fh_write);
                rc =-1;
            }
        }
    }
    else
    {
        LOG_ERROR("open file failed, fh=%d!", fh_open);
        rc = -1;
    }

    eat_fs_Close(fh_open);
    return rc;

}

static int upgrade_execute(const void *data, unsigned int len)
{
    u32 APP_DATA_RUN_BASE;  //app run addr
    u32 APP_DATA_STORAGE_BASE;  //app data storage addr

    eat_bool rc;


    APP_DATA_RUN_BASE = eat_get_app_base_addr(); //get app addr
    LOG_DEBUG("APP_DATA_RUN_BASE : %#x",APP_DATA_RUN_BASE);

    APP_DATA_STORAGE_BASE = APP_DATA_RUN_BASE + UPGRADE_DATA_OFFSET;// offset: 150K, size 150K

    rc = eat_flash_erase((void*)APP_DATA_STORAGE_BASE , len);//erase the flash to write new app_data_storage
    if(EAT_FALSE == rc)
    {
        LOG_ERROR("Erase flash failed [0x%08x, %dKByte]", APP_DATA_STORAGE_BASE,  len / 1024);
        return rc;
    }

    rc = eat_flash_write((void*)APP_DATA_STORAGE_BASE , data , len);//write the new app_data_storage
    if(EAT_FALSE == rc)
    {
        LOG_ERROR("Write Flash Failed.");
        return rc;
    }

    fs_delete_file(UPGRADE_FILE_NAME);

    //upgrade app
    eat_update_app((void*)(APP_DATA_RUN_BASE),(void*)(APP_DATA_STORAGE_BASE), len, EAT_PIN_NUM, EAT_PIN_NUM,EAT_FALSE);

    return EAT_TRUE;
}

/*
 * Function :upgrade_do
 * Description:Perform the upgrade process.
 * Parameters :
 *      None.
 *
 * Returns:
 *      0:  success
 *      other: failed
 *
 */
int upgrade_do(void)
{
    char *app_data = 0;
    char *app_data_uncompress = 0;
    UINT app_dataLen = 0;
    UINT app_dataLen_uncompress = 0;

    int rc;

    app_dataLen_uncompress = upgrade_getAppsize();
    if (app_dataLen_uncompress == 0)
    {
        LOG_ERROR("get app file size error");
        return -1;
    }

    app_data_uncompress = malloc(app_dataLen_uncompress);
    if (!app_dataLen_uncompress)
    {
        LOG_DEBUG("alloc failed");
        return -1;
    }

    rc = upgrade_getAppContent(app_data_uncompress, app_dataLen_uncompress);

    if(rc == SUCCESS)
    {
        LOG_DEBUG("get app data success.");
    }
    else
    {
        LOG_ERROR("get app data failed!",);
        return -1;
    }


    app_dataLen = app_dataLen_uncompress * 2;
    app_data = malloc(app_dataLen);
    if (!app_data)
    {
        LOG_DEBUG("alloc failed");
        free(app_data_uncompress);
        return -1;
    }
    //decompress
    rc = miniLZO_decompress(app_data_uncompress, app_dataLen_uncompress, app_data, &app_dataLen);
    if(rc < LZO_E_OK)
    {
        LOG_ERROR("decompress error %d",rc);
        free(app_data_uncompress);
        free(app_data);
        return -1;
    }

    if (upgrade_execute(app_data, app_dataLen) != EAT_TRUE)
    {
        LOG_ERROR("execute update failed");
    }

    free(app_data_uncompress);
    free(app_data);

    return 0;
}


