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
#include "cmd_def.h"
#include "packet.h"
#include "connectivity_rf_parameters.h"
#define REBOOT_DBG

#define	POWER_CTL		"/dev/power_ctl"
#define	DLOADER_PATH		"/dev/download"
#define	UART_DEVICE_NAME	"/dev/ttyS0"
#define LOOP_DEV	"/proc/mdbg/loopcheck"

#define DOWNLOAD_IOCTL_BASE	'z'
#define DOWNLOAD_POWER_ON	_IO(DOWNLOAD_IOCTL_BASE, 0x01)
#define DOWNLOAD_POWER_OFF	_IO(DOWNLOAD_IOCTL_BASE, 0x02)
#define DOWNLOAD_POWER_RST	_IO(DOWNLOAD_IOCTL_BASE, 0x03)
#define MARLIN_SET_VERSION	_IO(DOWNLOAD_IOCTL_BASE, 0x08)

#define WCN_SOCKET_NAME			"external_wcn"
#define WCN_SOCKET_SLOG_NAME	"external_wcn_slog"

#define WCN_MAX_CLIENT_NUM		(10)


#define WCN_SOCKET_TYPE_SLOG		1
#define WCN_SOCKET_TYPE_WCND		2
/*slog*/
#define EXTERNAL_WCN_ALIVE		"WCN-EXTERNAL-ALIVE"
#define WCN_CMD_START_DUMP_WCN	"WCN-EXTERNAL-DUMP"
#define WCN_DUMP_LOG_COMPLETE 	"persist.sys.sprd.wcnlog.result"

/* property to marlin version : AA/BA/00 */
#define MARLIN_HARDWARE_VERSION "hw.marlin.version"
#define MARLIN_HW_VERSION_BA "BA"
#define MARLIN_HW_VERSION_AA "AA"
#define MARLIN_HW_VERSION_DEFAULT "00"
/*wcn*/
#define WCN_CMD_REBOOT_WCN		"rebootwcn"
#define WCN_CMD_DUMP_WCN		"dumpwcn"
#define WCN_CMD_START_WCN		"startwcn"
#define WCN_CMD_STOP_WCN		"stopwcn"
#define WCN_RESP_REBOOT_WCN		"rebootwcn-ok"
#define WCN_RESP_DUMP_WCN		"dumpwcn-ok"
#define WCN_RESP_START_WCN		"startwcn-ok"
#define WCN_RESP_STOP_WCN		"stopwcn-ok"
#define SOCKET_BUFFER_SIZE 		(128)
#define POWERUP_MAX_RETRY       20  /* how many times we retry to power up the chip */

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
};

typedef struct  _NV_HEADER {
	unsigned int magic;
	unsigned int len;
	unsigned int checksum;
	unsigned int version;
}nv_header_t;

#define NV_HEAD_MAGIC   0x00004e56

//#define	FDL_PACKET_SIZE 	256
#define	FDL_PACKET_SIZE 	(1*1024)
#define LS_PACKET_SIZE		(256)
#ifdef WCN_EXTEN_15C
#define HS_PACKET_SIZE		(128*1024)
#else
#define HS_PACKET_SIZE		(32*1024)
#endif

#define FDL_CP_PWRON_DLY	(160*1000)//us
#define FDL_CP_UART_TIMEOUT	(3000)//(200) //ms

#define	DL_FAILURE		(-1)
#define DL_SUCCESS		(0)

#define MS_IN_SEC 1000
#define NS_IN_MS  1000000

static DOWNLOAD_STA_E download_state = DOWNLOAD_INIT;
char test_buffer[HS_PACKET_SIZE+128]={0};
static char *uart_dev = UART_DEVICE_NAME;
static int fdl_cp_poweron_delay = FDL_CP_PWRON_DLY;

int speed_arr[] = {B921600,B115200, B38400, B19200, B9600, B4800, B2400, B1200, B300,
                   B921600,B115200, B38400, B19200, B9600, B4800, B2400, B1200, B300, };
int name_arr[] = {921600,115200,38400,  19200, 9600,  4800,  2400,  1200,  300,
        921600, 115200,38400,  19200,  9600, 4800, 2400, 1200,  300, };


struct wifi_calibration
{
	wifi_rf_t wifi_rf_cali;
	wifi_cali_cp_t wifi_cali_cp;
};
static struct wifi_calibration wifi_data;


