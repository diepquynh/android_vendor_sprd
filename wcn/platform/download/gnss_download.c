#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <termios.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <utils/Log.h>
#include <cutils/sockets.h>
#include <time.h>
#include <cutils/properties.h>

#include "packet.h"

#define REBOOT_DBG

#define	POWER_CTL		"/dev/power_ctl"
#define	UART_DEVICE_NAME	"/dev/ttyS3"
#define CONFIG_XML_PATH "/data/gnss/config/config.xml"

#define DOWNLOAD_IOCTL_BASE	'z'
#define DOWNLOAD_POWER_ON	_IO(DOWNLOAD_IOCTL_BASE, 0x01)
#define DOWNLOAD_POWER_OFF	_IO(DOWNLOAD_IOCTL_BASE, 0x02)
#define GNSS_CHIP_EN		_IO(DOWNLOAD_IOCTL_BASE, 0x04)
#define GNSS_CHIP_DIS		_IO(DOWNLOAD_IOCTL_BASE, 0x05)

#define GNSS_SOCKET_NAME			"gnss_server"
#define GNSS_SOCKET_SLOG_NAME		"gnss_slog"
#define WCN_MAX_CLIENT_NUM		(10)


#define WCN_SOCKET_TYPE_SLOG		1
#define WCN_SOCKET_TYPE_WCND		2
#define WCN_SOCKET_TYPE_GNSS		3


/*wcn*/
#define WCN_CMD_REBOOT_GNSS		"rebootgnss"
#define WCN_CMD_DUMP_GNSS		"dumpgnss"
#define WCN_CMD_START_GNSS		"startgnss"
#define WCN_CMD_STOP_GNSS		"stopgnss"
#define WCN_RESP_REBOOT_GNSS	"rebootgnss-ok"
#define WCN_RESP_DUMP_GNSS		"dumpgnss-ok"
#define WCN_RESP_START_GNSS		"startgnss-ok"
#define WCN_RESP_STOP_GNSS		"stopgnss-ok"
#define SOCKET_BUFFER_SIZE 		(128)

typedef struct structWcnClient{
	int sockfd;
	int type;
}WcnClient;

typedef struct pmanager {
	pthread_mutex_t client_fds_lock;
	WcnClient client_fds[WCN_MAX_CLIENT_NUM];
	int selfcmd_sockets[2];
	int listen_fd;
	int listen_slog_fd;
	bool flag_connect;
	bool flag_reboot;
	bool flag_start;
	bool flag_stop;
	bool flag_dump;
}pmanager_t;

pmanager_t pmanager;

typedef enum _DOWNLOAD_STATE {
		DOWNLOAD_INIT,
        DOWNLOAD_START,
        DOWNLOAD_BOOTCOMP,
} DOWNLOAD_STA_E;

struct image_info {
	char *image_path;
	unsigned int image_size;
	unsigned int address;
	unsigned int jump_address;
};

typedef struct  _NV_HEADER {
	unsigned int magic;
	unsigned int len;
	unsigned int checksum;
	unsigned int version;
}nv_header_t;

#define NV_HEAD_MAGIC   0x00004e56

#define	FDL_PACKET_SIZE 	(1*1024)
#define LS_PACKET_SIZE		(256)
#define HS_PACKET_SIZE		(32*1024)

#define FDL_CP_UART_TIMEOUT	(3000)//(200) //ms

#define	DL_FAILURE		(-1)
#define DL_SUCCESS		(0)

#define MS_IN_SEC 1000
#define NS_IN_MS  1000000

static DOWNLOAD_STA_E download_state = DOWNLOAD_INIT;
char test_buffer[HS_PACKET_SIZE+128]={0};
static char *uart_dev = UART_DEVICE_NAME;

int speed_arr[] = {B2500000, B2000000, B921600,B115200, B38400, B19200, B9600, B4800, B2400, B1200, B300,
                   B921600,B115200, B38400, B19200, B9600, B4800, B2400, B1200, B300, };
