#include "testitem.h"

static int thread_run;
static float pressure_value=0.0;
static int pressure_pass=0;
static int cur_row = 2;
static int sensor_id;
static sensors_event_t data;

time_t begin_time,over_time;

extern struct sensors_poll_device_t *device;
extern struct sensor_t const *list;
extern int senloaded;

static void psensor_show()
{
	char buf[64];
	int row = cur_row;

	ui_clear_rows(row, 2);

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%s %5.1f hPa", TEXT_PrS_Value, pressure_value);

	if(pressure_pass == 1) {
		ui_set_color(CL_GREEN);
	} else {
		ui_set_color(CL_RED);
	}
	ui_show_text(row, 0, buf);
	gr_flip();
}

static void *psensor_thread()
{
	int cnt=0;
	int i,n;
	int type_num;
	static const size_t numEvents = 16;
	sensors_event_t buffer[numEvents];
	sensors_event_t data_last={0};

	type_num = list[sensor_id].type;
	LOGD("activate sensor success!!!: sensor name = %s, type_num = %d;", list[sensor_id].name, type_num);

	do {
		LOGD("mmitest here while\n!");
		n = device->poll(device, buffer, numEvents);
		LOGD("mmitest here afterpoll\n n = %d",n);
		if (n < 0) {
			LOGD("mmitest poll() failed\n");
			break;
		}
		for (i = 0; i < n; i++) {
			data = buffer[i];

//			if (data.version != sizeof(sensors_event_t)) {
				LOGD("mmitestsensor incorrect event version (version=%d, expected=%d)!!",data.version, sizeof(sensors_event_t));
//				break;
//			}
			LOGD("(type_num = %d, data.type = %d)!!", type_num, data.type);
			if (type_num == data.type){
				LOGD("mmitest pressure value=<%5.1f>\n",data.pressure);
				if((pressure_value != data.pressure) && (900 <= data.pressure) && (1100 >= data.pressure))
					cnt++;
				if(cnt>=1)
					pressure_pass = 1;
				pressure_value = data.pressure;
				psensor_show();
			}

		}

		if(pressure_pass == 1){
			ui_push_result(RL_PASS);
			ui_set_color(CL_GREEN);
			ui_show_text(cur_row+2, 0, TEXT_TEST_PASS);
			gr_flip();
			sleep(1);
			break;
		}
	} while (1 == thread_run);

	return NULL;
}

int test_psensor_start(void)
{
	int ret = 0;
	const char *ptr = "Pressure";
	pthread_t thread;
	cur_row = 2;

	ui_fill_locked();
	ui_show_title(MENU_TEST_PSENSOR);
	ui_set_color(CL_WHITE);

	begin_time=time(NULL);
	cur_row = ui_show_text(cur_row, 0, TEXT_SENSOR_DEV_INFO);
//	cur_row = ui_show_text(cur_row, 0, BOARD_HAVE_ACC);
//	cur_row = ui_show_text(cur_row, 0, SENSOR_HUB_LIGHT);
	if(senloaded < 0){
		cur_row = ui_show_text(cur_row, 0, TEXT_SENSOR_OPEN);
		gr_flip();
		senloaded = sensor_load();
		ui_clear_rows((cur_row-1), 1);
		ui_set_color(CL_WHITE);
	}else{
		cur_row++;
	}

	cur_row = ui_show_text(cur_row, 0, TEXT_PRS_OPER1);
	
	if(senloaded < 0){
		ui_set_color(CL_RED);
		ui_show_text(cur_row, 0, TEXT_SENSOR_OPEN_FAIL);
		gr_flip();
		sleep(1);
		goto end;
	}else{
		//enable pressure sensor
		sensor_id = sensor_enable(ptr);
		LOGD("test Psensor ID is %d~", sensor_id);
		if(sensor_id < 0){
			ui_show_text(cur_row, 0, TEXT_SENSOR_OPEN_FAIL);
			goto end;
		}
		thread_run = 1;
		pthread_create(&thread, NULL, (void*)psensor_thread, NULL);
		ret = ui_handle_button(NULL,NULL,NULL);//, TEXT_GOBACK
		thread_run = 0;
		pthread_join(thread, NULL); /* wait "handle key" thread exit. */

		sensor_disable(ptr);
	}

end:
	save_result(CASE_TEST_PRESSOR,ret);
	over_time=time(NULL);
	LOGD("mmitest casetime lsensor is %ld s",(over_time-begin_time));
	return ret;
}

