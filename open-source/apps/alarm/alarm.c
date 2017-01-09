#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <linux/input.h>
#include <linux/reboot.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include "boot_alarm.h"
#include "battery.h"
#include "common.h"
#include <errno.h>
#include <linux/android_alarm.h>

#include <sys/vfs.h>
#include <dirent.h>
#include <sys/mount.h>

extern int check_input_dev(const char * cmstr);
void *refreshen_screen(void *data);
void free_alarm_surface(int image_pos);
void *get_input_event(void*data);
void parse_alarm_event(struct input_event *ev);
int add_alarm_db_list(void);
int alarm_init(void);
void boot_alarm_exit(void);
int parse_db_item(char *db_buf);
int get_value(const char *file);
int gr_init(void);
void init_surface(int image_pos);
void print_images(int image_pos);
void gr_exit(void);
void gr_flip(void);
void show_boot_logo(int pos);
int set_next_alarm_time(void);
int ev_init(void);
int ev_get(struct input_event *ev, int dont_wait);

static pthread_t alarm_t_ShowUi = -1;
static int alarm_BootSystem;
unsigned boot_alarm = 0;
int time_dm;
int mount_flag = -1;

struct boot_status gs_boot_state = {0};
struct alarm_db *g_alarm_db_list = NULL;
struct alarm_sec *g_alarm_sec_list = NULL;
struct alarm_sec *alarm_cur = NULL;
int g_alarm_button_event = BOOT_STATE_NONE;
char * g_alarm_ring_path = BOOT_ALARM_DEFAULT_RING;
int g_alarm_last_time = 0;
int g_alarm_snooze_time = 0;
extern char g_brightness[20];

void system_shutdown(void)
{
	gs_boot_state.alarm_state = BOOT_ALARM_EXIT;
	set_next_alarm_time();
	sync();
	reboot(LINUX_REBOOT_CMD_POWER_OFF);
}

static int alarm_mount_externalsd(void)
{
        int ret = -1;
        DIR *dir;
        struct dirent *ptr;
        char sd[21] = "/dev/block/";
        char *device_emmc = "mmcblk1p1";
	char *device_nand = "mmcblk0p1";
	char *device =NULL;
	int flag = 0;
	char value[10] = {0};
        dir = opendir("/dev/block");
	property_get("ro.storage.flash_type", value, "10");
	flag = atoi(value);

	if( flag == 1){
		while( (ptr=readdir(dir)) != NULL ){
			if(strcmp(ptr->d_name, device_nand) == 0){
				ret = 0;
				device = device_nand;
				LOGE("sdcard:%s\n", device);
				break;
			}
		}
        }else if( flag == 2){
		while( (ptr=readdir(dir)) != NULL ){
			if(strcmp(ptr->d_name, device_emmc) == 0){
				ret = 0;
				device = device_emmc;
				LOGE("sdcard:%s\n", device);
				break;
			}
		}
	}else{
		while( (ptr=readdir(dir)) != NULL ){
			if(strcmp(ptr->d_name, device_emmc) == 0){
				ret = 0;
				device = device_emmc;
				flag = 2;
				LOGE("sdcard:%s\n", device);
				break;
			}else if(strncmp(ptr->d_name, "mmcblk0p",8) == 0){
				if(strcmp(ptr->d_name, device_nand) > 0){
					flag = 1;
				}else if(flag != 1){
					flag =3;
				}
			}
		}
		if(flag == 2)
			device = device_emmc;
		else if(flag == 3){
			device = device_nand;
			LOGE("sdcard:%s\n", device);
			ret = 0;
		}
        }
	closedir(dir);
	if(ret == -1){
		LOGE("no external sdcard\n");
		return ret;
	}
	strcat(sd, device);

        ret = mount(sd, "/storage/sdcard0", "vfat", MS_NOATIME | MS_NODEV | MS_NODIRATIME | MS_NOSUID , "utf8=true");

        if(ret == 0){
                LOGE("alarm_mount sdmount %s done\n", sd);
                return ret;
        }
        LOGE("Can't mount %s\n(%s)\n", sd, strerror(errno));
        return -1;
}

static void alarm_umountsd(void)
{
	int ret;
	int n = 50;
	if(mount_flag < 0)
		return;
	while(n){
		ret= umount("/storage/sdcard0");
		if(ret == 0){
			LOGE("alarm_umountsd done\n");
			return;
		}
		LOGE("Can't umount(%s)\n",strerror(errno));
		usleep(500000);
		n--;
	}
}
//add list g_alarm_db_list form alarms.db
int add_alarm_db_list(void)
{
	FILE *fp;
	char db_item[1024]= {0,};
	char cmd[1024]= {0};

	sprintf(cmd,"/system/xbin/sqlite3 %s \" select * from alarm_templates\"",BOOT_ALARM_DB_FILE);
	fp = popen(cmd,"r");
	if(fp == NULL )
	{
		ERROR("popen db");
		return -1;
	}

	char *res = fgets(db_item,1024,fp);
	while(res != NULL)
	{
		parse_db_item(db_item);
		memset(db_item,0,1024);
		res = fgets(db_item,1024,fp);
	}

	pclose(fp);
	return 0;
}

