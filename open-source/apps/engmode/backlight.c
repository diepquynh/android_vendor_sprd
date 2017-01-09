/*
 * File:         backlight.c
 * Based on:
 * Author:       Yunlong Wang <Yunlong.Wang @spreadtrum.com>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define SPRD_BACKLIGHTTEST_TITLE	"SpreadTrum BACKLIGHT TEST"
#define SPRD_BACKLIGHTTEST_TITLE_Y 	80


#define LCD_BACKLIGHT_DEV			"/sys/class/leds/lcd-backlight/brightness"
#define KEY_BACKLIGHT_DEV 			"/sys/class/leds/button-backlight/brightness"
#define LCD_BACKLIGHT_MAX_DEV		"/sys/class/leds/lcd-backlight/max_brightness"
#define KEY_BACKLIGHT_MAX_DEV 		"/sys/class/leds/button-backlight/max_brightness"

#define SPRD_DBG(...)
static int max_lcd, max_key;

/*
 *Set LCD backlight brightness level
 */
static int eng_lcdbacklight_test(int brightness)
{
    int fd;
    int ret;
    char buffer[8];

    fd = open(LCD_BACKLIGHT_DEV, O_RDWR);

    if(fd < 0) {
        SPRD_DBG("%s: open %s fail",__func__, LCD_BACKLIGHT_DEV);
        return -1;
    }

    memset(buffer, 0, sizeof(buffer));
    sprintf(buffer, "%d", brightness);
    ret = write(fd, buffer, strlen(buffer));

    close(fd);

    return 0;
}

/*
 *Set LED backlight brightness level
 */
static int eng_keybacklight_test(int brightness)
{
    int fd;
    int ret;
    char buffer[8];

    fd = open(KEY_BACKLIGHT_DEV, O_RDWR);

    if(fd < 0) {
        SPRD_DBG("%s: open %s fail",__func__, KEY_BACKLIGHT_DEV);
        return -1;
    }

    memset(buffer, 0, sizeof(buffer));
    sprintf(buffer, "%d", brightness);
    ret = write(fd, buffer, strlen(buffer));

    close(fd);

    return 0;
}

/*
 *Flicker the backlight
 */
static void eng_backlighttest_toggle(int lcd_max, int key_max)
{
    eng_lcdbacklight_test(0);
    eng_keybacklight_test(0);

    sleep(1);

    eng_lcdbacklight_test(lcd_max);
    eng_keybacklight_test(key_max);

    sleep(1);
}

void backlight_on(void)
{
    eng_lcdbacklight_test(max_lcd/2);
    eng_keybacklight_test(max_key/2);
}

void backlight_off(void)
{
    eng_lcdbacklight_test(0);
    eng_keybacklight_test(0);
}

int turnoff_lcd_backlight(void)
{
    system("echo 0 >/sys/devices/platform/sprd_backlight/backlight/sprd_backlight/brightness");
    return 0;
}

int turnoff_calibration_backlight(void)
{
    system("echo 0 >/sys/class/backlight/sprd_backlight/brightness");
    return 0;
}

void backlight_init(void)
{
    int i;
    int fd, ret;
    int light_on;
    char buffer[8];

    max_lcd=0;
    max_key=0;

    //get lcd backlight max level
    fd = open(LCD_BACKLIGHT_MAX_DEV, O_RDONLY);
    if(fd < 0) {
        SPRD_DBG("%s: open %s fail",__func__, LCD_BACKLIGHT_MAX_DEV);
    } else {
        memset(buffer, 0, sizeof(buffer));
        ret = read(fd, buffer, sizeof(buffer));
        max_lcd = atoi(buffer);
        if(ret < 0){
            SPRD_DBG("%s: read %s fail",__func__, LCD_BACKLIGHT_MAX_DEV);
        }

        close(fd);
    }

    //get key backlight max level
    fd = open(KEY_BACKLIGHT_MAX_DEV, O_RDONLY);
    if(fd < 0) {
        SPRD_DBG("%s: open %s fail",__func__, KEY_BACKLIGHT_MAX_DEV);
    } else {
        memset(buffer, 0, sizeof(buffer));
        ret = read(fd, buffer, sizeof(buffer));
        max_key = atoi(buffer);
        if(ret < 0){
            SPRD_DBG("%s: read %s fail",__func__, KEY_BACKLIGHT_MAX_DEV);
        }

        close(fd);
    }

    SPRD_DBG("%s max_lcd=%d; max_key=%d",__func__, max_lcd, max_key);

    //show screen
    //	eng_backlighttest_show();
    //
    //	//flicker the backlight
    //	sleep(1);
    //	for(i=0; i<3; i++) {
    //		eng_backlighttest_toggle(255, 255);
    //	}
    //	sleep(1);
}