static int download_images_count=2;	// download_images_count and download_image_info[] must keep matched!
struct image_info download_image_info[] = {
	{
		//fdl
		#ifdef PARTITION_PATH_4.4
		"/dev/block/platform/sprd-sdhci.3/by-name/wcnfdl",
		#else
		"/dev/block/platform/sdio_emmc/by-name/wcnfdl",
		#endif
		0x2000,
		0x80000000,
	},
	{
		//img
		#ifdef PARTITION_PATH_4.4
		"/dev/block/platform/sprd-sdhci.3/by-name/wcnmodem",
		#else
		"/dev/block/platform/sdio_emmc/by-name/wcnmodem",
		#endif
		0xc0000,
		0x100000,
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
	    //set raw data mode
           // Opt.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
            Opt.c_oflag &= ~OPOST;
            Opt.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
            Opt.c_cflag &= ~(CSIZE | PARENB | CRTSCTS);
            Opt.c_cflag |= CS8;

	    //Opt.c_iflag = ~(ICANON|ECHO|ECHOE|ISIG);
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
		#if 0
		int try_count=0;
		ms_delta = delta_miliseconds(&tm_begin, &tm_end);
		if(ms_delta > FDL_CP_UART_TIMEOUT){
			loopcount++;
			if(loopcount == 5) {
				DOWNLOAD_LOGE("need to hard reset \n");
				return -1;
			}
			
			if(-1 == clock_gettime(CLOCK_MONOTONIC, &tm_begin)){
				DOWNLOAD_LOGE("get tm_begin error \n");
				return -1;
			}
		}
		#endif
		hand_shake = 0x7E7E7E7E;
		ret = write(uart_fd,&hand_shake,3);
		if(ret < 0){
			DOWNLOAD_LOGD("UART Send HandShake %s %d\n",strerror(errno),uart_fd);
			fsync(uart_fd);
			close(uart_fd);
			//uart_fd = open_uart_device(1,115200);
			//continue;
			return -1;
		}
		//try_count ++;
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

#ifdef GET_MARLIN_CHIPID
#define IMG_HEAD_SIZE	12
#define IMG_HEAD_MAGIC "WCNM"
#define IMG_MARLINAA_TAG "MLAA"
#define IMG_MARLINBA_TAG "MLBA"
#define BSL_REP_IS_2331_AA 0xAA		    /* 0xAA */
#define BSL_REP_IS_2331_BA	0xBA		    /* 0xBA */

typedef unsigned int uint32;
struct head{
	char magic[4];		//"WCNM"
	uint32 version;		//0
	uint32 img_count;	//image count
};

struct imageinfo{
	char tag[4];		//"MLAA","MLBA"
	uint32 offset;		//image offset
	uint32 size;		//image size
};

struct img_flag{
	int is_combine_flag;	// 0: is not combine image: 1: is combine image
	int which_img_flag;	// 1: is the one image,2: is the second image, just is_combine_flag = 1 is valid.
};

static struct head marlin_imghead;
static struct imageinfo marlin_imginfo;
struct img_flag marlin_img_flag = {0,0};
/******************************************************
(step2): if yes,judge which image we need to parse(judge chip id)
return value:
0: ok
other: not ok
******************************************************/
int parse_image_info(int  image_fd,char *strc)
{
	struct imageinfo *imginfo;
	int read_len;
	int i,ret=0;
	char tag[5]="";
	int packet_size=IMG_HEAD_SIZE;

	DOWNLOAD_LOGD("parse_image_info enter... \n");
	imginfo = & marlin_imginfo;
	read_len = read(image_fd,imginfo,packet_size);
	DOWNLOAD_LOGD("Step 2 Read imghead read_len=: %d \n",read_len);
	if(read_len == packet_size ){
		DOWNLOAD_LOGD("step2 image.offset= %d,image->size=%d \n",imginfo->offset,imginfo->size);
		for(i=0;i<4;i++){
			tag[i]=imginfo->tag[i];
		}
		tag[i]='\0';
		DOWNLOAD_LOGD("need compare string=%s, tag= %s\n",strc,tag);

		ret=strncmp(strc,tag,4);
		DOWNLOAD_LOGD("compare  char result ret = %d",ret);
		if(ret==0){
			DOWNLOAD_LOGD("step2 find the image tag. \n");
		}
	}
	else{
		DOWNLOAD_LOGD("parse_image_info is less then the need read!!!\n");
		ret=-1;
	}
	DOWNLOAD_LOGD("parse_image_info exit... \n");
	return ret;
}

/******************************************************
	(setp0): open image
	(step1): read head to parse if it is the combine image
	(step2): if yes,judge which image we need to parse(judge chip id)
	(step3): parse the image
	(step4): download the image
return value:
0: ok
other: not ok
******************************************************/
int parse_wcnimg_info(struct image_info *info,int chiptype)
{
	struct imageinfo *imginfo;
	struct head *imghead;
	int image_fd,read_len;
	int i,ret=-1;
	char magic[5]="";
	char tag[5]="";
	int packet_size=IMG_HEAD_SIZE;
	int step = 0;

	DOWNLOAD_LOGD("parse_image_header file enter...,open file: %s \n",info->image_path);
	image_fd = open(info->image_path, O_RDONLY,0);
	if(image_fd < 0){
		DOWNLOAD_LOGE("open file: %s error = %d\n", info->image_path,errno);
		return DL_SUCCESS;
	}

	imghead = & marlin_imghead;
	read_len = read(image_fd,imghead,packet_size);
	DOWNLOAD_LOGD("Read imghead read_len=: %d \n",read_len);
	if(read_len > 0 ){
		if(read_len == packet_size ){
			for(i=0;i<4;i++){
				magic[i]=imghead->magic[i];
			}
			magic[i]='\0';
			DOWNLOAD_LOGD("IMG_HEAD_MAGIC = %s, magic= %s \n",IMG_HEAD_MAGIC,magic);
			ret=strncmp(IMG_HEAD_MAGIC,magic,4);
			DOWNLOAD_LOGD("compare char ret = %d",ret);
			if(ret==0){
				DOWNLOAD_LOGD("the firmware is combine two in one,need to parse it \n");
				//TODO,Check how many firmware combine in
				step = 2;
				//marlin_img_flag.is_combine_flag = 1;//just the combine image info is right,it will be set this flag
			}
		}
	}
	//(step2): if yes,judge which image we need to parse(judge chip id)
	if(step == 2){
		DOWNLOAD_LOGD("Do step 2... \n");
		ret = parse_image_info(image_fd,IMG_MARLINAA_TAG);
		if(ret != 0){
			DOWNLOAD_LOGD("parse_image_info can't find the right info!!ret = %d \n",ret);
			return ret;
		}
		if(chiptype == BSL_REP_IS_2331_BA ){
			ret = parse_image_info(image_fd,IMG_MARLINBA_TAG);
			if(ret != 0){
				DOWNLOAD_LOGD("parse_image_info can't find the right info!!ret = %d \n",ret);
				return ret;
			}
		}
		marlin_img_flag.is_combine_flag = 1;//just the combine image info is right,it will be set this flag
		step = 3;
	}
	//(step3): parse the image

	close(image_fd);
	return ret;
}
#endif

int download_image(int channel_fd,struct image_info *info)
{
	int packet_size,trans_size=HS_PACKET_SIZE;
	int image_fd;
	int read_len;
	char *buffer;
	char nvbuf[512];
	int i,image_size;
	int count = 0;
	int offset = 0;
	int ret=DL_SUCCESS;
	unsigned int chip_id=0;

        if(info->image_path == NULL)
                return DL_SUCCESS;
        if(info->image_size < HS_PACKET_SIZE)
                trans_size = LS_PACKET_SIZE;


#ifdef GET_MARLIN_CHIPID
        //initial the marlin_img_flag
        marlin_img_flag.is_combine_flag = 0;
        marlin_img_flag.which_img_flag= 0;

        //added for read chipid from cp2 to ap by tingle.xu
        chip_id = send_getchipid_message(channel_fd,1);

		if(0x2331FF > chip_id){
			DOWNLOAD_LOGD(">>>>>>>Response is marlin AA CHIP\n");
			marlin_img_flag.which_img_flag = BSL_REP_IS_2331_AA;
			property_set(MARLIN_HARDWARE_VERSION, MARLIN_HW_VERSION_AA);
            DOWNLOAD_LOGD("Loading marlin AA image to download....\n");
            ret = parse_wcnimg_info(info,BSL_REP_IS_2331_AA);
		}
		else if((0x23310FFF < chip_id)&&(0x233110FF > chip_id)){
			DOWNLOAD_LOGD(">>>>>>>Response is marlin BA CHIP\n");
			marlin_img_flag.which_img_flag = BSL_REP_IS_2331_BA;
			property_set(MARLIN_HARDWARE_VERSION, MARLIN_HW_VERSION_BA);
            DOWNLOAD_LOGD("Loading marlin BA image to download...\n");
            ret = parse_wcnimg_info(info,BSL_REP_IS_2331_BA);
		}
		else{
			DOWNLOAD_LOGD(">>>>>>>Response is other chip!\n");
			property_set(MARLIN_HARDWARE_VERSION, MARLIN_HW_VERSION_DEFAULT);
		}
#endif
	image_fd = open(info->image_path, O_RDONLY,0);

        if(image_fd < 0){
                DOWNLOAD_LOGE("open file: %s error = %d\n", info->image_path,errno);
		return DL_SUCCESS;
       }
	image_size = info->image_size;
#ifdef GET_MARLIN_CHIPID
        DOWNLOAD_LOGD("info->address = %d\n",info->address);
        DOWNLOAD_LOGD("info->image_size = %d,marlin_img_flag.is_combine_flag=%d \n",image_size,marlin_img_flag.is_combine_flag);
        if(marlin_img_flag.is_combine_flag == 1){
                DOWNLOAD_LOGD("marlin_imginfo.offset = %d,AA=0x%x,BA=0x%x\n",marlin_imginfo.offset,BSL_REP_IS_2331_AA,BSL_REP_IS_2331_BA);
                if((marlin_img_flag.which_img_flag == BSL_REP_IS_2331_AA)||(marlin_img_flag.which_img_flag == BSL_REP_IS_2331_BA)){
                        lseek(image_fd,marlin_imginfo.offset,SEEK_SET);

                        image_size = marlin_imginfo.size;
                        DOWNLOAD_LOGD("marlin_imginfo.size = %d \n",marlin_imginfo.size);
                }
                else{
                        DOWNLOAD_LOGD("marlin_img_flag.which_img_flag = %x \n",marlin_img_flag.which_img_flag);
                        DOWNLOAD_LOGD("the image is combine ,but is not the AA or BA !!!exit!!\n");
						close(image_fd);
                        return -1;
                }
        }
		count = (image_size+trans_size-1)/trans_size;
		DOWNLOAD_LOGD("=====image_size = %d, count=%d,trans_size=%d address=%d =====\n",image_size,count,trans_size,info->address);
#endif
	count = (image_size+trans_size-1)/trans_size;
	ret = send_start_message(channel_fd,count*trans_size,info->address,1);
	if (ret != DL_SUCCESS){
		close(image_fd);
		return DL_FAILURE;
	}
	for(i=0;i<count;i++){
		packet_size = trans_size;
		buffer = (char *)&test_buffer[8];
		do{
			read_len = read(image_fd,buffer,packet_size);
			if(read_len > 0){
				packet_size -= read_len;
				buffer += read_len;
			  }else{
			  	break;
			  }
		}while(packet_size > 0);
		
		if(image_size < trans_size){
			for(i=image_size;i<trans_size;i++)
				test_buffer[i+8] = 0xFF;
			image_size = 0;
		}else{
			image_size -= trans_size;
		}
		
		ret = send_data_message(channel_fd,test_buffer,trans_size,1,trans_size,image_fd);
		if(ret != DL_SUCCESS){
			close(image_fd);
			return DL_FAILURE;
		}
	}

	ret = send_end_message(channel_fd,1);
	close(image_fd);
	return ret;
}

int download_images(int channel_fd)
{
	struct image_info *info;
	int i ,ret=DL_SUCCESS;
	int image_count = download_images_count - 1;

	info = &download_image_info[1];
	for(i=0;i<image_count;i++){
		ret = download_image(channel_fd,info);
		if(ret != DL_SUCCESS)
			return DL_FAILURE;
		info++;
	}
	ret = send_exec_message(channel_fd,download_image_info[1].address,1);
	return ret;
}

void * load_fdl2memory(int *length)
{
	int fdl_fd;
	int read_len,size;
	char *buffer = NULL;
	char *ret_val = NULL;
	struct image_info *info;
	char nvbuf[512];
	nv_header_t *nv_head;

	info = &download_image_info[0];
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
	}while(size > 0);
	close(fdl_fd);
	if(length)
		*length = info->image_size;
	return ret_val;
}
static int download_fdl(int uart_fd)
{
	int size=0,ret;
	int data_size=0;
	int offset=0;
	int translated_size=0;
	int ack_size = 0;
	char *buffer,data = 0;
	char *ret_val = NULL;
	char test_buffer1[256]={0};

	buffer = load_fdl2memory(&size);
	DOWNLOAD_LOGD("fdl image info : address %p size %x\n",buffer,size);
	if(buffer == NULL)
		return DL_FAILURE;
	ret_val = buffer;
	ret = send_start_message(uart_fd,size,download_image_info[0].address,0);
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
	ret = send_exec_message(uart_fd,download_image_info[0].address,0);
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

static void download_hw_rst(void)
{
	int fd;
	fd = open(POWER_CTL, O_RDWR);
	ioctl(fd,DOWNLOAD_POWER_RST,NULL);
	close(fd);
}

static void download_set_marlin_version(void)
{
	int fd;

	DOWNLOAD_LOGD("download_set_marlin_version\n");
	fd = open(POWER_CTL, O_RDWR);
	ioctl(fd,MARLIN_SET_VERSION,NULL);
	close(fd);
}

static void download_wifi_calibration(int download_fd)
{
	int ret=0;
	int retry_cnt = 0;
	DOWNLOAD_LOGD("start download calibration\n");

	ret = write(download_fd,"start_calibration",17);
	//DOWNLOAD_LOGD("wifi_rf_t size:%d\n",sizeof(wifi_data.wifi_rf_cali));
	//DOWNLOAD_LOGD("wifi_cali_cp_t size:%d\n",sizeof(wifi_data.wifi_cali_cp));

	/* start calibration*/
	get_connectivity_rf_param(&wifi_data.wifi_rf_cali);
	ret = write(download_fd,&wifi_data.wifi_rf_cali,sizeof(wifi_data.wifi_rf_cali));
	#ifndef WCN_EXTEN_15C
	do{
                ret = read(download_fd,&wifi_data.wifi_cali_cp,sizeof(wifi_data.wifi_cali_cp));
                usleep(1000);
                retry_cnt++;
        }while((ret <= 0) && (retry_cnt <= 2000));

	if(ret <= 0)
                DOWNLOAD_LOGD("wait cali fail!!!!\n");

	if(!wifi_data.wifi_rf_cali.wifi_cali.cali_config.is_calibrated){
		wlan_save_cali_data_to_file(&wifi_data.wifi_cali_cp);
	}
	#else
	if(!wifi_data.wifi_rf_cali.wifi_cali.cali_config.is_calibrated){
		do{
        	        ret = read(download_fd,&wifi_data.wifi_cali_cp,sizeof(wifi_data.wifi_cali_cp));
                	usleep(1000);
                	retry_cnt++;
        	}while((ret <= 0) && (retry_cnt <= 2000));
        
		if(ret <= 0)
                	DOWNLOAD_LOGD("wait cali fail!!!!\n");

		wlan_save_cali_data_to_file(&wifi_data.wifi_cali_cp);
	}

#endif
	ret = write(download_fd,"end_calibration",15);

	DOWNLOAD_LOGD("end download calibration\n");
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

			if(pmanager->client_fds[i].type == WCN_SOCKET_TYPE_WCND){
				DOWNLOAD_LOGE("close wcnd client_fds[%d]=-1",i);
				close(pmanager->client_fds[i].sockfd);
				pmanager->client_fds[i].sockfd = -1;
			}
		}
	}