static char read_buf[300] = {0};
static char read_buf1[300] = {0};
char *get_ring_file(void)
{
	int fd = -1;
	char *alarm_name = "/productinfo/alarm_flag";
	int alarm_flag_fd = -1;
	int ret;
	unsigned long ring_length = 0;
	unsigned long snooze_length = 0;
	struct stat s;
	char file_path1[100] = {0};

	alarm_flag_fd = open(alarm_name, O_RDWR);
	if(alarm_flag_fd < 0){
		LOGE("%s open error: %s\n", alarm_name, strerror(errno));
		return NULL;
	}
	ret = read(alarm_flag_fd, read_buf, sizeof(read_buf));
	if(ret < 0){
		close(alarm_flag_fd);
		LOGD("read %s failed\n", alarm_name);
		return 0;
	}else{
		LOGD("%s get: %s\n", alarm_name, read_buf);
	}
	char *rel_time = strtok(read_buf, " \t\n");
	char *abs_time = strtok(NULL, " \t\n");
	char *ring_time = strtok(NULL, " \t\n");
	char *snooze_time = strtok(NULL, " \t\n");
	char *time_mdm = strtok(NULL, " \t\n");
	char *file_path = strtok(NULL, "\t\n");
	if(ring_time != NULL)
		ring_length = strtoul(ring_time, (char **)NULL, 10);
	else
		ring_length = 1;

	if(ring_length == 0)
		ring_length = 1;

	if (file_path != NULL && stat(file_path, &s) != 0) {
		if(!strncmp(file_path,"/storage/emulated/",18)){
			char *temp_sd =strstr(file_path,"temp_sd");
			if(temp_sd){
				sprintf(file_path1,"/data/media/%s",temp_sd);
			}else
				sprintf(file_path1,"/data/media/%s",file_path+18);
			file_path = file_path1;
		}
		if(file_path != NULL && stat(file_path, &s) != 0){
			LOGE("%s cannot find '%s'\n", file_path,strerror(errno));
			file_path = NULL;
		}
	}

	if(snooze_time != NULL)
		snooze_length = strtoul(snooze_time, (char **)NULL, 10);
	else
		snooze_length = 1;

	if(snooze_length == 0)
		snooze_length = 10;

	g_alarm_ring_path = file_path;
	g_alarm_last_time = ring_length * 60;
	g_alarm_snooze_time = snooze_length * 60;
	LOGD("g_alarm_ring_path: %s \ng_alarm_last_time:%d\ng_alarm_snooze_time:%d\n", \
			g_alarm_ring_path, g_alarm_last_time, g_alarm_snooze_time);
	close(alarm_flag_fd);
	return file_path;
}
int get_poweron_file(void)
{

	char *poweron_name = "/productinfo/poweron_timeinmillis";
	int poweron_flag_fd = -1;
	int ret;
	int reltime_power = 0;
	int abstime_power = 0;

	poweron_flag_fd = open(poweron_name, O_RDWR);
	if(poweron_flag_fd < 0){
		LOGE("%s open error: %s\n", poweron_name, strerror(errno));
		return 0;
	}
	ret = read(poweron_flag_fd, read_buf1, sizeof(read_buf1));
	if(ret < 0){
		close(poweron_flag_fd);
		LOGD("read %s failed\n", poweron_name);
		return 0;
	}else{
		LOGD("%s get: %s\n", poweron_name, read_buf1);
	}
	char *rel_time = strtok(read_buf1, " \t\n");
	char *abs_time = strtok(NULL, " \t\n");
	if(rel_time != NULL)
		reltime_power = strtoul(rel_time, NULL, 10);
	if(abs_time != NULL)
		abstime_power = strtoul(abs_time, NULL, 10);
	close(poweron_flag_fd);

	LOGD("abstime_power=%d\n", abstime_power);
	return abstime_power;
}
void  get_time_dm(void){
	char *alarm_name = "/productinfo/alarm_flag";
	int alarm_flag_fd = -1;
	int ret;
	alarm_flag_fd = open(alarm_name, O_RDWR);
	if(alarm_flag_fd < 0){
		LOGE("%s open error: %s\n", alarm_name, strerror(errno));
		return 0;
	}

	ret = read(alarm_flag_fd, read_buf, sizeof(read_buf));
	if(ret < 0){
		close(alarm_flag_fd);
		LOGD("read %s failed\n", alarm_name);
		return 0;
	}else{
		LOGD("%s get: %s\n", alarm_name, read_buf);
	}
	char *rel_time = strtok(read_buf, " \t\n");
	char *abs_time = strtok(NULL, " \t\n");
	char *ring_time = strtok(NULL, " \t\n");
	char *snooze_time = strtok(NULL, " \t\n");
	char *time_mdm = strtok(NULL, " \t\n");
	char *file_path = strtok(NULL, " \t\n");
	if(time_mdm != NULL){
		time_dm = atoi(time_mdm);
	}
	close(alarm_flag_fd);
}
int update_ring_file(void)
{

	int fd = -1;
	char *alarm_name = "/productinfo/alarm_flag";
	int alarm_flag_fd = -1;
	int ret;
	unsigned long timenow = 0;
	unsigned long reltime = 0;
	unsigned long abstime = 0;
	unsigned long abstime_aralm = 0;
	unsigned long abstime_power = 0;
	struct timespec ts;
	struct alarm_sec *fire_alarm;
	char buf[512] = {0,};

	alarm_flag_fd = open(alarm_name, O_RDWR);
	if(alarm_flag_fd < 0){
		LOGE("%s open error: %s\n", alarm_name, strerror(errno));
		return 0;
	}

	ret = read(alarm_flag_fd, read_buf, sizeof(read_buf));
	if(ret < 0){
		close(alarm_flag_fd);
		LOGD("read %s failed\n", alarm_name);
		return 0;
	}else{
		LOGD("%s get: %s\n", alarm_name, read_buf);
	}
	char *rel_time = strtok(read_buf, " \t\n");
	char *abs_time = strtok(NULL, " \t\n");
	char *ring_time = strtok(NULL, " \t\n");
	char *snooze_time = strtok(NULL, " \t\n");
	char *time_mdm = strtok(NULL, " \t\n");
	char *file_path = strtok(NULL, " \t\n");
	if(rel_time != NULL)
		reltime = strtoul(rel_time, NULL, 10);
	if(abs_time != NULL)
		abstime_aralm = strtoul(abs_time, NULL, 10);
	close(alarm_flag_fd);
	timenow = time(NULL);
	int latest_time = get_latest_alarm_time(&fire_alarm);
	abstime_power = get_poweron_file();
	if(latest_time >0){
		abstime =timenow + latest_time;
		reltime=abstime -abstime_aralm +reltime;
		if(abstime < abstime_power || abstime_power <= timenow){
			LOGD("timenow=%d,abstime=%d,reltime=%d\n", timenow, abstime,reltime);
			sprintf(buf,"%d\n%d\n%s\n%s\n%s\n%s\n", reltime, abstime,ring_time,snooze_time,time_mdm,file_path);
			alarm_flag_fd = open(alarm_name, O_CREAT | O_RDWR,0664);
			if(alarm_flag_fd < 0){
				LOGE("%s open error: %s\n", alarm_name, strerror(errno));
				return 0;
			}
			if(write(alarm_flag_fd,buf, 512) < 0)
			{
				close(alarm_flag_fd);
				return 0;
			}
			close(alarm_flag_fd);
			LOGD("write alarm_flag success, str=%s\n",buf);
		}
	}
	if(abstime_power > timenow && (abstime_power < abstime || abstime < timenow))
		ts.tv_sec = abstime_power;
	else if(abstime > timenow)
		ts.tv_sec = abstime;
	else
		return 0;
	ts.tv_nsec = 0;
	fd =open("/dev/alarm", O_RDWR);
	if(fd<0){
		LOGE("%s:  can't open file /dev/alarm.\n",__func__);
		return 0;
	}
	int result = ioctl(fd, ANDROID_ALARM_SET(4), &ts);
	if (result < 0)
	{
		close(fd);
		LOGE("Unable to set alarm to %lld: %s\n", abstime, strerror(errno));
		return 0;
	}
	//Can not close the fd, prevent rtc is cleared
        //close(fd);
	return 1;

}
char *add_ring_file_time(void)
{
	char *alarm_name = "/productinfo/alarm_flag";
	int alarm_flag_fd = -1;
	int ret;
	int day = 0;
	int secs = 0;
	time_t abs_secs;
	struct alarm_sec *sec_item = NULL;
	struct alarm_sec *p_sec = g_alarm_sec_list;
	struct tm *tm = NULL;

	alarm_flag_fd = open(alarm_name, O_RDWR);
	if(alarm_flag_fd < 0){
		LOGE("%s open error: %s\n", alarm_name, strerror(errno));
		return NULL;
	}
	ret = read(alarm_flag_fd, read_buf, sizeof(read_buf));
	if(ret < 0){
		close(alarm_flag_fd);
		LOGD("read %s failed\n", alarm_name);
		return 0;
	}else{
		LOGD("%s get: %s\n", alarm_name, read_buf);
	}
	char *rel_time = strtok(read_buf, " \t\n");
	char *abs_time = strtok(NULL, " \t\n");
	close(alarm_flag_fd);
	if(abs_time != NULL)
		abs_secs = strtoul(abs_time, NULL, 10);
	tm = localtime(&abs_secs);
	day = (tm->tm_wday == 0)?6:(tm->tm_wday - 1);
	secs = day*24*3600 + tm->tm_hour*3600 + tm->tm_min*60 + tm->tm_sec;
	while(p_sec != NULL)
	{
		if(p_sec->secs == secs)
		{
			return NULL;
		}
		p_sec = p_sec->next;
	}
	sec_item = (struct alarm_sec *)malloc(sizeof(struct alarm_sec));
	if(sec_item == NULL)
	{
		goto end;
	}

	sec_item->day = day;
	sec_item->secs = secs;
	sec_item->db_id = 12;
	sec_item->vibrate= 1;
	sec_item->alert ="0";
	sec_item->next = NULL;

end:
	return sec_item;
}

