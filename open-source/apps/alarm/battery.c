#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/kd.h>
#include <errno.h>
#include <linux/input.h>
#include <linux/reboot.h>
#include <sys/time.h>
#include <pthread.h>
#include "battery.h"
#include "common.h"
#include "boot_alarm.h"

int g_sleep = 0;
char g_brightness[20] = {0,};
static int usb_online_flag = USB_ON_LINE;
int  set_state(char *state);
int get_value(const char *file);
extern struct boot_status gs_boot_state;
extern int g_alarm_button_event;

int check_input_dev(const char * cmstr)
{
	FILE *fp;
	char cmdline[1024];
	char *tmpstr;

	fp=fopen("/proc/bus/input/devices","r");
	if(fp==NULL) return -1;
	fgets(cmdline,1023,fp);
	while(!feof(fp))
	{
		if(strncmp(cmdline,"N:",2))
		{
			fgets(cmdline,1023,fp);
			continue;
		}
		if(strstr(cmdline,cmstr)==NULL)
		{
			fgets(cmdline,1023,fp);
			continue;
		}
		fgets(cmdline,1023,fp);
		while(!feof(fp))
		{
			if(strncmp(cmdline,"H:",2))
			{
				fgets(cmdline,1023,fp);
				continue;
			}
			tmpstr=strstr(cmdline,"event");
			if(tmpstr==NULL)
				break;
			fclose(fp);
			return atoi(&tmpstr[5]);
		}
	}
	fclose(fp);
	return -1;

}

void parse_key_event(struct input_event *ev)
{
	struct timeval start_time={0};
	struct timeval now_time={0};
	long long time_diff_temp;
	int ret;
	int time_left;
	//long key-press
	if((ev->type == EV_KEY) && (ev->code == KEY_POWER))
	{
		while(gettimeofday(&start_time, (struct timezone *)0)<0);
		time_left =2000;
rechk_pwr_key:
		ret = ev_get(ev, time_left);
		if(ret == -1){ /* time out */
			if(g_sleep == 0)
			{
				gs_boot_state.power_key_event = KEY_LONG_PRESS;
				LOGD("gs_boot_state.power_key_event = KEY_LONG_PRESS\n");
			}
			else
			{
				gs_boot_state.power_key_event = KEY_SHORT_PRESS;
				LOGD("gs_boot_state.power_key_event = KEY_SHORT_PRESS\n");
			}
		}
		else if((ev->type == EV_KEY) && (ev->code == KEY_POWER))
		{
			LOGD("gs_boot_state.power_key_event = return \n");
			return;
		}
		else
		{
			parse_alarm_event(&ev);
			while(gettimeofday(&now_time, (struct timezone *)0)<0);
			time_diff_temp = timeval_diff(now_time, start_time);
			time_diff_temp = (time_diff_temp + 1000)/1000;
			if(time_diff_temp <2000){
				time_left = 2000 - time_diff_temp;
				goto rechk_pwr_key;
			}else{
				if(g_sleep == 0)
				{
					gs_boot_state.power_key_event = KEY_LONG_PRESS;
					LOGD("gs_boot_state.power_key_event = KEY_LONG_PRESS,alarm\n");
				}
				else
				{
					gs_boot_state.power_key_event = KEY_SHORT_PRESS;
					LOGD("gs_boot_state.power_key_event = KEY_SHORT_PRESS,alarm\n");
				}
			}
		}
	}
	return;
}

//set lcd backlight brightness
int  set_state(char *state)
{
	int fd;
	char *plight = NULL;
	if(strcmp(state,LCD_BRIGHTNESS_ON) == 0)
	{
		plight = g_brightness;
		g_sleep = 0;
	}
	else if(strcmp(state,LCD_BRIGHTNESS_OFF) == 0)
	{
		plight = "0";
		g_sleep = 1;
	}
	else
	{
		LOGD(" the value of state: %s is not what we expected.\n", state);
		return -1;
	}
	fd = open(LCD_BACKLIGHT_BRIGHTNESS_PATH,O_RDWR|O_TRUNC);
	if(fd < 0)
	{
		ERROR("can't set  brightness\n");
		return -1;
	}
	LOGD("set backlight to %s\n", plight);
	if(write(fd, plight, strlen(plight)) < 0)
	{
		ERROR("write brightness failed\n");
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
}

int get_value(const char *file)
{
	int fd;
	long int value = 0;
	char battery_state[512] = "";

	fd= open(file,O_RDONLY);
	if(fd < 0)
	{
		return -1;
	}
	if(read(fd, battery_state,512) < 0)
	{
		close(fd);
		return -1;
	}
	close(fd);
	value = strtol(battery_state, NULL, 10);

	return value;
}
