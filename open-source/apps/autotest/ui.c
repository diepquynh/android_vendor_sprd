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
#include <linux/input.h>

#include "common.h"
#include "./minui/minui.h"
#include "ui.h"
#include "./res/string_cn.h"

static pthread_mutex_t gUpdateMutex = PTHREAD_MUTEX_INITIALIZER;
static gr_surface gBackgroundIcon[NUM_BACKGROUND_ICONS];

static const struct { gr_surface* surface; const char *name; } BITMAPS[] = {
    { &gBackgroundIcon[BACKGROUND_ICON_INSTALLING], "icon_installing" },
    { &gBackgroundIcon[BACKGROUND_ICON_ERROR],      "icon_error" },
    { NULL,                             NULL },
};

gr_surface gCurrentIcon = NULL;

// Set to 1 when both graphics pages are the same (except for the progress bar)
static int gPagesIdentical = 0;

// Log text overlay, displayed when a magic key is pressed
static char text[MAX_ROWS][MAX_COLS];
int text_cols = 0, text_rows = 0;
static int text_col = 0, text_row = 0, text_top = 0;
static int show_text = 0;




static int _utf8_strlen(const char* str);
static int _utf8_to_clen(const char* str, int len);

// Clear the screen and draw the currently selected background icon (if any).
// Should only be called with gUpdateMutex locked.
void draw_background_locked(gr_surface icon)
{
    gPagesIdentical = 0;
    //gr_color(0, 0, 0, 255);
	ui_set_color(CL_SCREEN_BG);
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

static void draw_text_line(int row, const char* t) {
  if (t[0] != '\0') {
    gr_text(0, (row+1)*CHAR_HEIGHT-1, t);
  }
}

// Updates only the progress bar, if possible, otherwise redraws the screen.
// Should only be called with gUpdateMutex locked.
static void update_progress_locked(void)
{
    if (show_text || !gPagesIdentical) {
        gPagesIdentical = 1;
    } else {
    }
    gr_flip();
}


void ui_init(void)
{
    gr_init();
    text_col = text_row = 0;
    text_rows = gr_fb_height() / CHAR_HEIGHT;
	LOGD("mmitest fb_h=%d, rows=%d\n", gr_fb_height(), text_rows);
    if (text_rows > MAX_ROWS) text_rows = MAX_ROWS;
    text_top = 1;

    text_cols = gr_fb_width() / CHAR_WIDTH;
	LOGD("mmitest fb_w=%d, cols=%d\n", gr_fb_width(), text_cols);
    if (text_cols > MAX_COLS - 1) text_cols = MAX_COLS - 1;

}

void ui_set_background(int icon)
{
    pthread_mutex_lock(&gUpdateMutex);
    gCurrentIcon = gBackgroundIcon[icon];
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
    }
    pthread_mutex_unlock(&gUpdateMutex);
}



void temp_set_visible(int is_show)
{
	show_text = is_show;
}

int ui_text_visible()
{
    pthread_mutex_lock(&gUpdateMutex);
    int visible = show_text;
    pthread_mutex_unlock(&gUpdateMutex);
    LOGD("ui_text_visible %d\n",visible);
    return visible;
}

void ui_show_title(const char* title)
{
	int width = gr_fb_width();
	ui_set_color(CL_TITLE_BG);
	gr_fill(0, 0, gr_fb_width(), 2*CHAR_HEIGHT);
	ui_set_color(CL_TITLE_FG);
	gr_text(0, CHAR_HEIGHT + (CHAR_HEIGHT>>1), title);
}