int alarm_init(void)
{

	gr_init();
	gr_clean();
	get_time_dm();
	add_alarm_db_list();
	if(creat_alarm_sec_list() == NULL)
	{
		gs_boot_state.alarm_init = 0;
		return ERR_BOOT_ALARM_NOT_ALARM_ITEM;
	}
	mount_flag = alarm_mount_externalsd();
	get_ring_file();
	sprintf(g_brightness,"%d", 100);
	gs_boot_state.alarm_state = BOOT_ALARM_ALARMING;
	gs_boot_state.power_key_event = BOOT_STATE_NONE;

	init_surface(ALARM_WARNING_IMG_POS);
	init_surface(ALARM_TIME_NUM_POS);
	init_surface(ALARM_BOOT_LOGO_POS);

	gs_boot_state.alarm_init = 1;

	return 0;
}

void * pthread_vibrate(void *cookie)
{
	while((gs_boot_state.alarm_state != BOOT_ALARM_EXIT)&&(gs_boot_state.alarm_state != BOOT_ALARM_ANOTHER))
	{
		if(gs_boot_state.alarm_state == BOOT_ALARM_ALARMING)
		{
			alarm_Set_Vib("1000");
		}
		sleep(2);
	}
	return NULL;
}

//set next alarm time
int set_next_alarm_time(void)
{
	int next_secs = get_latest_alarm_time(NULL);
	if(next_secs < 0)
	{
		return -1;
	}
	alarm_Set_NewHardWareAlarm(next_secs - 20);
	return 0;
}

