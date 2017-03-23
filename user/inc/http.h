/*
 * ftp.h
 *
 *  Created on: 2017/01/04
 *      Author: lc
 */
#ifndef __USER_FTP_H__
#define __USER_FTP_H__

void http_modem_run(u8 * buf, u16 len);
void http_download_file(const unsigned char *localFileName, unsigned char * serverFileName);
void http_upload_file(const unsigned char *localFileName);

#define MAX_SERVERFILENAME_LEN 64


#endif/*_USER_FTP_H_*/
