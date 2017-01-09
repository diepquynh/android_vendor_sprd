#include "testitem.h"

static int thread_run=0;
static int cur_row = 2;
static int x_row;
static time_t begin_time,over_time;
static int sensor_id;
static sensors_event_t data;
static int asencali_result=0;

extern struct sensors_poll_device_t *device;
extern struct sensor_t const *list;
extern int senloaded;

int SenCaliCmd(const char * cmd)
{
    int fd = -1;
    int ret = -1;
    int bytes_to_write = strlen(cmd);

    if(cmd == NULL){
        return -1;
    }

    if(fd < 0) {
        fd = open(SENCALI, O_WRONLY | O_NONBLOCK);
    }

    if(fd < 0) {
        return -1;
    }else{
        ret = write(fd, cmd, bytes_to_write);
        if( ret != bytes_to_write) {
			LOGD("cali_write_cmd ret is %d, byte is %d", ret, bytes_to_write);
			return -1;
        }
    }

    if(fd > 0) {
        close(fd);
    }

    return 0;
}

static void *asensorcali_thread()
{
	int i,n;
	int type_num = 0;
	static const size_t numEvents = 16;
	sensors_event_t buffer[numEvents];
	char asinfo[64];

	begin_time=time(NULL);

	type_num = list[sensor_id].type;
	LOGD("activate sensor: %s success!!! type_num = %d.", list[sensor_id].name, type_num);

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
//#if 0
//			if (data.version != sizeof(sensors_event_t)) {
				LOGD("mmitestsensor incorrect event version (version=%d, expected=%d)!!",data.version, sizeof(sensors_event_t));
//				break;
//			}
//#endif
			if (type_num == data.type){
				LOGD("mmitest value=<%5.1f,%5.1f,%5.1f>\n",data.data[0], data.data[1], data.data[2]);
				ui_clear_rows(x_row, 3);
				ui_set_color(CL_GREEN);
				memset(asinfo,0,strlen(asinfo));
				sprintf(asinfo,"%s %5.1f", TEXT_GS_X, data.data[0]);
				cur_row = ui_show_text(x_row, 0, asinfo);
				memset(asinfo,0,strlen(asinfo));
				sprintf(asinfo,"%s %5.1f", TEXT_GS_Y, data.data[1]);
				cur_row = ui_show_text(cur_row, 0, asinfo);
				memset(asinfo,0,strlen(asinfo));
				sprintf(asinfo,"%s %5.1f", TEXT_GS_Z, data.data[2]);
				cur_row = ui_show_text(cur_row, 0, asinfo);
				gr_flip();
				usleep(2*1000);
			}
		}

	} while (thread_run);

    return NULL;
}