int name_arr[] = {2500000, 2000000, 921600,115200,38400,  19200, 9600,  4800,  2400,  1200,  300,
        921600, 115200,38400,  19200,  9600, 4800, 2400, 1200,  300, };

char  gPath[128];

struct image_info download_image_info[] = {
	{
		//fdl
		#ifdef PARTITION_PATH_4.4
		"/dev/block/platform/sprd-sdhci.3/by-name/gnssfdl",
		#else
		"/system/etc/gnssfdl.bin",
		#endif
		0x1400,
		0x200120,
		0x0
	},
	{
		//img
		#ifdef PARTITION_PATH_4.4
		"/dev/block/platform/sprd-sdhci.3/by-name/gnssmodem",
		#else
		"/system/etc/gnssmodem.bin",
		#endif
		0x57800,
		0x200120,
		0x0,
	},
};

unsigned int delta_miliseconds(struct timespec *begin, struct timespec *end)
{
    long ns;
    unsigned int ms;
    time_t sec;

    if(NULL == begin || NULL == end){
        return 0;
    }

    ns = end->tv_nsec - begin->tv_nsec;
    sec = end->tv_sec - begin->tv_sec;
    ms = sec * MS_IN_SEC + ns / NS_IN_MS;
	if(ms == 0)
		ms = 1;
	return ms;
}

void set_raw_data_speed(int fd, int speed)
{
    unsigned long   	i;
    int   		status;
    struct termios   	Opt;

    tcflush(fd,TCIOFLUSH);
    tcgetattr(fd, &Opt);
    for ( i= 0;  i  < sizeof(speed_arr) / sizeof(int);  i++){
        if  (speed == name_arr[i])
        {
            Opt.c_oflag &= ~OPOST;
            Opt.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
            Opt.c_cflag &= ~(CSIZE | PARENB | CRTSCTS);
            Opt.c_cflag |= CS8;
	    	Opt.c_oflag = ~OPOST;

			cfmakeraw(&Opt);
            //set baudrate
            cfsetispeed(&Opt, speed_arr[i]);
            cfsetospeed(&Opt, speed_arr[i]);
            status = tcsetattr(fd, TCSANOW, &Opt);
            if  (status != 0)
                perror("tcsetattr fd1");
            return;
        }
    }
}

int open_uart_device(int polling_mode,int speed)
{
    int fd;
    if(polling_mode == 1)
	fd = open(uart_dev, O_RDWR|O_NONBLOCK );
    else
	fd = open(uart_dev, O_RDWR);
    if(fd >= 0)
	    set_raw_data_speed(fd,speed);
    return fd;
}

static int try_to_connect_device(int uart_fd)
{
	unsigned long hand_shake = 0x7E7E7E7E;
	char buffer[64]={0};
	char *version_string = (char *)buffer;
	char *data = version_string;
	int i,ret;
	int status = 0;
	int loopcount = 0;
	unsigned long long ms_delta;
	struct timespec tm_begin, tm_end;

	DOWNLOAD_LOGD("try to connect device......uart_fd = %d \n", uart_fd);

	if(-1 == clock_gettime(CLOCK_MONOTONIC, &tm_begin)){
		DOWNLOAD_LOGE("get tm_begin error \n");
		return -1;
	}

	for(;;){
		if(-1 == clock_gettime(CLOCK_MONOTONIC, &tm_end)){
			DOWNLOAD_LOGE("get tm_end error \n");
			return -1;
		}

		if(delta_miliseconds(&tm_begin, &tm_end) >= 1000)
		{
			DOWNLOAD_LOGD("try to connect device timeout\n");
			return -1;
		}

		hand_shake = 0x7E7E7E7E;
		ret = write(uart_fd,&hand_shake,3);
		if(ret < 0){
			DOWNLOAD_LOGD("UART Send HandShake %s %d\n",strerror(errno),ret);
			fsync(uart_fd);
			close(uart_fd);
			return -1;
		}
		usleep(300);
		write(uart_fd,&hand_shake,3);
		data = version_string;
		ret = read(uart_fd,version_string,1);
		if(ret == 1){
			DOWNLOAD_LOGD("end %d 0x%x\n",ret,version_string[0]);
			if(version_string[0]==0x7E){
				data++;
				do{
					ret = read(uart_fd,data,1);
					if(ret == 1){
				 		if(*data == 0x7E){
							status = 1;
							DOWNLOAD_LOGD("Version string received:");

							i=0;
							do{
								DOWNLOAD_LOGD("0x%02x",version_string[i]);
								i++;
							}while(data > &version_string[i]);
							DOWNLOAD_LOGD("0x%02x",version_string[i]);
							DOWNLOAD_LOGD("\n");
							break;
						}
						data++;
						if ( (data - version_string) >= sizeof(buffer)) {
							DOWNLOAD_LOGD("invalid version: rubbish data in driver");
							break;
						}
					}  else {
						if(-1 == clock_gettime(CLOCK_MONOTONIC, &tm_end)){
							DOWNLOAD_LOGE("get tm_end error \n");
							return -1;
						}
					}
				}while(delta_miliseconds(&tm_begin, &tm_end) < FDL_CP_UART_TIMEOUT);
			}
		}
		if(status == 1)
			return uart_fd;
	}
}

