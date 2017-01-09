#include "testitem.h"

struct test_key key_info[8];
int key_cnt = 0;

/*
struct test_key test_key_info[8] = {
	{KEY_VOLUMEDOWN, 0,  TEXT_KEY_VOLUMEDOWN},
	{KEY_VOLUMEUP,	 0,  TEXT_KEY_VOLUMEUP},
	{KEY_POWER,		 0,  TEXT_KEY_POWER},
#ifndef SCXJIAOTU
	{0, 			 0,  TEXT_KEY_MENU},
	{KEY_BACK,		 0,  TEXT_KEY_BACK},
#ifdef SP7731
	{KEY_HOME,		 0,  TEXT_KEY_HOMEPAGE},
#else
	{KEY_HOMEPAGE,	 0,  TEXT_KEY_HOMEPAGE},
#endif
#if defined (SP9838) || defined (SP9830AEC)
	{KEY_CAMERA,	 0,  TEXT_KEY_CAMERA},
#endif
#endif
};
*/

int test_key_start(void)
{
	int ret;
	struct timespec ntime;
	ntime.tv_sec= time(NULL)+KEY_TIMEOUT;
	ntime.tv_nsec=0;
	int menu_count=0;
	int key = -1;
	int i = 0;
	int cur_row = 2;
	int count = 0;

	LOGD("mmitest start");
	ui_fill_locked();
	ui_show_title(MENU_TEST_KEY);

	ui_set_color(CL_GREEN);
	cur_row=ui_show_text(cur_row, 0, TEXT_KEY_ILLUSTRATE);

	for(i = 0; i < key_cnt; i++) {
		key_info[i].done = 0;
	}
	for(;;) {
		cur_row = 4;
		for(i = 0; i < key_cnt; i++) {
			if(key_info[i].done) {
				ui_set_color(CL_GREEN);
			} else {
				ui_set_color(CL_RED);
			}
			cur_row = ui_show_text(cur_row, 0, key_info[i].key_shown);
		}
		gr_flip();
		if((count >= key_cnt)) break;
		if(key==ETIMEDOUT) break;
		key = ui_wait_key(&ntime);
		LOGD("mmitest key = %d",key);
		for(i = 0; i < key_cnt; i++) {
			if((key_info[i].key == key)
				&&(key_info[i].done == 0))	{
				key_info[i].done = 1;
				count++;
			}
		}
		LOGD("mmitest count=%d",count);

	}

	LOGD("mmitest key over");

	if(key==ETIMEDOUT){
		ui_set_color(CL_RED);
		ui_show_text(cur_row+2, 0, TEXT_TEST_FAIL);
		gr_flip();
		sleep(1);
		ret=RL_FAIL;
	}else{
		ui_set_color(CL_GREEN);
		ui_show_text(cur_row+2, 0, TEXT_TEST_PASS);
		gr_flip();
		sleep(1);
		ret=RL_PASS;
	}
	save_result(CASE_TEST_KEY,ret);
	return ret;
}