int cali_asensor_start(void)
{
	pthread_t thread;
	const char *ptr = "Accelerometer Sensor";
	int ret;
	int err;
	int cali_fd;
	char write_buf[1024] = {0};
	char calibuf[128] = {0};

	ui_fill_locked();
	ui_show_title(MENU_CALI_ACCSOR);
	cur_row = 2;
	ui_set_color(CL_WHITE);
	cur_row = ui_show_text(cur_row, 0, TEXT_SENSOR_DEV_INFO);
//	cur_row = ui_show_text(cur_row, 0, BOARD_HAVE_ACC);
	cur_row = ui_show_text(cur_row, 0, ptr);

	if(senloaded < 0){
		cur_row = ui_show_text(cur_row, 0, TEXT_SENSOR_OPEN);
		gr_flip();
		senloaded = sensor_load();
		ui_clear_rows((cur_row-1), 1);
	}else{
		cur_row++;
	}

	if(senloaded < 0){
		ui_set_color(CL_RED);
		ui_show_text(cur_row, 0, TEXT_SENSOR_OPEN_FAIL);
		gr_flip();
		sleep(1);
		goto end;
	}

	ui_set_color(CL_GREEN);
	cur_row = ui_show_text(cur_row, 0, AS_CALI_OPER1);
	x_row = cur_row+1;
	gr_flip();

	snprintf(write_buf, sizeof(write_buf) - 1,"%d %d 1", CALIB_EN, SENSOR_TYPE_ACCELEROMETER);
	ret = SenCaliCmd((const char *)write_buf);
	LOGD("sensor cali cmd write:%s, ret = %d", write_buf, ret);
	//enable gsensor
	sensor_id = sensor_enable(ptr);
	LOGD("test Asensor ID is %d~", sensor_id);
	if(sensor_id < 0){
		ui_show_text(cur_row, 0, TEXT_SENSOR_OPEN_FAIL);
		goto end;
	}

	thread_run=1;
	pthread_create(&thread, NULL, (void*)asensorcali_thread, NULL);
	sleep(4);
	snprintf(write_buf, sizeof(write_buf) - 1,"%d %d 1", CALIB_CHECK_STATUS, SENSOR_TYPE_ACCELEROMETER);
	ret = SenCaliCmd((const char *)write_buf);
	LOGD("sensor cali cmd write:%s, ret = %d", write_buf, ret);
	cali_fd = open(SENDATA,O_RDWR);
	if(cali_fd < 0){
		LOGE("open sensor cali data: %s faild",SENDATA);
		asencali_result = RL_FAIL;
		goto end;
	}else{
		memset(calibuf, 0, sizeof(calibuf));
		ret = read(cali_fd, calibuf, sizeof(calibuf));
		if (ret <= 0) {
			LOGE("mmitest [fp:%d] read calibrator_data length error", cali_fd);
			asencali_result = RL_FAIL;
			goto end;
		}
		LOGD("calibrator_data = %s; %d", calibuf, atoi(calibuf));
	}

	if (2 == atoi(calibuf)){
		snprintf(write_buf, sizeof(write_buf) - 1,"%d %d 1", CALIB_DATA_READ, SENSOR_TYPE_ACCELEROMETER);
		ret = SenCaliCmd((const char *)write_buf);
		LOGD("sensor cali cmd write:%s, ret = %d", write_buf, ret);
		cali_fd = open(SENDATA,O_RDWR);
		if(cali_fd < 0){
			LOGE("open sensor cali data: %s faild",SENDATA);
			asencali_result = RL_FAIL;
			goto end;
		}else{
			memset(calibuf, 0, sizeof(calibuf));
			ret = read(cali_fd, calibuf, sizeof(calibuf));
			if (ret <= 0) {
				LOGE("mmitest [fp:%d] read calibrator_data length error", cali_fd);
				asencali_result = RL_FAIL;
				goto end;
			}
			LOGD("calibrator_data = %s; %d", calibuf, atoi(calibuf));
		}
	}else{
		asencali_result = RL_FAIL;
		goto end;
	}

	if (!(atoi(calibuf))){
		asencali_result = RL_PASS;
	}else{
		asencali_result = RL_FAIL;
	}

end:
	thread_run = 0;
	pthread_join(thread, NULL); /* wait "handle key" thread exit. */
	if(RL_PASS == asencali_result){
		ui_set_color(CL_GREEN);
		ui_show_text(cur_row+1, 0, TEXT_CALI_PASS);
	}
	else if(RL_FAIL == asencali_result){
		ui_set_color(CL_RED);
		ui_show_text(cur_row+1, 0, TEXT_CALI_FAIL);
	}else{
		ui_set_color(CL_WHITE);
		ui_show_text(cur_row+1, 0, TEXT_CALI_NA);
	}
	gr_flip();
	sleep(1);
	sensor_disable(ptr);
	gr_flip();

	save_result(CASE_CALI_ACCSOR,asencali_result);
	over_time=time(NULL);
	LOGD("mmitest casetime gsensor is %ld s",(over_time-begin_time));
	return asencali_result;
}

