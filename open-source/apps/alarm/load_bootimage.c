#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <linux/input.h>
#include <linux/reboot.h>
#include <pthread.h>
#include "minui.h"
#include "battery.h"
#include "boot_alarm.h"
#include "common.h"

#define PROGRESSBAR_INDETERMINATE_STATES 6

extern int load_black_background(void);
extern int get_value(char *file);
extern struct boot_status gs_boot_state;
extern int time_dm;
static gr_surface gProgressBarIndeterminate[PROGRESSBAR_INDETERMINATE_STATES];

static gr_surface gLowPower[3];

gr_surface my_surface;
void draw_image()
{
	struct timeval time_start,time_end;
	int width = gr_get_width(my_surface);
	int height = gr_get_height(my_surface);

	int dx = (gr_fb_width() - width)/2;
	int dy = (gr_fb_height() - height)/2;
	gettimeofday(&time_start,NULL);
	while(1)
	{
		gr_blit(my_surface, 0, 0, width, height, dx, dy);
		gr_flip();
		gettimeofday(&time_end,NULL);
		if((time_end.tv_sec - time_start.tv_sec) >= 3)
		{
			break;
		}
	}
}

void load_image(char *image)
{
	gr_init();
	int result = res_create_surface(image, &my_surface);
	if (result < 0) {
		if (result == -2) {
			printf("Bitmap %s missing header\n",image );
		} else {
			printf("Missing bitmap %s\n(Code %d)\n", image, result);
		}
		my_surface = NULL;
	}
	draw_image();
}


gr_surface alarm_main_surf[ALARM_WARNING_IMG_NUM + ALARM_TIME_NUM + ALARM_BOOT_LOGO_NUM]={0};

typedef struct { gr_surface* surface; const char *name; } ImageBitmaps;

ImageBitmaps Alarm_Main_Bitmaps[] =
{
	{&alarm_main_surf[ALARM_WARNING_IMG_POS], ALARM_MAIN_BACKGROUND_IMAGE},
	{&alarm_main_surf[ALARM_WARNING_IMG_POS+1], ALARM_MAIN_LEFT_IMAGE},
	{&alarm_main_surf[ALARM_WARNING_IMG_POS+2], ALARM_MAIN_MID_IMAGE},
	{&alarm_main_surf[ALARM_WARNING_IMG_POS+3], ALARM_MAIN_RIGHT_IMAGE},
	{&alarm_main_surf[ALARM_WARNING_IMG_POS+4], ALARM_BUTTON_BG_IMAGE},
	{NULL,NULL},
	{&alarm_main_surf[ALARM_TIME_NUM_POS],ALARM_TIME_NUM_0},
	{&alarm_main_surf[ALARM_TIME_NUM_POS+1],ALARM_TIME_NUM_1},
	{&alarm_main_surf[ALARM_TIME_NUM_POS+2],ALARM_TIME_NUM_2},
	{&alarm_main_surf[ALARM_TIME_NUM_POS+3],ALARM_TIME_NUM_3},
	{&alarm_main_surf[ALARM_TIME_NUM_POS+4],ALARM_TIME_NUM_4},
	{&alarm_main_surf[ALARM_TIME_NUM_POS+5],ALARM_TIME_NUM_5},
	{&alarm_main_surf[ALARM_TIME_NUM_POS+6],ALARM_TIME_NUM_6},
	{&alarm_main_surf[ALARM_TIME_NUM_POS+7],ALARM_TIME_NUM_7},
	{&alarm_main_surf[ALARM_TIME_NUM_POS+8],ALARM_TIME_NUM_8},
	{&alarm_main_surf[ALARM_TIME_NUM_POS+9],ALARM_TIME_NUM_9},
	{&alarm_main_surf[ALARM_TIME_NUM_DOT_POS],ALARM_TIME_NUM_DOT},
	{NULL,NULL},
	{&alarm_main_surf[ALARM_BOOT_LOGO_POS],ALARM_BOOT_LOGO},
	{NULL,NULL}
};