static void * load_fdl2memory(int *length,struct image_info *info)
{
	int fdl_fd;
	int read_len,size;
	char *buffer = NULL;
	char *ret_val = NULL;
	char nvbuf[512];
	nv_header_t *nv_head;

	fdl_fd = open(info->image_path, O_RDONLY,0);
	if(fdl_fd < 0){
		DOWNLOAD_LOGE("open file %s error = %d\n", info->image_path, errno);
		return NULL;
	}

	read_len = read(fdl_fd,nvbuf, 512);
	nv_head = (nv_header_t*) nvbuf;
	if(nv_head->magic != NV_HEAD_MAGIC)
	{
		lseek(fdl_fd,SEEK_SET,0);
	}
	DOWNLOAD_LOGD("nvbuf.magic  0x%x \n",nv_head->magic);

	size = info->image_size;
    buffer = malloc(size+4);
    if(buffer == NULL){
        close(fdl_fd);
        DOWNLOAD_LOGE("no memory\n");
        return NULL;
    }
    ret_val = buffer;
	do{
		read_len = read(fdl_fd,buffer,size);
		if(read_len > 0)
		{
			size -= read_len;
			buffer += read_len;
		}
	}while((read_len > 0) && (size > 0));
	close(fdl_fd);
	if(length)
		*length = info->image_size;
	return ret_val;
}
static int download_fdl(int uart_fd,struct image_info *info)
{
	int size=0,ret;
	int data_size=0;
	int offset=0;
	int translated_size=0;
	int ack_size = 0;
	char *buffer,data = 0;
	char *ret_val = NULL;
	char test_buffer1[256]={0};

	buffer = load_fdl2memory(&size,info);
	DOWNLOAD_LOGD("fdl image info : address %p size %x\n",buffer,size);
	if(buffer == NULL)
		return DL_FAILURE;
	ret_val = buffer;
	ret = send_start_message(uart_fd,size,info->address,0);
	if(ret == DL_FAILURE){
                free(ret_val);
                return ret;
        }
	while(size){
		ret = send_data_message(uart_fd,buffer,FDL_PACKET_SIZE,0,0,0);
		if(ret == DL_FAILURE){
			free(ret_val);
			return ret;
		}
		buffer += FDL_PACKET_SIZE;
		size -= FDL_PACKET_SIZE;
	}
	DOWNLOAD_LOGD("send_end_message\n");
	ret = send_end_message(uart_fd,0);
	if(ret == DL_FAILURE){
		free(ret_val);
		return ret;
	}
	DOWNLOAD_LOGD("send_exec_message\n");
	ret = send_exec_message(uart_fd,info->jump_address,0);
	free(ret_val);
	return ret;
}

