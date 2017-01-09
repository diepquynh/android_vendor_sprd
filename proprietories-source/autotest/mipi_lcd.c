#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include "./minui/minui.h"
#include "common.h"

#include <cutils/android_reboot.h>
#include "./res/string_cn.h"
#include "ui.h"

bool inited = false;
void test_result_init(void)
{
    if(!inited){
	ui_init();
	inited = true;
    }
    LOGD("the autoest after ui_init\n");
    ui_set_background(BACKGROUND_ICON_NONE);
}

void test_lcd_uinit(void)
{
    LOGD("test_lcd_uinit\n");
	ui_set_color(CL_BLACK);
	gr_fill(0, 0, gr_fb_width(), gr_fb_height());
	gr_flip();
}
int test_lcd_start(void)
{
	int ret = 0;
	ui_fill_locked();
	ui_fill_screen(255, 255, 255);
	gr_flip();
	usleep(500*1000);

	ui_fill_screen(0, 0, 0);
	gr_flip();
	usleep(500*1000);

	ui_fill_screen(255, 0, 0);
	gr_flip();
	usleep(500*1000);

	ui_fill_screen(0, 255, 0);
	gr_flip();
	usleep(500*1000);

	ui_fill_screen(0, 0, 255);
	gr_flip();
	usleep(500*1000);



	ui_show_title(MENU_TEST_LCD);
	gr_color(255, 255, 255, 255);
	ui_show_text(3, 0, TEXT_FINISH);
	ui_set_color(CL_GREEN);
	ui_show_text(4, 0, LCD_TEST_TIPS);//+++++++++++++++++
	ret = ui_handle_button(TEXT_PASS, TEXT_FAIL);//, TEXT_GOBACK
	//save_result(CASE_TEST_LCD,ret);
	return ret;
}

//>>----------add the five colors split display the funciton
void test_lcd_splitdisp(int data)
{
    ui_fill_locked();
    switch(data){
	case 0:
	    ui_fill_screen(255, 255, 255);
	    gr_flip();
	    break;
	case 1:
	    ui_fill_screen(255, 0, 0);
	    gr_flip();
	    break;

	case 2:
	    ui_fill_screen(0, 255, 0);
	    gr_flip();
	    break;

	case 3:
	    ui_fill_screen(0, 0, 255);
	    gr_flip();
	    break;

	case 4:
	    ui_fill_screen(0, 0, 0);
	    gr_flip();
	    break;
	default:
	    break;
    }
}

void test_bl_lcd_start(void)
{
	int ret = 0;
	ui_fill_locked();
	ui_fill_screen(255, 255, 255);
	gr_flip();
}



