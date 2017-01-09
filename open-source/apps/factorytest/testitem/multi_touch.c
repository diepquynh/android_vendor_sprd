#include "testitem.h"

typedef struct _tp_pos{
	int x;
	int y;
}tp_pos;

tp_pos cur_pos;
tp_pos last_pos;
tp_pos center1,center2;
int radius;


extern int width;
extern int height;
extern int tp_width;
extern int tp_height;
unsigned char *fbp;
static int firstdraw = 0;
static int thread_run = 0;
static tp_pos std_point1,std_point2;
int cur_row = 2;
extern int tp_flag;
extern struct fb_var_screeninfo vinfo;
extern struct fb_fix_screeninfo finfo;

void ui_draw_cricle(tp_pos center1,tp_pos center2);

static int multitouch_handle_event(struct input_event *event)
{
	static int flag1=0;
	static int flag2=0;
	static int sync_flag = 0;
	static tp_pos draw_point1,draw_point2;

	if(event->type == EV_ABS){
		switch(event->code){
			case ABS_MT_POSITION_X:
				cur_pos.x = event->value*width/tp_width;;
				sync_flag = 1;
				LOGD("mmitesthhlX:%0x\n", event->value);
				break;
			case ABS_MT_POSITION_Y:
				cur_pos.y = event->value*height/tp_height;
				LOGD("mmitesthhlY:%0x\n", event->value);

				if(1 == sync_flag){
					LOGD("mmitesthhl cur:%0x-%0x\n", cur_pos.x, cur_pos.y);
					if(last_pos.x != 0 && last_pos.y != 0){
						if (firstdraw == 0){
							if(((abs(last_pos.x - cur_pos.x) > (width-3*(height>>3))) && (abs(last_pos.y - cur_pos.y) > (height/2)) )
							&&(((abs(last_pos.x - center1.x) < (height >>4)) && (abs(last_pos.y - center1.y) < (height >>4)) ) || ((abs(last_pos.x - center2.x)< (height >>4)) && (abs(last_pos.y - center2.y)< (height >>4)) ) )
							&&(((abs(cur_pos.x - center1.x) < (height >>4)) && (abs(cur_pos.y - center1.y)< (height >>4)) ) || ((abs(cur_pos.x - center2.x)< (height >>4)) && (abs(cur_pos.y - center2.y)< (height >>4)) ) )){


								draw_point1 = last_pos;
								draw_point2 = cur_pos;
								ui_draw_cricle(draw_point1,draw_point2);
								gr_flip();
								std_point1 = draw_point1;
								std_point2 = draw_point2;
								LOGD("mmitesthhl cur:%0x-%0x,last:%0x-%0x \n", cur_pos.x, cur_pos.y,last_pos.x, last_pos.y);
								firstdraw = 1;
							}
						}else{
							if((abs(cur_pos.x-std_point1.x) < (2*radius)) && (abs(cur_pos.y-std_point1.y) < (2*radius)) ){
								draw_point1 = cur_pos;
								flag1 = 1;
							}else if((abs(cur_pos.x-std_point2.x) < (2*radius)) && (abs(cur_pos.y-std_point2.y) < (2*radius)) ){
								draw_point2 = cur_pos;
								flag2 = 1;
							}
							if(flag1&&flag2){
								ui_draw_cricle(draw_point2,draw_point1);
								gr_flip();
								std_point1 = draw_point1;
								std_point2 = draw_point2;
								flag1 = 0 ;
								flag2 = 0 ;
								if( (firstdraw != 0) && ((abs(draw_point1.x-draw_point2.x) < (4*radius)) && (abs(draw_point1.y-draw_point2.y) < (8*radius))))
									ui_push_result(1);
								LOGD("mmitesthhl draw_point1:%0x-%0x,draw_point2:%0x-%0x \n", draw_point1.x, draw_point1.y,draw_point2.x, draw_point2.y);
							}
						}
					}
					last_pos=cur_pos;
				}
				break;
			default:
				sync_flag = 0;
		}
	}else if(event->type == EV_SYN){
		sync_flag = 0;
		switch(event->code){
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

static void* multitouch_thread()
{
	int ret;
	fd_set rfds;
	struct input_event event;
	struct timeval timeout;
	struct input_absinfo absinfo_x;
	struct input_absinfo absinfo_y;
	int fd = -1;

	if(strlen(TS_DEV_NAME)){
		fd = get_sensor_name(TS_DEV_NAME);
		//fd = find_input_dev(O_RDONLY, SPRD_TS_INPUT_DEV);
		if(fd < 0) {
			LOGD("mmitest open %s tp fail",TS_DEV_NAME);
			goto exit;
		}
	}else{
		LOGD("mmitest get tp sensor %s failed",TS_DEV_NAME);
		goto exit;
	}

	memset(&cur_pos, 0, sizeof(tp_pos));
	memset(&last_pos, 0, sizeof(tp_pos));

	if(ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), &absinfo_x)) {
		LOGE("mmitest can not get absinfo\n");
		goto exit;
	}

	if(ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), &absinfo_y)) {
		LOGE("mmitest can not get absinfo");
		goto exit;
	}
	tp_width = absinfo_x.maximum;
	tp_height = absinfo_y.maximum;
	LOGD("mmitest [%s]tp width=%d, height=%d", __func__, tp_width, tp_height);
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
					multitouch_handle_event(&event);

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