static void download_power_on(bool enable)
{
	int fd;
	fd = open(POWER_CTL, O_RDWR);
	if(enable)
	{
		ioctl(fd,DOWNLOAD_POWER_ON,NULL);
	}
	else
	{
		ioctl(fd,DOWNLOAD_POWER_OFF,NULL);
	}
	close(fd);
}


static int send_notify_to_client(pmanager_t *pmanager, char *info_str,int type)
{
	int i, ret;
	char *buf;
	int len;

	if(!pmanager || !info_str) return -1;

	DOWNLOAD_LOGD("send_notify_to_client:%s",info_str);

	pthread_mutex_lock(&pmanager->client_fds_lock);

	/* info socket clients that WCN with str info */
	for(i = 0; i < WCN_MAX_CLIENT_NUM; i++)
	{
		DOWNLOAD_LOGD("client_fds[%d].sockfd=%d\n",i, pmanager->client_fds[i].sockfd);

		if((pmanager->client_fds[i].type == type) && (pmanager->client_fds[i].sockfd >= 0)){
			buf = info_str;
			len = strlen(buf) + 1;

			ret = write(pmanager->client_fds[i].sockfd, buf, len);
			if(ret < 0){
				DOWNLOAD_LOGE("reset client_fds[%d]=-1",i);
				close(pmanager->client_fds[i].sockfd);
				pmanager->client_fds[i].sockfd = -1;
			}

			if(pmanager->client_fds[i].type == WCN_SOCKET_TYPE_GNSS){
				DOWNLOAD_LOGE("close gnss client_fds[%d]=-1",i);
				close(pmanager->client_fds[i].sockfd);
				pmanager->client_fds[i].sockfd = -1;
			}
		}
	}

	pthread_mutex_unlock(&pmanager->client_fds_lock);

	return 0;
}




static void download_gnss_chip_en(bool enable)
{
	int fd;
	fd = open(POWER_CTL, O_RDWR);
	if(enable)
	{
		ioctl(fd,GNSS_CHIP_EN,NULL);
	}
	else
	{
		ioctl(fd,GNSS_CHIP_DIS,NULL);
	}
	close(fd);
}

int download_gnss_entry(void)
{
	int uart_fd;
	int download_fd = -1;
	int ret=0;
	char value[PROPERTY_VALUE_MAX] = {'\0'};

	DOWNLOAD_LOGD("download_gnss_entry\n");

	if(pmanager.flag_stop){
		download_gnss_chip_en(0);
		download_power_on(0);
		ret = send_notify_to_client(&pmanager, WCN_RESP_STOP_GNSS,WCN_SOCKET_TYPE_GNSS);
		ret = send_notify_to_client(&pmanager, WCN_RESP_STOP_GNSS,WCN_SOCKET_TYPE_SLOG);
		pmanager.flag_stop = 0;
		return 0;
	}

reboot_device:
	if(!pmanager.flag_dump) {
		download_power_on(1);
		download_gnss_chip_en(1);
	}
    uart_fd = open_uart_device(1,115200);
    if(uart_fd < 0)
    {
		DOWNLOAD_LOGE("open_uart_device fail\n");
		return -1;
    }
	
	ret = try_to_connect_device(uart_fd);
	if(ret < 0) {
        close(uart_fd);
	if(!pmanager.flag_dump) {
		download_gnss_chip_en(0);
		download_power_on(0);
	}
	    goto reboot_device;
    }

	uart_fd = ret;
	if(!pmanager.flag_dump){
		ret = send_uart_speed_message(uart_fd, 2000000, 0);
		if(ret < 0) {
			DOWNLOAD_LOGE("setting uart speed failed!\n");
			close(uart_fd);
			download_gnss_chip_en(0);
			download_power_on(0);
			goto reboot_device;
		}
		DOWNLOAD_LOGD("setting uart speed success!\n");
	}
	ret = send_connect_message(uart_fd,0);
	if(ret < 0) {
		DOWNLOAD_LOGE("connect failed!\n");
		close(uart_fd);
	if(!pmanager.flag_dump) {
		download_gnss_chip_en(0);
		download_power_on(0);
	}
		goto reboot_device;
	}
	DOWNLOAD_LOGD("connect success!\n");

	if(pmanager.flag_dump)
		ret = download_fdl(uart_fd,&download_image_info[0]);
	else
		ret = download_fdl(uart_fd,&download_image_info[1]);
    if(ret == DL_FAILURE){
	    close(uart_fd);
	if(!pmanager.flag_dump) {
		download_gnss_chip_en(0);
		download_power_on(0);
	}
	    goto reboot_device;
    }

	fsync(uart_fd);
    close(uart_fd);

	if(pmanager.flag_reboot){
		ret = send_notify_to_client(&pmanager, WCN_RESP_REBOOT_GNSS,WCN_SOCKET_TYPE_GNSS);
		ret = send_notify_to_client(&pmanager, WCN_RESP_REBOOT_GNSS,WCN_SOCKET_TYPE_SLOG);
		pmanager.flag_reboot = 0;
	}

	if(pmanager.flag_start){
		ret = send_notify_to_client(&pmanager, WCN_RESP_START_GNSS,WCN_SOCKET_TYPE_GNSS);
		ret = send_notify_to_client(&pmanager, WCN_RESP_START_GNSS,WCN_SOCKET_TYPE_SLOG);
		pmanager.flag_start = 0;
	}

    if(pmanager.flag_dump){
		ret = send_notify_to_client(&pmanager, WCN_RESP_DUMP_GNSS,WCN_SOCKET_TYPE_GNSS);
		ret = send_notify_to_client(&pmanager, WCN_RESP_DUMP_GNSS,WCN_SOCKET_TYPE_SLOG);
		pmanager.flag_dump = 0;
	}

    return 0;
}

