/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <linux/input.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/reboot.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/rtc.h>
#include <cutils/properties.h>

#include "common.h"
#include "minui/minui.h"
#include "recovery_ui.h"
#include "battery.h"
#include <errno.h>

#define MAX_COLS 64
#define MAX_ROWS 32

#define CHAR_WIDTH 10
#define CHAR_HEIGHT 18

static pthread_mutex_t gUpdateMutex = PTHREAD_MUTEX_INITIALIZER;
static gr_surface gBackgroundIcon[NUM_BACKGROUND_ICONS];
static gr_surface gProgressBarIndeterminate[PROGRESSBAR_INDETERMINATE_STATES];
static gr_surface gProgressBarEmpty;
static gr_surface gProgressBarFill;
static gr_surface gBootLogo;
int alarm_flag_check(void);

static const struct { gr_surface* surface; const char *name; } BITMAPS[] = {
	{ &gProgressBarIndeterminate[0],    "indeterminate0" },
	{ &gProgressBarIndeterminate[1],    "indeterminate1" },
	{ &gProgressBarIndeterminate[2],    "indeterminate2" },
	{ &gProgressBarIndeterminate[3],    "indeterminate3" },
	{ &gProgressBarIndeterminate[4],    "indeterminate4" },
	{ &gProgressBarIndeterminate[5],    "indeterminate5" },
	{ &gProgressBarIndeterminate[6],    "indeterminate6" },
	{ &gProgressBarEmpty,               "indeterminate0" },
	{ &gProgressBarFill,                "indeterminate6" },
	{ NULL,                             NULL },
};

static gr_surface gCurrentIcon = NULL;

static enum ProgressBarType {
	PROGRESSBAR_TYPE_NONE,
		PROGRESSBAR_TYPE_INDETERMINATE,
		PROGRESSBAR_TYPE_NORMAL,
} gProgressBarType = PROGRESSBAR_TYPE_NONE;

// Progress bar scope of current operation
static float gProgressScopeStart = 0, gProgressScopeSize = 0, gProgress = 0;
static time_t gProgressScopeTime, gProgressScopeDuration;

// Set to 1 when both graphics pages are the same (except for the progress bar)
static int gPagesIdentical = 0;

// Log text overlay, displayed when a magic key is pressed
static char text[MAX_ROWS][MAX_COLS];
static int text_cols = 0, text_rows = 0;
static int text_col = 0, text_row = 0, text_top = 0;
static int show_text = 0;

static char menu[MAX_ROWS][MAX_COLS];
static int show_menu = 0;
static int menu_top = 0, menu_items = 0, menu_sel = 0;

// Key event input queue
static pthread_mutex_t key_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t key_queue_cond = PTHREAD_COND_INITIALIZER;
static int key_queue[256], key_queue_len = 0;
static volatile char key_pressed[KEY_MAX + 1];
void led_off(void);
void led_on(int color);
// Clear the screen and draw the currently selected background icon (if any).
// Should only be called with gUpdateMutex locked.
static void draw_background_locked(gr_surface icon)
{
	gPagesIdentical = 0;
	gr_color(0, 0, 0, 255);
	gr_fill(0, 0, gr_fb_width(), gr_fb_height());

	if (icon) {
		int iconWidth = gr_get_width(icon);
		int iconHeight = gr_get_height(icon);
		int iconX = (gr_fb_width() - iconWidth) / 2;
		int iconY = (gr_fb_height() - iconHeight) / 2;
		gr_blit(icon, 0, 0, iconWidth, iconHeight, iconX, iconY);
	}
}

