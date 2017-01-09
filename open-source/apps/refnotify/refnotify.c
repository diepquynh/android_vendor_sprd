#define LOG_TAG "refnotify"

#include <ctype.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include <cutils/sockets.h>
#include <dirent.h>
#include <fcntl.h>
#include <mtd/mtd-user.h>
#include <poll.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/reboot.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <linux/rtc.h>

#define REF_DEBUG

#ifdef REF_DEBUG
#define REF_LOGD(x...) ALOGD( x )
#define REF_LOGE(x...) ALOGE( x )
#else
#define REF_LOGD(x...)
#define REF_LOGE(x...)
#endif

#define TD_NOTIFY_DEV "/dev/spipe_td8"
#define W_NOTIFY_DEV "/dev/spipe_w8"
#define LTE_NOTIFY_DEV "/dev/spipe_lte8"
//#define TD_NOTIFY_DEV "/dev/spimux18"
//#define L_NOTIFY_DEV "/dev/sdiomux18"

#define TD_FIFO_PATH "/cache/td_timesyncfifo"
#define W_FIFO_PATH "/cache/w_timesyncfifo"
#define L_FIFO_PATH "/cache/l_timesyncfifo"

#define TIME_SERVER_SOCK_NAME "cp_time_sync_server"

#define RTC_DEV "/dev/rtc0"

/*used to get sleep/wake state of display*/
#define WakeFileName  "/sys/power/wait_for_fb_wake"
#define SleepFileName  "/sys/power/wait_for_fb_sleep"

#define IQ_DEV "/sys/module/sprd_iq/parameters/iq_base"
#define IQ_BASE_LENGTH 32

#define FREQ_RGB_FB_DEV  "/sys/class/graphics/fb0/dynamic_pclk"
#define FREQ_MIPI_FB_DEV "/sys/class/graphics/fb0/dynamic_mipi_clk"
#define BM_DEV "/dev/sprd_bm"

enum sci_bm_cmd_index {
	BM_STATE = 0x0,
	BM_CHANNELS,
	BM_AXI_DEBUG_SET,
	BM_AHB_DEBUG_SET,
	BM_PERFORM_SET,
	BM_PERFORM_UNSET,
	BM_OCCUR,
	BM_CONTINUE_SET,
	BM_CONTINUE_UNSET,
	BM_DFS_SET,
	BM_DFS_UNSET,
	BM_PANIC_SET,
	BM_PANIC_UNSET,
	BM_BW_CNT_START,
	BM_BW_CNT_STOP,
	BM_BW_CNT_RESUME,
	BM_BW_CNT,
	BM_BW_CNT_CLR,
	BM_DBG_INT_CLR,
	BM_DBG_INT_SET,
	BM_CMD_MAX,
};

enum {
	REF_DISABLE_LCD = 0,
	REF_MIPI_LCD,
	REF_RGB_LCD
};


enum {
	REF_PSFREQ_CMD,
	REF_SETTIME_CMD,
	REF_SETDATE_CMD,
	REF_GETTIME_CMD,
	REF_GETDATE_CMD,
	REF_AUTODLOADER_CMD,
	REF_SLEEP_CMD,
	REF_WAKE_CMD,
	REF_RESET_CMD,
	REF_IQ_CMD,
	REF_CPTIME_CMD,
	REF_CMD_MAX
};

struct ref_date {
	uint8_t mday;
	uint8_t mon;
	uint16_t year;
	uint8_t wday;
};

struct ref_time {
	uint8_t sec;
	uint8_t min;
	uint8_t hour;
};

struct iq_info {
	uint32_t base;
	uint32_t length;
};

struct refnotify_cmd {
	int cmd_type;
	uint32_t length;
};

struct ref_lcdfreq {
	uint32_t clk;
	uint32_t divisor;
	uint32_t type;
};

struct time_sync
{
	struct timeval tv;
	unsigned int sys_cnt;
	long uptime;
	char *path;
	int svr_socket;
};

struct modem_timestamp
{
	uint32_t magic_number;       /* magic number for verify the structure */
	uint32_t tv_sec;             /* clock time, seconds since 1970.01.01 */
	uint32_t tv_usec;            /* clock time, microeseconds part */
	uint32_t sys_cnt;            /* modem's time */
};

static struct time_sync time_info;

static char *g_fifopath = NULL;

static pthread_t g_timetid = -1;

