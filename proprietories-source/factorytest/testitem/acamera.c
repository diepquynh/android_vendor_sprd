#include "testitem.h"

static void *sprd_handle_camera_dl;
extern char autotest_flag;
extern void gr_camera_flip(char flag);
extern int test_case_support(unsigned char id);

int test_acamera_start(void)
{
	volatile int  rtn = RL_FAIL;
	char lib_full_name[60] = { 0 };
	char prop[PROPERTY_VALUE_MAX] = { 0 };
	minui_backend* backend_t = gr_backend_get();
	GRSurface* draw_t = gr_draw_get();

	LOGD("enter auxiliary back camera test");
	ui_fill_locked();
	ui_show_title(MENU_TEST_ACAMERA);

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

	pf_eng_tst_camera_init eng_tst_camera_init = (pf_eng_tst_camera_init)dlsym(sprd_handle_camera_dl,"eng_tst_camera_init" );

	if(eng_tst_camera_init)
	{
		if(eng_tst_camera_init(2, backend_t, draw_t))   //init back camera and start preview
			LOGE(" fail to call eng_test_camera_init");
	}
	else
	{
		LOGE("fail to find eng_test_camera_init()");
		rtn = RL_FAIL;
		goto go_exit;
	}

	rtn = ui_handle_button(TEXT_PASS,NULL,TEXT_FAIL);//, TEXT_GOBACK

	typedef void (*pf_eng_tst_camera_deinit)(void);
	pf_eng_tst_camera_deinit eng_tst_camera_deinit = (pf_eng_tst_camera_deinit)dlsym(sprd_handle_camera_dl,"eng_tst_camera_deinit" );

	if(eng_tst_camera_deinit)
		eng_tst_camera_deinit();   //init back camera and start preview
	else
		LOGD("fail to find eng_test_camera_close");

	gr_flip();

go_exit:

	save_result(CASE_TEST_ACAMERA,rtn);
	return rtn;
}
