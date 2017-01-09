#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <cutils/sockets.h>
#include <pthread.h>
#include <utils/Log.h>
#include "engopt.h"
#include "eng_hardware_test.h"
#include <string.h>
static char socket_read_buf[SOCKET_BUF_LEN] = {0};
static char socket_write_buf[SOCKET_BUF_LEN] = {0};
#define EUT_WIFI_RXPKTCNT_REQ_RET "+SPWIFITEST:RXPACKCOUNT="
#define EUT_WIFI_PARAM_MAXCNT  6







static int str_split(char* buf,char* delim,char* destrArr[],int maxcnt) {
	char *p;
	int i =0;
	ENG_LOG("buf =%s ",buf );
	p = strtok(buf, delim);
	while(p != NULL) {
		destrArr[i] = p;
		ENG_LOG("aaacmd i=%d  cmdbuf=%s \n",i,p );
		p = strtok(NULL, delim);
		i++;
	}
	if(i>maxcnt) {
		
		ENG_LOG("str_split exceed the max count maxcnt=%d  i=%d  \n",maxcnt,i );
		return -1;
	}
       return i;
}



static void hardware_sprd_wifi_test(char* buf)
{
	char req[256] = {0};
	int channel = 0;
	int rate = 0;
	int powerLevel = 0;
	char cmd[16] = {0};
	char* parameterArr[EUT_WIFI_PARAM_MAXCNT] = {0};
	int parameterCnt = 0;
	memset(req, 0, sizeof(req));
	memset(cmd, 0, sizeof(cmd));
	parameterCnt = str_split(buf+TYPE_OFFSET," ",parameterArr,EUT_WIFI_PARAM_MAXCNT);
	if( parameterCnt < 1){
		memcpy(socket_write_buf,TEST_ERROR,ERROR_LEN);
		return ;
	}
	
//	ENG_LOG("hardware_sprd_wifi_test buf =%s len1=%d  len2=%d \n",buf+TYPE_OFFSET,strlen(buf+TYPE_OFFSET),strlen(EUT_START));
//	ENG_LOG("hardware_sprd_wifi_test buf =%c,%c,%c,%c,%c,%cyaxun",*(buf+7),*(buf+8),*(buf+9),*(buf+10),*(buf+11),*(buf+12));
#if 1 
	if (0 == strncmp(buf+TYPE_OFFSET,WIFI_EUT_START,strlen(WIFI_EUT_START))) {
		ENG_LOG("hardware_sprd_wifi_test START");
		wifieut(OPEN_WIFI,req);
		if (!strcmp(req,EUT_WIFI_OK)) {
			memcpy(socket_write_buf,TEST_OK,OK_LEN);
		} else {
			memcpy(socket_write_buf,TEST_ERROR,ERROR_LEN);
		}
	} else if (0 == strncmp(buf+TYPE_OFFSET,WIFI_EUT_STOP,strlen(WIFI_EUT_STOP))) {
		ENG_LOG("hardware_sprd_wifi_test STOP");
		wifieut(CLOSE_WIFI,req);
		if (!strcmp(req,EUT_WIFI_OK)) {
			memcpy(socket_write_buf,TEST_OK,OK_LEN);
		} else {
			memcpy(socket_write_buf,TEST_ERROR,ERROR_LEN);
		}
	} else if (0 == strncmp(buf+TYPE_OFFSET,WIFI_EUT_CW_START,strlen(WIFI_EUT_CW_START))) {
		ENG_LOG("hardware_sprd_wifi_test CW_START");
		wifi_tx_mode_bcm(0,req);

		wifi_tx(OPEN_WIFI,req);
		if (!strcmp(req,EUT_WIFI_OK)) {
			memcpy(socket_write_buf,TEST_OK,OK_LEN);
		} else {
			memcpy(socket_write_buf,TEST_ERROR,ERROR_LEN);
		}
	} else if (0 == strncmp(buf+TYPE_OFFSET,WIFI_EUT_TX_STOP,strlen(WIFI_EUT_TX_STOP))) {
		ENG_LOG("hardware_sprd_wifi_test type TX_STOP");
	
		wifi_tx(CLOSE_WIFI,req);
		if (!strcmp(req,EUT_WIFI_OK)) {
			memcpy(socket_write_buf,TEST_OK,OK_LEN);
		} else {
			memcpy(socket_write_buf,TEST_ERROR,ERROR_LEN);
		}
	} else if (0 == strncmp(buf+TYPE_OFFSET,WIFI_EUT_TX_START,strlen(WIFI_EUT_TX_START))) {
		ENG_LOG("hardware_sprd_wifi_test type TX_START");
		wifi_tx_mode_bcm(1,req);
		wifi_tx(OPEN_WIFI,req);
		if (!strcmp(req,EUT_WIFI_OK)) {
			memcpy(socket_write_buf,TEST_OK,OK_LEN);
		} else {
			memcpy(socket_write_buf,TEST_ERROR,ERROR_LEN);
		} 
	} else if (0 == strncmp(buf+TYPE_OFFSET,WIFI_EUT_RX_STOP,strlen(WIFI_EUT_RX_STOP))) {
		ENG_LOG("hardware_sprd_wifi_test type RX_STOP");
		wifi_rx(CLOSE_WIFI,req);
		if (!strcmp(req,EUT_WIFI_OK)) {
			memcpy(socket_write_buf,TEST_OK,OK_LEN);
		} else {
			memcpy(socket_write_buf,TEST_ERROR,ERROR_LEN);
		}
	}else if (0 == strncmp(buf+TYPE_OFFSET,WIFI_EUT_RX_START,strlen(WIFI_EUT_RX_START))) {
		ENG_LOG("hardware_sprd_wifi_test RX_START");
		wifi_rx(OPEN_WIFI,req);
		if (!strcmp(req,EUT_WIFI_OK)) {
			wifi_clr_rxpackcount(req);
			if (!strcmp(req,EUT_WIFI_OK)) {
				memcpy(socket_write_buf,TEST_OK,OK_LEN);
			} else {
				memcpy(socket_write_buf,TEST_ERROR,ERROR_LEN);
			}
		}else {
			memcpy(socket_write_buf,TEST_ERROR,ERROR_LEN);
		}
	}else if (0 == strncmp(buf+TYPE_OFFSET,WIFI_EUT_GET_RXOK,strlen(WIFI_EUT_GET_RXOK))){
		ENG_LOG("hardware_sprd_wifi_test GET_RXOK");
		wifi_rxpackcount(req);
		if (0 == strncmp(req,EUT_WIFI_RXPKTCNT_REQ_RET,strlen(EUT_WIFI_RXPKTCNT_REQ_RET))) {
			memcpy(socket_write_buf,TEST_OK,OK_LEN);
			memcpy(socket_write_buf+OK_LEN,req,256);
		} else {
			memcpy(socket_write_buf,TEST_ERROR,ERROR_LEN);
		}
	}else if (0 == strncmp(buf+TYPE_OFFSET,WIFI_EUT_SET_CHANNEL,strlen(WIFI_EUT_SET_CHANNEL))){
		ENG_LOG("hardware_sprd_wifi_test SET_CHANNEL");
		int channel = 0;
		if(NULL ==parameterArr[1]) {
			memcpy(socket_write_buf,TEST_ERROR,ERROR_LEN);
			ENG_LOG("hardware_sprd_wifi_test parameterArr error!");
			return ;
		}
		channel = atoi(parameterArr[1]);
		set_wifi_ch(channel,req);
		if (!strcmp(req,EUT_WIFI_OK)) {
			memcpy(socket_write_buf,TEST_OK,OK_LEN);
		} else {
			memcpy(socket_write_buf,TEST_ERROR,ERROR_LEN);
		} 
	}else if (0 == strncmp(buf+TYPE_OFFSET,WIFI_EUT_SET_RATE,strlen(WIFI_EUT_SET_RATE))){
		ENG_LOG("hardware_sprd_wifi_test SET_RATE");
		if(NULL ==parameterArr[1]) {
			memcpy(socket_write_buf,TEST_ERROR,ERROR_LEN);
			ENG_LOG("hardware_sprd_wifi_test parameterArr error!");
			return;
		}
		set_wifi_rate(parameterArr[1],req);
		if (!strcmp(req,EUT_WIFI_OK)) {
			memcpy(socket_write_buf,TEST_OK,OK_LEN);
		} else {
			memcpy(socket_write_buf,TEST_ERROR,ERROR_LEN);
		} 
	}else if (0 == strncmp(buf+TYPE_OFFSET,WIFI_EUT_SET_POWER,strlen(WIFI_EUT_SET_POWER))){
		ENG_LOG("hardware_sprd_wifi_test TX_POWER");
		long power = 0;
		if(NULL ==parameterArr[1]) {
			memcpy(socket_write_buf,TEST_ERROR,ERROR_LEN);
			ENG_LOG("hardware_sprd_wifi_test parameterArr error!");
			return;
		}
		power = (long)atoi(parameterArr[1]);
		wifi_tx_pwrlv(power,req);
		if (!strcmp(req,EUT_WIFI_OK)) {
			memcpy(socket_write_buf,TEST_OK,OK_LEN);
		} else {
			memcpy(socket_write_buf,TEST_ERROR,ERROR_LEN);
		} 


	}else if (0 == strncmp(buf+TYPE_OFFSET,CMD_POWER_SAVE,strlen(CMD_POWER_SAVE))){
		ENG_LOG("hardware_sprd_wifi_test CMD_POWER_SAVE");
		int powersave_mode = 0;
		if(NULL ==parameterArr[1]) {
			memcpy(socket_write_buf,TEST_ERROR,ERROR_LEN);
			ENG_LOG("hardware_sprd_wifi_test parameterArr error!");
			return;
		}
		powersave_mode = atoi(parameterArr[1]);
		wifi_pm(powersave_mode,req);
		if (!strcmp(req,EUT_WIFI_OK)) {
			memcpy(socket_write_buf,TEST_OK,OK_LEN);
		} else {
			memcpy(socket_write_buf,TEST_ERROR,ERROR_LEN);
		}
	}else if (0 == strncmp(buf+TYPE_OFFSET,WIFI_EUT_BANDWIDTH,strlen(WIFI_EUT_BANDWIDTH))){
		ENG_LOG("hardware_sprd_wifi_test WIFI_EUT_BANDWIDTH");
		int bandwidth = 0;
		if(NULL ==parameterArr[1]) {
			memcpy(socket_write_buf,TEST_ERROR,ERROR_LEN);
			ENG_LOG("hardware_sprd_wifi_test parameterArr error!");
			return;
		}
		bandwidth = atoi(parameterArr[1]);
		wifi_bw_set(bandwidth,req);
		if (!strcmp(req,EUT_WIFI_OK)) {
			memcpy(socket_write_buf,TEST_OK,OK_LEN);
		} else {
			memcpy(socket_write_buf,TEST_ERROR,ERROR_LEN);
		}
	}else if (0 == strncmp(buf+TYPE_OFFSET,WIFI_EUT_SET_BAND,strlen(WIFI_EUT_SET_BAND))){
		ENG_LOG("hardware_sprd_wifi_test WIFI_EUT_SET_BAND");
		int band = 0;
		if(NULL ==parameterArr[1]) {
			memcpy(socket_write_buf,TEST_ERROR,ERROR_LEN);
			ENG_LOG("hardware_sprd_wifi_test parameterArr error!");
			return;
		}
		band = atoi(parameterArr[1]);
		wifiband(band,req);
		if (!strcmp(req,EUT_WIFI_OK)) {
			memcpy(socket_write_buf,TEST_OK,OK_LEN);
		} else {
			memcpy(socket_write_buf,TEST_ERROR,ERROR_LEN);
		}
	}else if (0 == strncmp(buf+TYPE_OFFSET,WIFI_EUT_SET_PREAMBLE,strlen(WIFI_EUT_SET_PREAMBLE))){
		ENG_LOG("hardware_sprd_wifi_test WIFI_EUT_SET_PREAMBLE");
		int preamble = 0;
		if(NULL ==parameterArr[1]) {
			memcpy(socket_write_buf,TEST_ERROR,ERROR_LEN);
			ENG_LOG("hardware_sprd_wifi_test parameterArr error!");
			return;
		}
		preamble = atoi(parameterArr[1]);
		wifi_eut_preamble_set(preamble,req);
		if (!strcmp(req,EUT_WIFI_OK)) {
			memcpy(socket_write_buf,TEST_OK,OK_LEN);
		} else {
			memcpy(socket_write_buf,TEST_ERROR,ERROR_LEN);
		}
	}else if (0 == strncmp(buf+TYPE_OFFSET,WIFI_EUT_SET_MODE,strlen(WIFI_EUT_SET_MODE))){
		ENG_LOG("hardware_sprd_wifi_test WIFI_EUT_SET_MODE");
		int mode = 0;
		if(NULL ==parameterArr[1]) {
			memcpy(socket_write_buf,TEST_ERROR,ERROR_LEN);
			ENG_LOG("hardware_sprd_wifi_test parameterArr error!");
			return;
		}
		mode = atoi(parameterArr[1]);
		wifi_tx_mode_bcm(mode,req);
		if (!strcmp(req,EUT_WIFI_OK)) {
			memcpy(socket_write_buf,TEST_OK,OK_LEN);
		} else {
			memcpy(socket_write_buf,TEST_ERROR,ERROR_LEN);
		}
	}else if (0 == strncmp(buf+TYPE_OFFSET,WIFI_EUT_SET_TXTONE,strlen(WIFI_EUT_SET_TXTONE))){
		ENG_LOG("hardware_sprd_wifi_test WIFI_EUT_SET_TXTONE");
		int tx_tone = 0;
		if(NULL ==parameterArr[1]) {
			memcpy(socket_write_buf,TEST_ERROR,ERROR_LEN);
			ENG_LOG("hardware_sprd_wifi_test parameterArr error!");
			return;
		}
		tx_tone = atoi(parameterArr[1]);
		wifi_tone_set(tx_tone,req);
		if (!strcmp(req,EUT_WIFI_OK)) {
			memcpy(socket_write_buf,TEST_OK,OK_LEN);
		} else {
			memcpy(socket_write_buf,TEST_ERROR,ERROR_LEN);
		}
	}else {
		ENG_LOG("hardware_sprd_wifi_test  shit  error");
		memcpy(socket_write_buf,TEST_ERROR,ERROR_LEN);
	}
#endif
}





