
#include "nvitem_common.h"
#include "nvitem_fs.h"
#include "nvitem_config.h"
#ifdef CONFIG_NAND_UBI_VOL
#include <sys/ioctl.h>
#include <ubi-user.h>
#endif
#include "cutils/properties.h"

typedef struct  _NV_HEADER {
     uint32 magic;
     uint32 len;
     uint32 checksum;
     uint32 version;
}nv_header_t;
#define NV_HEAD_MAGIC   0x00004e56
#define NV_VERSION      101

#define PRODUCT_PARTITION_PATH    "ro.product.partitionpath"
#define PERSIST_MODEM_CHAR        "persist.modem."
#define RO_MODEM_CHAR             "ro.modem."
#define RO_MODEM_FIXNVSIZE        ".fixnv_size"
#define RO_MODEM_RUNNVSIZE        ".runnv_size"
#define NV_PATH          ".nvp"
#define NV_TAIL                   ".nv"
/*note:
#define PROPERTY_KEY_MAX   32   //strlen(PRODUCT_PARTITION_PATH) will be letter than MAX
#define PROPERTY_VALUE_MAX 92
*/


static unsigned short calc_checksum(unsigned char *dat, unsigned long len)
{
    unsigned short num = 0;
    unsigned long chkSum = 0;
    while(len>1){
        num = (unsigned short)(*dat);
        dat++;
        num |= (((unsigned short)(*dat))<<8);
        dat++;
        chkSum += (unsigned long)num;
        len -= 2;
    }
    if(len){
        chkSum += *dat;
    }
    chkSum = (chkSum >> 16) + (chkSum & 0xffff);
    chkSum += (chkSum >> 16);
    return (~chkSum);
}



/*
	TRUE(1): pass
	FALSE(0): fail
*/
static BOOLEAN _chkNVEcc(uint8* buf, uint32 size,uint16 checksum)
{
	uint16 crc,crcOri;

	crc = calc_checksum(buf,size);

	return (crc == checksum);
}

#ifndef WIN32
static RAM_NV_CONFIG _ramdiskCfg[RAMNV_NUM+1] = 
{
        {1,	"/fixnv/fixnv.bin",				"",	0x20000	},
        {2,	"/runtimenv/runtimenv.bin",		"",	0x40000	},
        {0,	"",							"",	0		}
};

char fixnv_ori_path[100];
char fixnv_bak_path[100];
char runnv_ori_path[100];
char runnv_bak_path[100];
uint32 fixnv_size,runnv_size;
extern char argv1[10];
extern char channel_path[95];

