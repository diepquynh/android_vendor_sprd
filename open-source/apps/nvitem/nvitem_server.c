
#include "nvitem_common.h"
#include "nvitem_channel.h"
#include "nvitem_sync.h"
#include "nvitem_packet.h"
#include "nvitem_os.h"
#include "nvitem_buf.h"
#include "nvitem_fs.h"
#include "cutils/properties.h"

static void *pSaveTask(void* ptr)
{
	ptr = ptr;// warning clean
	do
	{
		waiteEvent();
		NVITEM_PRINT("pSaveTask up\n");
		saveToDisk();
	}while(1);
	return 0;
}

//char  argv1[10];

BOOLEAN is_cali_mode;

int main(int argc, char *argv[])
{
#ifndef WIN32
	pthread_t pTheadHandle;
#endif
	if(3 != argc)
	{
		NVITEM_PRINT("Usage:\n");
		NVITEM_PRINT("\tnvitemd modemtype is_cali_mode\n");
		return 0;
	}
	if(strlen(argv[1]) > 10){
		NVITEM_PRINT("modem type length is too long\n");
		return 0;
	}
	strcpy(argv1,argv[1]);
	NVITEM_PRINT("argv1 %s argv[1] %s\n",argv1,argv[1]);
	is_cali_mode = atoi(argv[2]);
    NVITEM_PRINT("is_cali_mode %d \n",is_cali_mode);
	initEvent();
	if(!initArgs()){
		NVITEM_PRINT("init args failed,check the system.prop file\n");
		return 0;
	}
	initBuf();

//---------------------------------------------------
#ifndef WIN32
// create another task
	pthread_create(&pTheadHandle, NULL, (void*)pSaveTask, NULL);
#endif
//---------------------------------------------------

	do
	{
		channel_open();
		NVITEM_PRINT("channel open\n");
		_initPacket();
		_syncInit();
		syncAnalyzer();
		NVITEM_PRINT("channel close\n");
		channel_close();
	}while(1);
	return 0;
}