static void store_client_fd(WcnClient client_fds[], int fd,int type)
{
	if(!client_fds) return;

	int i = 0;

	for (i = 0; i < WCN_MAX_CLIENT_NUM; i++)
	{
		if((pmanager.flag_connect == 1) && (type == WCN_SOCKET_TYPE_SLOG))
		{
			if(client_fds[i].type == WCN_SOCKET_TYPE_SLOG)
			{
				client_fds[i].sockfd = fd;
				client_fds[i].type = type;
				return;
			}
		}
		else if(client_fds[i].sockfd == -1) //invalid fd
		{
			client_fds[i].sockfd = fd;
			client_fds[i].type = type;
			return;
		}
		else if(client_fds[i].sockfd == fd)
		{
			DOWNLOAD_LOGD("%s: Somethine error happens. restore the same fd:%d", __FUNCTION__, fd);
			return;
		}
	}

	if(i == WCN_MAX_CLIENT_NUM)
	{
		DOWNLOAD_LOGD("ERRORR::%s: client_fds is FULL", __FUNCTION__);
		client_fds[i-1].sockfd = fd;
		client_fds[i-1].type = type;
		return;
	}
}


static void *client_listen_thread(void *arg)
{
	pmanager_t *pmanager = (pmanager_t *)arg;

	if(!pmanager)
	{
		DOWNLOAD_LOGE("%s: unexcept NULL pmanager", __FUNCTION__);
		exit(-1);
	}

	while(1)
	{
		int  i = 0;
		fd_set read_fds;
		int rc = 0;
		int max = -1;
		struct sockaddr addr;
		socklen_t alen;
		int c;

		FD_ZERO(&read_fds);

		max = pmanager->listen_fd;
		FD_SET(pmanager->listen_fd, &read_fds);
		FD_SET(pmanager->listen_slog_fd, &read_fds);
		if (pmanager->listen_slog_fd > max)
			max = pmanager->listen_slog_fd;

		if ((rc = select(max + 1, &read_fds, NULL, NULL, NULL)) < 0){
			if (errno == EINTR)
				continue;

			sleep(1);
			continue;
		}else if (!rc)
			continue;

		if (FD_ISSET(pmanager->listen_slog_fd, &read_fds)){
			do {
				alen = sizeof(addr);
				c = accept(pmanager->listen_slog_fd, &addr, &alen);
				DOWNLOAD_LOGD("%s got %d from accept", GNSS_SOCKET_SLOG_NAME, c);
			} while (c < 0 && errno == EINTR);

			if (c < 0){
				DOWNLOAD_LOGE("accept %s failed (%s)", GNSS_SOCKET_SLOG_NAME,strerror(errno));
				sleep(1);
				continue;
			}

			store_client_fd(pmanager->client_fds, c,WCN_SOCKET_TYPE_SLOG);
			pmanager->flag_connect = 1;
		}else if(FD_ISSET(pmanager->listen_fd, &read_fds)) {
			do {
				alen = sizeof(addr);
				c = accept(pmanager->listen_fd, &addr, &alen);
				DOWNLOAD_LOGD("%s got %d from accept", GNSS_SOCKET_NAME, c);
			} while (c < 0 && errno == EINTR);

			if (c < 0){
				DOWNLOAD_LOGE("accept %s failed (%s)", GNSS_SOCKET_NAME,strerror(errno));
				sleep(1);
				continue;
			}

			store_client_fd(pmanager->client_fds, c,WCN_SOCKET_TYPE_GNSS);
			write(pmanager->selfcmd_sockets[0], "new_self_cmd", 12);
		}
	}
}

