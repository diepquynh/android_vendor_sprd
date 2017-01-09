/**************************************************************
* Copyright (c) 2001,西安西电捷通无线网络通信有限公司
* All rights reserved.
* 
* 文件名称：alg_comm.h
* 摘    要：定义WAPID中用的的算法实现函数的原型
* 
* 当前版本：1.1
* 作    者：王月辉yhwang@iwncomm.com
* 完成日期：2005年6月10日
*
* 取代版本：1.0 
* 原作者  ：王月辉yhwang@iwncomm.com
* 完成日期：2004年1月10日
*************************************************************/

#ifndef _ALG_COMM_H
#define _ALG_COMM_H

void update_gnonce(unsigned long *gnonce, int type);
int overflow(unsigned long * gnonce);
void add(unsigned long *a, unsigned long b, unsigned short len);
//void smash_random(unsigned char *buffer, int len );
void get_random(unsigned char *buffer, int len);
int mhash_sha256(unsigned char *data, unsigned length, unsigned char *digest);
void KD_hmac_sha256(unsigned char *text,unsigned text_len,unsigned char *key,
					unsigned key_len, unsigned char  *output,unsigned length);
int wapi_hmac_sha256(unsigned char *text, int text_len, 
				unsigned char *key, unsigned key_len,
				unsigned char *digest, unsigned digest_length);

#endif
