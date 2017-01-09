#include "testitem.h"

static int x_pressed=0;
static int y_pressed=0;
static int eng_ts_success=-1;
static int thread_run = 0;
static int firstdraw = 0;
int tp_flag = 0;

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

static int tp_handle_event(struct input_event *event)
{
	int keycode;
	static int count=0;
	count++;
	if(event->type == EV_ABS)
	{
		switch(event->code)
		{
			case ABS_MT_POSITION_X:
				x_pressed = 1;
				cur_pos.x = event->value*width/tp_width;
				LOGD("mmitesthhlX:%d", event->value);
				break;
			case ABS_MT_POSITION_Y:
				y_pressed = 1;
				cur_pos.y = event->value*height/tp_height;
				LOGD("mmitesthhlY:%d", event->value);
				if( (abs(last_pos.x - cur_pos.x) > (rect_w - (rect_w >> 1))) || (abs( last_pos.y - cur_pos.y) > (rect_h - (rect_h >> 1))))
				{
					ui_set_color(CL_WHITE);
					if(last_pos.x != 0 && last_pos.y != 0)
					{
						ui_draw_line_mid(last_pos.x, last_pos.y, cur_pos.x, cur_pos.y);
						area_rectangle_check(cur_pos.x, cur_pos.y);
						gr_flip();
						LOGD("mmitesthhl %d-%d", cur_pos.x, cur_pos.y);
					}
					last_pos.x = cur_pos.x;
					last_pos.y = cur_pos.y;
				}
				break;
		}
	}
	else if(event->type == EV_SYN)
	{
		switch(event->code)
		{
			case SYN_MT_REPORT:
				LOGD("mmitest SYN_MT_RP:%d", event->value);
				break;
			case SYN_REPORT:
				LOGD("mmitest SYN_RP:%d", event->value);
				break;
		}
	}
	return 0;
}


static void* tp_thread()
{
	int ret;
	int fd = -1;
	fd_set rfds;
	struct input_event event;
	struct timeval timeout;
	struct input_absinfo absinfo_x;
	struct input_absinfo absinfo_y;

	if(strlen(TS_DEV_NAME)){
		fd = get_sensor_name(TS_DEV_NAME);
		//fd = find_input_dev(O_RDONLY, SPRD_TS_INPUT_DEV);
		if(fd < 0) {
			LOGD("mmitest open %s tp fail",TS_DEV_NAME);
			goto exit;
		}
	}else{
		LOGD("mmitest get tp event %s failed",TS_DEV_NAME);
		goto exit;
	}

	memset(&cur_pos, 0, sizeof(tp_pos));
	memset(&last_pos, 0, sizeof(tp_pos));

	if(ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), &absinfo_x)) {
		LOGD("mmitest can not get absinfo");
		goto exit;
	}

	if(ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), &absinfo_y)) {
		LOGD("mmitest can not get absinfo");
		goto exit;
	}
	tp_width = absinfo_x.maximum;
	tp_height = absinfo_y.maximum;
	LOGD("mmitest tp_width=%d, tp_height=%d", tp_width, tp_height);
	while(thread_run==1) {
	    FD_ZERO(&rfds);
	    FD_SET(fd, &rfds);
		timeout.tv_sec=1;
		timeout.tv_usec=0;

		//waiting for touch
		ret = select(fd+1, &rfds, NULL, NULL, &timeout);
		if(ret < 0) {
			//LOGD("mmitest [%s]: error from select (%d): %s",__FUNCTION__, fd, strerror(errno));
			continue;
		}else if(ret == 0) {
			//LOGD("mmitest [%s]: timeout, %d", __FUNCTION__, timeout.tv_sec);
			continue;
		}else {
			if(FD_ISSET(fd, &rfds)) {
				//read input event
				ret = read(fd, &event, sizeof(event));
				if (ret == sizeof(event)) {
					//LOGD("mmitest [%s]: timeout, %d", __FUNCTION__, sizeof(event));
					//handle key pressed
					tp_handle_event(&event);
				} else {
					//LOGD("mmitest [%s]: read event too small %d", __func__, ret);
				}
			} else {
				//firstdraw = 0;
				//LOGD("%s: fd is not set", __FUNCTION__);
			}
		}
	}
exit:
	ui_push_result(RL_FAIL);
	if(fd >= 0)
		close(fd);
	return NULL;
}


void area_rectangle_init(void)
{
    int i = 0;
	/*
    int width = gr_fb_width();
    int height = gr_fb_height();
    int rect_w = width / (AREA_COL-1);
    int rect_h = height / (AREA_ROW-1);
	*/
    LOGD("width=%d, height=%d", width, height);
    LOGD("rect_w=%d, rect_h=%d", rect_w, rect_h);
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
			LOGD("rect_cnt=%d", rect_cnt);
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
			LOGD("rect_cnt=%d", rect_cnt);
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
				LOGD("rect_cnt=%d", rect_cnt);
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
				LOGD("rect_cnt=%d", rect_cnt);
			}
		}
	}
	if(rect_cnt >= (AREA_ROW+AREA_COL)*2)
	{
		ui_push_result(RL_PASS);
	}
}

/*
 * Handle TouchScreen input event
 */
int test_tp_start(void)
{
	pthread_t t;
	int ret = -1;
	width = gr_fb_width();
	height = gr_fb_height();
	tp_width = width;
	tp_height = height;
	rect_w = width / AREA_COL;
	rect_h = height / AREA_ROW;
	tp_flag = 1;
	gr_tp_flag(tp_flag);
	ui_fill_locked();
	area_rectangle_init();
	thread_run=1;

	pthread_create(&t, NULL, (void*)tp_thread, NULL);

	do{
		ret = ui_wait_button(NULL,NULL,NULL);
	}
	while(KEY_VOLUMEDOWN != ret && KEY_VOLUMEUP != ret && KEY_POWER != ret);

	thread_run=0;
	pthread_join(t, NULL); /* wait "handle key" thread exit. */

	if(KEY_VOLUMEDOWN == ret)
		save_result(CASE_TEST_TP, RL_PASS);
	else if(KEY_POWER == ret)
		save_result(CASE_TEST_TP, RL_FAIL);

	tp_flag = 0;
	gr_tp_flag(tp_flag);

	return ret;
}
