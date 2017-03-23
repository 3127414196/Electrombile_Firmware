/*
 * ftp.h
 *
 *  Created on: 2017/01/04
 *      Author: lc
 */
#ifndef __USER_FTP_H__
#define __USER_FTP_H__

void ftp_modem_run(u8 * buf);
void ftp_download_file(char *localFileName, char *serverFileName);
void ftp_upload_file(char *localFileName, char *serverFileName);

#define MAX_SERVERFILENAME_LEN 64

enum FTP_ERROR_CODE
{
    FTP_SUCCESS,
    FTP_FTPCID_ERROR,
    FTP_FTPSERV_ERROR,
    FTP_FTPUN_ERROR,
    FTP_FTPSERVNAME_ERROR,
    FTP_FTPPUTPATH_ERROR,
    FTP_FTPGETPATH_ERROR,
    FTP_FTPPUTFILE_ERROR,
    FTP_FTPGETFILE_ERROR
};


#endif/*_USER_FTP_H_*/