//if there hava a power key event,
//start system return 1, else if
//there isn't any event untill sleep timeout
//return 0;
int alarm_sleep(int secs)
{
	int i;
	for(i = 0; i < secs; i++)
	{
		//check the power key event while sleep
		if(gs_boot_state.power_key_event != BOOT_STATE_NONE)
		{
			return 1;
		}
		usleep(500000);
		usleep(500000);
	}
	return 0;
}
inline long long timeval_diff(struct timeval big, struct timeval small)
{
	return (long long)(big.tv_sec-small.tv_sec)*1000000 + big.tv_usec - small.tv_usec;
}

void system_poweron(void)
{
	gs_boot_state.battery_state = BATTERY_STATE_EXIT;
	gs_boot_state.alarm_state = BOOT_ALARM_EXIT;
	alarm_umountsd();
	boot_alarm_exit();
	sync();
	while(1){
		LOGD(" %s: %d, %s\n", __func__, __LINE__,"system will power on");
		__reboot(LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2,
				LINUX_REBOOT_CMD_RESTART2, "normal");
		LOGD(" %s: %d, %s\n", __func__, __LINE__,"reboot failed in alarm mode");
	}
}

int  main(int argc, char **argv)
{
	char alarm_prop[10] = {0};
	pthread_t t;
	pthread_t button_t;
	pthread_t vibrate_t;
	struct alarm_sec  *alarm_item_bak = NULL;
	int next_alarm_time = 0;
	struct timeval start_time={0};
	struct timeval now_time={0};
	long long time_diff_temp;
	struct alarm_sec *fire_alarm;
	int timer = 1;
	log_init();
	LOGD("alarm start\n");

	property_get("ro.bootmode", alarm_prop, "");
	LOGD("alarm_prop: %s\n", alarm_prop);
	if(!alarm_prop[0] || strncmp(alarm_prop, "alarm", 5)){
		LOGE("not power off alarm\n");
		return EXIT_SUCCESS;
	}
	sleep(5);

	if(alarm_init() == ERR_BOOT_ALARM_NOT_ALARM_ITEM)
	{
		gs_boot_state.alarm_state = BOOT_ALARM_EXIT;
		LOGE("alarm init failed\n");
		return ERR_BOOT_ALARM_NOT_ALARM_ITEM;
	}
	//wait untill  alarm time
LOGD("%s: line: %d &fire_alarm %p\n", __func__, __LINE__, &fire_alarm);
	while((next_alarm_time = get_latest_alarm_time(&fire_alarm)) != 0)
	{
		if((next_alarm_time < 0)||(next_alarm_time > 180))
		{
			gs_boot_state.alarm_state = BOOT_ALARM_EXIT;
			LOGE("alarm expire: %d\n", next_alarm_time);
			boot_alarm_exit();
			return 0;
		}
	}
	if(pthread_create(&t,NULL, refreshen_screen, NULL))
	{
		gs_boot_state.alarm_state = BOOT_ALARM_EXIT;
		LOGE("refreshen_screen thread creat fail\n");
		return -1;
	}
	if(pthread_create(&button_t,NULL,get_input_event , NULL))
	{
		gs_boot_state.alarm_state = BOOT_ALARM_EXIT;
		LOGE("get_input_event thread creat fail\n");
		return -1;
	}

	musicProcess(g_alarm_ring_path,1);
	while(gettimeofday(&start_time, (struct timezone *)0)<0);
	if((alarm_cur != NULL)&&(alarm_cur->vibrate == 1))
	{
		pthread_create(&vibrate_t,NULL,pthread_vibrate , NULL);
	}
	alarm_item_bak = alarm_cur;
	print_images(ALARM_WARNING_IMG_POS);
	set_state(LCD_BRIGHTNESS_ON);

	int abstime_power = get_poweron_file();
	gs_boot_state.power_key_event = BOOT_STATE_NONE;
	while(gs_boot_state.power_key_event == BOOT_STATE_NONE)
	{
		//left button
		if(g_alarm_button_event == ALARM_BUTTON_EVENT_LEFT)
		{

			//sleep
			if(gs_boot_state.alarm_state == BOOT_ALARM_ALARMING)
			{
				musicProcess(g_alarm_ring_path,0);
				update_delay_time(alarm_item_bak);
				set_state(LCD_BRIGHTNESS_OFF);
				system("echo 0 > /sys/class/leds/keyboard-backlight/brightness");
				gs_boot_state.alarm_state = BOOT_ALARM_SLEEP;
				update_alarm_sec_list(alarm_item_bak,g_alarm_snooze_time);
				int latest = get_latest_alarm_time(&fire_alarm);
				LOGD("latest=%d,g_alarm_snooze_time =%d\n",latest,g_alarm_snooze_time);
				if(latest > 0)
				{
					int powertime = abstime_power - time(NULL);
					if(powertime > 0 && powertime < latest)
					{
						if(alarm_sleep(powertime) != 0)
						{
							//start system
							break;
						}
						break;
					}
					if(alarm_sleep(latest) != 0)
					{
						//start system
						break;
					}
				}
				gr_clean();
				gs_boot_state.alarm_state = BOOT_ALARM_ALARMING;
				print_images(ALARM_WARNING_IMG_POS);
				set_state(LCD_BRIGHTNESS_ON);
				system("echo 1 > /sys/class/leds/keyboard-backlight/brightness");
				musicProcess(g_alarm_ring_path,1);
				while(gettimeofday(&start_time, (struct timezone *)0)<0);
				timer = 1;

			}
			usleep(500000);
			g_alarm_button_event = BOOT_STATE_NONE;
		}
		else if( g_alarm_button_event == ALARM_BUTTON_EVENT_MID)
		{
			//stop
			//stop music
			musicProcess(g_alarm_ring_path, 0);
			gs_boot_state.alarm_state = BOOT_ALARM_STOP;
			update_ring_file();
			gr_clean();
			usleep(500000);
			usleep(500000);
			system_shutdown();
			LOGE("shutdown system\n");
			return 0;
		}
		else if( g_alarm_button_event == ALARM_BUTTON_EVENT_RIGHT)
		{
			//power on
			if(gs_boot_state.alarm_state == BOOT_ALARM_ALARMING)
			{
				musicProcess(g_alarm_ring_path,0);
				update_delay_time(alarm_item_bak);
				break;
			}
		}

		//check another alarm comming in alarming
		if((get_latest_alarm_time(&fire_alarm) == 0) &&
			(alarm_item_bak != alarm_cur))
		{
			LOGD("alarm : another alarm come\n");
			gs_boot_state.alarm_state = BOOT_ALARM_ANOTHER;
			musicProcess(g_alarm_ring_path, 0);
			sleep(1);
			gr_clean();
			gs_boot_state.alarm_state = BOOT_ALARM_ALARMING;
			if((alarm_cur->vibrate == 1))
			{
				pthread_create(&vibrate_t,NULL,pthread_vibrate , NULL);
			}
			alarm_item_bak = alarm_cur;
			print_images(ALARM_WARNING_IMG_POS);
			musicProcess(g_alarm_ring_path,1);
			while(gettimeofday(&start_time, (struct timezone *)0)<0);
			timer = 1;
		}
		// snooze if alarm last 1min
		if(gs_boot_state.alarm_state == BOOT_ALARM_ALARMING){
			while(gettimeofday(&now_time, (struct timezone *)0)<0);
			time_diff_temp = timeval_diff(now_time, start_time);
			if(time_diff_temp > 60*timer*1000000){
				print_images(ALARM_WARNING_IMG_POS);
				timer++;
			}
			if(g_alarm_last_time > 0){
				if(time_diff_temp > g_alarm_last_time*1000000){
					g_alarm_button_event = ALARM_BUTTON_EVENT_LEFT;
				}
			}
		}
		if(time(NULL) == abstime_power)
		{
			musicProcess(g_alarm_ring_path,0);
			break;
		}
		usleep(5000);
	}
	system_poweron();
	return 0;
}

