#include "testitem.h"

static int thread_run;
static float magnetic_data[3];
static int x_row;
static int sensor_id;
static sensors_event_t data;

time_t begin_time,over_time;

extern struct sensors_poll_device_t *device;
extern struct sensor_t const *list;
extern int senloaded;
static pthread_mutex_t msinfo_mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t msinfo_cond = PTHREAD_COND_INITIALIZER;
int success = 0;			//test success
char xyz=0;				//if X,Y,Z axis fit the result

static void *Msensor_show(void)
{
	int cur_row;
	char msxinfo[64] = "0.0";
	char msyinfo[64] = "0.0";
	char mszinfo[64] = "0.0";
	char disp_flag = 0;

	while(thread_run)
	{
		memset(msxinfo, 0, sizeof(msxinfo));
		memset(msyinfo, 0, sizeof(msyinfo));
		memset(mszinfo, 0, sizeof(mszinfo));
		pthread_mutex_lock(&msinfo_mtx);		//display info, lock another thread
		pthread_cond_wait(&msinfo_cond, &msinfo_mtx);//wait another thread produce data

		snprintf(msxinfo,sizeof(msxinfo), "%s  %5.1f", TEXT_MS_X, data.data[0]);
		snprintf(msyinfo,sizeof(msyinfo), "%s  %5.1f", TEXT_MS_Y, data.data[1]);
		snprintf(mszinfo,sizeof(mszinfo), "%s  %5.1f", TEXT_MS_Z, data.data[2]);
		disp_flag = xyz;

		pthread_mutex_unlock(&msinfo_mtx);		//get data success, unlock another thread
		LOGD("mmi test msensor data: <%s,%s,%s>\n",msxinfo, msyinfo, mszinfo);

		ui_clear_rows(x_row, 3);
		if(disp_flag & 0x01)
			ui_set_color(CL_GREEN);
		else
			ui_set_color(CL_WHITE);
		cur_row = ui_show_text(x_row, 0, msxinfo);

		if(disp_flag & 0x02)
			ui_set_color(CL_GREEN);
		else
			ui_set_color(CL_WHITE);
		cur_row = ui_show_text(cur_row, 0, msyinfo);

		if(disp_flag & 0x04)
			ui_set_color(CL_GREEN);
		else
			ui_set_color(CL_WHITE);
		cur_row = ui_show_text(cur_row, 0, mszinfo);
		gr_flip();
	}

	if(1 == success)
	{
		disp_flag = xyz;
		ui_clear_rows(x_row, 3);
		if(disp_flag & 0x01)
			ui_set_color(CL_GREEN);
		else
			ui_set_color(CL_WHITE);
		cur_row = ui_show_text(x_row, 0, msxinfo);

		if(disp_flag & 0x02)
			ui_set_color(CL_GREEN);
		else
			ui_set_color(CL_WHITE);
		cur_row = ui_show_text(cur_row, 0, msyinfo);

		if(disp_flag & 0x04)
			ui_set_color(CL_GREEN);
		else
			ui_set_color(CL_WHITE);
		cur_row = ui_show_text(cur_row, 0, mszinfo);

		ui_push_result(RL_PASS);
		ui_set_color(CL_GREEN);
		ui_show_text(12, 0, TEXT_TEST_PASS);
		gr_flip();
		LOGD("mmitest msensor test pass");
		sleep(1);
	}
	return NULL;
}

static void *msensor_thread()
{
	int i;
	int data_num = -1;
	int type = 0;
	unsigned char first_time = 1;
	static const size_t numEvents = 100;
	sensors_event_t buffer[numEvents];
	sensors_event_t first_data = {0};
	pthread_detach(pthread_self());

	type = list[sensor_id].type;
	LOGD("activate sensor: %s success! type = %d.", list[sensor_id].name, type);

	do
	{
		if(first_time)		//at the first time, wait 1 sec and discard time data
		{
			sleep(1);
			data_num = device->poll(device, buffer, numEvents);
			LOGD("mmitest: discard the first time data, data_num = %d", data_num);
		}

		data_num = device->poll(device, buffer, numEvents);
		LOGD("device poll data_num = %d",data_num);
		if (data_num < 0)
		{
			LOGD("device poll failed!\n");
			break;
		}

		pthread_mutex_lock(&msinfo_mtx);		//add lock
		for (i = 0; i < data_num; i++)
		{
			data = buffer[i];

			if (type == data.type)
			{
				if(first_time)
				{
					first_data = data;
					first_time = 0;
					continue;
				}

				if(abs(first_data.data[0]-data.data[0]) > 20)		// X axis fit the result
					xyz = xyz | 0x01;

				if(abs(first_data.data[1]-data.data[1]) > 20)		// Y axis fit the result
					xyz = xyz | 0x02;

				if(abs(first_data.data[2]-data.data[2]) > 20)		// Z axis fit the result
					xyz = xyz | 0x04;
			}
		}
		pthread_mutex_unlock(&msinfo_mtx);
		pthread_cond_signal(&msinfo_cond);

		if(xyz == 7)		// X, Y, Z axis fit the result, success and exit thread
		{
			success = 1;
			thread_run = 0;
			break;
		}
	}
	while (thread_run);

	return NULL;
}

int test_msensor_start(void)
{
	int ret = 0;
	const char *ptr = "Magnetic field Sensor";
	pthread_t t_Produce_data;
	pthread_t t_Msensor_show;
	int cur_row = 2;

	ui_fill_locked();
	ui_show_title(MENU_TEST_MSENSOR);
	ui_set_color(CL_WHITE);

	begin_time=time(NULL);
	cur_row = ui_show_text(cur_row, 0, TEXT_SENSOR_DEV_INFO);
	cur_row = ui_show_text(cur_row, 0, ptr);
	if(senloaded < 0)
	{
		cur_row = ui_show_text(cur_row, 0, TEXT_SENSOR_OPEN);
		gr_flip();
		senloaded = sensor_load();
		ui_clear_rows((cur_row-1), 1);
		ui_set_color(CL_WHITE);
	}
	else
		cur_row++;

	cur_row = ui_show_text(cur_row, 0, TEXT_MS_OPER1);
	cur_row = ui_show_text(cur_row, 0, TEXT_MS_OPER2);
	x_row=cur_row;
	gr_flip();

	if(senloaded < 0)
	{
		ui_set_color(CL_RED);
		ui_show_text(cur_row, 0, TEXT_SENSOR_OPEN_FAIL);
		gr_flip();
		sleep(1);
		goto end;
	}
	else
	{
		//enable msensor
		sensor_id = sensor_enable(ptr);
		LOGD("test msensor ID is %d~", sensor_id);
		if(sensor_id < 0)
		{
			ui_show_text(cur_row, 0, TEXT_SENSOR_OPEN_FAIL);
			goto end;
		}

		thread_run = 1;
		xyz=0;			//init value
		success = 0;		//init value
		pthread_create(&t_Produce_data, NULL, (void*)msensor_thread, NULL);
		pthread_create(&t_Msensor_show, NULL, (void*)Msensor_show, NULL);
		ret = ui_handle_button(NULL,NULL,NULL);//TEXT_GOBACK
		thread_run = 0;
		pthread_join(t_Produce_data, NULL);				// wait thread exit.
		pthread_join(t_Msensor_show, NULL);			//wait thread exit.

		sensor_disable(ptr);
	}

end:
	gr_flip();
	save_result(CASE_TEST_MAGSOR,ret);
	over_time=time(NULL);
	LOGD("mmitest casetime msensor is %ld s",(over_time-begin_time));
	return ret;
}