// Draw the progress bar (if any) on the screen.  Does not flip pages.
// Should only be called with gUpdateMutex locked.
static void draw_text_xy(int row, int col, const char* t) {
	if (t[0] != '\0') {
		gr_text(col, row, t,0);
	}
}
char bat[10]={0};
#define LED_GREEN         1
#define LED_RED           2
#define LED_BLUE          3
static void draw_progress_locked(int level)
{
	if (gProgressBarType == PROGRESSBAR_TYPE_NONE) return;

	int iconHeight = gr_get_height(gBackgroundIcon[BACKGROUND_ICON_INSTALLING]);
	int width = gr_get_width(gProgressBarEmpty);
	int height = gr_get_height(gProgressBarEmpty);

	int dx = (gr_fb_width() - width)/2;
	int dy = (gr_fb_height() - height)/2;
	static int frame = 0;
	static int led_flag = 0;

	// Erase behind the progress bar (in case this was a progress-only update)
	gr_color(0, 0, 0, 255);
	gr_fill(0, 0, gr_fb_width(), gr_fb_height());

	gr_color(64, 96, 255, 255);
	if(level > 100)
		level = 100;
	else if (level < 0)
		level = 0;
	if(level < 90){
		if(led_flag!= LED_RED){
			led_on(LED_RED);
			led_flag = LED_RED;
		}

	}else{
		if(led_flag!= LED_GREEN){
			led_on(LED_GREEN);
			led_flag = LED_GREEN;
		}
	}
	sprintf(bat, "%d%%%c", level, '\0');
	draw_text_xy((dy + height), (gr_fb_width()/2 - 20), bat);

	if (gProgressBarType == PROGRESSBAR_TYPE_NORMAL) {
		frame = level * (PROGRESSBAR_INDETERMINATE_STATES - 1) / 100;
		gr_blit(gProgressBarIndeterminate[frame], 0, 0, width, height, dx, dy);
	}

	if (gProgressBarType == PROGRESSBAR_TYPE_INDETERMINATE) {
		gr_blit(gProgressBarIndeterminate[frame], 0, 0, width, height, dx, dy);
		frame = (frame + 1);
		if(frame >= PROGRESSBAR_INDETERMINATE_STATES) {
			frame = level * (PROGRESSBAR_INDETERMINATE_STATES - 1) / 100;
		}
	}
}

static void draw_text_line(int row, const char* t) {
	if (t[0] != '\0') {
		gr_text(0, (row+1)*CHAR_HEIGHT-1, t,0);
	}
}

// Redraw everything on the screen.  Does not flip pages.
// Should only be called with gUpdateMutex locked.
static void draw_screen_locked(void)
{
	draw_background_locked(gCurrentIcon);
	//draw_progress_locked(0);

	if (show_text) {
		gr_color(0, 0, 0, 160);
		gr_fill(0, 0, gr_fb_width(), gr_fb_height());

		int i = 0;
		if (show_menu) {
			gr_color(64, 96, 255, 255);
			gr_fill(0, (menu_top+menu_sel) * CHAR_HEIGHT,
					gr_fb_width(), (menu_top+menu_sel+1)*CHAR_HEIGHT+1);

			for (; i < menu_top + menu_items; ++i) {
				if (i == menu_top + menu_sel) {
					gr_color(255, 255, 255, 255);
					draw_text_line(i, menu[i]);
					gr_color(64, 96, 255, 255);
				} else {
					draw_text_line(i, menu[i]);
				}
			}
			gr_fill(0, i*CHAR_HEIGHT+CHAR_HEIGHT/2-1,
					gr_fb_width(), i*CHAR_HEIGHT+CHAR_HEIGHT/2+1);
			++i;
		}

		gr_color(255, 255, 0, 255);

		for (; i < text_rows; ++i) {
			draw_text_line(i, text[(i+text_top) % text_rows]);
		}
	}
}

// Redraw everything on the screen and flip the screen (make it visible).
// Should only be called with gUpdateMutex locked.
static void update_screen_locked(void)
{
	draw_screen_locked();
	gr_flip();
}