void *refreshen_screen(void *data)
{
	while(gs_boot_state.alarm_state != BOOT_ALARM_EXIT)
	{
		gr_flip();
		usleep(5000);
	}
	return NULL;
}

void boot_alarm_exit(void)
{
	struct alarm_sec *p_sec = NULL;
	struct alarm_db  *p_db = NULL;

	musicProcess(g_alarm_ring_path,0);
	gr_clean();
	set_next_alarm_time();

	boot_alarm = 0;
	g_alarm_button_event = BOOT_STATE_NONE;

	gs_boot_state.alarm_init = 0;
	gs_boot_state.power_key_event = BOOT_STATE_NONE;

	while(g_alarm_sec_list != NULL)
	{
		p_sec = g_alarm_sec_list;
		g_alarm_sec_list = g_alarm_sec_list->next;
		free(p_sec);
	}
	while(g_alarm_db_list != NULL)
	{
		p_db = g_alarm_db_list;
		g_alarm_db_list=g_alarm_db_list->next;
		free(p_db);
	}

	//show_boot_logo(ALARM_BOOT_LOGO_POS);
	usleep(50000);
	free_alarm_surface(ALARM_WARNING_IMG_POS);
	free_alarm_surface(ALARM_TIME_NUM_POS);
	free_alarm_surface(ALARM_BOOT_LOGO_POS);
	set_state(LCD_BRIGHTNESS_ON);
	return;
}

int parse_db_item(char *db_buf)
{
	if(db_buf == NULL)
		return 0;

	struct alarm_db *p_db_item = NULL;
	char buf[1024] = {0,};

	strncpy(buf, db_buf, 1024);
	p_db_item = (struct alarm_db *)malloc(sizeof(struct alarm_db));
	if(p_db_item == NULL)
	{
		return -1;
	}

	p_db_item->id        =  atoi(strtok(buf, "|"));
	p_db_item->hour      =  atoi(strtok(NULL, "|"));
	p_db_item->min       =  atoi(strtok(NULL, "|"));
	p_db_item->dayofweek =  atoi(strtok(NULL, "|"));
	p_db_item->enable      =  atoi(strtok(NULL, "|"));
	p_db_item->vibrate      =  atoi(strtok(NULL, "|"));
	strncpy(p_db_item->message, strtok(NULL, "|"), 512);

	char *tmp_p = strtok(NULL,"|");
	if(tmp_p == NULL)
	{
		strncpy(p_db_item->alert,p_db_item->message, 512);
		memset(p_db_item->message, 0,512 );
	}
	else
	{
		strncpy(p_db_item->alert, tmp_p, 512);
	}
	p_db_item->next = NULL;

	struct  alarm_db *p = g_alarm_db_list;
	if(p == NULL)
	{
		g_alarm_db_list = p_db_item;
	}
	else
	{
		while(p->next != NULL)
			p = p->next;
		p->next = p_db_item;
	}

	return 0;
}

void show_alarm_seclist(struct alarm_sec *p)
{
	LOGD("alarm show_alarm_seclist\n");
	while(p != NULL)
	{
		ERROR("secs = %d\n",p->secs);
		ERROR("id = %d\n",p->db_id);
		ERROR("day = %d\n",p->day);
		ERROR("vib = %d\n",p->vibrate);
		ERROR("ala = %s\n",p->alert);
		p= p->next;
	}
}

