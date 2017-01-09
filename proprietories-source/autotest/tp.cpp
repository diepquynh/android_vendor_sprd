#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include "key_common.h"
#include "ui.h"

extern int ui_wait_key(struct timespec *ntime);
extern void ui_clear_key_queue();

#define DBG_ENABLE_DBGMSG
#define DBG_ENABLE_WRNMSG
#define DBG_ENABLE_ERRMSG
#define DBG_ENABLE_INFMSG
#define DBG_ENABLE_FUNINF
#include "debug.h"

#define USE_UI	1

extern volatile unsigned char skd_tp_r;

static int result_has_set;

static int x_pressed=0;
static int y_pressed=0;
static int thread_run = 0;
static int firstdraw = 0;

typedef struct _tp_pos{
	int x;
	int y;
}tp_pos;

typedef struct _area_info {
	tp_pos p1;
	tp_pos p2;
	tp_pos p3;
	tp_pos p4;
	int drawed;
}area_info;

tp_pos cur_pos;
tp_pos last_pos;


//#define SPRD_TS_INPUT_DEV		"focaltech_ts"


#define AREA_ROW  11
#define AREA_COL  5
area_info rect_r1 [AREA_ROW];
area_info rect_r2 [AREA_ROW];
area_info rect_c1 [AREA_COL];
area_info rect_c2 [AREA_COL];
int width = 0;
int height = 0;
int tp_width = 0;
int tp_height = 0;
int rect_w = 0;
int rect_h = 0;
int rect_cnt = 0;
int saw_mt_report = 0;

void area_rectangle_check(int x, int y);

static int find_input_dev(int mode)
{
	int fd = -1;
	int ret = -1;
	const char *dirname = "/dev/input";
	char devname[PATH_MAX];
	char *filename;
	DIR *dir;
	struct dirent *de;
	char name[128];
	const char TP_DEV[2][20]={{"focaltech_ts"}, {"msg2138_ts"}};

	dir = opendir(dirname);
	if (dir == NULL) {
		return -1;
	}

	strcpy(devname, dirname);
	filename = devname + strlen(devname);
	*filename++ = '/';

	while ((de = readdir(dir))) {
		if ((de->d_name[0] == '.' && de->d_name[1] == '\0') ||
				(de->d_name[0] == '.' && de->d_name[1] == '.' &&
				 de->d_name[2] == '\0')) {
			/* ignore .(current) and ..(top) directory */
			continue;
		}
		strcpy(filename, de->d_name);
		fd = open(devname, mode);

		if (fd >= 0) {
			memset(name, 0, sizeof(name));
			if (ioctl(fd, EVIOCGNAME(sizeof(name) - 1), &name) < 0){

			} else {

				LOGD("TP-=-name is %s\n", name);
				if (0==strcmp(name, &TP_DEV[0][0]) ||
					0 == strcmp(name, &TP_DEV[1][0])) {
					ret = fd; //get the sensor name from the event
					goto END;
				}
			}
			close(fd);
		}
	}
END:
	closedir(dir);

	return ret;
}
static int tp_handle_event(struct input_event *event)
{
	int keycode;
	static int count=0;
	count++;
	if(event->type == EV_ABS){
		switch(event->code){
		case ABS_MT_POSITION_X:
			x_pressed = 1;
			cur_pos.x = event->value*width/tp_width;
			LOGD("mmitesthhlX:%d\n", event->value);
			break;
		case ABS_MT_POSITION_Y:
			y_pressed = 1;
			cur_pos.y = event->value*height/tp_height;
			LOGD("mmitesthhlY:%d\n", event->value);

			if(0) {
				firstdraw = 1;
				last_pos.x = cur_pos.x;
				last_pos.y = cur_pos.y;
			} else {
				if(last_pos.x != cur_pos.x || last_pos.y != cur_pos.y){
					ui_set_color(CL_WHITE);
					if(last_pos.x != 0 && last_pos.y != 0){
						ui_draw_line_mid(last_pos.x, last_pos.y, cur_pos.x, cur_pos.y);
						area_rectangle_check(cur_pos.x, cur_pos.y);
						gr_flip();
						LOGD("mmitesthhl %d-%d\n", cur_pos.x, cur_pos.y);
					}
				}
				last_pos.x = cur_pos.x;
				last_pos.y = cur_pos.y;
			}
			break;
		}

	} else if(event->type == EV_SYN) {
		switch(event->code) {
		case SYN_MT_REPORT:
			LOGD("mmitest SYN_MT_RP:%d\n", event->value);
			break;
		case SYN_REPORT:
			LOGD("mmitest SYN_RP:%d\n", event->value);
			break;
		}
	}
	return 0;
}