static void *wcn_exception_listen(void *arg)
{
	int  ret,i;
	char buffer[SOCKET_BUFFER_SIZE];
	fd_set readset;
	int result, max = 0;
	struct timeval timeout;
	pmanager_t *pmanager = (pmanager_t *)arg;

	if(!pmanager)
	{
		DOWNLOAD_LOGE("%s: unexcept NULL pmanager", __FUNCTION__);
		exit(-1);
	}

	while(1){
		timeout.tv_sec = 3;
		timeout.tv_usec = 0;
		FD_ZERO(&readset);

		FD_SET(pmanager->selfcmd_sockets[1], &readset);
		max = pmanager->selfcmd_sockets[1];

		for(i = 0; i < WCN_MAX_CLIENT_NUM; i++){
			if((pmanager->client_fds[i].sockfd >= 0) && (pmanager->client_fds[i].type == WCN_SOCKET_TYPE_GNSS)){
				max = pmanager->client_fds[i].sockfd> max ? pmanager->client_fds[i].sockfd : max;
				FD_SET(pmanager->client_fds[i].sockfd, &readset);
			}
		}

		result = select(max + 1, &readset, NULL, NULL, &timeout);
		if(result == 0)
			continue;

		if(result < 0){
			sleep(1);
			continue;
		}

		memset(buffer, 0, SOCKET_BUFFER_SIZE);

		if (FD_ISSET(pmanager->selfcmd_sockets[1], &readset)) {
			ret = read(pmanager->selfcmd_sockets[1], buffer, SOCKET_BUFFER_SIZE);
			DOWNLOAD_LOGD("sockfd get %d %d bytes %s", pmanager->selfcmd_sockets[1],ret, buffer);
			continue;
		}

		for(i = 0; i < WCN_MAX_CLIENT_NUM; i++){
			if((pmanager->client_fds[i].sockfd >= 0) && FD_ISSET(pmanager->client_fds[i].sockfd, &readset)){
				if(pmanager->client_fds[i].type == WCN_SOCKET_TYPE_GNSS){
					ret = read(pmanager->client_fds[i].sockfd, buffer, SOCKET_BUFFER_SIZE);
					DOWNLOAD_LOGD("sockfd get %d %d bytes %s", pmanager->client_fds[i].sockfd,ret, buffer);
					if(strcmp(buffer,WCN_CMD_REBOOT_GNSS) == 0){
						pmanager->flag_reboot = 1;
						download_gnss_entry();
					}else if(strncmp(buffer,WCN_CMD_DUMP_GNSS,8) == 0){
						char* path = NULL;
						int size = 0;

						path = strstr(buffer, "path=");
						if(path != NULL) {
							path+=5;
							download_image_info[0].image_path = path;
						}

						DOWNLOAD_LOGD("download path= \"%s\" (size=%d)\n",
							download_image_info[0].image_path,
							download_image_info[0].image_size);
						pmanager->flag_dump = 1;
						download_gnss_entry();
					}else if(strncmp(buffer,WCN_CMD_START_GNSS, 9) == 0){
						char* string = NULL;
						int size = 0;

						string = strstr(buffer, "gnss_status");
						if(string != NULL) {
							usleep(300*1000);
							while(download_state != DOWNLOAD_BOOTCOMP) {
								usleep(300*1000);
							}

							send_notify_to_client(pmanager,
								WCN_RESP_START_GNSS,WCN_SOCKET_TYPE_GNSS);
						} else {
							string = NULL;
							string = strstr(buffer, "path=");
							if(string != NULL) {
								string+=5;
								download_image_info[1].image_path = string;
							}

							DOWNLOAD_LOGD("download path= \"%s\" (size=%d)\n",
								download_image_info[1].image_path,
								download_image_info[1].image_size);
							pmanager->flag_start = 1;
							download_gnss_entry();
						}
					}else if(strcmp(buffer,WCN_CMD_STOP_GNSS) == 0){
						pmanager->flag_stop = 1;
						download_gnss_entry();
					}
				}
			}
		}
	}

	return NULL;
}