int update_delay_time( struct alarm_sec *alarm_item)
{
	int fd;
	time_t t;
	char buf[512] = {0,};

	if(alarm_item == NULL)
	{
		return -1;
	}
	t = time(NULL);
	t += (g_alarm_snooze_time);
	sprintf(buf,"%d\n%u000\n", alarm_item->db_id, t);

	fd = open(BOOT_ALARM_DELAY_TIME_FILE, O_CREAT|O_TRUNC|O_WRONLY,0664);
	if(fd < 0)
	{
		ERROR("open delay time failed!\n");
		return -1;
	}
	if(write(fd,buf, 512) < 0)
	{
		close(fd);
		return -1;
	}
	close(fd);
	ERROR("write delay time success, str=%s\n",buf);
	return 0;
}

struct alarm_sec *add_alarm_delay_item(void)
{
	int fd;
	int delay_id;
	char buf[512] = {0,};
	time_t abs_secs;
	struct tm *tm = NULL;
	struct alarm_sec *sec_item = NULL;
	struct alarm_db *p = g_alarm_db_list;

	if(access(BOOT_ALARM_DELAY_TIME_FILE, F_OK) != 0)
	{
		ERROR("alarm delay file not exist\n");
		goto end;
	}
	ERROR("add delay item\n");
	//get delay file content
	fd = open(BOOT_ALARM_DELAY_TIME_FILE, O_RDONLY);
	if(fd < 0)
	{
		goto end;
	}
	if(read(fd,buf, 512) < 0)
	{
		close(fd);
		goto end;
	}
	close(fd);

	delay_id = atoi(strtok(buf, "\n"));
	char *str_sec = strtok(NULL, "\n");

	//change microsecond to second
	str_sec[strlen(str_sec) - 3] = "\0";

	abs_secs = strtoul(str_sec, NULL, 10);
	tm = localtime(&abs_secs);
	ERROR("delay id = %d, secs = %d\n",delay_id, abs_secs);
	ERROR("delay hour = %d\n, delay min = %d\n", tm->tm_hour, tm->tm_min);
	ERROR("delay sec = %d\n, delay wday = %d\n", tm->tm_sec, tm->tm_wday);

	while(p != NULL)
	{
		if(p->id == delay_id)
		{
			sec_item = (struct alarm_sec *)malloc(sizeof(struct alarm_sec));
			if(sec_item == NULL)
			{
				goto end;
			}

			sec_item->day = (tm->tm_wday == 0)?6:(tm->tm_wday - 1);
			sec_item->secs = sec_item->day*24*3600 + tm->tm_hour*3600 + tm->tm_min*60 + tm->tm_sec;
			sec_item->db_id = p->id;
			sec_item->vibrate= p->vibrate;
			sec_item->alert = p->alert;
			sec_item->next = NULL;
			break;
		}
		p = p->next;
	}
end:
	return sec_item;

}

struct alarm_sec *creat_alarm_sec_list(void)
{
	struct alarm_db *p = g_alarm_db_list;
	struct alarm_sec *p_sec = g_alarm_sec_list;
	struct alarm_sec *sec_item = NULL;

	if(p == NULL)
	{
		LOGE("%s line: %d\n", __func__, __LINE__);
		return NULL;
	}

	while(p != NULL)
	{
		if(p->enable)
		{
			int dayth = 0;
			while(dayth < 7)
			{
				//just alarm one time
				if(!p->dayofweek)
				{
					sec_item = (struct alarm_sec *)malloc(sizeof(struct alarm_sec));
					if(sec_item == NULL)
					{
						return NULL;
					}

					sec_item->secs = p->hour*60*60 + p->min*60;
					dayth = check_alarm_day(sec_item->secs);
					ERROR("%s check_alarm_day %d\n", __func__, dayth);
					sec_item->secs += dayth * 24*60*60;
					sec_item->db_id = p->id;
					sec_item->vibrate= p->vibrate;
					sec_item->alert = p->alert;
					//if alarm one time, day = -1
					sec_item->day = -1;
					sec_item->next = NULL;

					if(p_sec == NULL)
					{
						g_alarm_sec_list = sec_item;
					}
					else
					{
						p_sec->next = sec_item;
					}
					p_sec = sec_item;
					break;
				}
				else if(((0x0001 << dayth) & p->dayofweek) != 0)
				{
					sec_item = (struct alarm_sec *)malloc(sizeof(struct alarm_sec));
					if(sec_item == NULL)
					{
						return NULL;
					}
					sec_item->secs = dayth *24*60*60 + p->hour*60*60 + p->min*60;
					sec_item->db_id = p->id;
					sec_item->vibrate= p->vibrate;
					sec_item->alert = p->alert;

					sec_item->day = dayth;
					sec_item->next = NULL;

					if(p_sec == NULL)
					{
						g_alarm_sec_list = sec_item;
						p_sec = g_alarm_sec_list;
					}
					else
					{
						p_sec->next = sec_item;
					}
					p_sec = sec_item;
				}
				dayth++;
			}
		}
		p = p->next;
	}
	if(p_sec)
	{
		p_sec->next = add_ring_file_time();
		show_alarm_seclist(g_alarm_sec_list);
	}else{
		g_alarm_sec_list =add_ring_file_time();
		show_alarm_seclist(g_alarm_sec_list);
	}
	return  g_alarm_sec_list;
}

