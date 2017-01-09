/**************************************************************
* Copyright (c) 2004,西安西电捷通无线网络通信有限公司
* All rights reserved.
* 
* 文件名称：alg_comm.c
* 摘    要：定义WAPID中用的的算法
* 
* 当前版本：1.1
* 作    者：王月辉yhwang@iwncomm.com
* 完成日期：2005年6月10日
*
* 取代版本：1.0 
* 原作者  ：王月辉yhwang@iwncomm.com
* 完成日期：2004年1月10日
*************************************************************/

/* Standard C library includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <openssl/rand.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include "alg_comm.h"
#include "wapi_common.h"
#define SHA256_DIGEST_SIZE 32
#define SHA256_DATA_SIZE 64
#define _SHA256_DIGEST_LENGTH 8

int overflow(unsigned long *gnonce)
{
	unsigned char flow[8]={0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};

	if (memcmp(gnonce, flow, 8) == 0)
		return 0;
	else
		return -1;
}

/*大数加*/
void add(unsigned long *a, unsigned long b, unsigned short len)
{
	int i = 0;
	unsigned long carry = 0;
	unsigned long *a1 = NULL;
	unsigned long ca1 = 0;
	unsigned long ca2 = 0;
	unsigned long ca3 = 0;
	unsigned long bb = b;

	for(i=len - 1; i>=0; i--)                         //分段32bits加                      
	{	
		a1=a+i; 
		ca1 = (*a1)&0x80000000;	
		ca2 = bb&0x80000000;
		*a1 += bb + carry;
		bb = 0;
		ca3 = (*a1)&0x80000000;	
		if(ca1==0x80000000 && ca2==0x80000000)  carry=1; 
		else if(ca1!=ca2 && ca3==0)  carry=1; 
        	else carry=0;
		a1++;
	}
	
}

void update_gnonce(unsigned long *gnonce, int type)
{
	add(gnonce, type+1, 4);
}

/*for 3Ci*/
static void smash_random(unsigned char *buffer, int len )
{
	 unsigned char smash_key[32] = {  0x09, 0x1A, 0x09, 0x1A, 0xFF, 0x90, 0x67, 0x90,
									0x7F, 0x48, 0x1B, 0xAF, 0x89, 0x72, 0x52, 0x8B,
									0x35, 0x43, 0x10, 0x13, 0x75, 0x67, 0x95, 0x4E,
									0x77, 0x40, 0xC5, 0x28, 0x63, 0x62, 0x8F, 0x75};
	KD_hmac_sha256(buffer, len, smash_key, 32, buffer, len);
}


/*取随机数*/
void get_random(unsigned char *buffer, int len)
{
	RAND_bytes(buffer, len);
	smash_random(buffer, len);
}


/*标准SHA256 hash算法*/
int mhash_sha256(unsigned char *data, unsigned length, unsigned char *digest)
{

	SHA256((const unsigned char *)data, length, digest);
        return 0;
}

/*HMAC-SHA256算法*/

int wapi_hmac_sha256(unsigned char *text, int text_len, 
	unsigned char *key, unsigned key_len, unsigned char *digest,
	unsigned digest_length)
{
	unsigned char out[SHA256_DIGEST_SIZE] = {0,};
	HMAC(EVP_sha256(),
			key,
			key_len,
			text,
			text_len,
			out,
			NULL);
	memcpy(digest, out, digest_length);
	return 0;
}

/*KD-HMAC-SHA256算法*/
void KD_hmac_sha256(unsigned char *text,unsigned text_len,unsigned char *key,
					unsigned key_len, unsigned char  *output, unsigned length)
{
	unsigned i;
	
	for(i=0;length/SHA256_DIGEST_SIZE;i++,length-=SHA256_DIGEST_SIZE){
		wapi_hmac_sha256(
			text,
			text_len,
			key,
			key_len, 
			&output[i*SHA256_DIGEST_SIZE],
			SHA256_DIGEST_SIZE);
		text=&output[i*SHA256_DIGEST_SIZE];
		text_len=SHA256_DIGEST_SIZE;
	}

	if(length>0)
		wapi_hmac_sha256(
			text,
			text_len,
			key,
			key_len, 
			&output[i*SHA256_DIGEST_SIZE],
			length);

}