static void tp_test_save_result(unsigned char result)
{
	if (result_has_set == 0)
		skd_tp_r = result;
	result_has_set = 1;
	return ;
}

static void* tp_thread(void *par)
{
	int ret;
	fd_set rfds;
	struct input_event event;
	struct timeval timeout;
	struct input_absinfo absinfo_x;
	struct input_absinfo absinfo_y;

	int fd = -1;

	fd = find_input_dev(O_RDONLY);
	if(fd < 0) {
		LOGD("autotest [%s]: open tp dev fail\n", __func__);//SPRD_TS_INPUT_DEV);
		tp_test_save_result(1);
		return NULL;
	}

	memset(&cur_pos, 0, sizeof(tp_pos));
	memset(&last_pos, 0, sizeof(tp_pos));

	if(ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), &absinfo_x)) {
		LOGD("mmitest can not get absinfo\n");
		tp_test_save_result(1);
		result_has_set = 1;
		close(fd);
		return NULL;
	}

	if(ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), &absinfo_y)) {
		LOGD("mmitest can not get absinfo\n");
		tp_test_save_result(1);
		close(fd);
		return NULL;
	}
	tp_width = absinfo_x.maximum;
	tp_height = absinfo_y.maximum;
	while(thread_run==1) {
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		timeout.tv_sec=1;
		timeout.tv_usec=0;
		//waiting for touch
		ret = select(fd+1, &rfds, NULL, NULL, &timeout);
		if(ret < 0) {
			LOGD("mmitest [%s]: error from select (%d): %s\n",__FUNCTION__, fd, strerror(errno));
			continue;
		} else if(ret == 0) {
			LOGD("mmitest [%s]: timeout, %d\n", __FUNCTION__, timeout.tv_sec);
			continue;
		} else {
			if(FD_ISSET(fd, &rfds)) {
				//read input event
				ret = read(fd, &event, sizeof(event));
				if (ret == sizeof(event)) {
					//LOGD("mmitest [%s]: timeout, %d\n", __FUNCTION__, sizeof(event));
					//handle key pressed
					tp_handle_event(&event);
				} else {
					LOGD("mmitest [%s]: read event too small %d\n", __func__, ret);
				}
			} else {
				//firstdraw = 0;
				LOGD("%s: fd is not set\n", __FUNCTION__);
			}
		}
	}
	close(fd);
	return NULL;
}