void parse_alarm_event(struct input_event *ev)
{
	static int x,y;

	if(ev->type == EV_ABS) {
		if(ev->code == ABS_MT_POSITION_X)
		{
			x = ev->value;
		}
		if(ev->code == ABS_MT_POSITION_Y)
		{
			y = ev->value;
			//left button
			if((x >= g_left_pos.x0)&&(x <= g_left_pos.x1)
					&&(y >= g_left_pos.y0)&&(y < g_left_pos.y1))
			{
				LOGD("%s get left button\n", __func__);
				g_alarm_button_event = ALARM_BUTTON_EVENT_LEFT;
			}
			//mid button
			if((x >= g_mid_pos.x0)&&(x <= g_mid_pos.x1)
					&&(y >= g_mid_pos.y0)&&(y < g_mid_pos.y1))
			{
				LOGD("%s get mid button\n", __func__);
				g_alarm_button_event = ALARM_BUTTON_EVENT_MID;
			}
			//right button
			else if((x >= g_right_pos.x0)&&(x <= g_right_pos.x1)
					&&(y >= g_right_pos.y0)&&(y < g_right_pos.y1))
			{
				LOGD("%s get right button\n", __func__);
				g_alarm_button_event = ALARM_BUTTON_EVENT_RIGHT;
			}
		}
	}else {
		x = 0;
		y = 0;
	}
	return;
}
void *get_input_event(void *data)
{
	struct input_event ev;
	int ret;

	ev_init();
	while(gs_boot_state.alarm_state != BOOT_ALARM_EXIT)
	{
		ret = ev_get(&ev, -1);
		if(ret == 0){
			parse_key_event(&ev);
			parse_alarm_event(&ev);
		}
	}
	return NULL;
}

int alarm_action(void)
{
	pid_t pid;
	pid = fork();
	if(pid<0)
	{
		ERROR("fork err");
		return 0;
	}
	if(pid ==0)
	{
		execl("/playtest","playtest",NULL);
	}
	return 0;
}

int check_alarm_day(int day_secs)
{
	time_t timep;
	struct tm *time_cur;
	int weekday = -1;

	time(&timep);
	time_cur = localtime(&timep);



	if(time_cur->tm_wday == 0)
	{
		weekday = 6;
	}
	else
	{
		weekday = time_cur->tm_wday-1;
	}
	if((time_cur->tm_hour*3600 + time_cur->tm_min*60 + time_cur->tm_sec - 40) > day_secs)
	{
		weekday += 1;
	}
	return (weekday == 7)?0:weekday;
}

#define LOCALTIME_WEEKDAY_SECS             1
#define LOCALTIME_WEEKDAY_REMAIN_SECS      2
int get_secs_in_weeks(int flag)
{
	time_t timep;
	struct tm *time_cur;
	int weekday = -1;

	time(&timep);
	time_cur = localtime(&timep);

	if(time_cur->tm_wday == 0)
	{
		weekday = 6;
	}
	else
	{
		weekday = time_cur->tm_wday-1;
	}

	if(flag == LOCALTIME_WEEKDAY_SECS)
	{
		return (weekday * 24*60*60 + time_cur->tm_hour *60*60 + time_cur->tm_min*60 + time_cur->tm_sec);
	}
	else if(flag == LOCALTIME_WEEKDAY_REMAIN_SECS)
	{
		return (6-weekday)*24*60*60 +
			(23 - time_cur->tm_hour)*60*60 +
			(59 - time_cur->tm_min)*60 +
			(60 - time_cur->tm_sec);
	}

	return -1;
}

//update the alarm database as the alarm_cur
void update_alarm_db(void)
{
	char cmd[1024] = {0,};

	if(alarm_cur->day == -1)
	{
		sprintf(cmd,"/system/xbin/sqlite3 %s \" update alarms set enabled=0 where _id=%d\"",BOOT_ALARM_DB_FILE, alarm_cur->db_id);
		system(cmd);
	}
}
void update_alarm_sec_list(struct alarm_sec *fire_alarm,int snoozetime)
{
	unsigned long timenow = 0;
	unsigned long secs = 0;
	struct tm *tm = NULL;
	int day = 0;
	struct alarm_sec *p1 = g_alarm_sec_list;
	if(p1 == NULL)
		return;
	while(p1->secs != fire_alarm->secs && p1->next !=NULL)
	{
		p1 = p1->next;
	}
	if(fire_alarm->secs == p1->secs)
	{
		timenow = time(NULL);
		tm = localtime(&timenow);
		day = (tm->tm_wday == 0)?6:(tm->tm_wday - 1);
		secs = day*24*3600 + tm->tm_hour*3600 + tm->tm_min*60 + tm->tm_sec;
		p1->secs=secs +snoozetime;
	}else
		LOGD("fire_alarm not found\n");
	return 0;

}
int del_alarm_sec_list(struct alarm_sec *fire_alarm)
{
	struct alarm_sec *head = g_alarm_sec_list;
	struct alarm_sec *p1,*p2;
	p1 = head;
	if(p1 == NULL)
		return -1;
	while(p1->secs != fire_alarm->secs && p1->next !=NULL)
	{
		p2 = p1;
		p1 = p1->next;
	}
	if(fire_alarm->secs == p1->secs)
	{
		if(p1 == head)
			g_alarm_sec_list = p1->next;
		else
			p2->next = p1->next;
		free(p1);
		p1 = NULL;
	}else
		LOGD("fire_alarm not found\n");
	return 0;

}
//this function will refresh alarm_cur
int get_latest_alarm_time(struct alarm_sec **fire_alarm)
{
	int cur_time = 0;
	int tmp = 0;
	int min = -1;
	static int first_ring = 0;
	static int first_alarm = 0;

	cur_time = get_secs_in_weeks(LOCALTIME_WEEKDAY_SECS);
	struct alarm_sec *p = g_alarm_sec_list;
	if(p == NULL)
	{
		if(fire_alarm != NULL)
			*fire_alarm = NULL;
		return min;
	}

	while(p != NULL)
	{
		tmp = p->secs - cur_time;
		if(tmp < 0)
		{
			if(first_alarm != 0 || tmp < -60)
				tmp+= 7* 24*60*60;
			else
				first_alarm = 1;
		}
		if(min == -1)
		{
			min = tmp;
			alarm_cur = p;
		}
		else if(min > tmp)
		{
			min = tmp;
			alarm_cur = p;
		}

              p = p->next;
	}
	//    next week
	if(min < 0)
	{
		if(first_ring != 0){
			min+= get_secs_in_weeks(LOCALTIME_WEEKDAY_REMAIN_SECS);
		}else{
			min = 0;
			first_ring = 1;
		}
	}
	if(min == 0)
	{
		if(fire_alarm != NULL)
			*fire_alarm = alarm_cur;
		//when alarm one time
		if(alarm_cur->day == -1)
		{
			update_alarm_db();
		}
		//alarm now
		LOGD("alarm get_latest_alarm_time, find one\n");
		LOGD("secs = %d\n",alarm_cur->secs);
		LOGD("id = %d\n",alarm_cur->db_id);
		LOGD("day = %d\n",alarm_cur->day);
		LOGD("vib = %d\n",alarm_cur->vibrate);
		LOGD("ala = %s\n",alarm_cur->alert);

		show_alarm_seclist(g_alarm_sec_list);
		return 0;
	}else{
		if(fire_alarm != NULL)
			*fire_alarm = NULL;
	}
	return min;
}


