#define LOG_TAG "fmradio"

extern "C" {
#include <cutils/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <bluedroid/bluetooth.h>
}

#ifdef FM_UDP_SERVER

int fm_enable_daemon()
{
	ALOGD("%s\n", __FUNCTION__);
        fm_enable();
	return 0;
}

int fm_disable_daemon()
{
	ALOGD("%s\n", __FUNCTION__);
        fm_disable();
	return 0;
}

int main(int argc, char *argv[])
{
	int sock;
	//sendto中使用的对方地址
	struct sockaddr_in toAddr;
	//在recvfrom中使用的对方主机地址
	struct sockaddr_in fromAddr;

	int recvLen;
	/*unsigned int*/socklen_t  addrLen;
	char recvBuffer[128];

	sock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);

	if(sock < 0)
	{
		printf("create socket failed.\r\n");
		exit(0);
	}

	memset(&fromAddr,0,sizeof(fromAddr));
	fromAddr.sin_family=AF_INET;
	fromAddr.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
	fromAddr.sin_port = htons(4000);

	if(bind(sock,(struct sockaddr*)&fromAddr,sizeof(fromAddr))<0)
	{
		printf("bind() failed.\r\n");
		close(sock);
		exit(1);
	}

	while(1){
		addrLen = sizeof(toAddr);
		if((recvLen = recvfrom(sock,recvBuffer,128,0,(struct sockaddr*)&toAddr,&addrLen))<0)
		{
			continue;
		}

		if(recvBuffer[0] == '1')
		{
			fm_enable();
		}else
		{
			fm_disable();
		}
	}

	return 0;
}

#else

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
}

int main(int argc, char *argv[])
{
	if(argc < 2)
	{
		printf("please input parameter.\r\n");
		exit(0);
	}
	int sock;
	//sendto中使用的对方地址
	struct sockaddr_in toAddr;
	//在recvfrom中使用的对方主机地址
	struct sockaddr_in fromAddr;

	sock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);

	if(sock < 0)
	{
		printf("create socket failed.\r\n");
		exit(1);
	}

	memset(&toAddr,0,sizeof(toAddr));
	toAddr.sin_family=AF_INET;
	toAddr.sin_addr.s_addr=inet_addr("127.0.0.1");
	toAddr.sin_port = htons(4000);

	if(sendto(sock,argv[1], 1, 0,(struct sockaddr*)&toAddr,sizeof(toAddr)) != 1)
	{
		printf("sendto() failed.\r\n");
		close(sock);
		exit(1);
	}

	close(sock);
	return 0;
}

#endif