void area_rectangle_init(void)
{
	int i = 0;


	LOGD("width=%d, height=%d\n", width, height);
	LOGD("rect_w=%d, rect_h=%d\n", rect_w, rect_h);
	area_info* prect = NULL;
	ui_set_color(CL_RED);
	ui_draw_line_mid(0, 0, 0, height);
	ui_draw_line_mid(rect_w, 0, rect_w, height);
	for(i = 0; i < AREA_ROW; i++) {
		rect_r1[i].p1.x = 0;
		rect_r1[i].p1.y = i*rect_h;
		rect_r1[i].p2.x = rect_w;
		rect_r1[i].p2.y = i*rect_h;
		rect_r1[i].p3.x = 0;
		rect_r1[i].p3.y = (i+1)*rect_h;
		rect_r1[i].p4.x = rect_w;
		rect_r1[i].p4.y = (i+1)*rect_h;
		rect_r1[i].drawed = 0;
		ui_draw_line_mid(rect_r1[i].p1.x, rect_r1[i].p1.y, rect_r1[i].p2.x, rect_r1[i].p2.y);
	}
	//ui_draw_line_mid(rect_r1[i].p3.x, rect_r1[i].p3.y, rect_r1[i].p4.x, rect_r1[i].p4.y);

	ui_draw_line_mid((width-rect_w), 0, (width-rect_w), height);
	ui_draw_line_mid(width-1, 0, width-1, height);
	for(i = 0; i < AREA_ROW; i++) {
		rect_r2[i].p1.x = (width-rect_w);
		rect_r2[i].p1.y = i*rect_h;
		rect_r2[i].p2.x = width-1;
		rect_r2[i].p2.y = i*rect_h;
		rect_r2[i].p3.x = (width-rect_w);
		rect_r2[i].p3.y = (i+1)*rect_h;
		rect_r2[i].p4.x = width-1;
		rect_r2[i].p4.y = (i+1)*rect_h;
		rect_r2[i].drawed = 0;
		ui_draw_line_mid(rect_r2[i].p1.x, rect_r2[i].p1.y, rect_r2[i].p2.x, rect_r2[i].p2.y);
	}
	//ui_draw_line_mid(rect_r2[i].p3.x, rect_r2[i].p3.y, rect_r2[i].p4.x, rect_r2[i].p4.y);

	ui_draw_line_mid(0+rect_w, 0, width-rect_w, 0);
	ui_draw_line_mid(0, rect_h, width, rect_h);
	for(i = 0; i < AREA_COL; i++) {
		rect_c1[i].p1.x = i*rect_w;
		rect_c1[i].p1.y = 0;
		rect_c1[i].p2.x = (i+1)*rect_w;
		rect_c1[i].p2.y = 0;
		rect_c1[i].p3.x = i*rect_w;
		rect_c1[i].p3.y = rect_h;
		rect_c1[i].p4.x = (i+1)*rect_w;
		rect_c1[i].p4.y = rect_h;
		rect_c1[i].drawed = 0;
		ui_draw_line_mid(rect_c1[i].p1.x, rect_c1[i].p1.y, rect_c1[i].p3.x, rect_c1[i].p3.y);
	}
	rect_c1[0].drawed = 1;
	rect_c1[AREA_COL-1].drawed = 1;
	//ui_draw_line_mid(rect_c1[i].p2.x, rect_c1[i].p2.y, rect_c1[i].p4.x, rect_c1[i].p4.y);

	ui_draw_line_mid(0, (rect_h*(AREA_ROW-1)), width, (rect_h*(AREA_ROW-1)));
	ui_draw_line_mid(0,	(rect_h*(AREA_ROW)), width, (rect_h*(AREA_ROW)));
	for(i = 0; i < AREA_COL; i++) {
		rect_c2[i].p1.x = i*rect_w;
		rect_c2[i].p1.y = (rect_h*(AREA_ROW-1));
		rect_c2[i].p2.x = (i+1)*rect_w;
		rect_c2[i].p2.y = (rect_h*(AREA_ROW-1));
		rect_c2[i].p3.x = i*rect_w;
		rect_c2[i].p3.y = (rect_h*(AREA_ROW));
		rect_c2[i].p4.x = (i+1)*rect_w;
		rect_c2[i].p4.y = (rect_h*(AREA_ROW));
		rect_c2[i].drawed = 0;
		ui_draw_line_mid(rect_c2[i].p1.x, rect_c2[i].p1.y, rect_c2[i].p3.x, rect_c2[i].p3.y);
	}
	rect_c2[0].drawed = 1;
	rect_c2[AREA_COL-1].drawed = 1;
	rect_cnt = 4;
	//ui_draw_line_mid(rect_c2[i].p2.x, rect_c2[i].p2.y, rect_c2[i].p4.x, rect_c2[i].p4.y);
	gr_flip();
}

