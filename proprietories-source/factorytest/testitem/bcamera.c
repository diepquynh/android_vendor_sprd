#include "testitem.h"

static void *sprd_handle_camera_dl;
extern char autotest_flag;
extern void gr_camera_flip(char flag);
extern int test_case_support(unsigned char id);

int open_flash_light(void)
{
	int ret = 0;
	ret = system("echo 0x30 > /sys/class/misc/sprd_flash/test") ? -1 : 0;		//for sprdroid 7.0
	if( -1 == ret )
		ret = system("echo 0x02 > /sys/devices/virtual/flash_test/flash_test/flash_value") ? -1 : 0;		//for sprdroid 6.0

	return ret;		// -1 error;    0 success
}

int close_flash_light(void)
{
	int ret = 0;
	ret = system("echo 0x31 > /sys/class/misc/sprd_flash/test") ? -1 : 0;		//for sprdroid 7.0
	if( -1 == ret )
		ret = system("echo 0x00 > /sys/devices/virtual/flash_test/flash_test/flash_value") ? -1 : 0;		//for sprdroid 6.0

	return ret;		// -1 error;    0 success
}

int test_bcamera_start(void)
{
	volatile int  rtn = RL_FAIL;
	char lib_full_name[60] = { 0 };
	char prop[PROPERTY_VALUE_MAX] = { 0 };
	minui_backend* backend_t = gr_backend_get();
	GRSurface* draw_t = gr_draw_get();

	LOGD("enter back camera test");
	ui_fill_locked();
	ui_show_title(MENU_TEST_BCAMERA);

	property_get("ro.board.platform", prop, NULL);
	snprintf(lib_full_name, sizeof(lib_full_name), "%scamera.%s.so", LIBRARY_PATH, prop);
	LOGD("mmitest %s",lib_full_name);
	sprd_handle_camera_dl = dlopen(lib_full_name,RTLD_NOW);
	if(sprd_handle_camera_dl == NULL)
	{
		LOGE("fail dlopen");
		rtn = RL_FAIL;
		goto go_exit;
	}

	typedef int (*pf_eng_tst_camera_init)(int32_t camera_id, minui_backend* backend, GRSurface* draw);
	typedef void (*pf_eng_tst_camera_deinit)(void);
	typedef void (*pf_eng_tst_camera_close)(void);
	typedef int (*pf_eng_tst_camera_capture)(int32_t camera_id);

	pf_eng_tst_camera_init eng_tst_camera_init = (pf_eng_tst_camera_init)dlsym(sprd_handle_camera_dl,"eng_tst_camera_init" );
	pf_eng_tst_camera_deinit eng_tst_camera_deinit = (pf_eng_tst_camera_deinit)dlsym(sprd_handle_camera_dl,"eng_tst_camera_deinit" );
	pf_eng_tst_camera_capture eng_camera_capture = (pf_eng_tst_camera_capture)dlsym(sprd_handle_camera_dl,"eng_camera_capture" );
	pf_eng_tst_camera_close eng_camera_close = (pf_eng_tst_camera_close)dlsym(sprd_handle_camera_dl,"eng_test_camera_close" );

go_bcamera_start:
	if(eng_tst_camera_init)
	{
		if(eng_tst_camera_init(0, backend_t, draw_t))   //init back camera and start preview
		{
			LOGE(" fail to call eng_test_camera_init");
			rtn = RL_FAIL;
		}
	}
	else
	{
		LOGE("fail to find eng_test_camera_init()");
		rtn = RL_FAIL;
		goto go_exit;
	}
	LOGD("open flash success.");
	if(test_case_support(CASE_TEST_FLASH))
		open_flash_light();		// open flashlight

	rtn = ui_handle_button(TEXT_PASS,TEXT_CAPTURE,TEXT_FAIL);//TEXT_GOBACK
	gr_flip();
	if(test_case_support(CASE_TEST_FLASH))
		close_flash_light();		// open flashlight

	if(eng_tst_camera_deinit)
		eng_tst_camera_deinit();   //init back camera and start preview
	else
		LOGD("fail to find eng_test_camera_close");

	if(RL_NEXT_PAGE == rtn)
	{
		LOGD("mmitest: switch to capture");
		sleep(1);
		if (eng_camera_capture)
			eng_camera_capture(0);
		usleep(100*1000);
		gr_flip();
		goto go_bcamera_start;
	}
	else if(RL_FAIL == rtn || RL_PASS == rtn)
	{
		if(eng_camera_close)
			eng_camera_close();

		gr_flip();
		goto go_exit;
	}
	else
	{
		if(eng_camera_close)
			eng_camera_close();

		gr_flip();
		return rtn;
	}

go_exit:
	gr_flip();
	save_result(CASE_TEST_BCAMERA,rtn);
	if(test_case_support(CASE_TEST_FLASH))
		save_result(CASE_TEST_FLASH,rtn);
	return rtn;
}
