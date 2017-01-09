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
#include "minui.h"
#include "ui.h"
#include "testitem.h"


static pthread_mutex_t gUpdateMutex = PTHREAD_MUTEX_INITIALIZER;
static gr_surface gBackgroundIcon[NUM_BACKGROUND_ICONS];
static gr_surface gProgressBarIndeterminate[PROGRESSBAR_INDETERMINATE_STATES];
static gr_surface gProgressBarEmpty;
static gr_surface gProgressBarFill;

static const struct { gr_surface* surface; const char *name; } BITMAPS[] = {
    { &gBackgroundIcon[BACKGROUND_ICON_INSTALLING], "icon_installing" },
    { &gBackgroundIcon[BACKGROUND_ICON_ERROR],      "icon_error" },
    { &gProgressBarIndeterminate[0],    "indeterminate1" },
    { &gProgressBarIndeterminate[1],    "indeterminate2" },
    { &gProgressBarIndeterminate[2],    "indeterminate3" },
    { &gProgressBarIndeterminate[3],    "indeterminate4" },
    { &gProgressBarIndeterminate[4],    "indeterminate5" },
    { &gProgressBarIndeterminate[5],    "indeterminate6" },
    { &gProgressBarEmpty,               "progress_empty" },
    { &gProgressBarFill,                "progress_fill" },
    { NULL,                             NULL },
};

gr_surface gCurrentIcon = NULL;

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
int text_cols = 0, text_rows = 0,text_tp_rows = 0;
static int text_col = 0, text_row = 0, text_top = 0;
static int show_text = 0;

static char menu[MAX_ROWS][MAX_COLS];
static char menu_title[MAX_COLS];
static int show_menu = 0;
static int menu_top = 0, menu_items = 0, menu_sel = 0;
unsigned char menu_change=0, menu_change_last=0;


// Key event input queue
static pthread_mutex_t key_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t key_queue_cond = PTHREAD_COND_INITIALIZER;
static int key_queue[256], key_queue_len = 0;
static volatile char key_pressed[KEY_MAX + 1];
int cwidth=0,cheight=0,ewidth=0,eheight=0;
int page_num = 0;
int page_shown = 0,page_shown_last = 0;
extern int version_change_page;
extern int tp_flag;
extern char autotest_flag;
extern void gr_font_size(int *cx, int *cy,int *ex, int *ey);
static int _utf8_strlen(const char* str);
static int _utf8_to_clen(const char* str, int len);
int ui_start_menu(char* title, const char** items, int sel);
void parse_title(char * buf, char gap, char* value);

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
void draw_progress_locked()
{
    if (gProgressBarType == PROGRESSBAR_TYPE_NONE) return;

    int iconHeight = gr_get_height(gBackgroundIcon[BACKGROUND_ICON_INSTALLING]);
    int width = gr_get_width(gProgressBarEmpty);
    int height = gr_get_height(gProgressBarEmpty);

    int dx = (gr_fb_width() - width)/2;
    int dy = (3*gr_fb_height() + iconHeight - 2*height)/4;

    // Erase behind the progress bar (in case this was a progress-only update)
    gr_color(0, 0, 0, 255);
    gr_fill(dx, dy, width, height);

    if (gProgressBarType == PROGRESSBAR_TYPE_NORMAL) {
        float progress = gProgressScopeStart + gProgress * gProgressScopeSize;
        int pos = (int) (progress * width);

        if (pos > 0) {
          gr_blit(gProgressBarFill, 0, 0, pos, height, dx, dy);
        }
        if (pos < width-1) {
          gr_blit(gProgressBarEmpty, pos, 0, width-pos, height, dx+pos, dy);
        }
    }

    if (gProgressBarType == PROGRESSBAR_TYPE_INDETERMINATE) {
        static int frame = 0;
        gr_blit(gProgressBarIndeterminate[frame], 0, 0, width, height, dx, dy);
        frame = (frame + 1) % PROGRESSBAR_INDETERMINATE_STATES;
    }
}