static void usage(void)
{
	fprintf(stderr,
	"\n"
	"Usage: refnotify [-t type] [-h]\n"
	"receive and do the notify from modem side \n"
	"  -t type     td type:0, wcdma type: other \n"
	"  -h           : show this help message\n\n");
}

static void time_sync_init(struct time_sync* sync)
{
	sync->svr_socket = -1;
}

static int set_nonblock(int fd)
{
	long flags = fcntl(fd, F_GETFL);
	flags |= O_NONBLOCK;
	return fcntl(fd, F_SETFL, flags);
}

static int refnotify_enable_busmonitor(int bEnable)
{
	int fd;
	int param;
	int cmd;

	fd = open(BM_DEV, O_RDWR);
	if (fd < 0) {
		REF_LOGE("refnotify_enable_busmonitor %s failed, error: %s", BM_DEV, strerror(errno));
		return -1;
	}

	cmd = bEnable ? BM_DBG_INT_SET : BM_DBG_INT_CLR;
	ioctl(fd, cmd, &param);

	REF_LOGD("refnotify_enable_busmonitor bEnable = %d, cmd = %d", bEnable, cmd);

	cmd = bEnable ? BM_PANIC_SET : BM_PANIC_UNSET;
        ioctl(fd, cmd, &bEnable);

	close(fd);

	return 0;
}

void RefNotify_DoAutodloader(struct refnotify_cmd *pcmd)
{
	sync();
	property_set("sys.powerctl", "reboot,autodloader");
	//__reboot(LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2,
						//LINUX_REBOOT_CMD_RESTART2, "autodloader");
}

void RefNotify_DoReset(struct refnotify_cmd *pcmd)
{
	sync();
	property_set("sys.powerctl", "reboot,normal");
	//__reboot(LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2,
						//LINUX_REBOOT_CMD_RESTART2, "normal");
}

int RefNotify_rtc_readtm(struct tm *ptm)
{
	int fd;
	fd = open(RTC_DEV, O_RDWR);
	if(fd < 0) {
		REF_LOGE(" %s failed, error: %s", __func__, strerror(errno));
		return -1;
	}
	memset(ptm, 0, sizeof(*ptm));
	ioctl(fd, RTC_RD_TIME, ptm);
	close(fd);
	return 0;
}

int RefNotify_rtc_writetm(struct tm *ptm)
{
	int fd;
	fd = open(RTC_DEV, O_RDWR);
	if(fd < 0) {
		REF_LOGE(" %s failed, error: %s", __func__, strerror(errno));
		return -1;
	}
	ioctl(fd, RTC_SET_TIME, ptm);
	close(fd);
	return 0;
}

void RefNotify_DoGetTime(int fd, struct refnotify_cmd *cmd)
{
	int ret, length = sizeof(struct refnotify_cmd) + sizeof(struct ref_time);
	struct tm timenow;
	struct ref_time *ptime = NULL;
	struct refnotify_cmd *pcmd = NULL;
	if(RefNotify_rtc_readtm(&timenow) < 0) {
		REF_LOGE("error to get rtc time \n");
		return;
	}
	pcmd = (struct refnotify_cmd*)malloc(length);
	if(pcmd == NULL)
		return ;
	pcmd->cmd_type = REF_GETTIME_CMD;
	pcmd->length = length;
	ptime = (struct ref_time*)(pcmd+1);
	ptime->sec = timenow.tm_sec;
	ptime->min = timenow.tm_min;
	ptime->hour = timenow.tm_hour;
	ret = write(fd, pcmd, length);
	if(ret != length) {
		REF_LOGE("RefNotify write %d return %d, errno = %s", length , ret, strerror(errno));
	}
	free(pcmd);
}

void RefNotify_DoGetDate(int fd, struct refnotify_cmd *cmd)
{
	int ret, length = sizeof(struct refnotify_cmd) + sizeof(struct ref_date);
	struct tm timenow;
	struct ref_date *pDate = NULL;
	struct refnotify_cmd *pcmd = NULL;

	if(RefNotify_rtc_readtm(&timenow) < 0) {
		REF_LOGE("error to get rtc time \n");
		return;
	}
	pcmd = (struct refnotify_cmd*)malloc(length);
	if(pcmd == NULL)
		return;

	pcmd->cmd_type = REF_GETDATE_CMD;
	pcmd->length = length;
	pDate = (struct ref_date*)(pcmd+1);
	pDate->mday= timenow.tm_mday;
	pDate->mon = timenow.tm_mon + 1;
	pDate->year = timenow.tm_year + 1900;
	pDate->wday = timenow.tm_wday;
	ret = write(fd, pcmd, length);
	if(ret != length) {
		REF_LOGE("RefNotify write %d return %d, errno = %s", length , ret, strerror(errno));
	}
	free(pcmd);
}