static void hardware_sprd_bt_test(char* buf)
{
	char req[32] = {0};
	#if 0
	memset(req, 0, sizeof(req));
	if (0 == strncmp(buf+TYPE_OFFSET,"START",strlen("START"))) {
		ENG_LOG("hardware_sprd_bt_test START");
		bteut(OPEN_BT, req);
		if (!strcmp(req,EUT_BT_OK)) {
			memcpy(socket_write_buf,TEST_OK,strlen(TEST_OK));
			socket_write_buf[strlen(TEST_OK)] = '\0';
		} else {
			memcpy(socket_write_buf,TEST_ERROR,strlen(TEST_ERROR));
		        socket_write_buf[strlen(TEST_ERROR)] = '\0';
		}
	} else if (0 == strncmp(buf+TYPE_OFFSET,"STOP",strlen("STOP"))) {
		ENG_LOG("hardware_sprd_bt_test STOP");
		bteut(CLOSE_BT, req);
		if (!strcmp(req,EUT_BT_OK)) {
			memcpy(socket_write_buf,TEST_OK,strlen(TEST_OK));
			socket_write_buf[strlen(TEST_OK)] = '\0';
		} else {
			memcpy(socket_write_buf,TEST_ERROR,strlen(TEST_ERROR));
			socket_write_buf[strlen(TEST_ERROR)] = '\0';
		}
	} else {
		memcpy(socket_write_buf,TEST_ERROR,strlen(TEST_ERROR));
		socket_write_buf[strlen(TEST_ERROR)] = '\0';

	}
	#endif
}