void area_rectangle_check(int x, int y)
{
	int i = 0;

	if(x >= 0 && x <= rect_w) {
		i = y / rect_h;
		if((i < AREA_ROW) && (rect_r1[i].drawed == 0)) {
			rect_r1[i].drawed = 1;
			ui_set_color(CL_GREEN);
			ui_draw_line_mid(rect_r1[i].p1.x, rect_r1[i].p1.y, rect_r1[i].p2.x, rect_r1[i].p2.y);
			ui_draw_line_mid(rect_r1[i].p1.x, rect_r1[i].p1.y, rect_r1[i].p3.x, rect_r1[i].p3.y);
			ui_draw_line_mid(rect_r1[i].p2.x, rect_r1[i].p2.y, rect_r1[i].p4.x, rect_r1[i].p4.y);
			ui_draw_line_mid(rect_r1[i].p3.x, rect_r1[i].p3.y, rect_r1[i].p4.x, rect_r1[i].p4.y);
			rect_cnt++;
			LOGD("rect_cnt=%d\n", rect_cnt);
		}
	} else if (x >= (width-rect_w) && x <= (width-1)) {
		i = y / rect_h;
		if((i < AREA_ROW) && (rect_r2[i].drawed == 0)) {
			rect_r2[i].drawed = 1;
			ui_set_color(CL_GREEN);
			ui_draw_line_mid(rect_r2[i].p1.x, rect_r2[i].p1.y, rect_r2[i].p2.x, rect_r2[i].p2.y);
			ui_draw_line_mid(rect_r2[i].p1.x, rect_r2[i].p1.y, rect_r2[i].p3.x, rect_r2[i].p3.y);
			ui_draw_line_mid(rect_r2[i].p2.x, rect_r2[i].p2.y, rect_r2[i].p4.x, rect_r2[i].p4.y);
			ui_draw_line_mid(rect_r2[i].p3.x, rect_r2[i].p3.y, rect_r2[i].p4.x, rect_r2[i].p4.y);
			rect_cnt++;
			LOGD("rect_cnt=%d\n", rect_cnt);
		}
	} else {
		i = x / rect_w;
		if(y >=0 && y <= rect_h) {
			if((i < AREA_COL) && (rect_c1[i].drawed == 0)) {
				rect_c1[i].drawed = 1;
				ui_set_color(CL_GREEN);
				ui_draw_line_mid(rect_c1[i].p1.x, rect_c1[i].p1.y, rect_c1[i].p2.x, rect_c1[i].p2.y);
				ui_draw_line_mid(rect_c1[i].p1.x, rect_c1[i].p1.y, rect_c1[i].p3.x, rect_c1[i].p3.y);
				ui_draw_line_mid(rect_c1[i].p2.x, rect_c1[i].p2.y, rect_c1[i].p4.x, rect_c1[i].p4.y);
				ui_draw_line_mid(rect_c1[i].p3.x, rect_c1[i].p3.y, rect_c1[i].p4.x, rect_c1[i].p4.y);
				rect_cnt++;
				LOGD("rect_cnt=%d\n", rect_cnt);
			}
		} else if(y >= (rect_h*(AREA_ROW-1)) && y <= (rect_h*(AREA_ROW))) {
			if((i < AREA_COL) && (rect_c2[i].drawed == 0)) {
				rect_c2[i].drawed = 1;
				ui_set_color(CL_GREEN);
				ui_draw_line_mid(rect_c2[i].p1.x, rect_c2[i].p1.y, rect_c2[i].p2.x, rect_c2[i].p2.y);
				ui_draw_line_mid(rect_c2[i].p1.x, rect_c2[i].p1.y, rect_c2[i].p3.x, rect_c2[i].p3.y);
				ui_draw_line_mid(rect_c2[i].p2.x, rect_c2[i].p2.y, rect_c2[i].p4.x, rect_c2[i].p4.y);
				ui_draw_line_mid(rect_c2[i].p3.x, rect_c2[i].p3.y, rect_c2[i].p4.x, rect_c2[i].p4.y);
				rect_cnt++;
				LOGD("rect_cnt=%d\n", rect_cnt);
			}
		}
	}
	if(rect_cnt >= (AREA_ROW+AREA_COL)*2) {
		tp_test_save_result(0);
	}
}

/*
 * Handle TouchScreen input event
 */