void ui_show_button(const char* left,const char* right)
{
	int width = gr_fb_width();
	ui_set_color(CL_GREEN);
	gr_fill(0, gr_fb_height()-2*CHAR_HEIGHT,
			6*CHAR_WIDTH, gr_fb_height());
	ui_set_color(CL_RED);
	gr_fill(gr_fb_width()-6*CHAR_WIDTH, gr_fb_height()-2*CHAR_HEIGHT,
			gr_fb_width(), gr_fb_height());
	ui_set_color(CL_RED);
	if(left) {
		gr_text(CHAR_WIDTH, gr_fb_height()-CHAR_WIDTH, left);
	}

	//if(centor) {
	//	gr_text(gr_fb_width()/2-(CHAR_WIDTH*2), gr_fb_height()-(CHAR_HEIGHT>>1), centor);
	//}
	ui_set_color(CL_GREEN);
	if(right) {
		gr_text(gr_fb_width()-5*CHAR_WIDTH, gr_fb_height()-CHAR_WIDTH, right);
	}
}

int ui_show_text(int row, int col, const char* text)
{
	int str_len = strlen(text);
	int utf8_len = _utf8_strlen(text);
	int clen = 0;
	int max_col = gr_fb_width() / CHAR_WIDTH;
	char temp[256];
	char* pos = text;
	int cur_row = row;

	//LOGD("[%s], len=%d, max_col=%d\n", __FUNCTION__, len, max_col);
	if(utf8_len == str_len) {
		while(str_len > 0) {
			memset(temp, 0, sizeof(temp));
			if(str_len > max_col) {
				memcpy(temp, pos, max_col);
				pos += max_col;
				str_len -= max_col;
			} else {
				memcpy(temp, pos, str_len);
				pos += str_len;
				str_len = 0;
			}
			gr_text(col*CHAR_WIDTH,
					(++cur_row)*CHAR_HEIGHT-1, temp);
		}
	} else {
		//LOGD("[%s], ut8_len=%d, max_col=%d\n", __FUNCTION__, utf8_len, max_col);
		//LOGD("[%s], clen=%d\n", __FUNCTION__, strlen(text));
		max_col -= 10; //for chinese word
		while(utf8_len > 0) {
			memset(temp, 0, sizeof(temp));
			if(utf8_len > max_col) {
				clen = _utf8_to_clen(pos, max_col);
				memcpy(temp, pos, clen);
				pos += clen;
				utf8_len -= max_col;
			} else {
				clen = _utf8_to_clen(pos, utf8_len);
				memcpy(temp, pos, clen);
				//pos += clen;
				utf8_len = 0;
			}
			//LOGD("[%s], clen=%d\n", __FUNCTION__, clen);
			gr_text(col*CHAR_WIDTH, (++cur_row)*CHAR_HEIGHT-1, temp);
		}
	}

	return cur_row;
}

void ui_fill_locked(void)
{
	draw_background_locked(gCurrentIcon);
}


extern int usbin_state;
int ui_handle_button(const char* left,const char* right)
{
	int key = -1;
	if(left != NULL || right != NULL) {//centor != NULL ||
		ui_show_button(left, right);// centor,
		gr_flip();
	}

	return 0;
}

void ui_fill_screen(unsigned char r,unsigned char g,unsigned char b)
{
	gr_color(r, g, b, 255);
	gr_fill(0, 0, gr_fb_width(), gr_fb_height());
}

void ui_clear_rows(int start, int n)
{
	ui_set_color(CL_SCREEN_BG);
	gr_fill(0, CHAR_HEIGHT*start, gr_fb_width(), CHAR_HEIGHT*(start+n));
}

void ui_clear_rows_cols(int row_start, int n1,int col_start,int n2)
{
	ui_set_color(CL_SCREEN_BG);
	gr_fill(CHAR_WIDTH*col_start, CHAR_HEIGHT*row_start,CHAR_WIDTH*(col_start+n2), CHAR_HEIGHT*(row_start+n1));
}

