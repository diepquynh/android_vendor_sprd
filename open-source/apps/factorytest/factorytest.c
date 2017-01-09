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
#include "minui.h"
#include "common.h"

#include "resource.h"
#include "testitem.h"
#include <cutils/android_reboot.h>

static int auto_all_test(void);
static int PCBA_auto_all_test(void);
static int show_phone_test_menu(void);
static int show_pcba_test_menu(void);
static int show_suggestion_test_menu(void);
static int show_phone_info_menu(void);

static int show_phone_test_result(void);
static int show_pcba_test_result(void);
static int show_suggestion_test_result(void);
static int phone_shutdown(void);
static int phone_reboot(void);

static int test_result_mkdir(void);
static int test_bt_wifi_init(void);
static int test_gps_init(void);
static int test_channel_init(void *fd);
static int test_printlog_init(void);
static int test_tel_init(void);
//static void test_bt_conf_init(void);
static void test_result_init(void);
static void test_item_init(void);
static void write_bin(char * pathname );
static void read_bin(void);
extern void temp_set_visible(int is_show);
extern int parse_config();
extern int gpsClose( void );
void* modem_init_func();

static pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER;

char pcba_phone=0;
char autotest_flag = 0;

extern int text_tp_rows,text_rows;
extern unsigned char menu_change;
extern unsigned cheight;

#define MULTI_TEST_CNT	5

#define SOCKET_NAME_MODEM_CTL "control_modem"
#define CMDLINE_SIZE	(0x1000)
static int control_fd = -1;