static int RefNotify_DoSetTime(struct refnotify_cmd *pcmd)
{
	int ret;
	time_t timer;
	struct timeval tv;
	struct tm timenow;
	struct ref_time *ptime = NULL;

	if(RefNotify_rtc_readtm(&timenow) < 0) {
		REF_LOGE("error to get rtc time \n");
		return -1;
	}
	ptime = (struct ref_time*)(pcmd+1);
	REF_LOGD("refnotify set time %d, %d, %d \n", ptime->sec, ptime->min, ptime->hour);
	timenow.tm_sec = ptime->sec;
	timenow.tm_min = ptime->min;
	timenow.tm_hour = ptime->hour;
	if(RefNotify_rtc_writetm(&timenow) < 0) {
		REF_LOGE("error to set rtc time \n");
		return -1;
	}
	timer = mktime(&timenow);
	if (timer < 0) {
		REF_LOGE("mktime failed  \n");
		return -1;
	}
	tv.tv_sec = timer;
	tv.tv_usec = 0;
	if(settimeofday(&tv, (struct timezone*)0) < 0){
		REF_LOGE("Set timer error \n");
		return -1;
	}
	return 0;
}

static int RefNotify_DoSetDate(struct refnotify_cmd *pcmd)
{
	int ret;
	time_t timer;
	struct timeval tv;
	struct tm timenow;
	struct ref_date *pdate = NULL;

	if(RefNotify_rtc_readtm(&timenow) < 0) {
		REF_LOGE("error to get rtc time \n");
		return -1;
	}

	pdate = (struct ref_date*)(pcmd+1);

	timenow.tm_mday= pdate->mday;
	timenow.tm_mon = pdate->mon - 1;
	timenow.tm_year = pdate->year - 1900;
	timenow.tm_wday = pdate->wday;
	if(RefNotify_rtc_writetm(&timenow) < 0) {
		REF_LOGE("error to set rtc time \n");
		return -1;
	}
	timer = mktime(&timenow);
	if (timer < 0) {
		REF_LOGE("mktime failed  \n");
		return -1;
	}
	tv.tv_sec = timer;
	tv.tv_usec = 0;
	if(settimeofday(&tv, (struct timezone*)0) < 0){
		REF_LOGE("Set timer error \n");
		return -1;
	}
	return 0;
}

static void RefNotify_DoGetIqInfo(int fd, struct refnotify_cmd *cmd)
{
	int iq_fd;
	int ret, length = sizeof(struct refnotify_cmd) + sizeof(struct iq_info);
	uint32_t base = 0xffffffff;
	char cbase[IQ_BASE_LENGTH + 1] = {0};
	struct refnotify_cmd *pcmd = NULL;
	struct iq_info *piq = NULL;
	iq_fd = open(IQ_DEV, O_RDONLY);
	if(iq_fd < 0) {
		REF_LOGE(" %s failed, error: %s", __func__, strerror(errno));
		return;
	}
	ret = read(iq_fd, cbase, IQ_BASE_LENGTH);
	REF_LOGD("iq_base %s \n", cbase);
	close(iq_fd);
	REF_LOGD("close \n");
	base = strtoul(cbase, (char**)NULL, 10);
	REF_LOGD("iq_base %d \n", base);

	if(ret < 0)
		return;
	pcmd = (struct refnotify_cmd*)malloc(length);
	if(pcmd == NULL)
		return;

	pcmd->cmd_type = REF_IQ_CMD;
	pcmd->length = length;
	piq = (struct iq_info*)(pcmd+1);
	piq->base = base;
	piq->length = 128*1024*1024;
	if(refnotify_enable_busmonitor(0)) {
		REF_LOGE("RefNotify disable busmonitor failed");
	}
	ret = write(fd, pcmd, length);
	if(ret != length) {
		REF_LOGE("RefNotify write %d return %d, errno = %s", length , ret, strerror(errno));
	}
	free(pcmd);
}