static int socket_init(pmanager_t *pmanager)
{
	pthread_t thread_client_id,thread_exception_id;
	int i = 0;

	memset(pmanager, 0, sizeof(struct pmanager));

	for(i=0; i<WCN_MAX_CLIENT_NUM; i++)
		pmanager->client_fds[i].sockfd = -1;

	pmanager->listen_fd = socket_local_server(GNSS_SOCKET_NAME, ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
	if(pmanager->listen_fd < 0) {
		DOWNLOAD_LOGE("%s: cannot create local socket server", __FUNCTION__);
		return -1;
	}

	pmanager->listen_slog_fd = socket_local_server(GNSS_SOCKET_SLOG_NAME, ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
	if(pmanager->listen_slog_fd < 0) {
		DOWNLOAD_LOGE("%s: cannot create local socket server", __FUNCTION__);
		return -1;
	}

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, pmanager->selfcmd_sockets) == -1) {

        DOWNLOAD_LOGE("%s: cannot create socketpair for self cmd socket", __FUNCTION__);
        return -1;
    }

	if (pthread_create(&thread_client_id, NULL, client_listen_thread, pmanager))
	{
		DOWNLOAD_LOGE("start_client_listener: pthread_create (%s)", strerror(errno));
		return -1;
	}

	if (pthread_create(&thread_exception_id, NULL, wcn_exception_listen, pmanager))
	{
		DOWNLOAD_LOGE("start_wcn_exception: pthread_create (%s)", strerror(errno));
		return -1;
	}

	return 0;
}

#ifdef REBOOT_DBG

static void download_signal_handler(int sig)
{
	DOWNLOAD_LOGD("sig:%d\n",sig);
	exit(0);
}

#endif


