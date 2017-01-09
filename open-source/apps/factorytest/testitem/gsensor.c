#include "testitem.h"

static int cur_row = 2;
static int x_row, y_row, z_row;
static time_t begin_time,over_time;
int gsensor_result=0;
int x_pass, y_pass, z_pass;

extern int sensor_start();
extern int sensor_stop();
extern int sensor_enable();

static int gsensor_check(int data)
{
	int ret = -1;
	int divisor = 1000;

	int start_1 = SPRD_GSENSOR_1G-SPRD_GSENSOR_OFFSET;
	int end_1 = SPRD_GSENSOR_1G+SPRD_GSENSOR_OFFSET;
	int start_2 = -SPRD_GSENSOR_1G-SPRD_GSENSOR_OFFSET;
	int end_2 = -SPRD_GSENSOR_1G+SPRD_GSENSOR_OFFSET;

	if(data > 10000)
		data = data/divisor;
	if( ((start_1<data)&&(data<end_1))||
		((start_2<data)&&(data<end_2)) ){
		ret = 0;
	}

	return ret;
}

static void *gsensor_thread()
{
	time_t start_time,now_time;
	int fd;
	fd_set rfds;
	int counter;
	int col = 5;
	int data[3]={0};
	struct input_event ev;
	struct timeval timeout;
	int ret,err;
	int divisor=1;

	x_pass = y_pass = z_pass = 0;
	begin_time=time(NULL);

	if(strlen(ACC_DEV_NAME)){
		fd = get_sensor_name(ACC_DEV_NAME);
		if(fd < 0) {
			ui_set_color(CL_RED);
			cur_row = ui_show_text(cur_row, 0, TEXT_SENSOR_OPEN_FAIL);
		}
	}else{
		LOGD("mmitest get tp sensor %s failed",ACC_DEV_NAME);
	}

	start_time=time(NULL);
	gr_flip();
	while(!(x_pass&y_pass&z_pass)) {
		//get data
		now_time=time(NULL);
		ui_set_color(CL_GREEN);
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		ret = select(fd+1, &rfds, NULL, NULL, &timeout);
		if(FD_ISSET(fd, &rfds)) {
			ret = read(fd, &ev, sizeof(ev));
			if(ret == sizeof(ev)){
				if(ev.type == EV_ABS) {
					switch(ev.code){
						case ABS_X:
							data[0] = ev.value;
							//LOGD("mmitest gsensor x-data[0]:%d", ev.value);
							break;
						case ABS_Y:
							data[1] = ev.value;
							//LOGD("mmitest gsensor x-data[1]:%d", ev.value);
							break;
						case ABS_Z:
							data[2] = ev.value;
							//LOGD("mmitest gsensor x-data[2]:%d", ev.value);
							break;
					}
				}
			}
		}

		if(x_pass == 0 && gsensor_check(data[0]) == 0) {
			x_pass = 1;
			LOGD("x_pass");
			ui_show_text(x_row, col, TEXT_SENSOR_PASS);
			gr_flip();
		}

		if(y_pass == 0 && gsensor_check(data[1]) == 0) {
			y_pass = 1;
			LOGD("y_pass");
			ui_show_text(y_row, col, TEXT_SENSOR_PASS);
			gr_flip();
		}

		if(z_pass == 0 && gsensor_check(data[2]) == 0) {
			z_pass = 1;
			LOGD("z_pass");
			ui_show_text(z_row, col, TEXT_SENSOR_PASS);
			gr_flip();
		}
		usleep(2*1000);
		if((now_time-start_time)>=GSENSOR_TIMEOUT) break;
    }

    if(x_pass&y_pass&z_pass)
	gsensor_result = RL_PASS;
    else
	gsensor_result = RL_FAIL;
    return NULL;
}

int test_gsensor_start(void)
{
	pthread_t thread;
	int ret;
	int err;

	ui_fill_locked();
	ui_show_title(MENU_TEST_GSENSOR);
	cur_row = 2;
	ui_set_color(CL_WHITE);
	cur_row = ui_show_text(cur_row, 0, TEXT_SENSOR_DEV_INFO);
	cur_row = ui_show_text(cur_row, 0, BOARD_HAVE_ACC);
	cur_row = ui_show_text(cur_row+1, 0, TEXT_GS_OPER1);
	cur_row = ui_show_text(cur_row, 0, TEXT_GS_OPER2);
	ui_set_color(CL_GREEN);
	x_row = cur_row;
	cur_row = ui_show_text(cur_row, 0, TEXT_GS_X);
	y_row = cur_row;
	cur_row = ui_show_text(cur_row, 0, TEXT_GS_Y);
	z_row = cur_row;
	cur_row = ui_show_text(cur_row, 0, TEXT_GS_Z);
	gr_flip();
	//enable gsensor
	sensor_start();
	sensor_enable();
	pthread_create(&thread, NULL, (void*)gsensor_thread, NULL);
	pthread_join(thread, NULL); /* wait "handle key" thread exit. */

	if(RL_PASS == gsensor_result){
		ui_set_color(CL_GREEN);
		ui_show_text(cur_row, 0, TEXT_TEST_PASS);
	}
	else if(RL_FAIL == gsensor_result){
		ui_set_color(CL_RED);
		ui_show_text(cur_row, 0, TEXT_TEST_FAIL);
	}else{
		ui_set_color(CL_WHITE);
		ui_show_text(cur_row, 0, TEXT_TEST_NA);
	}
	gr_flip();

	sensor_stop();
	save_result(CASE_TEST_GSENSOR,gsensor_result);
	over_time=time(NULL);
	LOGD("mmitest casetime gsensor is %ld s",(over_time-begin_time));
	return gsensor_result;
}