// Updates only the progress bar, if possible, otherwise redraws the screen.
// Should only be called with gUpdateMutex locked.
static void update_progress_locked(int level)
{
	if (show_text || !gPagesIdentical) {
		draw_screen_locked();    // Must redraw the whole screen
		gPagesIdentical = 1;
	} else {
		draw_progress_locked(level);  // Draw only the progress bar
	}
	gr_flip();
}

extern int is_exit;
// Keeps the progress bar updated, even when the process is otherwise busy.
static void *progress_thread(void *cookie)
{
	for (;!is_exit;) {
		usleep(1000000/ PROGRESSBAR_INDETERMINATE_FPS);
		pthread_mutex_lock(&gUpdateMutex);

		// update the progress bar animation, if active
		// skip this if we have a text overlay (too expensive to update)
		if (gProgressBarType == PROGRESSBAR_TYPE_INDETERMINATE && !show_text) {
			update_progress_locked(0);
		}

		// move the progress bar forward on timed intervals, if configured
		int duration = gProgressScopeDuration;
		if (gProgressBarType == PROGRESSBAR_TYPE_NORMAL && duration > 0) {
			int elapsed = time(NULL) - gProgressScopeTime;
			float progress = 1.0 * elapsed / duration;
			if (progress > 1.0) progress = 1.0;
			if (progress > gProgress) {
				gProgress = progress;
				update_progress_locked(0);
			}
		}

		pthread_mutex_unlock(&gUpdateMutex);
	}
	return NULL;
}

#define WakeFileName  "/sys/power/wait_for_fb_wake"
void *charge_thread(void *cookie)
{
	int fd, err;
	char buf;
	int bat_stat = 0;
	int bat_level = 0;
	for (;!is_exit;) {
		usleep(1000000/ PROGRESSBAR_INDETERMINATE_FPS);
		// update the progress bar animation, if active
		if(bat_stat == BATTERY_STATUS_CHARGING){
			gProgressBarType = PROGRESSBAR_TYPE_INDETERMINATE;
		}else{
			gProgressBarType = PROGRESSBAR_TYPE_NORMAL;
		}
		fd = open(WakeFileName, O_RDONLY, 0);
		if (fd < 0) {
		LOGD("Couldn't open file /sys/power/wait_for_fb_wake\n");
		return NULL;
		}
		do {
			err = read(fd, &buf, 1);
			LOGD("return from WakeFileName err: %d errno: %s\n", err, strerror(errno));
		} while (err < 0 && errno == EINTR);
		close(fd);
		bat_level = battery_capacity();
		bat_stat = battery_status();
		update_progress_locked(bat_level);
		usleep(500000);
	}

	usleep(200);
	return NULL;
}

void *power_thread(void *cookie)
{
	for (;!is_exit;) {
		if(battery_ac_online()==0 && battery_usb_online()==0){
			LOGE("charger not present, power off device\n");
			is_exit = 1;
			reboot(RB_POWER_OFF);
			usleep(200);
		}
	}
	return NULL;
}

void set_backlight(int bright)
{
	int fd;
	;
}
inline long long timeval_diff(struct timeval big, struct timeval small)
{
	return (long long)(big.tv_sec-small.tv_sec)*1000000 + big.tv_usec - small.tv_usec;
}
#define POWER_KEY_STATUS  "/sys/kernel/sprd_eic_button/status"
static int powerkey_status_get(void)
{
	int fd;
	int ret;
	char buffer[8];
	int powerkey_status= -1;
	fd = open(POWER_KEY_STATUS, O_RDWR);

	if(fd < 0) {
		LOGE("%s: open %s fail",__func__, POWER_KEY_STATUS);
		return -1;
	}

	memset(buffer, 0, sizeof(buffer));
	ret = read(fd, buffer, 8);
	if(ret < 0) {
		LOGE("%s: read %s fail",__func__, POWER_KEY_STATUS);
	} else
		powerkey_status = atoi(buffer);
	close(fd);
	LOGE("powerkey status: %d \n", powerkey_status);

	return powerkey_status;
}
void backlight_init(void);
void backlight_on(void);
void backlight_off(void);
int set_screen_state(int);
// Reads input events, handles special hot keys, and adds to the key queue.

