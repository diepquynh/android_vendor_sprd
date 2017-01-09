
#include "nvitem_common.h"
#include "nvitem_channel.h"

#ifndef WIN32
#define CMUX_NVITEMD_PATH	"/dev/ts0710mux20"
#define NVSYN_CMD		"AT+NVSYNLINK=21,1000,5000,512"

static int channel_fd = 0;
char channel_path[95];


void channel_open(void)
{
	int fd = -1;
	char cmd[64];
	int timeout=0;

do
{
	do
	{
		channel_fd = open( channel_path, O_RDWR);
		if(-1 == channel_fd)
		{
			sleep(1);
			continue;
		}
		else
		{
			break;
		}
	}while(1);

break;
#if 0		
	do
	{
		if(30 < timeout)
		{
			break;
		}
		fd = engapi_open(0);
		if(0 > fd)
		{
			sleep(1);
			timeout++;
			continue;
		}
		else
		{
			break;
		}
	}while(1);

	if(0 > fd){
		close(channel_fd);
		continue;
	}

	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "%d,%d,%s",ENG_AT_NOHANDLE_CMD,1,NVSYN_CMD);
	NVITEM_PRINT("engapi_write: %s !!!\n",cmd);
	if(strlen(cmd) != engapi_write(fd, cmd, strlen(cmd))){
		NVITEM_PRINT(" engapi_write : wirte fail !!!\n");
		engapi_close(fd);
		close(channel_fd);
		continue;
	}

	memset(cmd, 0, sizeof(cmd));
	engapi_read(fd, cmd, sizeof(cmd));
	NVITEM_PRINT(" engapi_read: %s !!!\n",cmd);

	if(strcasestr(cmd,"OK") != NULL)
	{
		engapi_close(fd);
		return;
	}

	engapi_close(fd);
	close(channel_fd);
	sleep(1);
	continue;
#endif
}while(1);

}
BOOLEAN channel_read(uint8* buf, uint32 size, uint32* hasRead)
{

	*hasRead = read(channel_fd,buf,size);
	if(*hasRead < size)
	{
		return 0;
	}
	return 1;
}

BOOLEAN channel_write(uint8* buf, uint32 size, uint32* hasWrite)
{
	write(channel_fd,buf,size);
	return 1;
}

void channel_close(void)
{
	close(channel_fd);
}

#else

#include <windows.h>
#include <Winnt.h>
#include <winioctl.h>

#define FILENAME	"D:\\analyzer\\commonTools\\testCode\\testdata\\packet.bin"
static HANDLE hDevice;
#define CHANNEL_BUFSIZE	2048
static uint8 channelBuf[CHANNEL_BUFSIZE];



void channel_open(void)
{
//	DWORD count;
	unsigned __int32 offsetH,offsetL;

	do
	{
		hDevice = CreateFile(FILENAME,  // drive to open
			GENERIC_READ|GENERIC_WRITE,                // no access to the drive
			FILE_SHARE_READ|FILE_SHARE_WRITE, 
			NULL,             // default security attributes
			OPEN_EXISTING,    // disposition
			0, //FILE_FLAG_OVERLAPPED,                // file attributes
			NULL
		);            // do not copy file attributes
		if (hDevice == INVALID_HANDLE_VALUE) // cannot open the drive
		{
			Sleep(1);
			continue;
		}
		else
		{
			return;
		}
	}while(1);
//	DeviceIoControl(hDevice,FSCTL_LOCK_VOLUME,   NULL,0,NULL,0,&count,NULL);

	offsetH = 0;
	offsetL = 0;
	SetFilePointer(hDevice,offsetL, &offsetH, FILE_BEGIN);

	return;

}

BOOLEAN channel_read(uint8* buf, uint32 size, uint32* hasRead)
{
	DWORD lpNumberOfBytesRead;
	BOOL bResult;

//	DeviceIoControl(hDevice,FSCTL_LOCK_VOLUME,   NULL,0,NULL,0,&count,NULL);

	bResult = ReadFile(hDevice, channelBuf, CHANNEL_BUFSIZE, &lpNumberOfBytesRead, 0);

	if(1 != bResult)
	{
		lpNumberOfBytesRead = GetLastError();
		return FALSE;
	}
//	DeviceIoControl(hDevice,FSCTL_UNLOCK_VOLUME, NULL,0,NULL,0,&count,NULL);

	memcpy(buf,channelBuf,size);
	*hasRead = (size<lpNumberOfBytesRead?size:lpNumberOfBytesRead);

	return TRUE;
}

BOOLEAN channel_write(uint8* buf, uint32 size, uint32* hasWrite)
{
	return TRUE;
}

void channel_close(void)
{
//	DWORD count;
//	DeviceIoControl(hDevice,FSCTL_UNLOCK_VOLUME, NULL,0,NULL,0,&count,NULL);
	CloseHandle(hDevice);
	hDevice = 0;

	return;
}

#endif