int initArgs(void)
{
	char   fixnv[95],runnv[95];
	char   fixnv_property[50],runnv_property[50];
	char   ro_prop[20];
	char   persist_prop[32];
	char   partition_path[100];
	char   nvp[50];
	char   path_char[95];
	char   channel_char[50];

	fixnv[0] = '\0';
	runnv[0] = '\0';
	path_char[0] = '\0';

	if(0 == strlen(argv1)){
		NVITEM_PRINT("invalid argv1\n");
		return 0;
	}
	//get system prop eg:ro.modem.t;ro.modem.w ....
	if(strlen(RO_MODEM_CHAR)+strlen(argv1) < 20){
		strcpy(ro_prop,RO_MODEM_CHAR);
		strcat(ro_prop,argv1);
	}
	NVITEM_PRINT("ro_prop %s\n",ro_prop);
	//get persist prop eg:persist.modem.t.enable
	if(strlen(PERSIST_MODEM_CHAR)+strlen(argv1)< 32){
		strcpy(persist_prop,PERSIST_MODEM_CHAR);
		strcat(persist_prop,argv1);
	}
	NVITEM_PRINT("persist_prop %s\n",persist_prop);
	//get channel
	if(strlen(ro_prop)+strlen(NV_TAIL) < 50){
		strcpy(channel_char,ro_prop);
		strcat(channel_char,NV_TAIL);
	}
	NVITEM_PRINT("channel_char %s\n",channel_char);
	property_get(channel_char, channel_path, "");
	if(0 == strlen(channel_path)){
		NVITEM_PRINT("invalid channel_path \n");
		return 0;
	}
	NVITEM_PRINT("channel_path %s \n",channel_path);

	//get nvsize
	if(strlen(ro_prop) + strlen(RO_MODEM_FIXNVSIZE) < 50){
		strcpy(fixnv_property,ro_prop);
		strcat(fixnv_property,RO_MODEM_FIXNVSIZE);
		property_get(fixnv_property,fixnv,"");
		if(0 == strlen(fixnv)){
			NVITEM_PRINT("invalid ro.modem.w.fixnv_size\n");
			return 0;
		}
		NVITEM_PRINT("fixnv_property is %s fixnv %s\n",fixnv_property,fixnv);
		fixnv_size = strtol(fixnv,0,16);
		NVITEM_PRINT("fixnv_size %x\n",fixnv_size);
	}

	if(strlen(ro_prop) + strlen(RO_MODEM_RUNNVSIZE) < 50){
		strcpy(runnv_property,ro_prop);
		strcat(runnv_property,RO_MODEM_RUNNVSIZE);
		property_get(runnv_property,runnv,"");
		if(0 == strlen(runnv)){
			NVITEM_PRINT("ro.modem.w.runnv_size\n");
			return 0;
		}
		NVITEM_PRINT("runnv_property is %s runnv %s\n",runnv_property,runnv);
		runnv_size = strtol(runnv,0,16);
		NVITEM_PRINT("runnv_size %x\n",runnv_size);
	}

	//get nv path: eg: partition_path+td+fixnv
	property_get(PRODUCT_PARTITION_PATH, partition_path, "");
	NVITEM_PRINT("partition_path %s\n",partition_path);

	if(strlen(persist_prop) + strlen(NV_PATH) < 50){
		strcpy(nvp,persist_prop);
		strcat(nvp,NV_PATH);
		property_get(nvp,path_char,"");
		if(0 == strlen(path_char)){
			NVITEM_PRINT("invalid ro.modem.w.nvp  \n");
			return 0;
		}
		NVITEM_PRINT("path_char %s nvp %s\n",path_char,nvp);
	}

	if(100 > strlen(partition_path)+strlen(path_char)+strlen("fixnv1")){
		strcpy(fixnv_ori_path,partition_path);
		strcat(fixnv_ori_path,path_char);
		strcat(fixnv_ori_path,"fixnv1");

		strcpy(fixnv_bak_path,partition_path);
		strcat(fixnv_bak_path,path_char);
		strcat(fixnv_bak_path,"fixnv2");
		NVITEM_PRINT("fixnv_ori_path %s fixnv_bak_path %s\n",fixnv_ori_path,fixnv_bak_path);
	}
	else{
		NVITEM_PRINT("fixnv path too long! check it \n");
		return 0;
	}

	if(100 > strlen(partition_path)+strlen(path_char)+strlen("runtimenv1")){
		strcpy(runnv_ori_path,partition_path);
		strcat(runnv_ori_path,path_char);
		strcat(runnv_ori_path,"runtimenv1");

		strcpy(runnv_bak_path,partition_path);
		strcat(runnv_bak_path,path_char);
		strcat(runnv_bak_path,"runtimenv2");
		NVITEM_PRINT("runnv_ori_path %s runnv_bak_path %s\n",runnv_ori_path,runnv_bak_path);
	}
	else{
		NVITEM_PRINT("runnv path too long! check it \n");
		return 0;
	}
	return 1;
}


const RAM_NV_CONFIG*	ramDisk_Init(void)
{
	if(strlen(fixnv_ori_path) > 100 || strlen(fixnv_bak_path) > 100
		|| strlen(runnv_ori_path) > 100 || strlen(runnv_bak_path)>100){
		NVITEM_PRINT("init config fail: invalid param!\n");
		return _ramdiskCfg;
	}

	memset(_ramdiskCfg,0,sizeof(_ramdiskCfg));
	_ramdiskCfg[0].partId = 1;
	strcpy(_ramdiskCfg[0].image_path,fixnv_ori_path);
	strcpy(_ramdiskCfg[0].imageBak_path,fixnv_bak_path);
	_ramdiskCfg[0].image_size = fixnv_size;
	NVITEM_PRINT("i = 0\t0x%x\t%32s\t%32s\t0x%8x\n",_ramdiskCfg[0].partId,_ramdiskCfg[0].image_path,_ramdiskCfg[0].imageBak_path,_ramdiskCfg[0].image_size);

	_ramdiskCfg[1].partId = 2;
	strcpy(_ramdiskCfg[1].image_path,runnv_ori_path);
	strcpy(_ramdiskCfg[1].imageBak_path,runnv_bak_path);
	_ramdiskCfg[1].image_size = runnv_size;

	NVITEM_PRINT("i = 1 \t0x%x\t%32s\t%32s\t0x%8x\n",_ramdiskCfg[1].partId,_ramdiskCfg[1].image_path,_ramdiskCfg[1].imageBak_path,_ramdiskCfg[1].image_size);
	return _ramdiskCfg;
}

RAMDISK_HANDLE	ramDisk_Open(uint32 partId)
{
	return (RAMDISK_HANDLE)partId;
}

