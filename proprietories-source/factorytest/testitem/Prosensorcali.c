#include "testitem.h"

static int cur_row = 2;
static int prosencali_result = RL_NA;
static int cali_result = -1;
extern int senloaded;

enum{
	NEAR	=	5,
	FAR	=	6,
};

static void prosensor_cali(int distant)
{
	int ret, row;
	int cali_fd;
	char write_buf[1024] = {0};
	char calibuf[128] = {0};

	snprintf(write_buf, sizeof(write_buf) - 1,"%d %d %d", CALIB_EN, SENSOR_TYPE_PROXIMITY, distant);
	ret = SenCaliCmd((const char *)write_buf);
	LOGD("sensor cali cmd write:%s, ret = %d", write_buf, ret);
	ui_set_color(CL_WHITE);
	row = ui_show_text(cur_row+1, 0, TEXT_SENSOR_CALI);
	gr_flip();
	sleep(4);
	snprintf(write_buf, sizeof(write_buf) - 1,"%d %d", CALIB_CHECK_STATUS, SENSOR_TYPE_PROXIMITY);
	ret = SenCaliCmd((const char *)write_buf);
	LOGD("sensor cali cmd write:%s, ret = %d", write_buf, ret);
	cali_fd = open(SENDATA,O_RDWR);
	if(cali_fd < 0){
		LOGE("open sensor cali data: %s faild",SENDATA);
		prosencali_result = RL_FAIL;
		goto end;
	}else{
		memset(calibuf, 0, sizeof(calibuf));
		ret = read(cali_fd, calibuf, sizeof(calibuf));
		if (ret <= 0) {
			LOGE("mmitest [fp:%d] read calibrator_data length error", cali_fd);
			prosencali_result = RL_FAIL;
			goto end;
		}
		LOGD("calibrator_data = %s; %d", calibuf, atoi(calibuf));
	}

	if (2 == atoi(calibuf)){
		snprintf(write_buf, sizeof(write_buf) - 1,"%d %d", CALIB_DATA_READ, SENSOR_TYPE_PROXIMITY);
		ret = SenCaliCmd((const char *)write_buf);
		LOGD("sensor cali cmd write:%s, ret = %d", write_buf, ret);
		cali_fd = open(SENDATA,O_RDWR);
		if(cali_fd < 0){
			LOGE("open sensor cali data: %s faild",SENDATA);
			prosencali_result = RL_FAIL;
			goto end;
		}else{
			memset(calibuf, 0, sizeof(calibuf));
			ret = read(cali_fd, calibuf, sizeof(calibuf));
			if (ret <= 0) {
				LOGE("mmitest [fp:%d] read calibrator_data length error", cali_fd);
				prosencali_result = RL_FAIL;
				goto end;
			}else if (!atoi(calibuf)){
				prosencali_result = RL_PASS;
			}else{
				prosencali_result = RL_FAIL;
			}
			LOGD("calibrator_data = %s; %d", calibuf, atoi(calibuf));
		}
	}else{
		prosencali_result = RL_FAIL;
		goto end;
	}

end:
	return;
}

static void *prosensorcali_thread()
{
	ui_set_color(CL_GREEN);
	ui_show_text(cur_row, 0, PRO_CALI_OPER2);
	gr_flip();
	do{
		prosencali_result = ui_handle_button(NULL, TEXT_START, NULL);
	}while(RL_PASS == prosencali_result);
	if(RL_NEXT_PAGE == prosencali_result){
		LOGD("mmitest prosensor cali 1 of 2 start !");
		prosensor_cali(NEAR);
		if(RL_FAIL == prosencali_result)
			goto end;
	} else {
		goto end;
	}
	LOGD("mmitest prosensor cali 2 or 2 ready to start !");
	ui_clear_rows(++cur_row, 1);
	ui_set_color(CL_GREEN);
	ui_show_text(cur_row, 0, PRO_CALI_OPER3);
	gr_flip();
	do{
		prosencali_result = ui_handle_button(NULL, TEXT_START, NULL);
	}while(RL_PASS == prosencali_result);
	if(RL_NEXT_PAGE == prosencali_result){
		LOGD("mmitest prosensor cali 2 of 2 start !");
		prosensor_cali(FAR);
	}
end:
	return NULL;
}

int cali_prosensor_start(void)
{
	const char *ptr = "Proximity Sensor";
	pthread_t thread;

	ui_fill_locked();
	ui_show_title(MENU_CALI_PROSOR);
	cur_row = 2;
	ui_set_color(CL_WHITE);
	cur_row = ui_show_text(cur_row, 0, TEXT_SENSOR_DEV_INFO);
//	cur_row = ui_show_text(cur_row, 0, BOARD_HAVE_ACC);
	cur_row = ui_show_text(cur_row, 0, ptr);
	gr_flip();
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
	pthread_create(&thread, NULL, (void*)prosensorcali_thread, NULL);
	pthread_join(thread, NULL); /* wait "handle key" thread exit. */

end:
	ui_clear_rows(++cur_row, 2);
	if(RL_PASS == prosencali_result){
		ui_set_color(CL_GREEN);
		ui_show_text(cur_row+1, 0, TEXT_CALI_PASS);
	}
	else if(RL_FAIL == prosencali_result){
		ui_set_color(CL_RED);
		ui_show_text(cur_row+1, 0, TEXT_CALI_FAIL);
	}else{
		ui_set_color(CL_WHITE);
		ui_show_text(cur_row+1, 0, TEXT_CALI_NA);
	}
	gr_flip();
	sleep(1);
	gr_flip();

	save_result(CASE_CALI_PROSOR, prosencali_result);
	return prosencali_result;
}