#define BACKLIGHT_ON_MS 3000
void *input_thread(void *write_fd)
{
	int fake_key = 0;
	struct timeval pwr_press_time = {0,0};
	struct input_event ev;
	int pwr_key;
	static int pre_key_code = 0xfff;
	int ret = 0;
	struct timeval start_time={0, 0};
	struct timeval now_time={0, 0};
	int i = 5;
	char cmd = 0;

	int time_left;
	long long time_diff_temp;

	char buf[8]={0};

	pwr_key = KEY_POWER;


	LOGD("%s pwr_key is %d\n", __func__, pwr_key);

	for (;!is_exit;) {
		time_left = BACKLIGHT_ON_MS;
first_key_check:
		while(gettimeofday(&start_time, (struct timezone *)0)<0);
		if(time_left > 0){
			ret = ev_get(&ev, time_left);
		}else{
			ret = -1;
		}
		LOGD(" %s: %d, ret:%d, ev.type:%d, ev.code:%d, ev.value:%d\n", __func__, \
				__LINE__, ret, ev.type, ev.code, ev.value);
		if(ret == -1) {/* time out */
			backlight_off();
			set_screen_state(0);
			continue;
		}
		LOGD(" %s: %d\n", __func__, __LINE__);
		if(ev.type != EV_KEY){
			while(gettimeofday(&now_time, (struct timezone *)0)<0);
			time_diff_temp = timeval_diff(now_time, start_time);
			time_diff_temp = (time_diff_temp + 1000)/1000;
			time_left = time_left - time_diff_temp;
			LOGD(" %s: %d time_left: %d\n", __func__, __LINE__, time_left);
			if(time_left <= 0){
				backlight_off();
				set_screen_state(0);
				continue;
			}else{
				goto first_key_check;
			}
		}
		LOGD(" %s: %d\n", __func__, __LINE__);
		if(ev.value != 1){
			continue;
		}
		LOGD(" %s: %d\n", __func__, __LINE__);
		if(ev.code == KEY_BRL_DOT8){ /* alarm event happen */
			if(alarm_flag_check()){
				set_screen_state(1);
				is_exit = 1;
				LOGD(" %s: %d, %s\n", __func__, __LINE__,"alarm happen 1, exit");
				__reboot(LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2,
						LINUX_REBOOT_CMD_RESTART2, "alarm");
				usleep(500000);
				LOGD(" %s: %d, %s\n", __func__, __LINE__,"alarm reboot failed");
				break;
			}else{
				while(gettimeofday(&now_time, (struct timezone *)0)<0);
				time_diff_temp = timeval_diff(now_time, start_time);
				time_diff_temp = (time_diff_temp + 1000)/1000;
				time_left = time_left - time_diff_temp;
				LOGD(" %s: %d time_left: %d\n", __func__, __LINE__, time_left);
				if(time_left <= 0){
					backlight_off();
					set_screen_state(0);
					continue;
				}else{
					goto first_key_check;
				}

			}
		}

		if(ev.code == pwr_key){
			set_screen_state(1);
			while(gettimeofday(&start_time, (struct timezone *)0)<0);
			time_left = 1500;
rechk_pwr_key:
			ret = ev_get(&ev, time_left);
			LOGD(" %s: %d, ret:%d, ev.type:%d, ev.code:%d, ev.value:%d\n", __func__, \
					__LINE__, ret, ev.type, ev.code, ev.value);
			if(ret == -1){ /* time out */
				LOGD(" %s: %d\n", __func__, __LINE__);
				is_exit = 1;
				__reboot(LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2,
						LINUX_REBOOT_CMD_RESTART2, "charger");
				usleep(500000);
				LOGD(" %s: %d, reboot failed\n", __func__, __LINE__);
				break;
			}else if(ev.code == KEY_BRL_DOT8 && alarm_flag_check()){
				is_exit = 1;
				LOGD(" %s: %d, %s\n", __func__, __LINE__,"alarm happen 2, exit");
				__reboot(LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2,
						LINUX_REBOOT_CMD_RESTART2, "alarm");
				usleep(500000);
				LOGD(" %s: %d, %s\n", __func__, __LINE__,"alarm reboot failed");
				break;
			}else if(ev.code == pwr_key){
				usleep(500000);
				if(powerkey_status_get() == 0){
				LOGD(" %s: %d %s\n", __func__, __LINE__, "power key up found");
				usleep(500000);
				backlight_on();
				continue;
				}else
				goto rechk_pwr_key;
			}else{
				while(gettimeofday(&now_time, (struct timezone *)0)<0);
				time_diff_temp = timeval_diff(now_time, start_time);
				time_diff_temp = (time_diff_temp + 1000)/1000;
				if(time_diff_temp <1500){
					time_left = 1500 - time_diff_temp;
					goto rechk_pwr_key;
				}else{
					LOGD(" %s: %d %s\n", __func__, __LINE__, "power key detect timeoutr, exit");
					is_exit = 1;
					__reboot(LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2,
						LINUX_REBOOT_CMD_RESTART2, "charger");
					usleep(500000);
					LOGD(" %s: %d, %s\n", __func__, __LINE__,"alarm reboot failed");
					break;
				}
			}
		}else{
			ret = ev_get(&ev, -1);
			LOGD(" %s: %d, ret:%d, ev.type:%d, ev.code:%d, ev.value:%d\n", __func__, \
					__LINE__, ret, ev.type, ev.code, ev.value);
			set_screen_state(1);

			if(ret == 0 && ev.code == KEY_BRL_DOT8 && alarm_flag_check()){ /* alarm event happen */
				set_screen_state(1);
				is_exit = 1;
				LOGD(" %s: %d, %s\n", __func__, __LINE__,"alarm happen 3, exit");
				__reboot(LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2,
						LINUX_REBOOT_CMD_RESTART2, "alarm");
				usleep(500000);
				LOGD(" %s: %d, %s\n", __func__, __LINE__,"alarm reboot failed");
				break;
			}else{
				usleep(500000);
				LOGD(" %s: %d %s\n", __func__, __LINE__, "other key, just recheck");
				continue;
			}
		}
	}
	LOGD(" %s exit\n", __func__);
	return NULL;
}

