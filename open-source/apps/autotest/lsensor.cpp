//#include "testitem.h"
//------------------------------------------------------------------------------
// enable or disable local debug
#define DBG_ENABLE_DBGMSG
#define DBG_ENABLE_WRNMSG
#define DBG_ENABLE_ERRMSG
#define DBG_ENABLE_INFMSG
#define DBG_ENABLE_FUNINF
#include "debug.h"
//------------------------------------------------------------------------------
#include "sensor.h"
#include "input.h"
#include "key_common.h"
#define _LSENSOR_TIMEOUT 20

static int thread_run;
static int proximity_value=1;
static int proximity_modifies=0;
static int light_value=0;
static int light_pass=0;
static int lpsensor_result=RESULT_FAIL;//// by yuebao

static void lsensor_show(void)
{
	int row = 3;
	char buf[64];

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

static int lsensor_enable(int enable)
{
	int fd;
	int ret = -1;

	LOGD("%s   enable=%d\n",__FUNCTION__,enable);
	fd = open(SPRD_PLS_CTL, O_RDWR);
	if(fd < 0 && 0 == strcmp(SPRD_PLS_CTL, "/dev/ltr_558als")) {
		/*try epl */
		fd = open("/dev/epl2182_pls", O_RDWR);
	}
	if(fd < 0) {
		LOGD("[%s]:open %s fail\n", __FUNCTION__, SPRD_PLS_CTL);
		return -1;
	}

	if(fd > 0) {
		if(ioctl(fd, LTR_IOCTL_SET_LFLAG, &enable) < 0) {
			LOGD("[%s]:set lflag %d fail, err:%s\n", __FUNCTION__, enable, strerror(errno));
			ret = -1;
		}
		if(ioctl(fd, LTR_IOCTL_SET_PFLAG, &enable) < 0) {
			LOGD("[%s]:set pflag %d fail, err:%s\n", __FUNCTION__, enable, strerror(errno));
			ret = -1;
		}
		close(fd);
	}

	return ret;
}

static void *lsensor_thread(void *param)
{
	int fd = -1;
	fd_set rfds;
	int cur_row = 3;
	time_t start_time,now_time;
	struct input_event ev;
	struct timeval timeout;
	int ret;
	int count=0;
	LOGD("autotest lsensor=%s\n",SPRD_PLS_INPUT_DEV);
	fd = find_input_dev(O_RDONLY, SPRD_PLS_INPUT_DEV);
	if(fd < 0 && 0 == strcmp(SPRD_PLS_INPUT_DEV, "alps_pxy")) {
		/* try proximity*/
		fd = find_input_dev(O_RDONLY, "proximity");
	}
	if(fd < 0) {
		ui_set_color(CL_RED);
		ui_show_text(cur_row, 0, TEXT_OPEN_DEV_FAIL);
		gr_flip();
		return NULL;
	}
	LOGD("autotest lsensor  thread_run=%d\n",thread_run);

	lsensor_enable(1);
	start_time=time(NULL);
	while(thread_run == 1) {
		now_time=time(NULL);


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
						case ABS_DISTANCE:
							proximity_modifies++;
							proximity_value = ev.value;
							LOGD("P:%d\n", ev.value);
							lsensor_show();
							break;
						case ABS_MISC:
							LOGD("L:%d\n", ev.value);
							if(light_value!=ev.value)
								count++;
							if(count>=2)
								light_pass = 1;
							light_value = ev.value;
							lsensor_show();
							break;
					}
				}
			}
		}
		LOGD("autotest   light_pass=%d,proximity_modifies=%d\n",light_pass,proximity_modifies);
		if((light_pass == 1 && proximity_modifies > 1))
		{
			lpsensor_result=RESULT_PASS;
//			ui_push_result(RL_PASS);
			ui_set_color(CL_GREEN);
			ui_show_text(5, 0, TEXT_TEST_PASS);
			gr_flip();
			goto func_end;
		}
		if ((now_time-start_time)>LSENSOR_TIMEOUT)
		{
		LOGD("autotest   %s:timeout LSENSOR_TIMEOUT=%s\n",__func__,LSENSOR_TIMEOUT);
//			ui_push_result(RL_PASS);
			ui_set_color(CL_BLUE);
			ui_show_text(5, 0, TEXT_TEST_TIMEOUT);
			gr_flip();
			goto func_end;
		}
	}
func_end:
	thread_run = 0;
	lsensor_enable(0);
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
	lpsensor_result=RESULT_FAIL;

       INFMSG("  yuebao %s:\n", __func__);

	ui_fill_locked();
	ui_show_title(MENU_TEST_LSENSOR);
	lsensor_show();
	thread_run = 1;
         INFMSG("  yuebao %s:thread_run=%d\n", __func__,thread_run);
	pthread_create(&thread, NULL, lsensor_thread, NULL);
//	ret = ui_handle_button(NULL, NULL);//, TEXT_GOBACK
	ui_clear_key_queue();
	ret = ui_wait_key_sec(LSENSOR_TIMEOUT);
	thread_run = 0;
	pthread_join(thread, NULL); /* wait "handle key" thread exit. */
//	save_result(CASE_TEST_LSENSOR,ret);
	INFMSG("lsensor, key ret=0x%X\n", ret);
	switch (ret)
	{
		case 0x72:/*v-,pass*/
		lpsensor_result = RESULT_PASS;
		break;
		case 0x73:/*v+,retry*/
		lpsensor_result = RESULT_AGAIN;
		break;
		case 0x74:/*power,fail*/
		lpsensor_result = RESULT_FAIL;
		break;
	}
	switch (lpsensor_result)
	{
		case RESULT_PASS:
		ui_set_color(CL_GREEN);//++++++++++
		ui_show_text(5, 0, TEXT_TEST_PASS);//++++++
		break;
		case RESULT_AGAIN:
		ui_set_color(CL_WHITE);//+++++++++++
		ui_show_text(5, 0, TEXT_TEST_NA);//+++++++++++
		break;
		default:
		ui_set_color(CL_RED);//+++++++++++
		ui_show_text(5, 0, TEXT_TEST_FAIL);//+++++++++
	}
	gr_flip();
	return lpsensor_result;
}

