#ifndef _BOOT_ALARM_H
#define _BOOT_ALARM_H

#define ALARM_MAIN_BACKGROUND_IMAGE  "icon_alarm"
#define ALARM_MAIN_LEFT_IMAGE        "button_sleep"
#define ALARM_MAIN_MID_IMAGE        "button_stop"
#define ALARM_MAIN_RIGHT_IMAGE       "button_poweron"
#define ALARM_BUTTON_BG_IMAGE        "button_bg"

#define ALARM_TIME_NUM_0                       "0"
#define ALARM_TIME_NUM_1                       "1"
#define ALARM_TIME_NUM_2                       "2"
#define ALARM_TIME_NUM_3                       "3"
#define ALARM_TIME_NUM_4                       "4"
#define ALARM_TIME_NUM_5                       "5"
#define ALARM_TIME_NUM_6                       "6"
#define ALARM_TIME_NUM_7                       "7"
#define ALARM_TIME_NUM_8                       "8"
#define ALARM_TIME_NUM_9                       "9"
#define ALARM_TIME_NUM_DOT                     "dot"

#define ALARM_BOOT_LOGO                     "boot_logo"

#define ALARM_WARNING_IMG_POS          0
#define ALARM_WARNING_IMG_NUM          6

#define ALARM_TIME_NUM_POS             (ALARM_WARNING_IMG_POS + ALARM_WARNING_IMG_NUM)
#define ALARM_TIME_NUM                 12
#define ALARM_TIME_NUM_DOT_POS         (ALARM_TIME_NUM_POS+10)

#define ALARM_BOOT_LOGO_POS         (ALARM_TIME_NUM_POS + ALARM_TIME_NUM)
#define ALARM_BOOT_LOGO_NUM				2


#define ERR_BOOT_ALARM_NOT_ALARM_ITEM    -11
#define BATTERY_STATE_PAUSE            1
#define BATTERY_STATE_EXIT             2
#define BATTERY_STATE_CHARGING         3

#define BOOT_ALARM_EXIT               0
#define BOOT_ALARM_ALARMING           1
#define BOOT_ALARM_STOP               2
#define BOOT_ALARM_ANOTHER            3
#define BOOT_ALARM_SLEEP              4

#define BOOT_STATE_NONE               0
#define ALARM_BUTTON_EVENT_LEFT       1
#define ALARM_BUTTON_EVENT_MID      2
#define ALARM_BUTTON_EVENT_RIGHT      3

#define BOOT_ALARM_DB_FILE          "/data/data/com.android.deskclock/databases/alarms.db"
#define BOOT_ALARM_DELAY_TIME_FILE   "/data/data/com.android.deskclock/files/alarm_delay.tmp"
#define MP3_PLAYER "/system/bin/mplayer"
#define BOOT_ALARM_SLEEP_TIME       (10*60)
#define BOOT_ALARM_LAST_TIME		(60)
#define BOOT_ALARM_DEFAULT_RING      "/system/res/sounds/default_ring.mp3"

#define VIB_PATH "/sys/class/timed_output/vibrator/enable"
#define RTC_TIME_PATH "/sys/class/rtc/rtc0/alarm_op"
#define ALARM_PARAM_PATH "/sys/class/rtc/rtc0/alarm_param"

#ifndef ABS_MT_POSITION_X
#define ABS_MT_POSITION_X     0x35
#endif
#ifndef ABS_MT_POSITION_Y
#define ABS_MT_POSITION_Y     0x36
#endif

#define WVGA_WIDTH 550
#define WVGA_HEIGHT 750

int musicProcess(char *filename, int op);
int  alarm_Set_Vib();
int  alarm_Set_NewHardWareAlarm(long int second);

struct alarm_db
{
	int id;
	int hour;
	int min;
	int dayofweek;
	int time;
	int enable;
	int vibrate;
	char message[512];
	char alert[512];
	struct alarm_db *next;
};

struct alarm_sec
{
	int day;
	int vibrate;
	char *alert;
	int secs;
	int db_id;
	struct alarm_sec *next;
};

struct position
{
	int width;
	int height;
	int x0;
	int y0;
	int x1;
	int y1;
};


struct boot_status
{
	int alarm_init;
	int battery_init;
	int battery_state;
	int alarm_state;
	int g_refresh_screen;
	int power_key_event;
};

struct position g_left_pos,g_right_pos, g_mid_pos;
struct position g_s_left_pos,g_s_right_pos, g_s_mid_pos;
struct alarm_sec *creat_alarm_sec_list(void);
int add_alarm_db_list(void);

void leds_set(unsigned char red, unsigned char green, unsigned short blue, int light_time, unsigned short  interval_time);
void free_bootcharge_surface(int image_pos);
void boot_alarm_exit();
void gr_clean(void);
int  set_state(char *state);
int get_latest_alarm_time(struct alarm_sec **fire_alarm);
void parse_key_event(struct input_event *ev);
void parse_alarm_event(struct input_event *ev);
char * uri_to_file(char *alert);

unsigned boot_alarm;
#endif