void ui_init(void)
{
	gr_init();
	ev_init();

	text_col = text_row = 0;
	text_rows = gr_fb_height() / CHAR_HEIGHT;
	if (text_rows > MAX_ROWS) text_rows = MAX_ROWS;
	text_top = 1;

	text_cols = gr_fb_width() / CHAR_WIDTH;
	if (text_cols > MAX_COLS - 1) text_cols = MAX_COLS - 1;

	int i;
	for (i = 0; BITMAPS[i].name != NULL; ++i) {
		int result = res_create_display_surface(BITMAPS[i].name, BITMAPS[i].surface);
		if (result < 0) {
			if (result == -2) {
				LOGI("Bitmap %s missing header\n", BITMAPS[i].name);
			} else {
				LOGE("Missing bitmap %s\n(Code %d)\n", BITMAPS[i].name, result);
			}
			*BITMAPS[i].surface = NULL;
		}
	}


}

void ui_set_background(int icon)
{
	pthread_mutex_lock(&gUpdateMutex);
	gCurrentIcon = gBackgroundIcon[icon];
	update_screen_locked();
	pthread_mutex_unlock(&gUpdateMutex);
}

void ui_show_indeterminate_progress()
{
	pthread_mutex_lock(&gUpdateMutex);
	if (gProgressBarType != PROGRESSBAR_TYPE_INDETERMINATE) {
		gProgressBarType = PROGRESSBAR_TYPE_INDETERMINATE;
		update_progress_locked(0);
	}
	pthread_mutex_unlock(&gUpdateMutex);
}

