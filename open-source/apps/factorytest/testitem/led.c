#include "testitem.h"

static int thread_run=0;
static int row = 2;

#define LED_BLUE			"/sys/class/leds/blue/brightness"
#define LED_RED			       "/sys/class/leds/red/brightness"
#define LED_GREEN			"/sys/class/leds/green/brightness"
extern int text_cols;
extern unsigned cwidth,ewidth;

static int ledSetValue_blue(int brightness)
{
	int fd;
	int ret;
	char buffer[8];

	fd = open(LED_BLUE, O_RDWR);

	if(fd < 0) {
		LOGE("open %s fail", LED_BLUE);
		return -1;
	}

	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%d", brightness);
	ret = write(fd, buffer, strlen(buffer));

	close(fd);

	return 0;
}

static int ledSetValue_green(int brightness)
{
	int fd;
	int ret;
	char buffer[8];

	fd = open(LED_GREEN, O_RDWR);

	if(fd < 0) {
		LOGE("open %s fail",LED_GREEN);
		return -1;
	}

	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%d", brightness);
	ret = write(fd, buffer, strlen(buffer));

	close(fd);

	return 0;
}

static int ledSetValue_red(int brightness)
{
	int fd;
	int ret;
	char buffer[8];

	fd = open(LED_RED, O_RDWR);

	if(fd < 0) {
		LOGE("open %s fail", LED_RED);
		return -1;
	}

	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%d", brightness);
	ret = write(fd, buffer, strlen(buffer));

	close(fd);

	return 0;
}

static void led_thread()
{
	int pos = 0;

	pos=(text_cols>>1)-strlen(TEXT_LED_BLUE)/3*cwidth/2/ewidth-1;
	LOGD("mmitest cwidth = %d,ewidth=%d,pos= %d",cwidth,ewidth,pos);

	ui_set_color(CL_GREEN);
	ui_show_text(row, 0, TEXT_LED_TIPS);
	gr_flip();

	while(1 == thread_run) {
		ledSetValue_red(200);
		ui_set_color(CL_RED);
		ui_show_text(6, pos, TEXT_LED_RED);
		gr_flip();
		usleep(500*1000);
		ledSetValue_red(0);
		ui_clear_rows(6, 1);
		gr_flip();
		ledSetValue_green(200);
		ui_clear_rows(6, 1);
		ui_set_color(CL_RED);
		ui_show_text(6, pos, TEXT_LED_GREEN);
		gr_flip();
		usleep(500*1000);
		ledSetValue_green(0);
		ui_clear_rows(6, 1);
		gr_flip();
		ledSetValue_blue(200);
		ui_set_color(CL_RED);
		ui_show_text(6, pos,TEXT_LED_BLUE);
		gr_flip();
		usleep(500*1000);
		ledSetValue_blue(0);
		ui_clear_rows(6, 1);
		gr_flip();
	}
}

int test_led_start(void)
{
	int ret;
	pthread_t thead;

	thread_run=1;

	ui_fill_locked();
	ui_show_title(MENU_TEST_LED);

	LOGD("mmitest start");
	pthread_create(&thead, NULL, (void*)led_thread, NULL);
	usleep(10*1000);
	ret = ui_handle_button(TEXT_PASS,NULL,TEXT_FAIL);
	thread_run=0;
	pthread_join(thead, NULL);

	save_result(CASE_TEST_LED,ret);
	return ret;
}
