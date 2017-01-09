/*
*  sprd_efuse_hw.c
*
*  Created on: 2014-06-05
*  Author: ronghua.yu
*  Modify: xianke.cui 2014-10-23
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <getopt.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sprd_efuse_hw.h>

#if defined(ANDROID)
#include <android/log.h>
#include <utils/Log.h>

#undef LOG_TAG
#define LOG_TAG		"sprd_efuse_hal"
#undef LOG_NDEBUG
#define LOG_NDEBUG	0 //LOGV
#undef LOG_NDDEBUG
#define LOG_NDDEBUG	0 //LOGD
#define BLOCK_SIZE 			(4)
#define UID_BLOCK_START                 (0)
#define UID_BLOCK_END                   (1)
#define UID_BLOCK_LEN                   (UID_BLOCK_END - UID_BLOCK_START + 1)
#define HASH_BLOCK_START                (7)
#define HASH_BLOCK_END                  (11)
#define HASH_BLOCK_LEN                  (HASH_BLOCK_END - HASH_BLOCK_START + 1)
#define SECURE_BOOT_BLOCK               (2)
#define SECURE_BOOT_BIT                 (0)
#define MIN(a, b)			((a) <= (b) ? (a) : (b))

#define HASH_BIT 			(0X80000000)

#if 1
#define DEV_NODE "/dev/sprd_efuse_otp"
#else
#define DEV_NODE "/dev/sprd_dummy_otp"
#endif

#define DEV_NODE_MAGIC "/sys/class/misc/sprd_efuse_otp/magic"

#define MAGIC_NUMBER		(0x8810)
#define LOGI(fmt, args...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, fmt, ##args)
#define LOGD(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##args)
#define LOGE(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, fmt, ##args)
#endif

//#define DEV_NODE_EFUSE_BLOCK	        "/sys/class/misc/sprd-efuse/efuse_block"

//#define DEV_NODE_EFUSE_DATA	        "/sys/class/misc/sprd-efuse/efuse_data"

int write_ok_flag = 0;


static int efuse_write(unsigned int blk, unsigned int val)
{
	char magic[8]={0}, buf[16] = {0};
	off_t curpos,offset;
	int fd = -1;
	int len = 0, ret = 0;
	char *pBuf = (char*)&val;

	ALOGD("%s()->Line:%d; blk = %d, data = 0x%08x \n", __FUNCTION__, __LINE__, blk, val);

	fd = open(DEV_NODE_MAGIC,O_WRONLY);
	if(fd < 0){
		ALOGE("CXK MAGIC open fail");
		return -1;
	}
	sprintf(magic,"%x",MAGIC_NUMBER);
	len = write(fd,magic,strlen(magic));
	if(len <= 0){
		ALOGE("%s()->Line:%d; write magic error\n", __FUNCTION__, __LINE__);
		close(fd);
		return -2;
	}
	close(fd);
	fd = open(DEV_NODE, O_RDWR);
	if(fd < 0) {
		ALOGE("%s()->Line:%d; %s open error, fd = %d. \n", __FUNCTION__, __LINE__, DEV_NODE, fd);
		return -3;
	}

	sprintf(buf, "%08x", val);

	offset = blk * BLOCK_SIZE;
	curpos = lseek(fd, offset, SEEK_CUR);
	if(curpos == -1){
		ALOGE("%s()->Line:%d; lseek error\n", __FUNCTION__, __LINE__);
		close(fd);
		return -4;
	}
	ALOGD("val=%x,buf=%s\n",val,buf);

	len = write(fd, pBuf, 4);
	ALOGD("pBuf[0~3]=%02x,%02x,%02x,%02x\n",pBuf[0],pBuf[1],pBuf[2],pBuf[3]);
	if(len <= 0) {
		ALOGE("%s()->Line:%d; write efuse block(%d) data(%s) error, retcode = %d; \n", __FUNCTION__,  __LINE__, blk, buf, len);
		close(fd);
		return -5;
	}
	close(fd);
	return len;
}

static int efuse_read(unsigned int blk, unsigned char* data)
{
	char buf[5] = {0};
	off_t curpos,offset;
	int fd = -1, ret = 0,i=0;
	if(data == 0)
		return -1;

	fd = open(DEV_NODE, O_RDWR);
	if(fd < 0) {
		ALOGE("%s()->Line:%d; %s open error, fd = %d. \n", __FUNCTION__, __LINE__, DEV_NODE, fd);
		return -2;
        }
	offset = blk * BLOCK_SIZE;
	curpos = lseek(fd, offset, SEEK_CUR);
	if(curpos == -1){
		ALOGE("%s()->Line:%d; lseek error\n", __FUNCTION__, __LINE__);
		close(fd);
		return -3;
	}
	ret = read(fd, buf, 4);
	if(ret <= 0) {
		ALOGE("%s()->Line:%d; read efuse block(%d) data error, retcode = %d; \n", __FUNCTION__,  __LINE__, blk, ret);
		close(fd);
		return -4;
	}
        close(fd);
	//*data = strtoul(buf1, 0, 16);
	sprintf(data,"%02x%02x%02x%02x",buf[3],buf[2],buf[1],buf[0]);
	ALOGD("buf=%s\n",data);
	return 1;
}


int efuse_secure_enable(void)
{
	int ret = 0;
	if(write_ok_flag ==0){
		return -1;
	}
	ret = efuse_write(SECURE_BOOT_BLOCK, (1 << SECURE_BOOT_BIT));
	if(ret <= 0)
		return -2;
	return 0;
}

int efuse_secure_is_enabled(void)
{
	unsigned char value[10] = {0};
	unsigned int _value = 0;
	int ret = efuse_read(SECURE_BOOT_BLOCK, value);
	_value = (unsigned int)(strtoul(value,0,16));
	if (ret <= 0)
		return 0;

	return (_value & (1 << SECURE_BOOT_BIT)) ? 1 : 0;
}

int efuse_is_hash_write(void)
{
	char read_hash[41] = {0},tmp_str[9]={0},*p=NULL;
	int value=0,i=0,cnt=0;	
	if(efuse_hash_read(read_hash,40) < 0){
		ALOGD("%s()->Line:%d; read hash string = %s,\n", __FUNCTION__, __LINE__,read_hash);
		return -1;
	}
	p = read_hash;
	for(i = 0; i < 40; i += 8){
		strncpy(tmp_str,&p[i],8);
		tmp_str[8]='\0';
		value = (unsigned int)(strtoul(tmp_str, 0, 16));
		if(value == 0)
			cnt++;	
	}
	if(cnt == 5)
		return 0;

	return 1;
}
int efuse_hash_read(unsigned char *hash, int count)
{
	unsigned char values[HASH_BLOCK_LEN*8+1] = { 0 };
	int i = 0, len = 0, ret = -1;

	ALOGD("%s()->Line:%d; count = %d \n", __FUNCTION__, __LINE__, count);

	if ((0 == hash) || (count < 1))
		return -1;

	len = MIN(count, HASH_BLOCK_LEN *8);
	for (i = HASH_BLOCK_START; i <= HASH_BLOCK_END; i++) {
		ret = efuse_read(i, &values[(i-HASH_BLOCK_START) * 8]);
		if (ret <= 0) {
			return -2;
		}
	}
	ALOGD("values=%s\n",values);
	strncpy( hash, values, len);
        //memcpy((unsigned char*) hash, (char*) &buf, len);

	ALOGD("%s()->Line:%d; hash = %s, len = %d \n", __FUNCTION__, __LINE__, hash, len-1);

	return (len-1);
}

int efuse_hash_write(unsigned char *hash, int count)
{
	int i = 0, j = HASH_BLOCK_START, len = 0, ret = 0;
	char buf[9] = { 0 },tmp_hash[9] = {0},read_hash[9]={0};
	unsigned char *p = hash;
	unsigned int value = 0;
	unsigned char *q=  (char*)&value;
	if ((0 == p) || (count < 1))
		return -1;
	write_ok_flag = 0;
	ALOGD("%s()->Line:%d; hash string = %s, len = %d \n", __FUNCTION__, __LINE__, hash, count);
	count = MIN(count,HASH_BLOCK_LEN * 8);
	for (i = 0; i < count; i += 8) {
		memset(buf, 0, sizeof(buf));
		strncpy((char*)buf, (char*)&p[i], 8);
		buf[8]='\0';
		value = (unsigned int)(strtoul(buf, 0, 16));
		value &= 0x7fffffff;
		sprintf(tmp_hash,"%02x%02x%02x%02x",q[3],q[2],q[1],q[0]);
		tmp_hash[8]='\0';
		ALOGD("tmp_hash=%s\n",tmp_hash);
		if((ret = efuse_write(j, value)) <= 0) {
			return -2;
		}
		if(efuse_read(j,read_hash) < 0){
			ALOGD("%s()->Line:%d; read hash error\n", __FUNCTION__, __LINE__);
			return -3;	
		}
		if(strncmp(tmp_hash,read_hash,8)){
			ALOGD("%s()->Line:%d READ STRING != TMP STRING\n", __FUNCTION__, __LINE__);
			return -4;
		}
		if(efuse_write(j, HASH_BIT )<0){
			ALOGD("%s()->Line:%d; WRITE HASH_BIT error\n", __FUNCTION__, __LINE__);
			return -5;
		}	
		len += ret;
		j++;
	}
	write_ok_flag = 1;
	return len;
}

int efuse_uid_read(unsigned char *uid, int count)
{
	unsigned char values[UID_BLOCK_LEN * 8 + 1] = { 0 };
	int i = 0, len = 0, ret = -1;

	ALOGD("%s()->Line:%d; count = %d \n", __FUNCTION__, __LINE__, count);

	if ((0 == uid) || (count < 1))
		return -1;

	len = MIN(count, sizeof(values));
	for (i = UID_BLOCK_START; i <= UID_BLOCK_END; i++) {
		ret = efuse_read(i, &values[(i - UID_BLOCK_START) * 8]);
		if (ret <= 0) {
			return -2;
		}
	}
	strncpy(uid, values, len);
        //uid = (unsigned char*)memcpy((unsigned char*) uid, (char*) &buf, len);

	ALOGD("%s()->Line:%d; uid = %s, len = %d \n", __FUNCTION__, __LINE__, uid, len-1);

	return (len-1);
}