menu_info menu_root[] = {
#define MENUTITLE(num,title, func) \
	[ K_ROOT_##title ] = {num,MENU_##title, func, },
#include "./res/menu_root.h"
#undef MENUTITLE
    [K_MENU_ROOT_CNT]={0,MENU_FACTORY_RESET, phone_shutdown,},
};

menu_info menu_auto_test[] = {
#define MENUTITLE(num,title, func) \
	[ A_PHONE_##title ] = {num,MENU_##title, func, },
#include "./res/menu_auto_test.h"
#undef MENUTITLE
	//[K_MENU_AUTO_TEST_CNT] = {0,0,MENU_BACK, 0,},
};

menu_info menu_phone_test[] = {
#define MENUTITLE(num,title, func) \
	[ K_PHONE_##title ] = {num,MENU_##title, func, },
#include "./res/menu_phone_test.h"
#undef MENUTITLE
	//[K_MENU_PHONE_TEST_CNT] = {0,0,MENU_BACK, 0,},
};

menu_info menu_not_auto_test[] = {
#define MENUTITLE(num,title, func) \
	[ D_PHONE_##title ] = {num,MENU_##title, func, },
#include "./res/menu_not_autotest.h"
#undef MENUTITLE
	//[K_MENU_AUTO_TEST_CNT] = {0,0,MENU_BACK, 0,},
};

menu_info menu_phone_info_testmenu[] = {
#define MENUTITLE(num,title, func) \
	[ B_PHONE_##title ] = {num,MENU_##title, func, },
#include "./res/menu_phone_info.h"
#undef MENUTITLE
	//[K_MENU_INFO_CNT] = {0,0,MENU_BACK, 0,},
};

static mmi_show_data mmi_data_table[] = {
    {CASE_TEST_LCD,               MENU_TEST_LCD,			test_lcd_start,NULL},
    {CASE_TEST_TP,                MENU_TEST_TP,			test_tp_start,NULL},
    {CASE_TEST_MULTITOUCH,        MENU_TEST_MULTI_TOUCH,    		test_multi_touch_start,NULL},
    {CASE_TEST_VIBRATOR,          MENU_TEST_VIBRATOR,	    	 	test_vb_bl_start,NULL},
    {CASE_TEST_BACKLIGHT,         MENU_TEST_BACKLIGHT,	    		test_vb_bl_start,NULL},
    {CASE_TEST_LED,               MENU_TEST_LED,			test_led_start,NULL},
    {CASE_TEST_KEY,               MENU_TEST_KEY,			test_key_start,NULL},
    {CASE_TEST_FCAMERA,           MENU_TEST_FCAMERA,			test_fcamera_start,NULL},
    {CASE_TEST_BCAMERA,           MENU_TEST_BCAMERA,			test_bcamera_start,NULL},
    {CASE_TEST_FLASH,             MENU_TEST_FLASH, 			test_bcamera_start,NULL},
    {CASE_TEST_MAINLOOP,          MENU_TEST_MAINLOOP,			test_mainloopback_start,NULL},
    {CASE_TEST_ASSISLOOP,         MENU_TEST_ASSISLOOP,			test_assisloopback_start,NULL},
    {CASE_TEST_RECEIVER,          MENU_TEST_RECEIVER,			test_receiver_start,NULL},
    {CASE_TEST_CHARGE,            MENU_TEST_CHARGE,			test_charge_start,NULL},
    {CASE_TEST_SDCARD,            MENU_TEST_SDCARD,			test_sdcard_start,NULL},
    {CASE_TEST_EMMC,              MENU_TEST_EMMC,			test_emmc_start,NULL},
    {CASE_TEST_SIMCARD,           MENU_TEST_SIMCARD,			test_sim_start,NULL},
    {CASE_TEST_RTC,               MENU_TEST_RTC,			test_rtc_start,NULL},
    {CASE_TEST_HEADSET,           MENU_TEST_HEADSET,			test_headset_start,NULL},
    {CASE_TEST_FM,                MENU_TEST_FM,			test_fm_start,NULL},
    {CASE_TEST_GSENSOR,           MENU_TEST_GSENSOR,			test_gsensor_start,NULL},
    {CASE_TEST_LPSOR,             MENU_TEST_LSENSOR,			test_lsensor_start,NULL},
    {CASE_TEST_BT,                MENU_TEST_BT,			test_bt_start,NULL},
    {CASE_TEST_WIFI,              MENU_TEST_WIFI,			test_wifi_start,NULL},
    {CASE_TEST_GPS,               MENU_TEST_GPS,			test_gps_start,NULL},
    {CASE_TEST_TEL,               MENU_TEST_TEL,			test_tel_start,NULL},
    {CASE_TEST_OTG,               MENU_TEST_OTG,			test_otg_start,NULL},
};

menu_info multi_test_item[MULTI_TEST_CNT] = {
	{CASE_TEST_SDCARD,MENU_TEST_SDCARD, &test_sdcard_pretest},
	{CASE_TEST_SIMCARD,MENU_TEST_SIMCARD, test_sim_pretest},
	{CASE_TEST_WIFI,MENU_TEST_WIFI, test_wifi_pretest},
	{CASE_TEST_BT,MENU_TEST_BT, test_bt_pretest},
	{CASE_TEST_GPS,MENU_TEST_GPS, test_gps_pretest}
};

mmi_result* get_result_ptr(unsigned char id)
{
    mmi_result*ptr = NULL;

    if(id<TOTAL_NUM){
        if(1 == pcba_phone)
            ptr=&pcba_result[id];
        else
            ptr=&phone_result[id];
    }
    return ptr;
}

void save_result(unsigned char id,char key_result)
{
    mmi_result*ptr ;

    if( (RESULT_PASS == key_result) || (RESULT_FAIL == key_result)){
        ptr = get_result_ptr(id);
        ptr->pass_faild=key_result;

        if(CASE_TEST_TEL == id ||CASE_TEST_OTG == id){
            pcba_phone = 1;
            ptr = get_result_ptr(id);
            ptr->pass_faild=key_result;
        }
    }
}

int test_case_support(unsigned char id)
{
    hardware_result*ptr ;

    ptr = &support_result[id];

    return ptr->support;
}

static void read_bin(void)
{
    int fd_pcba = -1 ,fd_whole = -1;
    int len;

    fd_whole = open(PHONETXTPATH,O_RDWR);
    if (fd_whole < 0){
        LOGE("mmitest open %s failed",PHONETXTPATH);
        goto out;
    }

    pthread_mutex_lock(&result_mutex);
    len=read(fd_whole,phone_result,sizeof(phone_result));
    if(len < 0){
        LOGE("read %s failed,len = %d",PHONETXTPATH,len);
        goto out;
    }

    fd_pcba = open(PCBATXTPATH,O_RDWR);
    if (fd_pcba < 0 ) {
        LOGE("mmitest open %s failed.",PHONETXTPATH);
        goto out;
    }
    len=read(fd_pcba,pcba_result,sizeof(pcba_result));
    if(len < 0){
        LOGE("read %s failed, len = %d",PCBATXTPATH,len);
        goto out;
    }
    pthread_mutex_unlock(&result_mutex);

out:
    if(fd_whole >=  0)
        close(fd_whole);
    if(fd_pcba >= 0)
        close(fd_pcba);
}

static void write_bin(char * pathname)
{
    int i = 0,nums = 0,nump = 0;
    int len;
    int fd;

    fd = open(pathname,O_RDWR);
    if (fd < 0 ) {
        LOGE("mmitest open %s failed.",pathname);
        goto out;
    }

    pthread_mutex_lock(&result_mutex);
    if(1 == pcba_phone){
        for(i = 0; i < TOTAL_NUM-1; i++){
            if((1 == pcba_result[i].eng_support) &&
				(CASE_TEST_EMMC != pcba_result[i].function_id) && (CASE_TEST_RTC !=pcba_result[i].function_id) && (CASE_TEST_OTG !=pcba_result[i].function_id) && (CASE_TEST_TEL !=pcba_result[i].function_id)){
			nums++;
			if( RESULT_FAIL == pcba_result[i].pass_faild){
				pcba_result[TOTAL_NUM-1].pass_faild = RESULT_FAIL;
				break;
			}else if(RESULT_PASS == pcba_result[i].pass_faild){
				nump++;
			}
            }
        }
        if(nump == nums)
            pcba_result[TOTAL_NUM-1].pass_faild = RESULT_PASS;
        len = write(fd,pcba_result,sizeof(pcba_result));
        if(len < 0){
            LOGE("write %s failed,len = %d",pathname,len);
            goto out;
        }
    }else {
        for(i = 0; i < TOTAL_NUM-1; i++){
            if((1 == phone_result[i].eng_support) &&
				(CASE_TEST_EMMC !=phone_result[i].function_id) && (CASE_TEST_RTC != phone_result[i].function_id) && (CASE_TEST_OTG !=phone_result[i].function_id) && (CASE_TEST_TEL != phone_result[i].function_id)){
			nums++;
			if( RESULT_FAIL == phone_result[i].pass_faild){
				phone_result[TOTAL_NUM-1].pass_faild = RESULT_FAIL;
				break;
			}else if(RESULT_PASS == phone_result[i].pass_faild){
				nump++;
			}
            }
        }
        if(nump == nums)
            phone_result[TOTAL_NUM-1].pass_faild = RESULT_PASS;
        len = write(fd,phone_result,sizeof(phone_result));
        if(len < 0){
            LOGE("write %s failed,len = %d",pathname,len);
            goto out;
        }
    }
    sync();
    pthread_mutex_unlock(&result_mutex);

    LOGD("%s pcba_phone = %d,len = %d",pathname,pcba_phone,len);

 out:
    if(fd >= 0 )
        close(fd);
    return ;
}

static int show_root_menu(void)
{
	int chosen_item = -1;
	int i = 0,time_consume = 0,total_time = 0;
	const char* items[K_MENU_ROOT_CNT+2];
	int menu_cnt = K_MENU_ROOT_CNT+1;
	menu_info* pmenu = menu_root;
	time_t start_time,end_time;

	temp_set_visible(1);

	for(i = 0; i < menu_cnt; i++) {
		items[i] = pmenu[i].title;
	}
	items[menu_cnt] = NULL;

	while(1) {
		chosen_item = ui_show_menu(MENU_TITLE_ROOT, items, 1, chosen_item,K_MENU_ROOT_CNT+1);
		LOGD("mmitest chosen_item = %d",  chosen_item);
		if(chosen_item >= 0 && chosen_item < menu_cnt) {
			if(pmenu[chosen_item].func != NULL) {
				LOGD("mmitest select menu = <%s>", pmenu[chosen_item].title);
				start_time = time(NULL);
				pmenu[chosen_item].func();
				end_time = time(NULL);
				time_consume = end_time -start_time;
				total_time += time_consume;
				LOGD("mmitest select menu = <%s> consume time = %d",  pmenu[chosen_item].title,time_consume);
				LOGD("mmitest select menu = <%s> total time consume = %d", MENU_TITLE_ROOT,total_time);
			}
		}
		LOGD("mmitest chosen_item = %d,text_tp_rows=%d", chosen_item,text_tp_rows);
		if(chosen_item < text_tp_rows)
			menu_change=0;
		else menu_change=2;
	}
	return 0;
}

static void show_multi_test_result(void)
{
	int ret = RL_NA;
	int row = 3;
	int menu_cnt = 0;
	char tmp[128];
	char* rl_str;
	int i,j;
	menu_info pmenu[MULTI_TEST_CNT] = {{0}};
	menu_info ptest[MULTI_TEST_CNT] = {{0}};
	ui_fill_locked();
	ui_show_title(MENU_MULTI_TEST);
	gr_flip();

	for(i = 0; i < MULTI_TEST_CNT; i++) {
		if(test_case_support(multi_test_item[i].num)){
			ptest[menu_cnt]=multi_test_item[i];
			LOGD("mmitest pmenu[%d].title=%s",menu_cnt,pmenu[menu_cnt].title);
			menu_cnt++;
		}
	}

	for(i = 0; i < menu_cnt; i++) {
		ret = ptest[i].func();
		if(ret == RL_PASS) {
			ui_set_color(CL_GREEN);
			rl_str = TEXT_PASS;
		} else {
			ui_set_color(CL_RED);
			rl_str = TEXT_FAIL;
		}
		memset(tmp, 0, sizeof(tmp));
		for(j=0;i<K_MENU_AUTO_TEST_CNT;j++){
			if(ptest[i].num==menu_auto_test[j].num)
				break;
		}
		sprintf(tmp, "%s: %s", (menu_auto_test[j].title+1), rl_str);
		row = ui_show_text(row, 0, tmp);
		gr_flip();
	}

	sleep(1);
}

static void fail_retest(menu_info* auto_test)
{
	int i;
	menu_info* pmenu = auto_test;
	mmi_result*ptr;
	int result = RL_NA;
	autotest_flag = 1;

	for(i = pcba_phone; i < K_MENU_AUTO_TEST_CNT; i++){
		ptr = get_result_ptr(pmenu[i].num);
		if(RL_FAIL == ptr->pass_faild){
			result = pmenu[i].func();
		}
		if((RL_NEXT_PAGE == result)||(RL_BACK == result)){
			break;
		}
	}
	autotest_flag = 0;

}

static int auto_all_test(void)
{
	int i = 0, j = 0;
	int menu_cnt = 0;
	int result = 0,time_consume = 0;
	int key;
	char* rl_str;
	time_t start_time,end_time;

	test_bt_wifi_init();
	menu_info pmenu[K_MENU_PHONE_TEST_CNT] = {{0}};
	pcba_phone=0;
	autotest_flag = 1;

	for(i = 0; i < K_MENU_AUTO_TEST_CNT; i++) {
		if(test_case_support(menu_auto_test[i].num)){
			pmenu[menu_cnt]=menu_auto_test[i];
			LOGD("mmitest pmenu[%d].title=%s",menu_cnt,pmenu[menu_cnt].title);
			menu_cnt++;
		}
	}

	for(i = 0; i < menu_cnt; i++){
		for(j = 0; j < MULTI_TEST_CNT; j++) {
			if(pmenu[i].num== multi_test_item[j].num) {
				LOGD("mmitest break, id=%d", i);
				break;
			}
		}
		if(j < MULTI_TEST_CNT) {
			continue;
		}
		LOGD("mmitest Do, id=%d", i);
		if(pmenu[i].func != NULL) {
			start_time = time(NULL);
			result = pmenu[i].func();
			end_time = time(NULL);
			time_consume = end_time -start_time;
			LOGD("mmitest select menu = <%s> consume time = %d", pmenu[i].title,time_consume);
			if((RL_NEXT_PAGE == result)||(RL_BACK == result)){
				autotest_flag = 0;
				goto end;
			}
		}

	}
	show_multi_test_result();
	autotest_flag = 0;
	gpsClose();
	wifiClose();
	key = ui_handle_button(NULL,TEXT_TEST_FAIL_CASE,TEXT_GOBACK);
	if(RL_NEXT_PAGE == key){
		LOGD("mmitest retest fail cases!");
		fail_retest(pmenu);
	}
end:
	write_bin(PHONETXTPATH);
	return 0;
}


static int PCBA_auto_all_test(void)
{
	int i = 0, j = 0;
	int menu_cnt = 0;
	int result = 0,time_consume = 0;
	int key;
	char* rl_str;
	time_t start_time,end_time;

	test_gps_init();
	test_bt_wifi_init();
	menu_info pmenu[K_MENU_PHONE_TEST_CNT] = {{0}};
	pcba_phone=1;
	autotest_flag = 1;

	for(i = 1; i < K_MENU_AUTO_TEST_CNT; i++) {
		if(test_case_support(menu_auto_test[i].num)){
			pmenu[menu_cnt]=menu_auto_test[i];
			LOGD("mmitest pmenu[%d].title=%s",menu_cnt,pmenu[menu_cnt].title);
			menu_cnt++;
		}
	}

	for(i = 0; i < menu_cnt; i++){
		for(j = 0; j < MULTI_TEST_CNT; j++) {
			if(pmenu[i].num == multi_test_item[j].num) {
				LOGD("mmitest break, id=%d", i);
				break;
			}
		}
		if(j < MULTI_TEST_CNT) {
			continue;
		}
		if(pmenu[i].func != NULL) {
			LOGD("mmitest Do id=%d", i);
			start_time = time(NULL);
			result = pmenu[i].func();
			end_time = time(NULL);
			time_consume = end_time -start_time;
			LOGD("mmitest select menu = <%s> consume time = %d", pmenu[i].title,time_consume);
			if((RL_NEXT_PAGE == result)||(RL_BACK == result)){
				autotest_flag = 0;
				goto end;
			}
		}

	}
	show_multi_test_result();
	autotest_flag = 0;
	gpsClose();
	wifiClose();
	key = ui_handle_button(NULL,TEXT_TEST_FAIL_CASE,TEXT_GOBACK);
	if(RL_NEXT_PAGE == key){
		LOGD("mmitest retest fail cases!");
		fail_retest(pmenu);
	}
end:

	write_bin(PCBATXTPATH);
	return 0;
}


static int show_phone_test_menu(void)
{
	int chosen_item = -1;
	int i = 0;
	const char* items[K_MENU_PHONE_TEST_CNT+1];
	int menu_cnt = 0;
	int result = 0,time_consume = 0;;
	time_t start_time,end_time;
      menu_info pmenu[K_MENU_PHONE_TEST_CNT] = {{0}};
	pcba_phone=0;

	for(i = 0; i < K_MENU_PHONE_TEST_CNT; i++) {
		if(test_case_support(menu_phone_test[i].num)){
			items[menu_cnt] = menu_phone_test[i].title;
			pmenu[menu_cnt]=menu_phone_test[i];
			LOGD("mmitest items[%d]=%s,pmenu[%d].title=%s",menu_cnt,items[menu_cnt],menu_cnt,pmenu[menu_cnt].title);
			menu_cnt++;
		}
	}
	items[menu_cnt] = NULL;

	while(1) {
		LOGD("mmitest back to main");
		chosen_item = ui_show_menu(MENU_TITLE_PHONETEST, items, 0, chosen_item,menu_cnt);
		LOGD("mmitest chosen_item = %d", chosen_item);
		if(chosen_item >= 0 && chosen_item < menu_cnt) {
			LOGD("mmitest select menu = <%s>", pmenu[chosen_item].title);
			if(chosen_item >= K_MENU_PHONE_TEST_CNT) {
				return 0;
			}
			if(pmenu[chosen_item].func != NULL) {
				start_time = time(NULL);
				result = pmenu[chosen_item].func();
				LOGD("mmitest result=%d", result);
				end_time = time(NULL);
				time_consume = end_time -start_time;
				LOGD("mmitest select menu = <%s> consume time = %d", pmenu[chosen_item].title,time_consume);
			}
			write_bin(PHONETXTPATH);
		}else if (chosen_item < 0){
			return 0;
		}
    }
	return 0;
}

static int show_pcba_test_menu(void)
{
	int chosen_item = -1;
	int i = 0;
	const char* items[K_MENU_PHONE_TEST_CNT];
	int menu_cnt = 0;
	int result = 0,time_consume = 0;
	time_t start_time,end_time;
	menu_info pmenu[K_MENU_PHONE_TEST_CNT-1] = {{0}};
	pcba_phone=1;

	for(i = 1; i < K_MENU_PHONE_TEST_CNT; i++) {
		if(test_case_support(menu_phone_test[i].num)){
			items[menu_cnt] = menu_phone_test[i].title;
			pmenu[menu_cnt]=menu_phone_test[i];
			LOGD("mmitest items[%d]=%s,pmenu[%d].title=%s",menu_cnt,items[menu_cnt],menu_cnt,pmenu[menu_cnt].title);
			menu_cnt++;
		}
	}
	items[menu_cnt] = NULL;

	while(1) {
		chosen_item = ui_show_menu(MENU_TITLE_PHONETEST, items, 0, chosen_item,menu_cnt);
		LOGD("mmitest chosen_item = %d", chosen_item);
		if(chosen_item >= 0 && chosen_item < menu_cnt) {
			LOGD("mmitest select menu = <%s>", pmenu[chosen_item].title);
			if(chosen_item >= K_MENU_PHONE_TEST_CNT-1) {
				return 0;
			}
			if(pmenu[chosen_item].func != NULL) {
				start_time = time(NULL);
				result = pmenu[chosen_item].func();
				LOGD("mmitest result=%d", result);
				end_time = time(NULL);
				time_consume = end_time -start_time;
				LOGD("mmitest select menu = <%s> consume time = %d", pmenu[chosen_item].title,time_consume);

			}
			write_bin(PCBATXTPATH);
		}
		else if (chosen_item < 0){
			return 0;
		}
    }
	return 0;
}

static int show_suggestion_test_menu(void)
{
	int chosen_item = -1;
	int i = 0;
	const char* items[K_MENU_NOT_AUTO_TEST_CNT+1];
	int menu_cnt = 0;
	int result = 0,time_consume = 0;
	time_t start_time,end_time;
	menu_info pmenu[K_MENU_NOT_AUTO_TEST_CNT] = {{0}};
	pcba_phone=2;

	for(i = 0; i < K_MENU_NOT_AUTO_TEST_CNT; i++) {
		if(test_case_support(menu_not_auto_test[i].num)){
			items[menu_cnt]=menu_not_auto_test[i].title;
			pmenu[menu_cnt]=menu_not_auto_test[i];
			LOGD("mmitest items[%d]=%s,pmenu[%d].title=%s",menu_cnt,items[menu_cnt],menu_cnt,pmenu[menu_cnt].title);
			menu_cnt++;
		}
	}
	items[menu_cnt] = NULL;

	while(1) {
		LOGD("mmitest back to main");
		chosen_item = ui_show_menu(MENU_NOT_AUTO_TEST, items, 0, chosen_item,menu_cnt);
		LOGD("mmitest chosen_item = %d",chosen_item);
		if(chosen_item >= 0 && chosen_item < menu_cnt) {
			LOGD("mmitest select menu = <%s>",  pmenu[chosen_item].title);
			if(chosen_item >= menu_cnt) {
				return 0;
			}
			if(pmenu[chosen_item].func != NULL) {
				start_time = time(NULL);
				result = pmenu[chosen_item].func();
				LOGD("mmitest result=%d", result);
				end_time = time(NULL);
				time_consume = end_time -start_time;
				LOGD("mmitest select menu = <%s> consume time = %d", pmenu[chosen_item].title,time_consume);
			}
			write_bin(PHONETXTPATH);
			write_bin(PCBATXTPATH);
		}else if (chosen_item < 0){
			return 0;
		}
	}
	return 0;
}

static int show_phone_info_menu(void)
{
        int chosen_item = -1;
        int i = 0;
        const char* items[K_MENU_INFO_CNT+1];
        int menu_cnt = K_MENU_INFO_CNT;
        int result = 0,time_consume = 0;
        time_t start_time,end_time;

        menu_info* pmenu = menu_phone_info_testmenu;
        for(i = 0; i < menu_cnt; i++) {
            items[i] = pmenu[i].title;
        }
        LOGD("mmitest menu_cnt=%d",menu_cnt);
        items[menu_cnt] = NULL;
        while(1) {
            chosen_item = ui_show_menu(MENU_PHONE_INFO, items, 0, chosen_item,K_MENU_INFO_CNT);
            LOGD("mmitest chosen_item = %d", chosen_item);
            if(chosen_item >= 0 && chosen_item < menu_cnt) {
                LOGD("mmitest select menu = <%s>", pmenu[chosen_item].title);
                if(chosen_item >= K_MENU_INFO_CNT) {
                    return 0;
                }
                if(pmenu[chosen_item].func != NULL) {
                        start_time = time(NULL);
                        result = pmenu[chosen_item].func();
                        end_time = time(NULL);
                        time_consume = end_time -start_time;
                        LOGD("mmitest select menu = <%s> consume time = %d", pmenu[chosen_item].title,time_consume);
                }
            }else if (chosen_item < 0){
                    return 0;
            }
        }
        return 0;
}

static int show_phone_test_result(void)
{
	int i = 0,menu_cnt = 0;
	unsigned char id,num;
	char tmp[64][64];
	mmi_result*ptr;
	int chosen_item = -1;
	const char* items[64+1];
	int result = 0,time_consume = 0;
	time_t start_time,end_time;

	pcba_phone=0;
	num = sizeof(mmi_data_table)/sizeof(mmi_data_table[0])-2;
	while(1) {
		menu_cnt = 0;
		for(i = 0; i < num; i++){
                    id = mmi_data_table[i].id;
			if(!test_case_support(id))
				continue;
                    ptr = get_result_ptr(id);
                    memset(tmp[menu_cnt], 0, sizeof(tmp[menu_cnt]));
                    switch(ptr->pass_faild) {
				case RL_NA:
					sprintf(tmp[menu_cnt],"%s:%s",(mmi_data_table[i].name+2),TEXT_NA);
					break;
				case RL_FAIL:
					sprintf(tmp[menu_cnt],"%s:%s",(mmi_data_table[i].name+2),TEXT_FAIL);
					break;
				case RL_PASS:
					sprintf(tmp[menu_cnt],"%s:%s",(mmi_data_table[i].name+2),TEXT_PASS);
					break;
				case RL_NS:
					sprintf(tmp[menu_cnt],"%s:%s",(mmi_data_table[i].name+2),TEXT_NS);
					break;
				default:
					sprintf(tmp[menu_cnt],"%s:%s",(mmi_data_table[i].name+2),TEXT_NA);
					break;
                    }
                    menu_phone_result_menu[menu_cnt].title=tmp[menu_cnt];
                    menu_phone_result_menu[menu_cnt].func= mmi_data_table[i].func;
                    items[menu_cnt] = menu_phone_result_menu[menu_cnt].title;
                    LOGD("mmitest <%d>-%s", menu_cnt, menu_phone_result_menu[menu_cnt].title);
			 menu_cnt++;
		}
		menu_result_info* pmenu = menu_phone_result_menu;
		items[menu_cnt] = NULL;
		chosen_item = ui_show_menu(MENU_PHONE_REPORT, items, 0, chosen_item,menu_cnt);
		LOGD("mmitest chosen_item = %d", chosen_item);
		if(chosen_item >= 0 && chosen_item < menu_cnt) {
                    LOGD("mmitest select menu = <%s>", pmenu[chosen_item].title);
                    if(chosen_item >= menu_cnt) {
                        return 0;
                    }
                    if(pmenu[chosen_item].func != NULL) {
                        start_time = time(NULL);
                        result = pmenu[chosen_item].func();
                        end_time = time(NULL);
                        time_consume = end_time -start_time;
                        LOGD("mmitest select menu = <%s> consume time = %d", pmenu[chosen_item].title,time_consume);
                    }
                    write_bin(PHONETXTPATH);
		}else if (chosen_item < 0){
                    return 0;
		}
	}

      return 0;
}

static int show_pcba_test_result(void)
{
	int i = 0,menu_cnt = 0;
	unsigned char id,num;
	char tmp[64][64];
	mmi_result*ptr;
	int chosen_item = -1;
	const char* items[64+1];
	int result = 0,time_consume = 0;
	time_t start_time,end_time;

	pcba_phone=1;
	num = sizeof(mmi_data_table)/sizeof(mmi_data_table[0])-2;
      while(1) {
		menu_cnt = 0;
		for(i = 1; i < num; i++){
                    id = mmi_data_table[i].id;
			 if(!test_case_support(id))
				continue;
                    ptr = get_result_ptr(id);
                    memset(tmp[menu_cnt], 0, sizeof(tmp[menu_cnt]));
                    switch(ptr->pass_faild) {
				case RL_NA:
					sprintf(tmp[menu_cnt],"%s:%s",(mmi_data_table[i].name+2),TEXT_NA);
					break;
				case RL_FAIL:
					sprintf(tmp[menu_cnt],"%s:%s",(mmi_data_table[i].name+2),TEXT_FAIL);
					break;
				case RL_PASS:
					sprintf(tmp[menu_cnt],"%s:%s",(mmi_data_table[i].name+2),TEXT_PASS);
					break;
				case RL_NS:
					sprintf(tmp[menu_cnt],"%s:%s",(mmi_data_table[i].name+2),TEXT_NS);
					break;
				default:
					sprintf(tmp[menu_cnt],"%s:%s",(mmi_data_table[i].name+2),TEXT_NA);
					break;
                    }
                    menu_pcba_result_menu[menu_cnt].title=tmp[menu_cnt];
                    menu_pcba_result_menu[menu_cnt].func= mmi_data_table[i].func;
                    items[menu_cnt] = menu_pcba_result_menu[menu_cnt].title;
                    LOGD("mmitest <%d>-%s",menu_cnt, menu_pcba_result_menu[menu_cnt].title);
			 menu_cnt++;
		}
		menu_result_info* pmenu = menu_pcba_result_menu;
		items[menu_cnt] = NULL;
		chosen_item = ui_show_menu(MENU_BOARD_REPORT, items, 0, chosen_item,menu_cnt);
		LOGD("mmitest chosen_item = %d", chosen_item);
		if(chosen_item >= 0 && chosen_item < menu_cnt) {
                    LOGD("mmitest select menu = <%s>", pmenu[chosen_item].title);
                    if(chosen_item >= menu_cnt) {
                        return 0;
                    }
                    if(pmenu[chosen_item].func != NULL) {
                        start_time = time(NULL);
                        result = pmenu[chosen_item].func();
                        end_time = time(NULL);
                        time_consume = end_time -start_time;
                        LOGD("mmitest select menu = <%s> consume time = %d", pmenu[chosen_item].title,time_consume);
                    }
                    write_bin(PCBATXTPATH);
		}else if (chosen_item < 0){
                    return 0;
		}
	}

      return 0;
}

static int show_suggestion_test_result(void)
{
	int i = 0,menu_cnt = 0;
	unsigned char id,num;
	char tmp[64][64];
	mmi_result*ptr;
	int chosen_item = -1;
	const char* items[3];
	int result = 0,time_consume = 0;
	time_t start_time,end_time;

	pcba_phone=2;
	num = sizeof(mmi_data_table)/sizeof(mmi_data_table[0]);
	while(1) {
		menu_cnt = 0;
		for(i = 0; i < 2; i++){
                    id = mmi_data_table[num-2+i].id;
			 if(!test_case_support(id))
				continue;
                    ptr = get_result_ptr(id);
                    memset(tmp[menu_cnt], 0, sizeof(tmp[menu_cnt]));
                    switch(ptr->pass_faild) {
				case RL_NA:
					sprintf(tmp[menu_cnt],"%s:%s",(mmi_data_table[num-2+i].name+2),TEXT_NA);
					break;
				case RL_FAIL:
					sprintf(tmp[menu_cnt],"%s:%s",(mmi_data_table[num-2+i].name+2),TEXT_FAIL);
					break;
				case RL_PASS:
					sprintf(tmp[menu_cnt],"%s:%s",(mmi_data_table[num-2+i].name+2),TEXT_PASS);
					break;
				case RL_NS:
					sprintf(tmp[menu_cnt],"%s:%s",(mmi_data_table[num-2+i].name+2),TEXT_NS);
					break;
				default:
					sprintf(tmp[menu_cnt],"%s:%s",(mmi_data_table[num-2+i].name+2),TEXT_NA);
					break;
                    }
                    menu_not_suggestion_result_menu[menu_cnt].title=tmp[menu_cnt];
                    menu_not_suggestion_result_menu[menu_cnt].func= mmi_data_table[num-2+i].func;
                    items[menu_cnt] = menu_not_suggestion_result_menu[menu_cnt].title;
                    LOGD("mmitest <%d>-%s", menu_cnt, menu_not_suggestion_result_menu[menu_cnt].title);
			 menu_cnt++;
		}
		menu_result_info* pmenu = menu_not_suggestion_result_menu;
		items[menu_cnt] = NULL;
		chosen_item = ui_show_menu(MENU_NOT_AUTO_REPORT, items, 0, chosen_item,menu_cnt);
		LOGD("mmitest chosen_item = %d", chosen_item);
		if(chosen_item >= 0 && chosen_item < menu_cnt) {
                    LOGD("mmitest select menu = <%s>", pmenu[chosen_item].title);
                    if(chosen_item >= menu_cnt) {
                        return 0;
                    }
                    if(pmenu[chosen_item].func != NULL) {
                        start_time = time(NULL);
                        result = pmenu[chosen_item].func();
                        end_time = time(NULL);
                        time_consume = end_time -start_time;
                        LOGD("mmitest select menu = <%s> consume time = %d", pmenu[chosen_item].title,time_consume);
                    }
                    write_bin(PHONETXTPATH);
                    write_bin(PCBATXTPATH);
		}else if (chosen_item < 0){
                    return 0;
		}
      }

      return 0;
}

static int phone_reboot(void)
{
    LOGD("==== phone_reboot enter ====\n");
    sync();
    android_reboot(ANDROID_RB_RESTART2, 0, "normal");
    return 0;
}
static int phone_shutdown(void)
{
    int fd;
    int ret = -1;
    int time_consume = 0;;
    time_t start_time,end_time;

    start_time = time(NULL);
    ret = mkdir("/cache/recovery/",S_IRWXU | S_IRWXG | S_IRWXO);
    if (-1 == ret && (errno != EEXIST)) {
        LOGE("mkdir /cache/recovery/ failed.");
    }

    fd=open("/cache/recovery/command",O_WRONLY|O_CREAT,0777);
    if (fd >= 0) {
        write(fd,"--wipe_data\n--locale=zh_CN", strlen("--wipe_data\n--locale=zh_CN") + 1);
        write(fd, "--reason=wipe_data_via_recovery\n", strlen("--reason=wipe_data_via_recovery\n") + 1);
        sync();
        close(fd);
    } else {
        LOGE("open /cache/recovery/command failed");
        return -1;
    }

    end_time = time(NULL);
    time_consume = end_time -start_time;
    LOGD("mmitest select menu = <%s> consume time = %d",MENU_FACTORY_RESET,time_consume);
    usleep(200*1000);
    android_reboot(ANDROID_RB_RESTART2, 0, "recovery");
    return 0;
}

void eng_bt_wifi_start(void)
{
    LOGD("==== eng_bt_wifi_start ====");
    if(test_case_support(CASE_TEST_WIFI))
    	eng_wifi_scan_start();
    if(test_case_support(CASE_TEST_BT))
    	eng_bt_scan_start();
    LOGD("==== eng_bt_wifi_end ====");
}

static int test_bt_wifi_init(void)
{
    pthread_t bt_wifi_init_thread;
    pthread_create(&bt_wifi_init_thread, NULL, (void *)eng_bt_wifi_start, NULL);
    return 0;
}

void test_gps_open(void)
{
    int ret;
    if(test_case_support(CASE_TEST_GPS)){
    	ret = gpsOpen();
	if( ret < 0){
		LOGE("gps open error = %d",ret);
    	}
    }
}

static int test_gps_init(void)
{
    pthread_t gps_init_thread;
    pthread_create(&gps_init_thread, NULL, (void *)test_gps_open, NULL);
    return 0;
}

static int test_channel_init(void *fd)
{
    int len;
    char atbuf[AT_BUFFER_SIZE];
    int fp = *((int*)fd);

    for(;;){
        memset(atbuf, 0, AT_BUFFER_SIZE);
        len= read(fp, atbuf, AT_BUFFER_SIZE);
        if (len <= 0) {
            LOGE("mmitest [fp:%d] read stty_lte0 length error",fp);
            sleep(1);
            continue;
        }
    }

    return 0;
}

int  test_printlog_thread()
{
    int ret = -1;
    int fd = -1;

    LOGD("test_printlog_thread start");

    if (0 != access("/data/local/factorytest_log",F_OK) ){
        ret = mkdir("/data/local/factorytest_log",S_IRWXU | S_IRWXG | S_IRWXO);
        if (-1 == ret && (errno != EEXIST)) {
            LOGE("mkdir /data/local/factorytest_log failed.");
            return 0;
        }
    }
    ret = chmod("/data/local/factorytest_log",S_IRWXU | S_IRWXG | S_IRWXO);
    if (-1 == ret) {
        LOGE("chmod /data/local/factorytest_log failed.");
        return 0;
    }

    if (0 == access("/data/local/factorytest_log/last_factorytest.log",F_OK)){
        ret = remove("/data/local/factorytest_log/last_factorytest.log");
        if (-1 == ret) {
            LOGE("remove failed.");
            return 0;
        }
    }

    if (0 == access("/data/local/factorytest_log/factorytest.log",F_OK)){
        ret =  rename("/data/local/factorytest_log/factorytest.log","/data/local/factorytest_log/last_factorytest.log");
        if (-1 == ret) {
            LOGE("rename failed.");
            return 0;
        }
    }

    fd = open("/data/local/factorytest_log/factorytest.log",O_RDWR|O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
    if (fd == -1 && (errno != EEXIST)) {
        LOGE("creat /data/local/factorytest_log/factorytest.log failed.");
        return 0;
    }
    if (fd >= 0 )
        close(fd);

    ret = chmod("/data/local/factorytest_log/factorytest.log",0777);
    if (-1 == ret) {
        LOGE("chmod /data/local/factorytest_log/factorytest.log failed.");
        return 0;
    }

    ret = system("logcat -v threadtime -f /data/local/factorytest_log/factorytest.log &");
    if(!WIFEXITED(ret) || WEXITSTATUS(ret) || -1 ==  ret){
        LOGE("system failed.");
        return 0;
    }

    system("sync");
    return 1;
}
static int test_tel_init(void)
{
    int i = 0,n=0;
    char tel_autofeedback_port[MAX_MODEM_COUNT][BUF_LEN] = {{0}};
    char property[PROPERTY_VALUE_MAX];
    int at_fd[MAX_MODEM_COUNT];
    pthread_t t[MAX_MODEM_COUNT];

    property_get("persist.modem.l.enable", property, "not_find");
    if(!strcmp(property, "1")){
	  sprintf(tel_autofeedback_port[n++],"/dev/stty_lte0");
	  sprintf(tel_autofeedback_port[n++],"/dev/stty_lte3");
    }else{
	  sprintf(tel_autofeedback_port[n++],"/dev/stty_w0");
	  sprintf(tel_autofeedback_port[n++],"/dev/stty_w3");
    }
    for(i = 0;i < n;i++){
	 LOGD("n=%d tel_autofeedback_port[%d] =%s",n,i,tel_autofeedback_port[i]);
	 at_fd[i]=open(tel_autofeedback_port[i],O_RDWR);
	 if(at_fd[i]<0){
		LOGE("open readback channel %d faild",i);
	 }
	 if (0 != pthread_create(&t[i], NULL, (void *)test_channel_init,(void*)&(at_fd[i]))){
		LOGE("mmitest read %s thread start failed.",tel_autofeedback_port[i]);
	 }
    }

    return 0;
}

static int test_result_mkdir(void)
{
    int i,ret,len = 0;
    int fd_pcba = -1,fd_whole = -1;
    mmi_result result[64];

    ret = chmod("/productinfo",0777);
    if (-1 == ret) {
        LOGE("mmitest chmod /productinfo failed.");
        goto out;
    }

    if (0 != access(PCBATXTPATH,F_OK)){
	fd_pcba = open(PCBATXTPATH,O_RDWR|O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
	if (fd_pcba < 0) {
	    LOGE("mmitest,create %s failed.",PCBATXTPATH);
	    goto out;
	}

	//init /productinfo/PCBAtest.txt
	for(i = 0;i < 64; i++){
	    result[i].type_id = 1;
	    result[i].function_id = i;
	    result[i].eng_support= support_result[i].support;
	    result[i].pass_faild = RESULT_NOT_TEST;
	}
      result[0].eng_support= 0;//lcd not support
      len = write(fd_pcba,result,sizeof(result));
	if(len < 0){
	    LOGE("mmitest %s write_len = %d.",PCBATXTPATH,len);
	    goto out;
	}
	fsync(fd_pcba);
    }

    if (0 != access(PHONETXTPATH,F_OK)){
	fd_whole = open(PHONETXTPATH,O_RDWR|O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
	if (fd_whole < 0) {
	    LOGE("mmitest create %s failed.",PHONETXTPATH);
	    goto out;
	}
	//init /productinfo/wholetest.txt
	for(i = 0;i < 64; i++){
	    result[i].type_id = 0;
	    result[i].function_id= i;
	    result[i].eng_support= support_result[i].support;
	    result[i].pass_faild = RESULT_NOT_TEST;
	}
	len = write(fd_whole,result,sizeof(result));
	if(len < 0){
	    LOGE("mmitest write %s failed! write_len = %d.",PHONETXTPATH,len);
          goto out;
	}
	fsync(fd_whole);
    }

out:
    if(fd_pcba >= 0)
	close(fd_pcba);
    if(fd_whole >= 0)
	close(fd_whole);

    return 0;
}

static void detect_modem_control()
{
    int  numRead;
    char buf[128];

reconnect:
    LOGD("try to connect socket %s...", SOCKET_NAME_MODEM_CTL);

    do{
      usleep(10*1000);
      control_fd = socket_local_client(SOCKET_NAME_MODEM_CTL,
            ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
	LOGD("connect socket %s control_fd=%d",SOCKET_NAME_MODEM_CTL,control_fd);
    }while(control_fd < 0);
    LOGD("connect socket %s success",SOCKET_NAME_MODEM_CTL);

    do{
       memset(buf, 0, sizeof(buf));
       LOGD("Monioring modem state on socket %s...", SOCKET_NAME_MODEM_CTL);
       do {
          numRead = read(control_fd, buf, sizeof(buf));
          LOGD("read %d from fd %d",numRead,control_fd);
       } while(numRead < 0 && errno == EINTR);

       if(numRead <= 0) {
           close(control_fd);
           goto reconnect;
       }

       LOGD("read numRead=%d, buf=%s", numRead, buf);
       if (strstr(buf, "Modem Alive")) {
           break;
       }
    }while(1);

    close(control_fd);
    return;
}

static int factrory_parse_cmdline(char * cmdvalue)
{
	int fd = 0,ret=0;
	char cmdline[CMDLINE_SIZE] = {0};
	char *str = NULL;
	int val;

	if(cmdvalue == NULL){
		LOGD("cmd_value = NULL");
		return -1;
	}
	fd = open("/proc/cmdline", O_RDONLY);
	if (fd >= 0) {
		if ((ret = read(fd, cmdline, sizeof(cmdline)-1))> 0){
			cmdline[ret] = '\0';
			LOGD("cmdline %s",cmdline);
			str = strstr(cmdline, "modem=");
			if ( str != NULL){
				str += (sizeof("modem=")-1);
				*(strchr(str,' ')) = '\0';
			} else {
				LOGD("cmdline 'modem=' is not exist");
				goto ERROR;
			}
			LOGD("cmdline len = %d, str=%s",strlen(str),str);
			if(!strcmp(cmdvalue,str))
				val=1;
			else
				val=0;
			close(fd);
			return val;
		} else {
			LOGD("cmdline modem=NULL");
			goto ERROR;
		}
        } else {
		LOGD("/proc/cmdline open error");
		return 0;
	}
ERROR:
	close(fd);
	return 0;
}

int main()
{
    char property[PROPERTY_VALUE_MAX] = {0};
    pthread_t t1,t2;

    LOGD("==== factory test start ====");
    //create log thread for capture native mmi log in data/local/factorytet_log
    if (0 != pthread_create(&t1, NULL, (void *)test_printlog_thread, NULL)){
        LOGE("mmitest test_printlog_thread start failed.");
    }
    if(factrory_parse_cmdline("shutdown"))
        detect_modem_control();
    //init SIM and calibration information
    if (0 != pthread_create(&t2, NULL,(void *)modem_init_func,NULL)){
        LOGE("mmitest create modem_init_func thread failed.");
    }

    //init ui show
    ui_init();
    ui_set_background(BACKGROUND_ICON_NONE);
    //init telephone,read empty Automatic feedback channel
    test_tel_init();

    //parse related file to init test result for result showing in native mmi or pc tool
    parse_config();
    test_result_mkdir();
    read_bin();

    show_root_menu();


    return 1;
}