void print_time_num(int time_icon_y)
{
	time_t timep;
	int hour_dec, hour_unit,mini_dec,mini_unit;
	struct tm *time_cur;

	time(&timep);
	time_cur = localtime(&timep);
	if(time_dm == 12){
		if(time_cur->tm_hour-12> 0){
			hour_dec = (time_cur->tm_hour-12)/10;
			hour_unit = (time_cur->tm_hour-12)%10;
		}else if(time_cur->tm_hour ==0){
			hour_dec = 1;
			hour_unit = 2;
		}else{
			hour_dec = time_cur->tm_hour/10;
			hour_unit = time_cur->tm_hour%10;
		}
        }else{
	    hour_dec = time_cur->tm_hour/10;
	    hour_unit = time_cur->tm_hour%10;
        }
	mini_dec = time_cur->tm_min/10;
	mini_unit = time_cur->tm_min%10;

	int time_img_width = gr_get_width(alarm_main_surf[ALARM_TIME_NUM_POS]);
	int time_img_heidht = gr_get_height(alarm_main_surf[ALARM_TIME_NUM_POS]);

	int dot_width  = gr_get_width(alarm_main_surf[ALARM_TIME_NUM_DOT_POS]);
	int dot_height = gr_get_height(alarm_main_surf[ALARM_TIME_NUM_DOT_POS]);

	int bx = (gr_fb_width() - time_img_width*4 - dot_width)/2;

	LOGD("show time secs\n");
	gr_blit(alarm_main_surf[ALARM_TIME_NUM_POS+hour_dec], 0, 0, time_img_width,time_img_heidht, bx, time_icon_y );
	gr_blit(alarm_main_surf[ALARM_TIME_NUM_POS+hour_unit], 0, 0, time_img_width,time_img_heidht, bx + time_img_width*1, time_icon_y );
	gr_blit(alarm_main_surf[ALARM_TIME_NUM_DOT_POS], 0, 0, dot_width, dot_height,bx +  time_img_width*2, time_icon_y );
	gr_blit(alarm_main_surf[ALARM_TIME_NUM_POS+mini_dec], 0, 0, time_img_width,time_img_heidht, bx + time_img_width*2 + dot_width, time_icon_y );
	gr_blit(alarm_main_surf[ALARM_TIME_NUM_POS+mini_unit], 0, 0, time_img_width,time_img_heidht, bx + time_img_width*3 + dot_width, time_icon_y );
}