int testTpStart(void)
{
	pthread_t t;
	int ret = 0;
	struct timespec ntime;
	ntime.tv_sec = time(NULL) + 20;
	ntime.tv_nsec = 0;


	skd_tp_r = 1;
	result_has_set = 0;

	width = gr_fb_width();
	height = gr_fb_height();
	tp_width = width;
	tp_height = height;
	rect_w = width / AREA_COL;
	rect_h = height / AREA_ROW;

	ui_fill_locked();
	area_rectangle_init();
	thread_run=1;

	pthread_create(&t, NULL, tp_thread, NULL);
	ui_clear_key_queue();
	ret = ui_wait_key(&ntime);
	thread_run=0;
	pthread_join(t, NULL); /* wait "handle key" thread exit. */

	if (ret == 0x72) {
		tp_test_save_result(0);
	} else {
		tp_test_save_result(1);
	}
	ui_fill_screen(0, 0, 255);
	gr_flip();
	ui_show_title(MENU_TEST_TP);
	if (skd_tp_r == 0) {
		gr_color(0, 255, 0, 255);
		ui_show_text(3, 0, TEXT_TEST_PASS);
	} else if (skd_tp_r == 1) {
		gr_color(255, 0, 0, 255);
		ui_show_text(3, 0, TEXT_TEST_FAIL);
	} else {
		gr_color(255, 255, 255, 255);
		ui_show_text(3, 0, TEXT_NA);
	}
	gr_flip();

	LOGD("[%s]--test result is %d\n", __func__, skd_tp_r);

	return ret;
}
//==============================================================================
//after this add 20160118 for SKD TP test.
int TouchPanel_Id(void)
{//false -1.
	int fd = -1;
	struct input_absinfo absinfo_x;
	tp_test_save_result(1);//default fail.
	result_has_set = 0;

	fd = find_input_dev(O_RDONLY);
	if(fd < 0) {
		LOGD("autotest [%s]: open tp dev fail", __func__);//SPRD_TS_INPUT_DEV);
		return -1;
	}
	/*get chip id first, if success, then registe input device*/
	if(ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), &absinfo_x)) {
		LOGD("mmitest can not get absinfo\n");
		close(fd);
		return NULL;
	}
	if (absinfo_x.maximum > 0)
		tp_test_save_result(0);
	else
                tp_test_save_result(1);

	//display
	ui_fill_screen(0, 0, 255);
	gr_flip();
	ui_show_title(MENU_TEST_TP);
	if (skd_tp_r == 0) {
		gr_color(0, 255, 0, 255);
		ui_show_text(3, 0, TEXT_TEST_PASS);
	} else if (skd_tp_r == 1) {
		gr_color(255, 0, 0, 255);
		ui_show_text(3, 0, TEXT_TEST_FAIL);
	} else {
		gr_color(255, 255, 255, 255);
		ui_show_text(3, 0, TEXT_NA);
	}
	gr_flip();
	return	skd_tp_r;
}
static int TouchPanel_event(struct input_event *event,
				tp_pos *tmp_pos)
{

	if (event->type == EV_ABS) {
		switch (event->code)
		{
			case ABS_MT_POSITION_X:
				tmp_pos->x = event->value*width/tp_width;
				x_pressed = 1;
			break;
			case ABS_MT_POSITION_Y:
				tmp_pos->y = event->value*height/tp_height;
				y_pressed = 1;

			break;
		}
		if (1 == x_pressed && 1 == y_pressed) {
			x_pressed = y_pressed =0;
			if (tmp_pos->x > width || tmp_pos->y > height)
				return 0;
			if (-1 == cur_pos.x && -1 == cur_pos.y) {
				cur_pos.x = tmp_pos->x;
				cur_pos.y = tmp_pos->y;
				firstdraw = 1;
				LOGD("last x=0x%X,y=0x%X\n", tmp_pos->x, tmp_pos->y);
			} else {
				if(tmp_pos->x != cur_pos.x ||
					tmp_pos->y != cur_pos.y) {
					last_pos.x = cur_pos.x;
					last_pos.y = cur_pos.y;
					cur_pos.x = tmp_pos->x;
					cur_pos.y = tmp_pos->y;
					firstdraw++;
					LOGD("cur x=0x%X,y=0x%X\n", tmp_pos->x, tmp_pos->y);
				}
			}
		}
	}
	return 0;
}
static void* TouchPanel_thread(void *par)
{
	int ret;
	fd_set rfds;
	struct input_event event;
	struct timeval timeout;
	struct input_absinfo absinfo_x;
	struct input_absinfo absinfo_y;
	tp_pos tmp_pos;
	int fd = -1;

	firstdraw = 0;
	x_pressed = y_pressed = 0;
	fd = find_input_dev(O_RDONLY);
	if(fd < 0) {
		LOGD("autotest [%s]: open tp dev fail\n", __func__);//SPRD_TS_INPUT_DEV);
		tp_test_save_result(1);
		return NULL;
	}
	memset(&cur_pos, 0xFF, sizeof(tp_pos));
	memset(&last_pos, 0xFF, sizeof(tp_pos));

	if(ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), &absinfo_x)) {
		LOGD("mmitest can not get absinfo\n");
		tp_test_save_result(1);
		close(fd);
		return NULL;
	}

	if(ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), &absinfo_y)) {
		LOGD("mmitest can not get absinfo\n");
		tp_test_save_result(1);
		close(fd);
		return NULL;
	}
	tp_width = absinfo_x.maximum;
	tp_height = absinfo_y.maximum;
	LOGD("tp_width=%d,h=%d\n", tp_width, tp_height);
	while(thread_run==1) {
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		timeout.tv_sec=1;
		timeout.tv_usec=0;
		//waiting for touch
		ret = select(fd+1, &rfds, NULL, NULL, &timeout);
		if(ret < 0) {
//			LOGD("mmitest [%s]: error from select (%d): %s",__FUNCTION__, fd, strerror(errno));
			continue;
		} else if(ret == 0) {
//			LOGD("mmitest [%s]: timeout, %d", __FUNCTION__, timeout.tv_sec);
			continue;
		} else {
			if(FD_ISSET(fd, &rfds)) {
				//read input event
				ret = read(fd, &event, sizeof(event));
				if (ret == sizeof(event)) {
					TouchPanel_event(&event, &tmp_pos);
				} else {
					LOGD("mmitest [%s]: read event too small %d\n", __func__, ret);
				}
			} else {
				LOGD("%s: fd is not set\n", __FUNCTION__);
			}
		}
		/*end */
		if(firstdraw >= 2) {
			thread_run = 0;
			break;
		}
	}
	LOGD("TouchPanel_thread exit\n");
	close(fd);
	return NULL;
}

