#include "testitem.h"

#define S_ON	1
#define S_OFF	0

static int thread_run;
static struct sensors_poll_device_t *device;
static struct sensors_module_t *module;
static struct sensor_t const *list;
static int count = 0;
static float magnetic_data[3];
static int col=5;
static int x_row,y_row,z_row;

static sensors_event_t data;


char const *getSensorName(int type)
{
	switch (type) {
	case SENSOR_TYPE_ACCELEROMETER:
		return "Acc";
	case SENSOR_TYPE_MAGNETIC_FIELD:
		return "Mag";
	case SENSOR_TYPE_ORIENTATION:
		return "Ori";
	case SENSOR_TYPE_GYROSCOPE:
		return "Gyr";
	case SENSOR_TYPE_LIGHT:
		return "Lux";
	case SENSOR_TYPE_PRESSURE:
		return "Bar";
	case SENSOR_TYPE_TEMPERATURE:
		return "Tmp";
	case SENSOR_TYPE_PROXIMITY:
		return "Prx";
	case SENSOR_TYPE_GRAVITY:
		return "Grv";
	case SENSOR_TYPE_LINEAR_ACCELERATION:
		return "Lac";
	case SENSOR_TYPE_ROTATION_VECTOR:
		return "Rot";
	case SENSOR_TYPE_RELATIVE_HUMIDITY:
		return "Hum";
	case SENSOR_TYPE_AMBIENT_TEMPERATURE:
		return "Tam";
	}
	return "ukn";
}


static int activate_sensors(int id, int delay, int opt)
{
	int err;
	err = device->activate(device, list[id].handle, 0);
	if (err != 0) {
		return 0;
	}
	if (!opt) {
		return 0;
	}
	err = device->activate(device, list[id].handle, 1);
	if (err != 0) {
		return 0;
	}
	device->setDelay(device, list[id].handle, ms2ns(delay));
	return err;
}

static void Msensor_show(void)
{
	int cur_row;
	char msxinfo[64];
	char msyinfo[64];
	char mszinfo[64];
	ui_clear_rows(x_row, 3);

	memset(msxinfo,0,strlen(msxinfo));
	memset(msyinfo,0,strlen(msyinfo));
	memset(mszinfo,0,strlen(mszinfo));
	sprintf(msxinfo,"%s  %5.1f",TEXT_MS_X,data.data[0]);
	sprintf(msyinfo,"%s  %5.1f",TEXT_MS_Y,data.data[1]);
	sprintf(mszinfo,"%s  %5.1f",TEXT_MS_Z,data.data[2]);
	ui_set_color(CL_GREEN);
	cur_row = ui_show_text(x_row, 0, msxinfo);
	cur_row = ui_show_text(cur_row, 0, msyinfo);
	cur_row = ui_show_text(cur_row, 0, mszinfo);
	gr_flip();
}



static int do_read(void)
{
	char onetime=1;
	int err;
	int opt,i,n;
	int type_num = 0;
	char *type = "Mag";
	int x_count=0,y_count=0,z_count=0;
	char x_flag=0,y_flag=0,z_flag=0;
	static const size_t numEvents = 16;
	sensors_event_t buffer[numEvents];
	sensors_event_t data_last={0};


	for (i = 1; i <= SENSOR_TYPE_AMBIENT_TEMPERATURE; i++) {
	if (strcmp(type, getSensorName(i)) == 0)
			type_num = i;
	}
	LOGD("mmitestsensor here tpye_num=%d",type_num);
	/*********activate_sensors(ON)***************/
	for (i = 0; i < count; i++) {
		err = activate_sensors(i, 1, S_ON);
		if (err != 0) {
			LOGD("mmitestsensor activate_sensors(ON) for '%s'failed",list[i].name);
			return 0;
		}
	}
	do {
		//LOGD("mmitest here while\n");
	    n = device->poll(device, buffer, numEvents);
		//LOGD("mmitest here afterpoll\n %d",n);
		if (n < 0) {
			//LOGD("mmitest poll() failed\n");
			break;
		}
		for (i = 0; i < n; i++) {
			data = buffer[i];

			if (data.version != sizeof(sensors_event_t)) {
				LOGD("mmitestsensor incorrect event version (version=%d, expected=%d",data.version, sizeof(sensors_event_t));
				break;
			}

			if (type_num == data.type){
				//LOGD("mmitest value=<%5.1f,%5.1f,%5.1f>\n",data.data[0], data.data[1], data.data[2]);
				Msensor_show();
				if(onetime&&data.data[0]&&data.data[1]&&data.data[2]){
					onetime=0;
					data_last=data;
				}
				if(abs(data_last.data[0]-data.data[0])>20){
					x_count++;
					//LOGD("mmitest X++");
				}
				if(abs(data_last.data[1]-data.data[1])>20){
					y_count++;
					//LOGD("mmitest y++");
				}
				if(abs(data_last.data[2]-data.data[2])>20){
					z_count++;
					//LOGD("mmitest z++");
				}
			}
		}

			if(z_count>=1&&y_count>=1&&x_count>=1)
				{
					//LOGD("mmitest pass and break");
					ui_push_result(RL_PASS);
					ui_set_color(CL_GREEN);//++++++++++
					ui_show_text(12, 0, TEXT_TEST_PASS);//++++++
					gr_flip();
					sleep(1);
					goto funend;
				}
	} while (1);

	/*********activate_sensors(OFF)***************/
funend:

	for (i = 0; i < count; i++) {
	err = activate_sensors(i, 0, S_OFF);
	if (err != 0) {
		LOGD("mmitestsensor activate_sensors(OFF) for '%s'failed",list[i].name);
		return 0;
		}
	}

	err = sensors_close(device);
	if (err != 0) {
		LOGD("mmitestsensor sensors_close() failed");
	}
	return err;
}

static void msensor_thread()
{
	int err,i;
	const char *ptr;
	int cur_row=2;
	col=5;
	//ui_set_color(CL_WHITE);
	//current_row=ui_show_text(current_row,0,"msensor test");
	//gr_flip();

	err=hw_get_module(SENSORS_HARDWARE_MODULE_ID,(hw_module_t const **)&module);
	if (err!= 0)
	{
		LOGD("mmitestsensor hw_get_module() failed");
		return ;
	}

	err = sensors_open(&module->common, &device);
	if (err != 0) {
		LOGD("mmitestsensor sensors_open() failed");
		return ;
	}
	count = module->get_sensors_list(module, &list);

	for(i=0;i<count;i++){
		LOGD("mmitestsensor here  %s",list[i].name);
	}
	ptr=list[1].name;
	ui_set_color(CL_WHITE);
	cur_row = ui_show_text(cur_row, 0, TEXT_SENSOR_DEV_INFO);
	cur_row = ui_show_text(cur_row, 0, ptr);
	cur_row = ui_show_text(cur_row+1, 0, TEXT_MS_OPER1);
	cur_row = ui_show_text(cur_row, 0, TEXT_MS_OPER2);
	x_row=cur_row;
	gr_flip();

	do_read();
	return;
}

int test_msensor_start(void)
{
	int ret=0;
	pthread_t thead;
	ui_fill_locked();
	ui_show_title(MENU_TEST_MSENSOR);

	LOGD("start");
	thread_run=1;
	pthread_create(&thead, NULL, (void*)msensor_thread, NULL);
	ret = ui_handle_button(TEXT_PASS,NULL,TEXT_FAIL);
	thread_run=0;

	pthread_join(thead, NULL);

	return ret;
}