void ui_draw_cricle(tp_pos center1,tp_pos center2)
{
    int x, y;
    gr_color(33, 16, 28, 255);
    gr_fill(0, 0, gr_fb_width(), gr_fb_height());
    ui_fill_locked();
    ui_show_title(MENU_TEST_MULTI_TOUCH);
    ui_set_color(CL_GREEN);
    cur_row=ui_show_text(2, 0, TEXT_MULTI_TOUCH_START);

    ui_set_color(CL_YELLOW);
    gr_fill(center1.x-radius,center1.y-radius,center1.x+radius, center1.y+radius);
    gr_fill(center2.x-radius,center2.y-radius,center2.x+radius, center2.y+radius);
    /*
    for (y = center1.y-radius; y <= center1.y+radius; y++)
	for (x = center1.x-radius; x <= center1.x+radius; x++)
		if (((x-center1.x )* (x-center1.x )) + ((y-center1.y ) * (y-center1.y )) <= (radius * radius))
			gr_fill(x,y,x+1, y+1);

    for (y = center2.y-radius; y <= center2.y+radius; y++)
	for (x = center2.x-radius; x <= center2.x+radius; x++)
		if (((x-center2.x )* (x-center2.x )) + ((y-center2.y ) * (y-center2.y )) <= (radius * radius))
			gr_fill(x,y,x+1, y+1);
    */

    gr_flip();
}

int test_multi_touch_start(void)
{
	pthread_t t;
	int ret = 0;

	width = gr_fb_width();
	height = gr_fb_height();
	radius = height >>4;
	cur_row = 2;
	firstdraw = 0;
	tp_flag = 1;

	ui_fill_locked();
	ui_show_title(MENU_TEST_MULTI_TOUCH);
	ui_set_color(CL_GREEN);
	cur_row=ui_show_text(cur_row, 0, TEXT_MULTI_TOUCH_START);

	center1.x = width-(height >>3);
	center1.y = height/4;
	center2.x = height >>3;
	center2.y = height - (height >>3);
	ui_draw_cricle(center1,center2);
	std_point1 = center1;
	std_point2 = center2;

	thread_run=1;
	pthread_create(&t, NULL, (void*)multitouch_thread, NULL);
	ret = ui_handle_button(NULL,NULL,NULL);// NULL,
	thread_run=0;
	tp_flag = 0;
	pthread_join(t, NULL); /* wait "handle key" thread exit. */

	save_result(CASE_TEST_MULTITOUCH,ret);

	return ret;
}