static void* timesync_task(void *arg)
{
	struct time_sync *ptime = (struct time_sync*)arg;
	struct pollfd pol_fd;

	REF_LOGD("Enter timesync_task\n");

	pol_fd.fd = ptime->svr_socket;
	pol_fd.events = POLLIN;
	pol_fd.revents = 0;
	while (1) {
		int cli_sock;
		struct timeval time_now;
		struct modem_timestamp time_stamp;
		struct sysinfo sinfo;
		int num;
		num = poll(&pol_fd, 1, -1);
		if (num <= 0 || !(pol_fd.revents & POLLIN)) {
			continue;
		}
		cli_sock = accept(ptime->svr_socket, 0, 0);
		if (cli_sock >= 0) {
			set_nonblock(cli_sock);

			gettimeofday(&time_now, 0);
			sysinfo(&sinfo);
			time_stamp.sys_cnt = ptime->sys_cnt + (sinfo.uptime - ptime->uptime)*1000;
			time_stamp.tv_sec = (uint32_t)(time_now.tv_sec);
			time_stamp.tv_usec = (uint32_t)(time_now.tv_usec);

			write(cli_sock, &time_stamp.tv_sec, sizeof(uint32_t) * 3);
			close(cli_sock);
		} else if (cli_sock < 0) {
			REF_LOGE("accept error %d", errno);
		}
	}
	return NULL;
}

static void RefNotify_DoTimesync(char * path, struct refnotify_cmd *pcmd)
{
	struct timezone tz;
	struct sysinfo info;
	int ret = 0;
	if(sysinfo(&info)) {
		REF_LOGE("get sysinfo failed \n");
	}

	if(NULL == path)
	{
		REF_LOGE("RefNotify_DoTimesync path is null! \n");
		return;
	}
	gettimeofday(&(time_info.tv), &tz);
	time_info.sys_cnt = *(unsigned int*)(pcmd+1);
	time_info.uptime = info.uptime;
	time_info.path = path;
	REF_LOGD("AP up time %lu == CP sys_cnt %u",
		 time_info.tv.tv_sec, time_info.sys_cnt);

	// Initialize time server socket
	if (time_info.svr_socket < 0) {
		int svr = socket_local_server(TIME_SERVER_SOCK_NAME,
					      ANDROID_SOCKET_NAMESPACE_ABSTRACT,
					      SOCK_STREAM);
		if (-1 == svr) {
			REF_LOGE("create time server socket error: %d", errno);
			return;
		}
		ret = set_nonblock(svr);
		if (ret < 0) {
			REF_LOGD("set_nonblock failed \n");
			return;
		}
		time_info.svr_socket = svr;
	}

	if(g_timetid == -1) {
		REF_LOGD("create timesync_task \n");
		pthread_create(&g_timetid, NULL,timesync_task, (void *)&time_info);
	}
}

static void RefNotify_DoFreqCmd(struct refnotify_cmd *pcmd)
{
	struct ref_lcdfreq *p_freq = (struct ref_lcdfreq *)(pcmd + 1);
	int fd, wr;
	char mipi_freq_str[12] = {"0"};//default value 0, if suspended
	char rgb_freq_str[32] = {"0,0"};//default value 0, if suspended

	REF_LOGD("%s: clk = %d, divisor = %d, type = %d\n",__func__, p_freq->clk, p_freq->divisor, p_freq->type);

#if 0
	char freq_fb_dev[64];
	if(property_get("ro.refnotify.freqdev",freq_fb_dev,NULL)>0){
		REF_LOGE("ro.refnotify.freqdev: %s", freq_fb_dev);
	}else{
		REF_LOGE("get ro.refnotify.freqdev failed");
		return;
	}
#endif

	if(REF_MIPI_LCD == p_freq->type)
	{
		fd = open(FREQ_MIPI_FB_DEV, O_RDWR, 0);
		if(fd < 0) {
			REF_LOGE("open %s failed, error: %s", FREQ_MIPI_FB_DEV, strerror(errno));
			return;
		}
		if(p_freq->clk){
			sprintf(mipi_freq_str, "%d", p_freq->clk);
		}
		REF_LOGD("%s:MIPI LCD, mipi_freq_str is %s", __func__, mipi_freq_str);
		wr = write(fd, mipi_freq_str, strlen(mipi_freq_str));
		if(wr < 0) {
			REF_LOGE("write %s failed, error: %s", FREQ_MIPI_FB_DEV, strerror(errno));
		}
		close(fd);
	}
	else if(REF_RGB_LCD == p_freq->type)
	{
		fd = open(FREQ_RGB_FB_DEV, O_RDWR, 0);
		if(fd < 0) {
			REF_LOGE("open %s failed, error: %s", FREQ_RGB_FB_DEV, strerror(errno));
			return;
		}
		if(p_freq->clk){
			sprintf(rgb_freq_str, "%d,%d", p_freq->clk, p_freq->divisor);
		}
		REF_LOGD("%s:RGB LCD, rgb_freq_str is %s", __func__, rgb_freq_str);
		wr = write(fd, rgb_freq_str, strlen(rgb_freq_str));
		if(wr < 0) {
			REF_LOGE("write %s failed, error: %s", FREQ_RGB_FB_DEV, strerror(errno));
		}
		close(fd);
	}
}

