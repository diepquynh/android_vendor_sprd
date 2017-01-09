char  argv1[10];

#ifdef WIN32
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#define NVREAD(fmt,args...) do {\
    printf("%s: " fmt,\
                         argv1, ##args);\
} while (0)

#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <pthread.h>
#include <modem_control.h>
#include "packet.h"

typedef unsigned int uint32;
typedef signed int int32;
typedef unsigned char uint8;
typedef unsigned int BOOLEAN;
#if 1
#include <cutils/log.h>
#define LOG_TAG "NV_READ"
#define NVREAD(fmt, ...)  do {  \
            ALOGD("%s: " fmt,   \
                                argv1, ##__VA_ARGS__);  \
    } while (0)
#else

#define LOG_TAG "NV_READ"
#define NVREAD printf
#endif
#endif
#define RAMNV_SECT_SIZE 512
#define MAGIC 0x00004e56
typedef struct _NV_HEADER {
	uint32 magic;
	uint32 len;
	uint32 checksum;
	uint32 version;
} nv_header_t;

unsigned short calc_Checksum(unsigned char *dat, unsigned long len)
{
	unsigned short num = 0;
	unsigned long chkSum = 0;
	while (len > 1) {
		num = (unsigned short)(*dat);
		dat++;
		num |= (((unsigned short)(*dat)) << 8);
		dat++;
		chkSum += (unsigned long)num;
		len -= 2;
	}
	if (len) {
		chkSum += *dat;
	}
	chkSum = (chkSum >> 16) + (chkSum & 0xffff);
	chkSum += (chkSum >> 16);
	return (~chkSum);
}
int read_nv_partition(char * path,char * Bak_path, char * path_out)
{
	int handle = 0,Bak_Handle = 0,out_handle;
	char *firstName, *secondName;
	char header[RAMNV_SECT_SIZE],Bak_header[RAMNV_SECT_SIZE];
	nv_header_t *header_ptr = NULL, *Bak_header_ptr = NULL;
	int32 ret,ret1,ret2,size,Bak_size,magic;
	int32 status = 0;
	uint8 * buf,*Bak_buf;
	unsigned short ecc;
	BOOLEAN result;
	
	header_ptr = (nv_header_t *)header;
	Bak_header_ptr = (nv_header_t *)Bak_header;
	MODEM_LOGD(" %s path=%s, bak_path=%s, out=%s\n",__func__,path,Bak_path,path_out);
	do{
		handle = open(path, O_RDWR, S_IRUSR | S_IWUSR /* S_IRWXU | S_IRWXG | S_IRWXO */);
		if(handle <= 0)
			return 0;
		else{
			ret1 = read(handle, header, RAMNV_SECT_SIZE);
			if(ret1 != RAMNV_SECT_SIZE)
				break;
			size = header_ptr->len;
			buf = malloc(size);
			if(0 == buf){
				close(handle);
				return 0;
			}
			memset(buf,0,size);
			ret2 = read(handle, buf, size);
			if(ret2 != size)
				break;
			ecc = calc_Checksum(buf,size);
			if(header_ptr->magic == MAGIC && ecc == header_ptr->checksum){
				status += 1;
			}
		}
	}while(0);

	do{
		Bak_Handle = open(Bak_path, O_RDWR, S_IRUSR | S_IWUSR);
		
		if (Bak_Handle <= 0){
			close(handle);
			free(buf);
			return 0;
		}
		else{
			ret1 = read(Bak_Handle, Bak_header, RAMNV_SECT_SIZE);
			if(ret1 != RAMNV_SECT_SIZE)
				break;
			Bak_size = Bak_header_ptr->len;
			Bak_buf = malloc(Bak_size);
			if(0 == Bak_buf){
				close(handle);
				close(Bak_Handle);
				free(buf);
				return 0;
			}
			memset(Bak_buf,0,Bak_size);
			ret2 = read(Bak_Handle, Bak_buf, Bak_size);
			if(ret2 != Bak_size)
				break;
			ecc = calc_Checksum(Bak_buf,Bak_size);
			if(Bak_header_ptr->magic == MAGIC && ecc == Bak_header_ptr->checksum){
				status += 1<<1;
			}
		}
	}while(0);
	
	out_handle = open(path_out, O_RDWR| O_CREAT, S_IRUSR | S_IWUSR);
	if(out_handle <= 0){
		close(handle);
		close(Bak_Handle);
		free(buf);
		free(Bak_buf);
		return 0;
	}
	modemd_enable_busmonitor(0);
	switch(status){
		case 0:
			NVREAD("both org and bak partition are damaged!\n");
			memset(buf,0,size);
			lseek(handle,0,SEEK_SET);
			ret = write(handle, buf, size);
			lseek(Bak_Handle,0,SEEK_SET);
			ret = write(Bak_Handle, buf, size);
			ret = write(out_handle, buf, size);
			result = 0;
			break;
		case 1:
			NVREAD("bak partition is damaged!\n");
			lseek(Bak_Handle,0,SEEK_SET);
			if(RAMNV_SECT_SIZE != write(Bak_Handle, header, RAMNV_SECT_SIZE)
				|| size != write(Bak_Handle, buf, size)){
				NVREAD("write backup partition error\n");
			}
			if(size != write(out_handle, buf, size)){
				result = 0;
				break;
			}
			result = 1;
			break;
		case 2:
			NVREAD("org partition is damaged!\n!");
			lseek(handle,0,SEEK_SET);
			if(RAMNV_SECT_SIZE != write(handle, Bak_header, RAMNV_SECT_SIZE)
				|| Bak_size != write(handle, Bak_buf, Bak_size)){
				NVREAD("write org partition error\n");
			}
			if(Bak_size != write(out_handle, Bak_buf, Bak_size)){
				result = 0;
				break;
			}
			result = 1;
			break;
		case 3:
			if(size != write(out_handle, buf, size)){
				NVREAD("%s write nv partition failed %s!\n",__func__,strerror(errno));
				result = 0;
				break;
			} else {
				NVREAD("both org and bak partition are ok!\n");
			}
			result = 1;
			break;
		default:
			NVREAD("%s: status error!\n",__FUNCTION__);
			result = 0;
			break;
	}
	modemd_enable_busmonitor(1);
	close(handle);
	close(Bak_Handle);
	close(out_handle);
	free(buf);
	free(Bak_buf);
	return result;
}

//int main(int argc, char *argv[])
//{
//	read_nv_partition(argv[1], argv[2], argv[3]);
//}