int _getIdx(RAMDISK_HANDLE handle)
{
	uint32 i;
	uint32 partId = (uint32)handle;

	for(i = 0; i < sizeof(_ramdiskCfg)/sizeof(RAM_NV_CONFIG); i++){
		if(0 == _ramdiskCfg[i].partId){
			return -1;
		}
		else if(partId == _ramdiskCfg[i].partId){
			return i;
		}
	}
	return -1;
}

/*
	1 read imagPath first, if succes return , then
	2 read imageBakPath, if fail , return, then
	3 fix imagePath

	note:
		first imagePath then imageBakPath
*/
BOOLEAN		ramDisk_Read(RAMDISK_HANDLE handle, uint8* buf, uint32 size)
{
	int ret1=0,ret2=0;
	int fileHandle = 0;;
	int idx;
	char header[RAMNV_SECT_SIZE];
	nv_header_t * header_ptr =NULL;
	char *firstName, *secondName;

	NVITEM_PRINT("ramDisk_Read enter\n");
	header_ptr = (nv_header_t *)header;
	idx = _getIdx(handle);
	if(-1 == idx){
		return 0;
	}
// 0 get read order
	if(rand()%2){
		firstName = _ramdiskCfg[idx].image_path;
		secondName = _ramdiskCfg[idx].imageBak_path;
	}
	else{
		secondName = _ramdiskCfg[idx].image_path;
		firstName = _ramdiskCfg[idx].imageBak_path;
	}
// 1 read first image
	memset(buf,0xFF,size);
	memset(header,0x00,RAMNV_SECT_SIZE);
	do{
		fileHandle = open(firstName, O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);
		if(0 > fileHandle){
			NVITEM_PRINT("partId%x:%s open file failed!\n",_ramdiskCfg[idx].partId,firstName);
			break;
		}
		ret1 = read(fileHandle, header, RAMNV_SECT_SIZE);
		ret2 = read(fileHandle,buf,size);
		close(fileHandle);

		if(RAMNV_SECT_SIZE != ret1){
			NVITEM_PRINT("partId%x:%s read first image head failed!\n",_ramdiskCfg[idx].partId,firstName);
			break;
		}

		//check crc
		if(ret2 == size){
			if(_chkNVEcc(buf, size,header_ptr->checksum)){
				NVITEM_PRINT("partId%x:%s read success!\n",_ramdiskCfg[idx].partId,firstName);
				return 1;
			}
			NVITEM_PRINT("partId%x:%s ECC error!\n",_ramdiskCfg[idx].partId,firstName);
		}
		NVITEM_PRINT("partId%x:%s read error!\n",_ramdiskCfg[idx].partId,firstName);
	}while(0);
    // 2 read second image
	memset(buf,0xFF,size);
	memset(header,0x00,RAMNV_SECT_SIZE);
	fileHandle = open(secondName, O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);
	if(fileHandle >= 0){
		ret1 = read(fileHandle, header, RAMNV_SECT_SIZE);
		ret2 = read(fileHandle, buf, size);
		close(fileHandle);
		if(RAMNV_SECT_SIZE != ret1){
			NVITEM_PRINT("partId%x:%s read second image header failed!\n",_ramdiskCfg[idx].partId,firstName);
			return 1;
		}
	}
	if(ret2 != size){
		NVITEM_PRINT("ret2 = 0x%x,size = 0x%x partId%x:%s read error !\n",ret2,size,_ramdiskCfg[idx].partId,secondName);
		return 1;
	}

	if(!_chkNVEcc(buf, size,header_ptr->checksum)){
		NVITEM_PRINT("partId%x:%s ECC error!\n",_ramdiskCfg[idx].partId,secondName);
		return 1;
	}
	fileHandle  = open(firstName, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
	if(fileHandle >= 0){
#ifdef CONFIG_NAND_UBI_VOL
		__s64 up_sz = size + RAMNV_SECT_SIZE;
		ioctl(fileHandle, UBI_IOCVOLUP, &up_sz);
#endif
		write(fileHandle, header, RAMNV_SECT_SIZE);
		write(fileHandle, buf, size);
		fsync(fileHandle);
		close(fileHandle);
	}
	NVITEM_PRINT("partId%x:%s read success!\n",_ramdiskCfg[idx].partId,secondName);
	return 1;
}

/*
	1 get Ecc first
	2 write  imageBakPath
	3 write imagePath

	note:
		first imageBakPath then imagePath
*/
BOOLEAN	ramDisk_Write(RAMDISK_HANDLE handle, uint8* buf, uint32 size)
{
	BOOLEAN ret;
	int fileHandle = 0;;
	int idx;
#ifdef CONFIG_NAND_UBI_VOL
	__s64 up_sz = size + RAMNV_SECT_SIZE;
#endif
	char header_buf[RAMNV_SECT_SIZE];
	nv_header_t *header_ptr = NULL;

	idx = _getIdx(handle);
	if(-1 == idx){
		return 0;
	}
	memset(header_buf,0x00,RAMNV_SECT_SIZE);
	header_ptr = header_buf;
	header_ptr->magic = NV_HEAD_MAGIC;
	header_ptr->len = size;
	header_ptr->version = NV_VERSION;
	header_ptr->checksum = (uint32)calc_checksum(buf,size);

// 1 get Ecc

// 2 write bakup image
	ret = 1;
	fileHandle = open(_ramdiskCfg[idx].imageBak_path, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
	if(fileHandle >= 0){
#ifdef CONFIG_NAND_UBI_VOL
	    ioctl(fileHandle, UBI_IOCVOLUP, &up_sz);
#endif
		if(RAMNV_SECT_SIZE != write(fileHandle, header_buf, RAMNV_SECT_SIZE)){
			ret = 0;
			NVITEM_PRINT("partId%x:bakup image header write fail!\n",_ramdiskCfg[idx].partId);
		}
		else{
			NVITEM_PRINT("write backup header");
		}
		if(size != write(fileHandle, buf, size)){
			ret =0;
			NVITEM_PRINT("partId%x:bakup image write fail!\n",_ramdiskCfg[idx].partId);
		}
		fsync(fileHandle);
		close(fileHandle);
	}
// 3 write origin image
	fileHandle = open(_ramdiskCfg[idx].image_path, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
	if(fileHandle >= 0){
#ifdef CONFIG_NAND_UBI_VOL
	    ioctl(fileHandle, UBI_IOCVOLUP, &up_sz);
#endif
		if(RAMNV_SECT_SIZE != write(fileHandle, header_buf, RAMNV_SECT_SIZE)){
			ret = 0;
			NVITEM_PRINT("partId%x:origin image header write fail!\n",_ramdiskCfg[idx].partId);
		}
		else{
			NVITEM_PRINT("write origin header");
		}
		if(size != write(fileHandle, buf, size)){
			NVITEM_PRINT("partId%x:origin image write fail!\n",_ramdiskCfg[idx].partId);
			ret = 0;
		}
		fsync(fileHandle);
		close(fileHandle);
		NVITEM_PRINT("partId%x:image write finished %d!\n",_ramdiskCfg[idx].partId,ret);
	}
	return ret;

}

void	ramDisk_Close(RAMDISK_HANDLE handle)
{
	return;
}



#else

const RAM_NV_CONFIG ram_nv_config[] = 
{
	{RAMBSD_FIXNV_ID,			(uint8*)"D:/analyzer/commonTools/testCode/testdata/fixnv",		"",	0x20000	},
	{RAMBSD_RUNNV_ID,			(uint8*)"D:/analyzer/commonTools/testCode/testdata/runingnv",	"",	0x40000	},
	{RAMBSD_PRODUCTINFO_ID,	(uint8*)"D:/analyzer/commonTools/testCode/testdata/productnv",	"",	0x20000	},
	{0,							0,														0,	0		}
};

const RAM_NV_CONFIG*	ramDisk_Init(void)
{
	return ram_nv_config;
}

int _getIdx(uint32 partId)
{
	int i;

	for(i = 0; i < sizeof(ram_nv_config)/sizeof(RAM_NV_CONFIG); i++){
		if(partId == ram_nv_config[i].partId){
			return i;
		}
	}
	return -1;
}

RAMDISK_HANDLE	ramDisk_Open(uint32 partId)
{
	FILE* DiskImg = 0;
	uint32 idx;

	idx = _getIdx(partId);
	if(-1 == idx)
	{
		return (RAMDISK_HANDLE)0;
	}
	DiskImg = fopen((char*)ram_nv_config[idx].image_path, "wb+");
	if(NULL == DiskImg)
	{
		return 0;
	}
	fseek( DiskImg, 0, SEEK_SET );

	return (RAMDISK_HANDLE)DiskImg;
}

BOOLEAN		ramDisk_Read(RAMDISK_HANDLE handle, uint8* buf, uint32 size)
 {
 	FILE* DiskImg = (FILE*)handle;

	fseek( DiskImg, 0, SEEK_SET );
	fread( buf, size, 1, DiskImg );

	return 1;
}


BOOLEAN		ramDisk_Write(RAMDISK_HANDLE handle, uint8* buf, uint32 size)
{
 	FILE* DiskImg = (FILE*)handle;

	fseek( DiskImg, 0, SEEK_SET );
	fwrite( buf, size, 1, DiskImg );
	fflush( DiskImg );

	return 1;
}


void			ramDisk_Close(RAMDISK_HANDLE handle)
{
 	FILE* DiskImg = (FILE*)handle;

	fclose(DiskImg);
	return;

}
#endif