static void draw_text_line(int row, const char* t)
{
  if (t[0] != '\0') {
    gr_text(0, row*CHAR_HEIGHT+(CHAR_HEIGHT>>1)+(cheight>>1), t);
  }
}

// Redraw everything on the screen.  Does not flip pages.
// Should only be called with gUpdateMutex locked.
static void draw_screen_locked(void)
{
    draw_background_locked(gCurrentIcon);
    draw_progress_locked();
    int menu_sel_tmp;
    int i = 0,m = 0;
    char result[16]={0};

    if (show_text) {
        //gr_color(0, 0, 0, 160);
        ui_set_color(CL_SCREEN_BG);
        gr_fill(0, 0, gr_fb_width(), gr_fb_height());

        if(menu_sel>=page_num)
		while((menu_sel_tmp=menu_sel-page_num*(i++))>=page_num);
        else
		menu_sel_tmp=menu_sel;
        if (show_menu) {
            ui_show_title(menu_title);
            //LOGD("mmitest menu_sel=%d,menu_sel_tmp=%d,menu_top=%d,menu_items=%d", menu_sel,menu_sel_tmp,menu_top,menu_items);
            ui_set_color(CL_MENU_HL_BG);
            gr_fill(0, (menu_top+menu_sel_tmp) * CHAR_HEIGHT,gr_fb_width(), (menu_top+menu_sel_tmp+1)*CHAR_HEIGHT+1);

            ui_set_color(CL_SCREEN_FG);
            //LOGD("mmitest menu_items=%d,page_num=%d", menu_items,page_num);
            if(menu_items >= page_num)
                m=page_num;
            else m= menu_items;
            for (i=0; i < menu_top+m; ++i) {
                //LOGD("mmitest menu_top=%d,m=%d,menu[%d]=%s\n", menu_top,m,i,menu[i]);
                if (i == menu_top + menu_sel_tmp) {
			 ui_set_color(CL_MENU_HL_FG);
                    draw_text_line(i, menu[i]);
			 ui_set_color(CL_SCREEN_FG);
                } else {
			 memset(result,0,sizeof(result));
			 parse_title(menu[i], ':', result);
			 if(NULL != strstr(result,TEXT_PASS))
			     ui_set_color(CL_GREEN);
			 else if(NULL != strstr(result,TEXT_FAIL))
			     ui_set_color(CL_RED);
			 else ui_set_color(CL_SCREEN_FG);
			 draw_text_line(i, menu[i]);
			 ui_set_color(CL_SCREEN_FG);
                }
            }
            gr_fill(0, i*CHAR_HEIGHT+CHAR_HEIGHT/((CHAR_HEIGHT>35)?CHAR_HEIGHT:2)-1,gr_fb_width(), i*CHAR_HEIGHT+CHAR_HEIGHT/((CHAR_HEIGHT>35)?CHAR_HEIGHT:2)-1+1);
            ui_show_button(NULL,TEXT_NEXT_PAGE,TEXT_GOBACK);
            ++i;
        }

        gr_color(255, 255, 0, 255);

        for (; i < text_tp_rows; ++i) {
            draw_text_line(i, text[(i+text_top) % text_tp_rows]);
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
static void update_progress_locked(void)
{
    if (show_text || !gPagesIdentical) {
        draw_screen_locked();    // Must redraw the whole screen
        gPagesIdentical = 1;
    } else {
        draw_progress_locked();  // Draw only the progress bar
    }
    gr_flip();
}

// Keeps the progress bar updated, even when the process is otherwise busy.
static void *progress_thread()
{
    for (;;) {
        usleep(1000000 / PROGRESSBAR_INDETERMINATE_FPS);
        pthread_mutex_lock(&gUpdateMutex);

        // update the progress bar animation, if active
        // skip this if we have a text overlay (too expensive to update)
        if (gProgressBarType == PROGRESSBAR_TYPE_INDETERMINATE && !show_text) {
            update_progress_locked();
        }

        // move the progress bar forward on timed intervals, if configured
        int duration = gProgressScopeDuration;
        if (gProgressBarType == PROGRESSBAR_TYPE_NORMAL && duration > 0) {
            int elapsed = time(NULL) - gProgressScopeTime;
            float progress = 1.0 * elapsed / duration;
            if (progress > 1.0) progress = 1.0;
            if (progress > gProgress) {
                gProgress = progress;
                update_progress_locked();
            }
        }

        pthread_mutex_unlock(&gUpdateMutex);
    }
    return NULL;
}

extern void touch_handle_input(int, struct input_event*);
// Reads input events, handles special hot keys, and adds to the key queue.
static void *input_thread()
{
    int rel_sum = 0;
    int fake_key = 0;
    int fd = -1;

    for (;;) {
        // wait for the next key event
        struct input_event ev;
        do {
            fd = ev_get(&ev, 0);
            SPRD_DBG("eventtype %d,eventcode %d,eventvalue %d",ev.type,ev.code,ev.value);
            if(fd != -1) {
                touch_handle_input(fd, &ev);
            }

            if (ev.type == EV_SYN) {
                continue;
            } else if (ev.type == EV_REL) {
                if (ev.code == REL_Y) {
                    // accumulate the up or down motion reported by
                    // the trackball.  When it exceeds a threshold
                    // (positive or negative), fake an up/down
                    // key event.
                    rel_sum += ev.value;
                    if (rel_sum > 3) {
                        fake_key = 1;
                        ev.type = EV_KEY;
                        ev.code = KEY_DOWN;
                        ev.value = 1;
                        rel_sum = 0;
                    } else if (rel_sum < -3) {
                        fake_key = 1;
                        ev.type = EV_KEY;
                        ev.code = KEY_UP;
                        ev.value = 1;
                        rel_sum = 0;
                    }
                }
            } else {
                rel_sum = 0;
            }
        }while (ev.type != EV_KEY || ev.code > KEY_MAX);
        pthread_mutex_lock(&key_queue_mutex);
        if (!fake_key) {
            // our "fake" keys only report a key-down event (no
            // key-up), so don't record them in the key_pressed
            // table.
            key_pressed[ev.code] = ev.value;
            SPRD_DBG("ev.value=%d", ev.value);
        }
        fake_key = 0;
        const int queue_max = sizeof(key_queue) / sizeof(key_queue[0]);
        if (ev.value > 0 && key_queue_len < queue_max) {
            key_queue[key_queue_len++] = ev.code;
            pthread_cond_signal(&key_queue_cond);
        }
        pthread_mutex_unlock(&key_queue_mutex);

    }
    return NULL;
}

void ui_push_result(int result)
{
	pthread_mutex_lock(&key_queue_mutex);
	const int queue_max = sizeof(key_queue) / sizeof(key_queue[0]);
	if (key_queue_len < queue_max) {
		switch(result) {
			case RL_NA: //cancel
				key_queue[key_queue_len++] = KEY_BACK;
				break;
			case RL_FAIL: //fail
				key_queue[key_queue_len++] = KEY_POWER;
				break;
			case RL_PASS: //ok
				key_queue[key_queue_len++] = KEY_VOLUMEDOWN;
				break;
			default: //cancel
			//	key_queue[key_queue_len++] = KEY_BACK;
				break;
		}
		pthread_cond_signal(&key_queue_cond);
	}
	pthread_mutex_unlock(&key_queue_mutex);
}

void ui_init(void)
{
    gr_init();
    ev_init();

    text_col = text_row = 0;
    text_tp_rows =   (gr_fb_height()-(gr_fb_height()>>3))/CHAR_HEIGHT-1;
    text_rows = gr_fb_height() / CHAR_HEIGHT;
    if (text_rows > MAX_ROWS) text_rows = MAX_ROWS;
    if (text_tp_rows > MAX_ROWS) text_tp_rows = MAX_ROWS;
    text_top = 1;

    text_cols = gr_fb_width() / CHAR_WIDTH;
    LOGD("mmitest fb_w=%d, cols=%d", gr_fb_width(), text_cols);
    if (text_cols > MAX_COLS - 1) text_cols = MAX_COLS - 1;
    gr_font_size(&cwidth, &cheight,&ewidth,&eheight);

    #ifdef SPRD_VIRTUAL_TOUCH
    page_num = (gr_fb_height()-(gr_fb_height()>>3)) / CHAR_HEIGHT-1;
    #else
    text_tp_rows = text_rows;
    page_num = text_rows-2;
    #endif
    LOGD("mmitest fb_h=%d, text_rows=%d,text_tp_rows=%d,page_num=%d", gr_fb_height(), text_rows,text_tp_rows,page_num);

    pthread_t t;
    pthread_create(&t, NULL, progress_thread, NULL);
    pthread_create(&t, NULL, input_thread, NULL);
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
        update_progress_locked();
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
    update_progress_locked();
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
            update_progress_locked();
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
    if (text_tp_rows > 0 && text_cols > 0) {
        char *ptr;
        for (ptr = buf; *ptr != '\0'; ++ptr) {
            if (*ptr == '\n' || text_col >= text_cols) {
                text[text_row][text_col] = '\0';
                text_col = 0;
                text_row = (text_row + 1) % text_tp_rows;
                if (text_row == text_top) text_top = (text_top + 1) % text_tp_rows;
            }
            if (*ptr != '\n') text[text_row][text_col++] = *ptr;
        }
        text[text_row][text_col] = '\0';
        update_screen_locked();
    }
    pthread_mutex_unlock(&gUpdateMutex);
}

int ui_start_menu(char* title, const char** items, int sel)
{
    int i;

    pthread_mutex_lock(&gUpdateMutex);
    //LOGD("mmitest text_rows=%d,text_cols=%d",text_rows,text_cols);
    if (text_tp_rows > 0 && text_cols > 0) {
        for (i = 0; i < ((CHAR_HEIGHT>35)?1:2); ++i) {
            menu[i][0] = '\0';
        }
        memset(menu_title, 0, sizeof(menu_title));
        strncpy(menu_title, title, text_cols-1);
        menu_title[text_cols-1] = '\0';
        menu_top = i;
        if( 0 == menu_change||1 == menu_change){
	      while (NULL != items[i-menu_top]) {
	            strncpy(menu[i], items[i-menu_top], text_cols-1);
	            menu[i][text_cols-1] = '\0';
	            //LOGD("mmitest uishow menu[%d]=%s",i,menu[i]);
	            i++;
	      }
        }else if(2 == menu_change||3 == menu_change){
	     //LOGD("mmitest uishow items=%s,page_num = %d,page_shown=%d",items[i-menu_top+page_num*page_shown],page_num,page_shown);
	     while (NULL != items[i-menu_top+page_num*page_shown]) {
		      strncpy(menu[i], items[page_num*page_shown+i-menu_top], text_cols-1);
		      menu[i][text_cols-1] = '\0';
		      i++;
	     }
        }
        menu_items = i - menu_top;

        show_menu = 1;
        if(sel >= 0 )
	     menu_sel = sel;
        else
	     menu_sel = 0;
	  //LOGD("mmitest uishow menu_sel=%d,sel = %d",menu_sel,sel);
        update_screen_locked();
    }
    pthread_mutex_unlock(&gUpdateMutex);
    return menu_sel;
}

int ui_menu_select(int sel)
{
    int old_sel;

    LOGD("sel = %d", sel);
    pthread_mutex_lock(&gUpdateMutex);
    if (show_menu > 0) {
        old_sel = menu_sel;
        menu_sel = sel;
        if (menu_sel < 0) menu_sel = menu_items-1;
        if (menu_sel >= menu_items) menu_sel = 0;
        sel = menu_sel;
        if (menu_sel != old_sel) update_screen_locked();
    }
    pthread_mutex_unlock(&gUpdateMutex);
    return sel;
}

int ui_update_menu(int sel,int itemnum)
{
    int old_sel;

    LOGD("mmitest sel = %d", sel);
    pthread_mutex_lock(&gUpdateMutex);
    if (show_menu > 0) {
        old_sel = menu_sel;
        menu_sel = sel;
        if(itemnum>(page_num-1)){
            menu_sel=menu_sel;
        }else{
            if (menu_sel < 0) menu_sel = menu_items-1;
            if (menu_sel >= menu_items) menu_sel = 0;
        }
        sel = menu_sel;
        if (menu_sel != old_sel) {
            update_screen_locked();
        }
    }
    pthread_mutex_unlock(&gUpdateMutex);
    return sel;
}

void ui_end_menu()
{
    int i;

    pthread_mutex_lock(&gUpdateMutex);
    if (show_menu > 0 && text_tp_rows > 0 && text_cols > 0) {
        show_menu = 0;
        update_screen_locked();
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
    LOGD("ui_text_visible %d",visible);
    return visible;
}

int ui_wait_key(struct timespec *ntime)
{
    int ret=0;
    int key=-1;

    pthread_mutex_lock(&key_queue_mutex);
    while (key_queue_len == 0) {
	ret=pthread_cond_timedwait(&key_queue_cond, &key_queue_mutex,ntime);
	if(ret == ETIMEDOUT)
            break;
    }

    if(0 == ret){
	key = key_queue[0];
	memcpy(&key_queue[0], &key_queue[1], sizeof(int) * --key_queue_len);
    }else
	key=ret;
    pthread_mutex_unlock(&key_queue_mutex);
    return key;
}

int ui_key_pressed(int key)
{
    // This is a volatile static array, don't bother locking
    return key_pressed[key];
}

void ui_clear_key_queue()
{
    pthread_mutex_lock(&key_queue_mutex);
    key_queue_len = 0;
    pthread_mutex_unlock(&key_queue_mutex);
}

int device_handle_key(int key_code, int visible)
{
	if (visible) {
		if((key_code>=KEY_VIR_ITEMS) && (key_code< KEY_VIR_ITEMS+text_tp_rows))
			return key_code;
		switch (key_code) {
			case KEY_VOLUMEDOWN:
				return HIGHLIGHT_DOWN;
			case KEY_VOLUMEUP:
				return SELECT_ITEM;
			case KEY_POWER:
				return GO_BACK;
			case KEY_VIR_NEXT_PAGE:
				return NEXT_PAGE;
			case KEY_VIR_FAIL:
				return GO_HOME;
			default:
				return NO_ACTION;

		}
	}

	return NO_ACTION;
}

int ui_show_menu(char* title, const char** items, int is_root, int sel,int itemnum)
{
     int selectedtmp=0;
     int selected = 0;
     int chosen_item = -1;
     struct timespec ntime;
     ntime.tv_sec= time(NULL);
     ntime.tv_nsec=0;

     ui_clear_key_queue();
     page_shown_last = page_shown;
     menu_change_last = menu_change;
     //menu_change = 0;
show:
    if(sel < text_tp_rows)
            page_shown = 0;
     LOGD("mmitest uishow menu_change=%d,page_shown=%d",menu_change,page_shown);
     selected = ui_start_menu(title, items, sel);
    while (chosen_item < 0) {
        int key = ui_wait_key(&ntime);
        if(ETIMEDOUT == key)
            continue;
        LOGD("mmitest ui_wait_key is %d",key);
        int visible = ui_text_visible();
        int action = device_handle_key(key, visible);
        LOGD("mmitest device_handle_key %d",action);
        if (action < 0) {
            switch (action) {
                case HIGHLIGHT_DOWN:
                        LOGD("mmitest HIGHLIGHT_DOWN");
                        ++selected;

			    //has more than one page
                        //LOGD("mmitest page_num=%d,itemnum=%d",page_num,itemnum);
                        if(itemnum >= page_num){
                            if(selected >= itemnum){
                                selected=0;
                                selectedtmp=selected;
                                menu_change=1;
                                page_shown = 0;
                                //LOGD("mmitest uishow start again");
                            }else if((((selected >= page_num) && (itemnum-selected>page_num)) || (page_shown == selected/page_num)) && (3!=menu_change)){
                                page_shown = selected/page_num;
                                selectedtmp=selected-page_num*page_shown;
                                menu_change=2;
                                //LOGD("mmitest uishow has more than one page ,this is the first and middle page, not end");
                            }else if(((selected >= page_num)  &&  (itemnum-selected<=page_num)) || (page_shown == selected/page_num)){
                                page_shown = selected/page_num;
                                selectedtmp=selected-page_num*page_shown;
                                menu_change=3;
                                //LOGD("mmitest uishow has more than one page,this is the last page");
                            }else{
                                page_shown = selected/page_num;
                                selectedtmp=selected-page_num*page_shown;
                                menu_change=0;
                                //LOGD("mmitest uishow only has one page or has more than one page,this is the first one");
                            }
                            //not only has one page,and change page
                            //has more middle page
                            if((menu_change!=0&&menu_change!=menu_change_last) || ( 2 == menu_change && 0 == (selected%page_num))){
                                ui_start_menu(title, items, selectedtmp);
                                //LOGD("mmitest uishow change page");
                            }
                            selectedtmp = ui_update_menu(selectedtmp,itemnum);
                            menu_change_last=menu_change;
                        }else{
                            menu_change=0;
                            selected = ui_update_menu(selected,itemnum);
                            //LOGD("mmitest uishow only has one page or has more page,this is the first");
                        }
                        LOGD("mmitest uishow select=%d",selected);
                        break;
                case NEXT_PAGE:
                        LOGD("mmitest KEY_VIR_NEXT_PAGE");
                        if( itemnum > page_num){
                            page_shown++;
                            selected=page_num*page_shown;
                            if(selected >= itemnum){
                                selected=0;
                                selectedtmp=selected;
                                menu_change=1;
                                page_shown = 0;
                                //LOGD("mmitest uishow has more than one page,start again");
                            }else if(((itemnum-page_num*page_shown )>=0) &&((itemnum-page_num*page_shown)<=page_num) ){
                                menu_change=3;
                                //LOGD("mmitest uishow has more than one page,this is the last one");
                            }else{
                                menu_change=2;
                                //LOGD("mmitest uishow has more than one page,this is not the last one");
                            }
                        }else{
                            menu_change=0;
                            selected = ui_update_menu(selected,itemnum);
                            LOGD("mmitest uishow one has one page");
                            break;
                        }
                        selectedtmp=selected-page_num*page_shown;
                        LOGD("mmitest itemnum=%d,page_shown=%d,menu_change = %d",itemnum,page_shown,menu_change);
                        ui_start_menu(title, items, selectedtmp);
                        //LOGD("mmitest uishow here5");
                        selectedtmp = ui_update_menu(selectedtmp,itemnum);
                        break;
                case SELECT_ITEM:
                        LOGD("mmitest SELECT_ITEM");
                        chosen_item = selected;
                        break;
                case GO_BACK:
                        LOGD("mmitest GO_BACK");
                        if(is_root) break;
                        else{
                            chosen_item = -1;
                            menu_change = 0;
                            page_shown = page_shown_last;
                            goto end;
                        }
                case GO_HOME:
                        LOGD("mmitest GO_HOME");
                        if(is_root){
                            chosen_item = -1;
                            menu_change = 0;
                            if(page_shown){
                                //page_shown = 0;
                                LOGD("mmitest sel=%d",sel);
                                goto show;
                            }else break;
                        }else{
                            chosen_item = -1;
                            menu_change = 0;
                            page_shown = page_shown_last;
                            goto end;
                        }
                case NO_ACTION:
                        LOGD("mmitest NO_ACTION");
                        break;
            }
        } else if((action>=KEY_VIR_ITEMS )&&(action< KEY_VIR_ITEMS+text_tp_rows)){
             LOGD("mmitest TOUCH SELECT_ITEM");
             selectedtmp = action-KEY_VIR_ITEMS;
             if((action-KEY_VIR_ITEMS)<(itemnum-page_shown*page_num)){
                 selectedtmp = ui_update_menu(selectedtmp,itemnum);
                 chosen_item = action-KEY_VIR_ITEMS + page_shown*page_num;
                 //LOGD("mmitest uishow action = %d,page_shown=%d,page_num=%d",action,page_shown,page_num);
                 LOGD("mmitest uishow chosen_item = %d",chosen_item);
             }else
                 chosen_item = -1;
             }
    }

end:
    ui_end_menu();
    return chosen_item;
}

void ui_show_title(const char* title)
{
	int width = gr_fb_width();
	ui_set_color(CL_TITLE_BG);
	gr_fill(0, 0, gr_fb_width(), ((CHAR_HEIGHT>35)?1:2)*CHAR_HEIGHT);
	ui_set_color(CL_TITLE_FG);
	gr_text(0, CHAR_HEIGHT/((CHAR_HEIGHT>35)?2:1)+cheight/2, title);
}

void ui_show_button(const char* left,const char* center,const char* right)
{
	int width = gr_fb_width();
	int height = gr_fb_height();

	#ifndef SPRD_VIRTUAL_TOUCH
	left = NULL;
	center = NULL;
	right = NULL;
	#endif
	if(left) {
		ui_set_color(CL_GREEN);
		gr_fill(0, height-(height>>3), height>>3, height);
		ui_set_color(CL_WHITE);
		gr_text((height>>4)-cwidth,height-(height>>4)+cheight/2, left);
	}

	if(right) {
		ui_set_color(CL_RED);
		gr_fill(width-(height>>3),height-(height>>3), width, height);
		ui_set_color(CL_WHITE);
		gr_text(width-(height>>3)+(height>>4)-cwidth, height-(height>>4)+cheight/2, right);
	}

	if(center) {
		ui_set_color(CL_BLUE);
		gr_fill((width/2)-(height>>4),height-(height>>3), (width/2)+(height>>4), height);
		ui_set_color(CL_WHITE);
		if(15 == strlen(center)){	//five characters
			gr_text(width/2-5*cwidth/2, height-(height>>4)+cheight/2, center);
		}else if(9 == strlen(center)){   //three characters
		    gr_text(width/2-3*cwidth/2, height-(height>>4)+cheight/2, center);
		}else{                      //two characters
		    gr_text(width/2-cwidth, height-(height>>4)+cheight/2, center);
		}
	}
}

int ui_show_text(int row, int col, const char* text)
{
	int str_len = strlen(text);
	int utf8_len = _utf8_strlen(text);
	int clen = 0;
	int max_col = gr_fb_width() / CHAR_WIDTH;
	char temp[256];
	const char* pos = text;
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
			gr_text(col*CHAR_WIDTH,CHAR_HEIGHT*((CHAR_HEIGHT>35)?1:2)+(cur_row-1)*cheight-1, temp);
			cur_row++;
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
			gr_text(col*CHAR_WIDTH,CHAR_HEIGHT*((CHAR_HEIGHT>35)?1:2)+(cur_row-1)*cheight-1, temp);
			cur_row++;
		}
	}

	return cur_row;
}

void ui_fill_locked(void)
{
	draw_background_locked(gCurrentIcon);
	draw_progress_locked();
}


extern int usbin_state;
int ui_handle_button(const char* left,const char* center,const char* right)
{
	int key = -1;
	int ret;
	struct timespec ntime;
	ntime.tv_sec= time(NULL);
	ntime.tv_nsec=0;

	if((1 == autotest_flag) && (left != NULL || right != NULL || center != NULL))
		center = TEXT_GOBACK ;
	if(left != NULL || right != NULL || center != NULL) {
		ui_show_button(left, center,right);
		gr_flip();
	}

	ui_clear_key_queue();
	LOGD("mmitest waite key");
	for(;;) {
		key = ui_wait_key(&ntime);
		if(ETIMEDOUT == key)
            continue;
		LOGD("mmitest key=%d",key);
		if(1==usbin_state){
			if((KEY_VIR_PASS==key) || (KEY_VOLUMEDOWN ==key))
				key=-1;
		}
		if(((1==tp_flag)&&(((KEY_VIR_PASS==key) || (KEY_VIR_FAIL==key) || (KEY_VIR_NEXT_PAGE==key))))
		|| (((NULL==left) && (KEY_VIR_PASS==key)) || ((NULL==center) && ((KEY_VOLUMEUP==key)||(KEY_VIR_NEXT_PAGE==key))) || ((NULL==right) && (KEY_VIR_FAIL==key)))
		|| ((1==version_change_page)&&(KEY_VIR_PASS== key)))
			key=-1;
		switch(key) {
			case KEY_VOLUMEDOWN:
				ret = RL_PASS;
				LOGD("mmitest keyVD solved");
				break;
			case KEY_VOLUMEUP:
				ret = RL_BACK;
				LOGD("mmitest keyVU solved");
				break;
			case KEY_POWER:
				ret = RL_FAIL;
				LOGD("mmitest keyP solved");
				break;
			case KEY_VIR_PASS:
				ret = RL_PASS;
				LOGD("mmitest vir pass solved");
				break;
			case KEY_VIR_FAIL:
				ret = RL_FAIL;
				LOGD("mmitest key vir fail solved");
				break;
			case KEY_VIR_NEXT_PAGE:
				ret = RL_NEXT_PAGE;
				LOGD("mmitest key vir next page solved");
				break;
			case KEY_BACK:
				ret = RL_NA;
				LOGD("mmitest keybk solved");
				break;
			default:
				continue;
		}
		usbin_state = 0;
		return ret;
	}
	//return 0;
}

void ui_fill_screen(unsigned char r,unsigned char g,unsigned char b)
{
	gr_color(r, g, b, 255);
	gr_fill(0, 0, gr_fb_width(), gr_fb_height());
}

void ui_clear_rows(int start, int n)
{
	ui_set_color(CL_SCREEN_BG);
	gr_fill(0, CHAR_HEIGHT*((CHAR_HEIGHT>35)?1:2)+(start-2)*cheight, gr_fb_width(), CHAR_HEIGHT*((CHAR_HEIGHT>35)?1:2)+(start-2+n)*cheight);
}

void ui_clear_rows_cols(int row_start, int n1,int col_start,int n2)
{
	ui_set_color(CL_SCREEN_BG);
	gr_fill(CHAR_WIDTH*col_start, CHAR_HEIGHT*((CHAR_HEIGHT>35)?1:2)+(row_start-2)*cheight,CHAR_WIDTH*(col_start+n2), CHAR_HEIGHT*((CHAR_HEIGHT>35)?1:2)+(row_start-2+n1)*cheight);
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
		case CL_YELLOW:
			gr_color(255, 255, 0, 255);
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
		case CL_MENU_HL_BG:
			gr_color(201, 143, 182, 255);
			break;
		case CL_MENU_HL_FG:
			gr_color(17, 9, 14, 255);
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

void parse_title(char * buf, char gap, char* value)
{
    char *str= NULL;

    if(buf != NULL && strlen(buf)){
        str = strchr(buf, gap);
        if(str != NULL){
	      str++;
            strncpy(value, str, strlen(str));
        }
    }
    return ;
}
