#include "testitem.h"

static int thread_run;
static int proximity_value=1;
static int proximity_modifies=0;
static int light_value=0;
static int light_pass=0;
static int cur_row = 2;
time_t begin_time,over_time;
struct sensors_poll_device_t *device;
struct sensors_module_t *module;
struct sensor_t const *list;
int type_num;

#define S_ON	1
#define S_OFF	0

extern int sensor_start();
extern int sensor_stop();
extern int sensor_enable();

static void lsensor_show()
{
	char buf[64];
	int row = cur_row;

	ui_clear_rows(row, 2);
	if(proximity_modifies >= 2) {
		ui_set_color(CL_GREEN);
	} else {
		ui_set_color(CL_RED);
	}

	if(proximity_value == 0){
		row = ui_show_text(row, 0, TEXT_PS_NEAR);
	} else {
		row = ui_show_text(row, 0, TEXT_PS_FAR);
	}

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%s %d", TEXT_LS_LUX, light_value);

	if(light_pass == 1) {
		ui_set_color(CL_GREEN);
	} else {
		ui_set_color(CL_RED);
	}
	ui_show_text(row, 0, buf);
	gr_flip();
}

static void *lsensor_thread()
{
	int max_fd = -1,fd = -1,fd_pro = -1,fd_lux = -1;
	fd_set rfds;
	time_t start_time,now_time;
	struct input_event ev;
	struct timeval timeout;
	int ret;
	int cnt=0;
	int err;

	begin_time=time(NULL);
	if(strlen(PXY_DEV_NAME)){
		fd_pro = get_sensor_name(PXY_DEV_NAME);
		if(fd_pro < 0) {
			ui_set_color(CL_RED);
			cur_row = ui_show_text(cur_row, 0, TEXT_SENSOR_OPEN_FAIL);
		}
	}else{
		LOGD("mmitest get pxy sensor %s failed",PXY_DEV_NAME);
	}

	if(strlen(LUX_DEV_NAME)){
		fd_lux = get_sensor_name(LUX_DEV_NAME);
		if(fd_lux < 0) {
			ui_set_color(CL_RED);
			cur_row = ui_show_text(cur_row, 0, TEXT_SENSOR_OPEN_FAIL);
		}
	}else{
		LOGD("mmitest get lux sensor %s failed",LUX_DEV_NAME);
	}

	while(thread_run == 1) {
		FD_ZERO(&rfds);
		FD_SET(fd_pro, &rfds);
		if(fd_lux >= 0 )
		    FD_SET(fd_lux, &rfds);
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		max_fd=fd_pro>fd_lux?fd_pro:fd_lux;
		ret = select(max_fd+1, &rfds, NULL, NULL, &timeout);
		if (ret > 0) {
		    if (FD_ISSET(fd_pro, &rfds)) {
		        fd = fd_pro;
		    } else if (FD_ISSET(fd_lux, &rfds)) {
		        fd = fd_lux;
		    }
		    ret = read(fd, &ev, sizeof(ev));
		    if(ret == sizeof(ev)){
		        if(ev.type == EV_ABS) {
					switch(ev.code){
						case ABS_DISTANCE:
							proximity_modifies++;
							proximity_value = ev.value;
							//LOGD("mmitest lsensor P:%d", ev.value);
							lsensor_show();
							break;
						case ABS_MISC:
							//LOGD("L:%d", ev.value);
							if(light_value!=ev.value)
								cnt++;
							if(cnt>=2)
								light_pass = 1;
							light_value = ev.value;
							lsensor_show();
							break;
					}
		        }
		    }
		}

		if((light_pass == 1 && proximity_modifies > 1)) //||(now_time-start_time)>LSENSOR_TIMEOUT
		{
			ui_push_result(RL_PASS);
			ui_set_color(CL_GREEN);
			ui_show_text(cur_row+2, 0, TEXT_TEST_PASS);
			gr_flip();
			sleep(1);
			goto func_end;
		}
	}
func_end:
        return NULL;
}

int test_lsensor_start(void)
{
	int ret = 0;
	pthread_t thread;
	proximity_value=1;
	proximity_modifies=0;
	light_value=0;
	light_pass=0;
	cur_row = 2;

	ui_fill_locked();
	ui_show_title(MENU_TEST_LSENSOR);
	ui_set_color(CL_WHITE);
	cur_row = ui_show_text(cur_row, 0, TEXT_SENSOR_DEV_INFO);
	cur_row = ui_show_text(cur_row, 0, BOARD_HAVE_ACC);
	cur_row++;
	cur_row = ui_show_text(cur_row, 0, TEXT_ACC_OPER);
	lsensor_show();
	
	//enable gsensor
	sensor_start();
	sensor_enable();
	thread_run = 1;
	pthread_create(&thread, NULL, (void*)lsensor_thread, NULL);
	ret = ui_handle_button(NULL,NULL,NULL);//, TEXT_GOBACK
	thread_run = 0;
	pthread_join(thread, NULL); /* wait "handle key" thread exit. */

	sensor_stop();
	save_result(CASE_TEST_LPSOR,ret);
	over_time=time(NULL);
	LOGD("mmitest casetime lsensor is %ld s",(over_time-begin_time));
	return ret;
}