static int gnss_get_img_mode(const char *str,char *val)
{
	FILE *fp = NULL;
	char * pdata = NULL;
	char *curptr,*begin,*end;
	int size = 0;

	if (str == NULL || val == NULL) {
		DOWNLOAD_LOGD("invalid arg\n");
		return -1;
	}

	if (access(CONFIG_XML_PATH,0) == -1) {
		DOWNLOAD_LOGD("config xml file is not exist\n");
		return -1;
	}

	fp = fopen(CONFIG_XML_PATH,"r+");
	if(fp == NULL) {
		DOWNLOAD_LOGD("open config fail %s\n", strerror(errno));
		return -1;
	}

	fseek(fp,0L,SEEK_END);
	size = ftell(fp);
	DOWNLOAD_LOGD("size is %d\n", size);
	pdata = calloc(size,1);
	if (pdata == NULL) {
	    fclose(fp);
	    return -1;
	}
	rewind(fp);
	fread(pdata,size,1,fp);

	if ((curptr = strstr(pdata, str)) != NULL) {
		if ((begin = strchr(curptr+strlen(str)+2, '\"')) != NULL) {
			if ((end = strchr(begin+1, '\"')) != NULL) {
				memcpy(val,begin+1,end-begin-1);
				DOWNLOAD_LOGD("img mode is %s\n", val);
			} else {
				fclose(fp);
				free(pdata);
				return -1;
			}
		} else {
			fclose(fp);
			free(pdata);
			return -1;
		}
	} else {
		DOWNLOAD_LOGD("can't get the %s item in config\n", str);
		fclose(fp);
		free(pdata);
		return -1;
	}

	fclose(fp);
	free(pdata);

	return 0;
}

static int gnss_get_image_path(void)
{
	char partition[PROPERTY_VALUE_MAX];
	char mode[PROPERTY_VALUE_MAX];
	char img_mode[16];

	memset(img_mode, 0, sizeof(img_mode));
	memset(partition,0,sizeof(partition));
	memset(mode,0,sizeof(mode));

	property_get("ro.ge2.partition",partition,NULL);
	property_get("ro.ge2.mode",mode,NULL);
	DOWNLOAD_LOGD("ro.ge2.partition:%s,ro.ge2.mode:%s",partition,mode);

	if (gnss_get_img_mode("GPS-IMG-MODE", img_mode) == 0) {
		if (strncmp(img_mode, "GNSSMODEM", sizeof("GNSSMODEM")) == 0) {
			DOWNLOAD_LOGD("gnssmodem\n");
			memcpy(mode, "gps_glnass", sizeof("gps_glnass"));
		} else if (strncmp(img_mode, "GNSSBDMODEM", sizeof("GNSSBDMODEM")) == 0) {
			DOWNLOAD_LOGD("gnssbdmodem\n");
			memcpy(mode, "gps_beidou", sizeof("gps_beidou"));
		}
	}
	DOWNLOAD_LOGD("real config partition:%s,mode:%s",partition,mode);

	if(0 == strncmp(partition,"true", sizeof("true"))){
		#ifdef PARTITION_PATH_4.4
		memcpy(gPath,"/dev/block/platform/sprd-sdhci.3/by-name/gnssmodem",
		sizeof("/dev/block/platform/sprd-sdhci.3/by-name/gnssmodem"));
		#else
		memcpy(gPath,"/dev/block/platform/sdio_emmc/by-name/temppartition",
		sizeof("/dev/block/platform/sdio_emmc/by-name/temppartition"));
		#endif
	}else{
		if(0 == strncmp(mode,"gps_beidou", sizeof("gps_beidou"))){
			memcpy(gPath,"/system/etc/gnssbdmodem.bin",sizeof("/system/etc/gnssbdmodem.bin"));
		}else{
			memcpy(gPath,"/system/etc/gnssmodem.bin",sizeof("/system/etc/gnssmodem.bin"));
		}
	}
	download_image_info[1].image_path = gPath;


	DOWNLOAD_LOGD("download path= \"%s\" (size=%d)\n",
	download_image_info[1].image_path,
	download_image_info[1].image_size);

	return 0;
}


int main(void)
{ 
	int ret=0;
#ifdef REBOOT_DBG
	/* Register signal handler */
	signal(SIGINT, download_signal_handler);
	signal(SIGKILL, download_signal_handler);
	signal(SIGTERM, download_signal_handler);
#endif
	signal(SIGPIPE, SIG_IGN);

	ret = socket_init(&pmanager);

	download_state = DOWNLOAD_START;
	do{
		#if 1
		if(download_state == DOWNLOAD_START && (!gnss_get_image_path())){
			download_gnss_entry();
			download_state = DOWNLOAD_BOOTCOMP;
		}
		#endif
		sleep(1000);
	}while(1);

	return 0;
}