static quit_flag = 0;
static pid_mp3_player_last = 0;
void sig_usr1_handler(int sig)
{
	quit_flag = 1;
	kill(pid_mp3_player_last, SIGUSR2);

}

static pid_t pid_mp3_player = -1;

int musicProcess(char *filename, int op)
{
	pid_t pid;
	FILE *pf;
	int len;
	int status;
	int ret = 0;

	LOGD("%s:%s operation code = %d!\n", __FILE__, __func__, op);
	/*
	 * There must be 10 parameters, arg[0] is the player app's name when called in a shell.
	 * So here arg[1] is useless.
	 */
	char *mp3_prm[] = {"mplayer", "0", NULL};

	if (0 == op) {
		if (-1 == pid_mp3_player) {
			return -1;
		}

		pid = pid_mp3_player;

		status =  kill(pid, SIGUSR1);
		if (0 == status) {
			pid_mp3_player = -1;
		}

		return status;
	}
	else if (op != 1)
	{
		LOGE("%s:%s wrong operation code [%d]!\n", __FILE__, __func__, op);
		return -1;
	}

	if (-1 != pid_mp3_player) {
		LOGE("%s:%s now one mp3 is playing, please close it first!\n", __FILE__, __func__);
		return -5;
	}
	if(filename == NULL)
		filename=BOOT_ALARM_DEFAULT_RING;
	pid = fork();
	pid_mp3_player = pid;

	if (0 == pid) {
		int status;

		mp3_prm[1] = filename;
		signal(SIGUSR1, sig_usr1_handler);
		while (1) {
			pid_mp3_player_last = fork();

			if (0 == pid_mp3_player_last) {
				ret = execv(MP3_PLAYER, mp3_prm);
				exit(0);
			}
			else {
				wait(&status);
				if (1 == quit_flag) {
					kill(pid_mp3_player_last, SIGKILL);
					exit(0);
					break;
				}
			}
		}
	}
	return pid;
}


int  alarm_Set_Vib(char *vibrate_time)
{
	int fd;
	if(vibrate_time == NULL)
	{
		return -1;
	}
	fd = open(VIB_PATH,O_RDWR|O_TRUNC);
	if(fd < 0)
	{
		ERROR("can't set  vib\n");
		return -1;
	}
	if(write(fd, vibrate_time, strlen(vibrate_time)) < 0)
	{
		ERROR("write vib failed\n");
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
}

int  alarm_Set_NewHardWareAlarm(long int second)
{
	int fd;
	char sec[11] = "";
	long int rtc_value = 0;
	char rtc_state[512] = "";

	fd= open(RTC_TIME_PATH,O_RDONLY);
	if(fd < 0)
	{
		return -1;
	}
	if(read(fd, rtc_state,512) < 0)
	{
		close(fd);
		return -1;
	}
	close(fd);
	rtc_value = strtol(rtc_state, NULL, 10);
	second=second+rtc_value;
	sprintf(sec, "%i", second);

	fd = open(ALARM_PARAM_PATH,O_RDWR|O_TRUNC);
	if(fd < 0)
	{
		ERROR("can't set  vib\n");
		return -1;
	}
	if(write(fd, sec, strlen(sec)) < 0)
	{
		ERROR("write vib failed\n");
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
}


static int alarm_Init_AlarmTimeList()
{
	return 1;
}

static int alarm_Read_AlarmTimeList(struct alarm_InfoStruct *info,int number)
{
	return 1;
}

static int alarm_Write_AlarmTimeList(struct alarm_InfoStruct *info,int number )
{
	return 1;
}

static void *alarm_ShowUi(void *data)
{
	return 1;
}

static int alarm_CheckAlarm()
{
	return 0;
}


static int enter_alarm()
{
	alarm_BootSystem=0;
	init_AlarmTimeList();
	while (1)
	{
		if (alarm_CheckAlarm())
		{

			kill(alarm_t_ShowUi, SIGKILL);
			pthread_create(&alarm_t_ShowUi, NULL, alarm_ShowUi, NULL);
		}
		if (alarm_BootSystem)break;

	}
	return 1;
}
