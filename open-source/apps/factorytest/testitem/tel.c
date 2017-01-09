#include "testitem.h"

extern pthread_mutex_t tel_mutex;

int test_tel_start(void)
{
	int cur_row=2;
	int ret,fd;
	char property[PROPERTY_VALUE_MAX];
	char moemd_tel_port[BUF_LEN];

	ui_fill_locked();
	ui_show_title(MENU_TEST_TEL);
	ui_set_color(CL_GREEN);
	cur_row = ui_show_text(cur_row, 0, TEL_TEST_START);
	cur_row = ui_show_text(cur_row, 0, TEL_TEST_TIPS);
	gr_flip();

	property_get(PROP_MODEM_LTE_ENABLE, property, "not_find");
	if(!strcmp(property, "1"))
	    sprintf(moemd_tel_port,"/dev/stty_lte2");
	else
	    sprintf(moemd_tel_port,"/dev/stty_w2");
	LOGD("mmitest tel test %s",moemd_tel_port);
	fd=open(moemd_tel_port,O_RDWR);
	if(fd<0){
	    LOGE("mmitest tel test failed");
	    save_result(CASE_TEST_TEL,RL_FAIL);
	    return RL_FAIL;
	}
	pthread_mutex_lock(&tel_mutex);
	tel_send_at(fd,"AT+SFUN=2",NULL,0, 0);       //open sim card
	ret = tel_send_at(fd,"AT+SFUN=4",NULL,0, 100); //open protocol stack and wait 100s,if exceed more than 20s,we regard rregistering network fail
	pthread_mutex_unlock(&tel_mutex);
	if(ret < 0 ){
		ui_set_color(CL_RED);
		ui_show_text(cur_row,0,TEL_DIAL_FAIL);
		gr_flip();
		sleep(1);
		return ret;
	}

	ret=tel_send_at(fd, "ATD112@1,#;", NULL,0, 0);  //call 112
	cur_row = ui_show_text(cur_row, 0, TEL_DIAL_OVER);
	usleep(200*1000);
	tel_send_at(fd,"AT+SSAM=1",NULL,0,0);      //open speaker
	gr_flip();

	ret = ui_handle_button(TEXT_PASS, NULL,TEXT_FAIL);
	tel_send_at(fd,"ATH",NULL,0, 0);                //hang up
	tel_send_at(fd,"AT",NULL,0, 0);
	close(fd);
	save_result(CASE_TEST_TEL,ret);
	return ret;
}