	pthread_mutex_unlock(&pmanager->client_fds_lock);

	return 0;
}

static int send_msg_to_mdbg(char *str)
{
	int loop_fd = -1;
	int ret=0;

	loop_fd = open(LOOP_DEV, O_RDWR|O_NONBLOCK);
	ret = write(loop_fd, str, strlen(str));
	if(ret < 0)
	{
		DOWNLOAD_LOGE("write %s failed, error:%s loop_fd:%d",LOOP_DEV, strerror(errno),loop_fd);
		close(loop_fd);
		return -1;
	}
	close(loop_fd);
	return 0;
}

int download_entry(void)
{
	int uart_fd;
	int download_fd = -1;
	int ret=0;
	char value[PROPERTY_VALUE_MAX] = {'\0'};
	bool reboot = false;
	int retry = POWERUP_MAX_RETRY;              //retry 3 times

	DOWNLOAD_LOGD("download_entry\n");

	if(pmanager.flag_stop){
		#ifdef ENABLE_POWER_CTL
		download_power_on(0);
		#endif
		ret = send_notify_to_client(&pmanager, WCN_RESP_STOP_WCN,WCN_SOCKET_TYPE_WCND);
		pmanager.flag_stop = 0;
		return 0;
	}
	#ifndef ENABLE_POWER_CTL
	if(pmanager.flag_start){
		send_msg_to_mdbg(WCN_CMD_START_WCN);
		ret = send_notify_to_client(&pmanager, WCN_RESP_START_WCN,WCN_SOCKET_TYPE_WCND);
		pmanager.flag_start = 0;
		return 0;
	}
	#endif
	if(pmanager.flag_reboot){
		download_power_on(0);
	}

reboot_device:
	if (reboot)
	{
		download_power_on(0);
		if (!(retry--))
		{
			DOWNLOAD_LOGE("failed to power up marlin, max retry reached\n");
			return -1;
		}
		usleep(30000);
		reboot=false;
	}
	download_power_on(1);
	download_hw_rst();
    uart_fd = open_uart_device(1,115200);
    if(uart_fd < 0)
    {
		DOWNLOAD_LOGE("open_uart_device fail %s\n",strerror(errno));
		reboot = true;
		goto reboot_device;
    }
    //download_hw_rst();
	
	//uart_fd = ret;
	if (DL_FAILURE == try_to_connect_device(uart_fd)
		|| DL_FAILURE == send_connect_message(uart_fd,0)
		|| DL_FAILURE == download_fdl(uart_fd))
	{
		close(uart_fd);
		reboot = true;
		goto reboot_device;
	}

	fsync(uart_fd);
    close(uart_fd);

    download_fd = open(DLOADER_PATH, O_RDWR);
    DOWNLOAD_LOGD("open dloader device successfully ... \n");

	if(pmanager.flag_dump){
		/*send dump cmmd and do dump*/
		DOWNLOAD_LOGD("start dump mem\n");
		send_msg_to_mdbg(WCN_CMD_DUMP_WCN);
		property_set(WCN_DUMP_LOG_COMPLETE, "0");
		ret = send_notify_to_client(&pmanager, WCN_CMD_START_DUMP_WCN,WCN_SOCKET_TYPE_SLOG);
		send_dump_mem_message(download_fd,0,0,1);
		close(download_fd);

		while (1)
		{
			sleep(1);
			if (property_get(WCN_DUMP_LOG_COMPLETE, value, NULL))
			{
				if (strcmp(value, "1") == 0)
				{
					break;
				}
			}
		}
		DOWNLOAD_LOGD("end dump mem\n");
	}else{
	    ret = download_images(download_fd);
	    if (ret == DL_FAILURE){
		    close(download_fd);
		    sleep(1);
		    reboot = true;
		    goto reboot_device;
	    }
	    DOWNLOAD_LOGD("download finished ......\n");

	    download_wifi_calibration(download_fd);
	    close(download_fd);
	}


	if(pmanager.flag_dump){
		ret = send_notify_to_client(&pmanager, WCN_RESP_DUMP_WCN,WCN_SOCKET_TYPE_WCND);
		pmanager.flag_dump = 0;
	}

	if(pmanager.flag_reboot){
		ret = send_notify_to_client(&pmanager, WCN_RESP_REBOOT_WCN,WCN_SOCKET_TYPE_WCND);
		pmanager.flag_reboot = 0;
	}

	if(pmanager.flag_start){
		#ifdef ENABLE_POWER_CTL
		send_msg_to_mdbg(WCN_CMD_START_WCN);
		#endif
		ret = send_notify_to_client(&pmanager, WCN_RESP_START_WCN,WCN_SOCKET_TYPE_WCND);
		pmanager.flag_start = 0;
	}
	#if 0
	if(pmanager.flag_connect){
		ret = send_notify_to_client(&pmanager, EXTERNAL_WCN_ALIVE,WCN_SOCKET_TYPE_SLOG);
		pmanager.flag_connect = 0;
	}
	#endif
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
				DOWNLOAD_LOGD("%s got %d from accept", WCN_SOCKET_SLOG_NAME, c);
			} while (c < 0 && errno == EINTR);

			if (c < 0){
				DOWNLOAD_LOGE("accept %s failed (%s)", WCN_SOCKET_SLOG_NAME,strerror(errno));
				sleep(1);
				continue;
			}

			store_client_fd(pmanager->client_fds, c,WCN_SOCKET_TYPE_SLOG);
			pmanager->flag_connect = 1;
		}else if(FD_ISSET(pmanager->listen_fd, &read_fds)) {
			do {
				alen = sizeof(addr);
				c = accept(pmanager->listen_fd, &addr, &alen);
				DOWNLOAD_LOGD("%s got %d from accept", WCN_SOCKET_NAME, c);
			} while (c < 0 && errno == EINTR);

			if (c < 0){
				DOWNLOAD_LOGE("accept %s failed (%s)", WCN_SOCKET_NAME,strerror(errno));
				sleep(1);
				continue;
			}

			store_client_fd(pmanager->client_fds, c,WCN_SOCKET_TYPE_WCND);
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
			if((pmanager->client_fds[i].sockfd >= 0) && (pmanager->client_fds[i].type == WCN_SOCKET_TYPE_WCND)){
				max = pmanager->client_fds[i].sockfd> max ? pmanager->client_fds[i].sockfd : max;
				FD_SET(pmanager->client_fds[i].sockfd, &readset);
			}
		}

		result = select(max + 1, &readset, NULL, NULL, NULL/*&timeout*/);
		if(result == 0)
			continue;

		if(result < 0){
			sleep(1);
			continue;
		}

		memset(buffer, 0, SOCKET_BUFFER_SIZE);

		if (FD_ISSET(pmanager->selfcmd_sockets[1], &readset)) {
			ret = read(pmanager->selfcmd_sockets[1], buffer, SOCKET_BUFFER_SIZE);
			//DOWNLOAD_LOGD("sockfd get %d %d bytes %s", pmanager->selfcmd_sockets[1],ret, buffer);
			continue;
		}

		for(i = 0; i < WCN_MAX_CLIENT_NUM; i++){
			if((pmanager->client_fds[i].sockfd >= 0) && FD_ISSET(pmanager->client_fds[i].sockfd, &readset)){
				if(pmanager->client_fds[i].type == WCN_SOCKET_TYPE_WCND){
					ret = read(pmanager->client_fds[i].sockfd, buffer, SOCKET_BUFFER_SIZE);
					//DOWNLOAD_LOGD("sockfd get %d %d bytes %s", pmanager->client_fds[i].sockfd,ret, buffer);
					if(strcmp(buffer,WCN_CMD_REBOOT_WCN) == 0){
						pmanager->flag_reboot = 1;
						//download_state = DOWNLOAD_START;
						download_entry();
					}else if(strcmp(buffer,WCN_CMD_DUMP_WCN) == 0){
						pmanager->flag_dump = 1;
						//download_state = DOWNLOAD_START;
						download_entry();
					}else if(strcmp(buffer,WCN_CMD_START_WCN) == 0){
						#ifndef ENABLE_POWER_CTL
						while(download_state != DOWNLOAD_BOOTCOMP) {
							DOWNLOAD_LOGD("sleep 100ms to wait for download complete.\n");
							usleep(100*1000);
						}
						#endif
						pmanager->flag_start = 1;
						//download_state = DOWNLOAD_START;
						download_entry();
					}else if(strcmp(buffer,WCN_CMD_STOP_WCN) == 0){
						pmanager->flag_stop = 1;
						download_entry();
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

	pmanager->listen_fd = socket_local_server(WCN_SOCKET_NAME, ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
	if(pmanager->listen_fd < 0) {
		DOWNLOAD_LOGE("%s: cannot create local socket server", __FUNCTION__);
		return -1;
	}

	pmanager->listen_slog_fd = socket_local_server(WCN_SOCKET_SLOG_NAME, ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
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

	#ifndef ENABLE_POWER_CTL
	download_state = DOWNLOAD_START;
	#endif

	#ifdef WCN_EXTEN_15C
	download_set_marlin_version();
	#endif

	do{
		#ifndef ENABLE_POWER_CTL
		if(download_state == DOWNLOAD_START){
			if(download_entry() == 0){
				download_state = DOWNLOAD_BOOTCOMP;
			}
		}
		#endif
		sleep(1000);
	}while(1);

	return 0;
}

