#include "testitem.h"

static int thread_run=0;
static int row = 2;

typedef enum{
	led_blue = 0,
	led_red = 1,
	led_green = 2,
	color_num
}led_color;

#define LED_BLUE			"/sys/class/leds/blue/brightness"
#define LED_RED			       "/sys/class/leds/red/brightness"
#define LED_GREEN			"/sys/class/leds/green/brightness"
extern int text_cols;
extern unsigned cwidth,ewidth;

static int LedSetValue(void *name, int brightness)
{
	int fd;
	int ret;
	char *Led_name = name;
	char buffer[8];

	fd = open(Led_name, O_RDWR);

	if(fd < 0){
		LOGE("open %s failed! %d IN", Led_name, __LINE__);
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
	led_color loop = led_blue;
	char *ledn = LED_BLUE;
	char *title;
	UI_COLOR color = CL_BLUE;

	pos=(text_cols>>1)-strlen(TEXT_LED_BLUE)/3*cwidth/2/ewidth-1;
	LOGD("mmitest cwidth = %d,ewidth=%d,pos= %d",cwidth,ewidth,pos);
	usleep(500*1000);

	while(1 == thread_run) {
		switch(loop++%color_num){
			case led_blue:
				ledn = LED_BLUE;
				color = CL_BLUE;
				title = TEXT_LED_BLUE;
				LOGD("get led name: %s! loop = %d!", ledn, loop);
				break;
			case led_red:
				ledn = LED_RED;
				color = CL_RED;
				title = TEXT_LED_RED;
				LOGD("get led name: %s! loop = %d!", ledn, loop);
				break;
			case led_green:
				ledn = LED_GREEN;
				color = CL_GREEN;
				title = TEXT_LED_GREEN;
				LOGD("get led name: %s! loop = %d!", ledn, loop);
				break;
			default:
				LOGD("get led name error: %s! loop = %d!", ledn, loop);
				break;
		}
		LedSetValue(ledn, 200);
		ui_set_color(color);
		ui_show_text(6, pos, title);
		gr_flip();
		usleep(500*1000);
		LedSetValue(ledn, 0);
		ui_clear_rows(6, 1);
		gr_flip();
	}
}

int test_led_start(void)
{
	int ret;
	pthread_t thead;

	ui_fill_locked();
	ui_show_title(MENU_TEST_LED);
	ui_set_color(CL_GREEN);
	ui_show_text(row, 0, TEXT_LED_TIPS);
	gr_flip();

	LOGD("mmitest start");
	thread_run=1;
	pthread_create(&thead, NULL, (void*)led_thread, NULL);
	usleep(10*1000);
	ret = ui_handle_button(TEXT_PASS,NULL,TEXT_FAIL);
	thread_run=0;
	pthread_join(thead, NULL);

	save_result(CASE_TEST_LED,ret);
	return ret;
}
