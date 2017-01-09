/* * File:         lcd.c
 * Based on:
 * Author:       Yunlong Wang <Yunlong.Wang @spreadtrum.com>
 *
 * Created:	  2011-05-12
 * Description:  SPRD LCD TEST
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include "testitem.h"

int test_lcd_start(void)
{
	int ret = 0;
	int row = 2;

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

	ui_fill_locked();
	ui_show_title(MENU_TEST_LCD);
	gr_color(255, 255, 255, 255);
	row=ui_show_text(row, 0, TEXT_FINISH);
	ui_set_color(CL_GREEN);
	row=ui_show_text(row, 0, LCD_TEST_TIPS);
	gr_flip();
	ret = ui_handle_button(TEXT_PASS,NULL,TEXT_FAIL);
	save_result(CASE_TEST_LCD,ret);
	return ret;
}