void RefNotify_DoCmd(int fd, struct refnotify_cmd *pcmd)
{
	REF_LOGD("%s, %d \n", __func__, pcmd->cmd_type);
	switch(pcmd->cmd_type) {
		case REF_PSFREQ_CMD:
			RefNotify_DoFreqCmd(pcmd);
			break;
		case REF_SETTIME_CMD:
			(void)RefNotify_DoSetTime(pcmd);
			break;
		case REF_SETDATE_CMD:
			(void)RefNotify_DoSetDate(pcmd);
			break;
		case REF_GETTIME_CMD:
			RefNotify_DoGetTime(fd, pcmd);
			break;
		case REF_GETDATE_CMD:
			RefNotify_DoGetDate(fd, pcmd);
			break;
		case REF_AUTODLOADER_CMD:
			RefNotify_DoAutodloader(pcmd);
			break;
		case REF_RESET_CMD:
			RefNotify_DoReset(pcmd);
			break;
		case REF_IQ_CMD:
			RefNotify_DoGetIqInfo(fd, pcmd);
			break;
		case REF_CPTIME_CMD:
			RefNotify_DoTimesync(g_fifopath, pcmd);
			break;
		default:
			break;
	}
}

/*a daemon to notify cp of sleeping to prevent cp sending request*/
void* sleep_monitor(void *arg)
{
	int fdw, fds, fd, r;
	struct refnotify_cmd cmd;
	char buf[1];
	fd = *((int*)arg);
	cmd.length = sizeof(struct refnotify_cmd);

	while(1){
		fds = open(SleepFileName, O_RDONLY);
		if (fds < 0) {
			REF_LOGE("Couldn't open file" SleepFileName);
		}else{
			/*we are awake, waiting for sleep*/
			r = read(fds, buf, 1);
			if(r < 0){
				REF_LOGE("wait_for_fb_sleep read error: %s", strerror(errno));
				close(fds);
				continue;
			}
			/*here, we are in sleep state*/
			cmd.cmd_type = REF_SLEEP_CMD;
			write(fd, &cmd, cmd.length);
			close(fds);
		}
		fdw = open(WakeFileName, O_RDONLY);
		if (fdw < 0) {
			REF_LOGE("Couldn't open file" WakeFileName);
		}else{
			/*we are sleeping, waiting for wake*/
			r = read(fdw, buf, 1);
			if(r < 0){
				REF_LOGE("wait_for_fb_wake read error: %s", strerror(errno));
				close(fdw);
				continue;
			}
			/*here, we are awake*/
			cmd.cmd_type = REF_WAKE_CMD;
			write(fd, &cmd, cmd.length);
			close(fdw);
		}
	}

	return NULL;
}