void ui_set_color(int cl)
{
	switch(cl) {
		case CL_WHITE:
			gr_color(255, 255, 255, 255);
			break;
		case CL_BLACK:
			gr_color(0, 0, 0, 255);
			break;
		case CL_RED:
			gr_color(255, 0, 0, 255);
			break;
		case CL_BLUE:
			gr_color(0, 0, 255, 255);
			break;
		case CL_GREEN:
			gr_color(0, 255, 0, 255);
			break;
		case CL_TITLE_BG:
			gr_color(62, 30, 51, 255);
			break;
		case CL_TITLE_FG:
			gr_color(235, 214, 228, 255);
			break;
		case CL_SCREEN_BG:
			gr_color(33, 16, 28, 255);
			break;
		case CL_SCREEN_FG:
			gr_color(255, 255, 255, 255);
			break;
		default:
			gr_color(0, 0, 0, 255);
			break;
	}

}

void ui_draw_line(int x1,int y1,int x2,int y2)
{
     int *xx1=&x1;
     int *yy1=&y1;
     int *xx2=&x2;
     int *yy2=&y2;

     int i=0;
     int j=0;
     int tekxx=*xx2-*xx1;
     int tekyy=*yy2-*yy1;

     if(*xx2>=*xx1)
     {
         for(i=*xx1;i<=*xx2;i++)
         {
             j=(i-*xx1)*tekyy/tekxx+*yy1;
	      gr_fill(i,j,i+1, j+1);
         }
     }
     else
     {
         for(i=*xx2;i<*xx1;i++)
         {
             j=(i-*xx2)*tekyy/tekxx+*yy2;
             gr_fill(i,j,i+1, j+1);
         }
     }
}

void ui_draw_line_mid(int x1,int y1,int x2,int y2)
{
	 int *xx1=&x1;
     int *yy1=&y1;
     int *xx2=&x2;
     int *yy2=&y2;

     int i=0;
     int j=0;
     int tekxx=*xx2-*xx1;
     int tekyy=*yy2-*yy1;

	 if(abs(tekxx) >= abs(tekyy)) {
		if(*xx2>=*xx1){
			for(i=*xx1;i<=*xx2;i++){
				j=(i-*xx1)*tekyy/tekxx+*yy1;
				if (j == *yy1 && i == *xx1)
					continue;
				gr_fill(i,j,i+1, j+1);
			}
		}
		else{
			for(i=*xx2;i<*xx1;i++){
				j=(i-*xx2)*tekyy/tekxx+*yy2;
				if (j == *yy2 && i == *xx2)
					continue;
				gr_fill(i,j,i+1, j+1);
			}
		}
	 } else {
		if(*yy2>=*yy1){
			for(j=*yy1;j<=*yy2;j++){
				i=(j-*yy1)*tekxx/tekyy+*xx1;
				if (j == *yy1 && i == *xx1)
					continue;
				gr_fill(i,j,i+1, j+1);
			}
		}
		else{
			for(j=*yy2;j<*yy1;j++){
				i=(j-*yy2)*tekxx/tekyy+*xx2;
				if (j == *yy2 && i == *xx2)
					continue;
				gr_fill(i,j,i+1, j+1);
			}
		}
	 }
 }


static int _utf8_strlen(const char* str)
{
	int i = 0;
	int count = 0;
	int len = strlen (str);
	unsigned char chr = 0;
	while (i < len)
	{
		chr = str[i];
		count++;
		i++;
		if(i >= len)
			break;

		if(chr & 0x80) {
			chr <<= 1;
			while (chr & 0x80) {
				i++;
				chr <<= 1;
			}
		}
	}
	return count;
}

static int _utf8_to_clen(const char* str, int len)
{
	int i = 0;
	int count = 0;
	//int min = strlen (str);
	unsigned char chr = 0;
	//min = min < len ? min : len;
	while (str[i]!='\0' && count < len)
	{
		chr = str[i];
		count++;
		i++;

		if(chr & 0x80) {
			chr <<= 1;
			while (chr & 0x80) {
				i++;
				chr <<= 1;
			}
		}
	}
	return i;
}
