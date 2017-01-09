#include "testitem.h"

static void *sprd_handle_camera_dl;

int test_facamera_start(void)
{
	volatile int  rtn = RL_FAIL;
	char lib_full_name[60] = { 0 };
	char prop[PROPERTY_VALUE_MAX] = { 0 };
	minui_backend* backend_t = gr_backend_get();
	GRSurface* draw_t = gr_draw_get();

	LOGD("enter front camera test");
	ui_fill_locked();
	ui_show_title(MENU_TEST_FACAMERA);

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
	pf_eng_tst_camera_init eng_tst_camera_init = (pf_eng_tst_camera_init)dlsym(sprd_handle_camera_dl,"eng_tst_camera_init" );

	if(eng_tst_camera_init)
	{
		if(eng_tst_camera_init(3, backend_t, draw_t))   //init front camera and start preview
		{
			LOGE("mmitest grh fail to call eng_test_camera_init ");
			rtn = RL_FAIL;
			goto go_exit;
		}
	}

	LOGD("mmitest start preview with front camera");
	rtn = ui_handle_button(TEXT_PASS,NULL,TEXT_FAIL);//, TEXT_GOBACK

	typedef void (*pf_eng_tst_camera_deinit)(void);
	pf_eng_tst_camera_deinit eng_tst_camera_deinit = (pf_eng_tst_camera_deinit)dlsym(sprd_handle_camera_dl,"eng_tst_camera_deinit" );
	if(eng_tst_camera_deinit)
		eng_tst_camera_deinit();   //init back camera and start preview

	gr_flip();

go_exit:
	save_result(CASE_TEST_FACAMERA,rtn);
	return rtn;
}
