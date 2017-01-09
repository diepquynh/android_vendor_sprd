#include "testitem.h"
//#include "factorylib.h"

#define LIB_FINGERSOR_PATH "/system/lib/libfactorylib.so"
typedef void (*SLFACT_INF_IND_FUNCT_t)(char *opInfo);

void test_fingersor_start(void)
{
	int ret = RL_NA, re = -1, res = -1, spi = -1, inter = -1, dead = -1, bubble = -1, fin = -1;
	int row = 2;
	void *sprd_handle_fingersor_dl;
	typedef int (*fingersor_dl)(void);
	typedef int (*fingersor_dl1)(SLFACT_INF_IND_FUNCT_t inInfoIndFct);

	ui_fill_locked();
	ui_show_title(MENU_TEST_FINGERSOR);
	ui_set_color(CL_WHITE);
	ui_show_text(row, 0, TEXT_FINGERSOR_OPER);
	gr_flip();

	do {
		res = ui_handle_button(NULL, TEXT_START, NULL);
	} while (RL_PASS == res);
	ui_clear_rows(row, 1);
	if (RL_NEXT_PAGE == res) {
		ui_set_color(CL_GREEN);
		ui_show_text(row, 0, FINGER_INIT);
		gr_flip();
	} else {
		goto exit;
	}

	sprd_handle_fingersor_dl = dlopen(LIB_FINGERSOR_PATH, RTLD_NOW);
	if(sprd_handle_fingersor_dl == NULL){
		LOGE("fingersor lib dlopen failed! %s, %d IN\n", dlerror(), __LINE__);
		return;
	}
	LOGD("get spi test func; %d IN\n", __LINE__);
	fingersor_dl factory_init = (fingersor_dl)dlsym(sprd_handle_fingersor_dl, "factory_init");
	fingersor_dl factory_deinit = (fingersor_dl)dlsym(sprd_handle_fingersor_dl, "factory_deinit");
	fingersor_dl spi_test = (fingersor_dl)dlsym(sprd_handle_fingersor_dl, "spi_test");
	fingersor_dl interrupt_test = (fingersor_dl)dlsym(sprd_handle_fingersor_dl, "interrupt_test");
	fingersor_dl reset_test = (fingersor_dl)dlsym(sprd_handle_fingersor_dl, "reset_test");
	fingersor_dl1 deadpixel_test = (fingersor_dl1)dlsym(sprd_handle_fingersor_dl, "deadpixel_test");
	fingersor_dl bubble_test = (fingersor_dl)dlsym(sprd_handle_fingersor_dl, "bubble_test");
	fingersor_dl finger_detect = (fingersor_dl)dlsym(sprd_handle_fingersor_dl, "finger_detect");
	if(!(factory_init && factory_deinit && spi_test && interrupt_test && finger_detect)){
		ret = RL_FAIL;
        ui_clear_rows(row, 1);
		ui_set_color(CL_RED);
		ui_show_text(row, 0, FINGER_DLOPEN_FAIL);
		gr_flip();
		sleep(1);
		LOGE("could not find symbol, %d IN\n", __LINE__);
		goto end;
	}
	LOGD("get symbol success! %d IN\n", __LINE__);
	factory_init();
	LOGD("spi_test start! %d IN\n\n", __LINE__);
	spi = spi_test();
	LOGD("interrupt_test start! %d IN\n\n", __LINE__);
	inter = interrupt_test();
	LOGD("reset_test start! %d IN\n\n", __LINE__);
	re = reset_test();
	LOGD("deadpixel_test start! %d IN\n\n", __LINE__);
	dead = deadpixel_test(NULL);
	LOGD("bubble_test start! %d IN\n\n", __LINE__);
	bubble = bubble_test();
	LOGD("spi_test: %d! interrupt_test: %d! reset_test: %d! deadpixel_test: %d! bubble_test: %d! %d IN\n", spi, inter, re, dead, bubble, __LINE__);
	if(abs(spi) || abs(inter) || abs(re) || abs(dead) || abs(bubble)){
		ret = RL_FAIL;
        ui_clear_rows(row, 1);
		ui_set_color(CL_RED);
		ui_show_text(row, 0, FINGER_SENSOR_BAD);
		gr_flip();
		sleep(1);
		factory_deinit();
		LOGE("sensor has defect! %d IN\n", __LINE__);
		goto end;
	}

    ui_clear_rows(row, 1);
	ui_set_color(CL_GREEN);
	ui_show_text(row, 0, TEXT_FINGERSOR_OPER1);
	gr_flip();

	do{
		res = ui_handle_button(NULL, TEXT_START, NULL);
	}while(RL_PASS == res);
	ui_clear_rows(row, 1);
	if(RL_NEXT_PAGE == res){
		ui_set_color(CL_GREEN);
		row = ui_show_text(row, 0, TEXT_SOUNDTRIGGER_OPER4);
		gr_flip();
		fin = finger_detect();
		LOGD("finger detect value : %d , %d IN\n\n", fin, __LINE__);
		sleep(1);
		if(!fin)
			ret = RL_PASS;
		else
			ret = RL_FAIL;
	}
	factory_deinit();
exit:
	if(ret == RL_PASS) {
		ui_set_color(CL_GREEN);
		ui_show_text(row, 0, TEXT_TEST_PASS);
	} else {
		ui_set_color(CL_RED);
		ui_show_text(row, 0, TEXT_TEST_FAIL);
	}
	gr_flip();
	sleep(1);

end:
	save_result(CASE_TEST_FINGERSOR,ret);
	return;
}