int main(int argc, char *argv[])
{
	char buf[128];
	char *pbuf = NULL;
	int opt;
	int type;
	int flag = 0 , fd, num;
	uint32_t numRead;
	char path[PROPERTY_VALUE_MAX+2] = {0};
	const char *pathnumber = "18";
	struct refnotify_cmd *pcmd = NULL;
	pthread_t tid;
	int mk_ret = 0;
	int cnt = 15;

	REF_LOGD("Enter RefNotify main \n");
	time_sync_init(&time_info);

	if (argc == 1 || (strcmp(argv[1], "-t") && strcmp(argv[1], "-h"))) {
		usage();
		exit(EXIT_FAILURE);
	}
	while ( -1 != (opt = getopt(argc, argv, "t:h"))) {
		switch (opt) {
			case 't':
				type = atoi(optarg);
				if (type == 0){
					flag = 0;
				} else if(type == 1){
					flag = 1;
				} else{
					flag = 2;
				}
				break;
			case 'h':
				usage();
				exit(0);
			default:
				usage();
				exit(EXIT_FAILURE);
		}
	}
	REF_LOGD("Enter RefNotify main flag %d \n", flag);
	if(flag == 0) {
		property_get("ro.modem.t.tty",path,"not_find");
		if(0 == strcmp(path, "/dev/spimux")){
			strcat(path, pathnumber);
		} else{
			strcpy(path, TD_NOTIFY_DEV);
		}
		REF_LOGD("get TD_NOTIFY_DEV property path is: %s \n", path);
		if( -1 == access(TD_FIFO_PATH, F_OK)){
			mk_ret = mkfifo(TD_FIFO_PATH, 0666);
			if(mk_ret == 0) {
				REF_LOGE("create tdfifo success");
			} else {
				REF_LOGE("create tdfifo failed, error: %s",strerror(errno));
				exit(EXIT_FAILURE);
			}
		}
		g_fifopath = TD_FIFO_PATH;
	} else if(flag == 1){
		strcpy(path, W_NOTIFY_DEV);
		REF_LOGD("get W_NOTIFY_DEV property path is: %s \n", path);
		if( -1 == access(W_FIFO_PATH, F_OK)){
			mk_ret = mkfifo(W_FIFO_PATH, 0666);
			if(mk_ret == 0) {
				REF_LOGE("create wfifo success");
			} else {
				REF_LOGE("create wfifo failed, error: %s",strerror(errno));
				exit(EXIT_FAILURE);
			}
		}
		g_fifopath = W_FIFO_PATH;
	} else if(flag == 2){
		property_get("ro.modem.l.tty",path,"not_find");
		if(0 == strcmp(path, "/dev/sdiomux")){
			strcat(path, pathnumber);
		} else{
			strcpy(path, LTE_NOTIFY_DEV);
		}
		REF_LOGD("get L_NOTIFY_DEV property path is: %s \n", path);
		if( -1 == access(L_FIFO_PATH, F_OK)){
			mk_ret = mkfifo(L_FIFO_PATH, 0666);
			if(mk_ret == 0) {
				REF_LOGE("create lfifo success");
			} else {
				REF_LOGE("create lfifo failed, error: %s",strerror(errno));
				exit(EXIT_FAILURE);
			}
		}
		g_fifopath = L_FIFO_PATH;
	}

	// Ignore SIGPIPE signal so that writing a broken pipe won't incur a SIGPIPE signal.
	struct sigaction action;

	memset(&action, 0, sizeof action);
	action.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &action, 0);

	do {
		fd = open(path, O_RDWR);
		if (fd < 0) {
			REF_LOGE("RefNotify open %s failed, error: %s", path, strerror(errno));
			cnt--;
			if(cnt > 0)
				sleep(1);
			else
				exit(EXIT_FAILURE);
		}
	} while(fd < 0);
	REF_LOGE("RefNotify open %s success", path);
	/*start a service to notify cp of sleep/wake state*/
	pthread_create(&tid, NULL,sleep_monitor, (void *)&fd);

	//RefNotify_DoGetIqInfo(fd, NULL);

	for (;;) {
		pbuf = buf;
		numRead = 0;
		REF_LOGD("RefNotify %s: wait for modem notify event ...", __func__);
		memset(buf, 0, sizeof(buf));
readheader:
		num = read(fd, pbuf + numRead, sizeof(struct refnotify_cmd));
		if(num < 0 && errno == EINTR) {
			REF_LOGE("RefNotify read %d return %d, errno = %s", fd , numRead, strerror(errno));
			sleep(1);
			goto readheader;
		} else if(num < 0) {
			REF_LOGE("RefNotify read %d return %d, errno = %s", fd , numRead, strerror(errno));
			exit(EXIT_FAILURE);
		}
		numRead += num;
		if (numRead < sizeof(struct refnotify_cmd)) {
			goto readheader;
		}
		pcmd = (struct refnotify_cmd*)buf;
		if(numRead < pcmd->length) {
readcontent:
			num = read(fd, pbuf + numRead, pcmd->length - numRead);
			if(num < 0 && errno == EINTR) {
				REF_LOGE("RefNotify read %d return %d, errno = %s", fd , numRead, strerror(errno));
				sleep(1);
				goto readcontent;
			} else if(num < 0) {
				REF_LOGE("RefNotify read %d return %d, errno = %s", fd , numRead, strerror(errno));
				exit(EXIT_FAILURE);
			}
			numRead += num;
			if (numRead < pcmd->length) {
				goto readcontent;
			}
		}
		REF_LOGD("RefNotify ready to do cmd \n");
		RefNotify_DoCmd(fd, (struct refnotify_cmd*)buf);
	}
	return 0;
}