static int hardware_test_function(char* buf)
{
	int type=0;

	//type = buf[0];

		if (0 == strncmp(buf,"eng wl",strlen("eng wl"))) {
			hardware_sprd_wifi_test(buf);
			type = SPRD_WIFI;
		} else if (0 == strncmp(buf,"eng  SOCKETCLOSE",strlen("eng  SOCKETCLOSE"))) {
			type = CLOSE_SOCKET;
		}else {
			ENG_LOG("not supported");
		}
ENG_LOG("hardware_test_function type = %d", type);
	#if 0
	switch(type) {
		case SPRD_WIFI:
			hardware_sprd_wifi_test(buf);
			break;
		case SPRD_BT:
			hardware_sprd_bt_test(buf);
			break;
		case CLOSE_SOCKET:
			ENG_LOG("hardware_test_function  CLOSE SOCKET");
			break;
		default:
			break;
	}
	#endif 

	return type;
}

static void hardwaretest_thread(void *fd)
{
	int soc_fd;
	int ret;
	int length;
	int type;
	fd_set readfds;

	soc_fd = *(int *)fd;
	ENG_LOG("hardwaretest_thread  soc_fd = %d", soc_fd);
	while(1) { 
		FD_ZERO(&readfds);
		FD_SET(soc_fd,&readfds);
		ret = select(soc_fd+1,&readfds,NULL,NULL,NULL);
		if (ret < 0) {
			ENG_LOG("hardwaretest_thread  ret = %d, break",ret);
			break;
		}
		memset(socket_read_buf,0,SOCKET_BUF_LEN);
		memset(socket_write_buf,0,SOCKET_BUF_LEN);
		if (FD_ISSET(soc_fd,&readfds)) {
			length = read(soc_fd,socket_read_buf,SOCKET_BUF_LEN);
			if (length <= 0) {
				ENG_LOG("hardwaretest_thread  length = %d, break",length);
				break;
			}
			ENG_LOG("hardwaretest_thread  socket_read_buf = %s,",socket_read_buf);
			type = hardware_test_function(socket_read_buf);
			if (type == CLOSE_SOCKET) {
				ENG_LOG("hardwaretest_thread  CLOSE_SOCKET break");
				break;
			}
			write(soc_fd,socket_write_buf,strlen(socket_write_buf));
		}
	}
	ENG_LOG("hardwaretest_thread  CLOSE_SOCKET");
	close(soc_fd);
}

static int hardwaretest_server(void)
{
	int ret;
	ret = socket_local_server ("hardwaretest",
				0, SOCK_STREAM);

	if (ret < 0) {
		ENG_LOG("hardwaretest server Unable to bind socket errno:%d", errno);
		exit (-1);
	}
	return ret;
}

int main(void)
{
	int socket;
	int fd;
	struct sockaddr addr;
	socklen_t alen;
	pthread_t thread_id;
	socket = hardwaretest_server();
	if (socket == -1) {
		return -1;
	}

	ENG_LOG("hardwaretest server start listen\n");

	alen = sizeof(addr);

	for (; ;) {
		if ((fd=accept(socket,&addr,&alen)) == -1) {
			ENG_LOG("hardwaretest server accept error\n");
			continue;
		}

		if (0 != pthread_create(&thread_id, NULL, (void *)hardwaretest_thread, &fd)) {
			ENG_LOG("hardwaretest thread create error\n");
		}
	}

	return 0;
}