void ui_show_progress(float portion, int seconds)
{
	pthread_mutex_lock(&gUpdateMutex);
	gProgressBarType = PROGRESSBAR_TYPE_NORMAL;
	gProgressScopeStart += gProgressScopeSize;
	gProgressScopeSize = portion;
	gProgressScopeTime = time(NULL);
	gProgressScopeDuration = seconds;
	gProgress = 0;
	update_progress_locked(0);
	pthread_mutex_unlock(&gUpdateMutex);
}

void ui_set_progress(float fraction)
{
	pthread_mutex_lock(&gUpdateMutex);
	if (fraction < 0.0) fraction = 0.0;
	if (fraction > 1.0) fraction = 1.0;
	if (gProgressBarType == PROGRESSBAR_TYPE_NORMAL && fraction > gProgress) {
		// Skip updates that aren't visibly different.
		int width = gr_get_width(gProgressBarIndeterminate[0]);
		float scale = width * gProgressScopeSize;
		if ((int) (gProgress * scale) != (int) (fraction * scale)) {
			gProgress = fraction;
			update_progress_locked(0);
		}
	}
	pthread_mutex_unlock(&gUpdateMutex);
}

void ui_reset_progress()
{
	pthread_mutex_lock(&gUpdateMutex);
	gProgressBarType = PROGRESSBAR_TYPE_NONE;
	gProgressScopeStart = gProgressScopeSize = 0;
	gProgressScopeTime = gProgressScopeDuration = 0;
	gProgress = 0;
	update_screen_locked();
	pthread_mutex_unlock(&gUpdateMutex);
}

void ui_print(const char *fmt, ...)
{
	char buf[256];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, 256, fmt, ap);
	va_end(ap);

	fputs(buf, stderr);

	// This can get called before ui_init(), so be careful.
	pthread_mutex_lock(&gUpdateMutex);
	if (text_rows > 0 && text_cols > 0) {
		char *ptr;
		for (ptr = buf; *ptr != '\0'; ++ptr) {
			if (*ptr == '\n' || text_col >= text_cols) {
				text[text_row][text_col] = '\0';
				text_col = 0;
				text_row = (text_row + 1) % text_rows;
				if (text_row == text_top) text_top = (text_top + 1) % text_rows;
			}
			if (*ptr != '\n') text[text_row][text_col++] = *ptr;
		}
		text[text_row][text_col] = '\0';
		update_screen_locked();
	}
	pthread_mutex_unlock(&gUpdateMutex);
}

void ui_start_menu(char** headers, char** items) {
	int i;
	pthread_mutex_lock(&gUpdateMutex);
	if (text_rows > 0 && text_cols > 0) {
		for (i = 0; i < text_rows; ++i) {
			if (headers[i] == NULL) break;
			strncpy(menu[i], headers[i], text_cols-1);
			menu[i][text_cols-1] = '\0';
		}
		menu_top = i;
		for (; i < text_rows; ++i) {
			if (items[i-menu_top] == NULL) break;
			strncpy(menu[i], items[i-menu_top], text_cols-1);
			menu[i][text_cols-1] = '\0';
		}
		menu_items = i - menu_top;
		show_menu = 1;
		menu_sel = 0;
		update_screen_locked();
	}
	pthread_mutex_unlock(&gUpdateMutex);
}

int ui_menu_select(int sel) {
	int old_sel;
	pthread_mutex_lock(&gUpdateMutex);
	if (show_menu > 0) {
		old_sel = menu_sel;
		menu_sel = sel;
		if (menu_sel < 0) menu_sel = 0;
		if (menu_sel >= menu_items) menu_sel = menu_items-1;
		sel = menu_sel;
		if (menu_sel != old_sel) update_screen_locked();
	}
	pthread_mutex_unlock(&gUpdateMutex);
	return sel;
}

