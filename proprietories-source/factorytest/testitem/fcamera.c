#include "testitem.h"

static void *sprd_handle_camera_dl;
static int eng_front_camera =0;
static int thread_run =0;
extern char autotest_flag;
extern void gr_camera_flip(char flag);

int test_fcamera_start(void)
{
	volatile int  rtn = RL_FAIL;
	char lib_full_name[60] = { 0 };
	char prop[PROPERTY_VALUE_MAX] = { 0 };
	minui_backend* backend_t = gr_backend_get();
	GRSurface* draw_t = gr_draw_get();

	LOGD("enter auxiliary front camera test");
	ui_fill_locked();
	ui_show_title(MENU_TEST_FCAMERA);

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

	LOGD("mmitest after open lib");
	typedef int (*pf_eng_tst_camera_init)(int32_t camera_id, minui_backend* backend, GRSurface* draw);
	typedef void (*pf_eng_tst_camera_deinit)(void);
	typedef void (*pf_eng_tst_camera_close)(void);
	typedef int (*pf_eng_tst_camera_capture)(int32_t camera_id);

	pf_eng_tst_camera_init eng_tst_camera_init = (pf_eng_tst_camera_init)dlsym(sprd_handle_camera_dl,"eng_tst_camera_init" );
	pf_eng_tst_camera_deinit eng_tst_camera_deinit = (pf_eng_tst_camera_deinit)dlsym(sprd_handle_camera_dl,"eng_tst_camera_deinit" );
	pf_eng_tst_camera_capture eng_camera_capture = (pf_eng_tst_camera_capture)dlsym(sprd_handle_camera_dl,"eng_camera_capture" );
	pf_eng_tst_camera_close eng_camera_close = (pf_eng_tst_camera_close)dlsym(sprd_handle_camera_dl,"eng_test_camera_close" );

go_fcamera_start:
	if(eng_tst_camera_init)
	{
		if(eng_tst_camera_init(1, backend_t, draw_t))   //init front camera and start preview
		{
			LOGE("fail to call eng_tst_camera_init ");
			rtn = RL_FAIL;
		}
	}
	else
	{
		LOGE("fail to find eng_tst_camera_init()");
		rtn = RL_FAIL;
		goto go_exit;
	}

	LOGD("mmitest start preview with front camera");
	rtn = ui_handle_button(TEXT_PASS,TEXT_CAPTURE,TEXT_FAIL);//, TEXT_GOBACK
	gr_flip();

	if(eng_tst_camera_deinit)   //init back camera and start preview
		eng_tst_camera_deinit();
	else
		LOGD("fail to find eng_tst_camera_deinit");

	if(RL_NEXT_PAGE == rtn)
	{
		LOGD("mmi test: switch to capture");
		sleep(1);
		if (eng_camera_capture)
			eng_camera_capture(0);

		usleep(100*1000);
		gr_flip();
		goto go_fcamera_start;
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
	save_result(CASE_TEST_FCAMERA,rtn);
	return rtn;
}
