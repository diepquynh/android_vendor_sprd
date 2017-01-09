#include "testitem.h"

int test_rtc_start(void)
{
	time_t now,last;
	struct tm *now_time;
	struct timeval tv_start,tv_end;
	char data[64]={0},clock[64]={0};
	size_t time_use=0;
	int cur_row = 2,i = 0;
	int ret = RL_FAIL; //fail

	ui_fill_locked();
	ui_show_title(MENU_TEST_RTC);
	gr_flip();

	gettimeofday(&tv_start,NULL);
	last=time(NULL);
	LOGD("start gettimeofday time last is %ld",last);

	do{
		now=time(NULL);
		//LOGD("start gettimeofday now time is %d",now);
		now_time = localtime(&now);
		cur_row = 2;
		ui_clear_rows(cur_row, 3);
		ui_set_color(CL_GREEN);
		memset(data,0,sizeof(data));
		strftime(data,80,"Data:%Y/%m/%d",now_time);
		//LOGD("Local time is %s",data);
		cur_row = ui_show_text(cur_row, 0, data);
		memset(clock,0,sizeof(clock));
		strftime(clock,80,"Time:%I:%M:%S",now_time);
		//LOGD("Local time is %s",clock);
		cur_row = ui_show_text(cur_row+1, 0, clock);
		gr_flip();
		if(last!=now) i++;
		last = now;
	}while(i < 3);

	gettimeofday(&tv_end,NULL);
	time_use=tv_end.tv_sec*1000000+tv_end.tv_usec-tv_start.tv_sec*1000000-tv_start.tv_usec;
	LOGD("time_use time is %d",time_use);
	if(time_use-2000000 < 999999 && time_use-2000000 > 0){
		ui_set_color(CL_GREEN);
		cur_row = ui_show_text(cur_row+1, 0, TEXT_TEST_PASS);
		ret = RL_PASS;
		gr_flip();
		sleep(1);
	}else{
		ui_set_color(CL_RED);
		cur_row = ui_show_text(cur_row+1, 0, TEXT_TEST_FAIL);
		gr_flip();
		sleep(1);
	}

	save_result(CASE_TEST_RTC,ret);
	return ret;
}