int TouchPanel_Point(unsigned char *rsp)
{//return data of bytes
	pthread_t t;
	int ret = 0;
	struct timespec ntime;
	x_pressed = 0;
	ntime.tv_sec = time(NULL) + 5;
	ntime.tv_nsec = 0;

	ui_fill_locked();
	ui_fill_screen(0, 0, 255);
	gr_flip();
	ui_show_title(MENU_TEST_TP);
	gr_flip();

	skd_tp_r = 1;
	result_has_set = 0;

	width = gr_fb_width();
	height = gr_fb_height();
	LOGD("fb width=%d,h=%d\n", width, height);

	thread_run=1;
	pthread_create(&t, NULL, TouchPanel_thread, NULL);
	ui_clear_key_queue();
//	ret = ui_wait_key(&ntime);

	//wait
	ret = 0;
	while (ntime.tv_sec > time(NULL) && ret <= 0 &&
		1 == thread_run) {
		usleep(20*1000);
		ret = ui_read_key();
	}
	thread_run=0;
	pthread_join(t, NULL); /* wait "handle key" thread exit. */
	LOGD("key=0x%X\n", ret);
	if (ret == 0x72) {/*v-, pass*/
		tp_test_save_result(0);
	} else if (ret == 0x73) {/*v+ repeat*/
		tp_test_save_result(2);/*repeat*/
	} else if (0x74 == ret) { /*fail*/
		tp_test_save_result(1);
	} else {/*fail*/
		if (firstdraw > 0)
			tp_test_save_result(0);
		else
			tp_test_save_result(1);//timeout
	}
	/*fill data for return*/
	ret = 0;
	if (-1 != last_pos.x && -1 != last_pos.y) {
		*(rsp + ret) = (unsigned char)(last_pos.x>>8);
		ret++;
		*(rsp + ret) = (unsigned char)last_pos.x;
		ret++;
		*(rsp + ret) = (unsigned char)(last_pos.y>>8);
		ret++;
		*(rsp + ret) = (unsigned char)last_pos.y;
		ret++;
	}
	if (-1 != cur_pos.x && -1 != cur_pos.y) {
		*(rsp + ret) = (unsigned char)(cur_pos.x>>8);
		ret++;
		*(rsp + ret) = (unsigned char)cur_pos.x;
		ret++;
		*(rsp + ret) = (unsigned char)(cur_pos.y>>8);
		ret++;
		*(rsp + ret) = (unsigned char)cur_pos.y;
		ret++;

	}
	if (0 == ret) {
		ret = 1;
		rsp[0] = skd_tp_r;
	}
	if (skd_tp_r == 0) {
		gr_color(0, 255, 0, 255);
		ui_show_text(3, 0, TEXT_TEST_PASS);
	} else if (skd_tp_r == 1) {
		gr_color(255, 0, 0, 255);
		ui_show_text(3, 0, TEXT_TEST_FAIL);
	} else {
		gr_color(255, 255, 255, 255);
		ui_show_text(3, 0, TEXT_NA);
	}
	gr_flip();
	return ret;
}


