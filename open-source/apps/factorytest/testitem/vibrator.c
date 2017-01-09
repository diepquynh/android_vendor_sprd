#include "testitem.h"


int enable_vibrator(int ms)
{
	int fd;
	int ret;
	char buf[8];

	fd = open(VIBRATOR_ENABLE_DEV, O_RDWR);
	if(fd < 0) {
		LOGE("open %s failed", VIBRATOR_ENABLE_DEV);
		return -1;
	}

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%d", ms);
	ret = write(fd, buf, strlen(buf));

	close(fd);

	return 0;
}

int test_vibrator_start(void)
{
	int ret = 0;
	int row = 2;
	ui_fill_locked();
	ui_show_title(MENU_TEST_VIBRATOR);
	ui_set_color(CL_WHITE);
	row = ui_show_text(row, 0, TEXT_VIB_START);
	gr_flip();

	enable_vibrator(1500);
	usleep(1500*1000);

	ui_set_color(CL_GREEN);
	row = ui_show_text(row, 0, TEXT_VIB_FINISH);
	ui_show_text(row, 0, TEXT_FINISH);
	gr_flip();
	ret = ui_handle_button(TEXT_PASS, NULL,TEXT_FAIL);//, TEXT_GOBACK
	return ret;
}