void ui_end_menu() {
	int i;
	pthread_mutex_lock(&gUpdateMutex);
	if (show_menu > 0 && text_rows > 0 && text_cols > 0) {
		show_menu = 0;
		update_screen_locked();
	}
	pthread_mutex_unlock(&gUpdateMutex);
}

int ui_text_visible()
{
	pthread_mutex_lock(&gUpdateMutex);
	int visible = show_text;
	pthread_mutex_unlock(&gUpdateMutex);
	return visible;
}

int ui_wait_key()
{
	pthread_mutex_lock(&key_queue_mutex);
	while (key_queue_len == 0) {
		pthread_cond_wait(&key_queue_cond, &key_queue_mutex);
	}

	int key = key_queue[0];
	memcpy(&key_queue[0], &key_queue[1], sizeof(int) * --key_queue_len);
	pthread_mutex_unlock(&key_queue_mutex);
	return key;
}

int ui_key_pressed(int key)
{
	// This is a volatile static array, don't bother locking
	return key_pressed[key];
}

void ui_clear_key_queue() {
	pthread_mutex_lock(&key_queue_mutex);
	key_queue_len = 0;
	pthread_mutex_unlock(&key_queue_mutex);
}

int alarm_flag_check(void)
{
    char *alarm_name = "/productinfo/alarm_flag";
    char *poweron_name = "/productinfo/poweron_timeinmillis";
    char *rtc_dev = "/dev/rtc0";
    int ret;
    int alarm_flag_fd = -1;
    int poweron_flag_fd = -1;
    int rtc_fd = -1;
    struct rtc_time now_rtc;
    char read_buf[30] = {0};
    char read_buf1[30] = {0};
    char * buf_pos = NULL;
    long alarm_time_millis = 0;
    long alarm_time_millis1 = 0;
    long now_rtc_sec = 0;
    struct timeval now_time={0};

    alarm_flag_fd = open(alarm_name, O_RDWR);
    poweron_flag_fd = open(poweron_name, O_RDWR);
    if(alarm_flag_fd >= 0){
	ret = read(alarm_flag_fd, read_buf, sizeof(read_buf));
	read_buf[sizeof(read_buf)-2] = '\n';
	read_buf[sizeof(read_buf)-1] = '\0';
	if(ret >= 0 && read_buf[0]!= 0xff){
          LOGD("%s get: %s\n", alarm_name, read_buf);
          buf_pos = strstr(read_buf, "\n");
          buf_pos += 1;
          alarm_time_millis = strtoul(buf_pos, (char **)NULL, 10);
          LOGD("trans to %lu\n", alarm_time_millis);
        }
       close(alarm_flag_fd);
     }
    if(poweron_flag_fd >= 0){
	ret = read(poweron_flag_fd, read_buf1, sizeof(read_buf1));
	read_buf1[sizeof(read_buf1)-2] = '\n';
	read_buf1[sizeof(read_buf1)-1] = '\0';
	if(ret >= 0 && read_buf1[0]!= 0xff){
          LOGD("%s get: %s\n", poweron_name, read_buf1);
          buf_pos = strstr(read_buf1, "\n");
          buf_pos += 1;
          alarm_time_millis1 = strtoul(buf_pos, (char **)NULL, 10);
          LOGD("trans to %lu\n", alarm_time_millis1);
        }
       close(poweron_flag_fd);
     }

       while(gettimeofday(&now_time, (struct timezone *)0)<0);
       LOGD("now time %lu\n", now_time.tv_sec);
       alarm_time_millis = alarm_time_millis - now_time.tv_sec;
       alarm_time_millis1 = alarm_time_millis1 - now_time.tv_sec;
       if((alarm_time_millis > -20 && alarm_time_millis < 180) || (alarm_time_millis1 > -20 && alarm_time_millis1 < 180))
          return 1;
       return 0;
}