void print_images(int image_pos)
{
	int icon_main_wide = gr_get_width(alarm_main_surf[image_pos]);
	int icon_main_height = gr_get_height(alarm_main_surf[image_pos]);

	int button_bg_wide = gr_get_width(alarm_main_surf[image_pos+4]);
	int button_bg_height = gr_get_height(alarm_main_surf[image_pos+4]);
	int button_bg_pos_x = (gr_fb_width() - 3 * button_bg_wide)/4;
	int button_bg_pos_y = gr_fb_height() - button_bg_height;

	int s_bg_wide = gr_get_width(alarm_main_surf[image_pos+1]);
	int s_bg_height = gr_get_height(alarm_main_surf[image_pos+1]);

	int bx = (gr_fb_width() - icon_main_wide)/2;
	int by = (gr_fb_height() - icon_main_height)/3;

	g_left_pos.width = button_bg_wide;
	g_left_pos.height = button_bg_height;

	g_left_pos.x0 = button_bg_pos_x;
	g_left_pos.y0 = button_bg_pos_y;
	g_left_pos.x1 = button_bg_pos_x + button_bg_wide;
	g_left_pos.y1 = button_bg_pos_y + button_bg_height;

	g_mid_pos.width = button_bg_wide;
	g_mid_pos.height = button_bg_height;

	g_mid_pos.x0 = g_left_pos.x1 + button_bg_pos_x;
	g_mid_pos.y0 = button_bg_pos_y;
	g_mid_pos.x1 = g_mid_pos.x0 + button_bg_wide;
	g_mid_pos.y1 = button_bg_pos_y + button_bg_height;

	g_right_pos.width = button_bg_wide;
	g_right_pos.height = button_bg_height;

	g_right_pos.x0 = g_mid_pos.x1 + button_bg_pos_x;
	g_right_pos.y0 = button_bg_pos_y;
	g_right_pos.x1 = g_right_pos.x0 + button_bg_wide;
	g_right_pos.y1 = button_bg_pos_y + button_bg_height;

	g_s_left_pos.width = s_bg_wide;
	g_s_left_pos.height = s_bg_height;

	g_s_left_pos.x0 = g_left_pos.x0 + (g_left_pos.width - g_s_left_pos.width)/2;
	LOGD("g_left_pos.y0: %d g_left_pos.height: %d g_s_left_pos.height:%d\n", g_left_pos.y0, g_left_pos.height, g_s_left_pos.height);
	g_s_left_pos.y0 = g_left_pos.y0 + (g_left_pos.height - g_s_left_pos.height)/2;

	g_s_mid_pos.width = s_bg_wide;
	g_s_mid_pos.height = s_bg_height;

	g_s_mid_pos.x0 = g_mid_pos.x0 + (g_mid_pos.width - g_s_mid_pos.width)/2;
	g_s_mid_pos.y0 = g_mid_pos.y0 + (g_mid_pos.height - g_s_mid_pos.height)/2;

	g_s_right_pos.width =  s_bg_wide;
	g_s_right_pos.height = s_bg_height;

	g_s_right_pos.x0 = g_right_pos.x0 + (g_right_pos.width - g_s_right_pos.width)/2;
	g_s_right_pos.y0 = g_right_pos.y0 + (g_right_pos.height - g_s_right_pos.height)/2;

	LOGD("left.x0 = %d\n",g_left_pos.x0);
	LOGD("left.y0 = %d\n",g_left_pos.y0);
	LOGD("left.x1 = %d\n",g_left_pos.x1);
	LOGD("left.y1 = %d\n",g_left_pos.y1);

	LOGD("mid.x0 = %d\n",g_mid_pos.x0);
	LOGD("mid.y0 = %d\n",g_mid_pos.y0);
	LOGD("mid.x1 = %d\n",g_mid_pos.x1);
	LOGD("mid.y1 = %d\n",g_mid_pos.y1);

	LOGD("right.x0 = %d\n",g_right_pos.x0);
	LOGD("right.y0 = %d\n",g_right_pos.y0);
	LOGD("right.x1 = %d\n",g_right_pos.x1);
	LOGD("right.y1 = %d\n",g_right_pos.y1);

	LOGD("s_left.x0 = %d\n",g_s_left_pos.x0);
	LOGD("s_left.y0 = %d\n",g_s_left_pos.y0);

	LOGD("s_mid.x0 = %d\n",g_s_mid_pos.x0);
	LOGD("s_mid.y0 = %d\n",g_s_mid_pos.y0);

	LOGD("s_right.x0 = %d\n",g_s_right_pos.x0);
	LOGD("s_right.y0 = %d\n",g_s_right_pos.y0);
	gr_color(0, 0, 0, 255);
	//gr_color(255, 255, 255, 255);
	gr_fill(0, 0, gr_fb_width(), gr_fb_height());

	gr_blit(alarm_main_surf[image_pos], 0, 0, icon_main_wide, icon_main_height, bx, by);
	gr_blit(alarm_main_surf[image_pos+4], 0, 0, button_bg_wide, button_bg_height, g_left_pos.x0, g_left_pos.y0);
	gr_blit(alarm_main_surf[image_pos+1], 0, 0, s_bg_wide, s_bg_height, g_s_left_pos.x0, g_s_left_pos.y0);
	gr_blit(alarm_main_surf[image_pos+4], 0, 0, button_bg_wide, button_bg_height, g_mid_pos.x0, g_mid_pos.y0);
	gr_blit(alarm_main_surf[image_pos+4], 0, 0, button_bg_wide, button_bg_height, g_right_pos.x0, g_right_pos.y0);

	gr_blit(alarm_main_surf[image_pos+2], 0, 0, s_bg_wide, s_bg_height, g_s_mid_pos.x0, g_s_mid_pos.y0);
	gr_blit(alarm_main_surf[image_pos+3], 0, 0, s_bg_wide, s_bg_height, g_s_right_pos.x0, g_s_right_pos.y0);

	int time_icon_y = by + icon_main_height + 30;
	if(image_pos == ALARM_WARNING_IMG_POS)
	{
		print_time_num(time_icon_y);
	}
	return ;
}

void show_boot_logo(int pos)
{
	gr_color(255, 255, 255, 255);
	gr_fill(0, 0, gr_fb_width(), gr_fb_height());

	gr_surface icon = alarm_main_surf[pos];
	int iconWidth = gr_get_width(icon);
	int iconHeight = gr_get_height(icon);
	int iconX = (gr_fb_width() - iconWidth) / 2;
	int iconY = (gr_fb_height() - iconHeight) / 2;
	gr_blit(icon, 0, 0, iconWidth, iconHeight, iconX, iconY);
	gr_flip();
	gr_flip();
	return;
}

void init_surface(int image_pos)
{
	int i = 0;
	char *prefix =NULL;
	char file_path[50] = {0};
	if(gr_fb_width() > WVGA_WIDTH || gr_fb_height() > WVGA_HEIGHT)
		prefix = "wvga/";
	else
		prefix = "hvga/";

	for(i = image_pos; Alarm_Main_Bitmaps[i].name != NULL; i++)
	{
		strcpy(file_path, prefix);
		strcat(file_path, Alarm_Main_Bitmaps[i].name);
		int result = res_create_surface(file_path, Alarm_Main_Bitmaps[i].surface);
		if (result < 0) {
			ERROR("creat surfaces failed!  %s", Alarm_Main_Bitmaps[i].name);
			*Alarm_Main_Bitmaps[i].surface = NULL;
		}
	}
}

void free_alarm_surface(int image_pos)
{
	int i = 0;
	for(i = image_pos; Alarm_Main_Bitmaps[i].name != NULL;i++)
	{
		res_free_surface(alarm_main_surf[i]);
	}
}


