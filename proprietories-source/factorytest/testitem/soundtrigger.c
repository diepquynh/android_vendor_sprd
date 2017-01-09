#include "testitem.h"
#include "sound_trigger.h"
#include "tinyalsa/asoundlib.h"

#define	SOUND_TRIGGER_DEV	"/dev/rt5512"
#define	SOUNDTRIGGER_DAT	"/system/etc/factorytest/SMicTD1.dat"
#define	SOUNDTRIGGER_FILE	"/system/etc/factorytest/sleep.pcm"
#define	STDEV_CTRL_MICBIAS2	"micbias2 power"
#define	STDEV_MIXER_DEV	(0)
#define	SOUND_TRIGGER_STOPPING_CAPTURING	(2)
struct mixer *mixer;
int fd ;
struct mixer_ctl *ctl_mic;
pthread_t callback_thread;

static sem_t g_sem;
static int thread_run = 0;
static int cur_row = 2;

static int soundtrigger_test_init(void)
{
	int ret;
	fd = open(SOUND_TRIGGER_DEV, O_RDWR);
	if(fd < 0)
	{
		LOGE("can't open dev! %d IN\n", __LINE__);
		return -1;
	}

	mixer = mixer_open(STDEV_MIXER_DEV);
    if (!mixer)
	{
		LOGE("can't open mixer! %d IN\n", __LINE__);
		return -1;
	}

    ctl_mic = mixer_get_ctl_by_name(mixer, STDEV_CTRL_MICBIAS2);
    if (!ctl_mic)
	{
		LOGE("can't get mixer name! %d IN\n", __LINE__);
		return -1;
	}

	ret = write(fd, "0" , 1);
	if (ret != 1) {
		LOGE("write dsp  error! %d IN\n", __LINE__);
		return -1;
	}
	return 1 ;
}

static int soundtrigger_test_load_module(void)
{
	int firmware_fd ;
	int ret = -1 ;
	unsigned char *firmware_data;
	int  firmware_size;
	struct stat firmware_stat;

	firmware_fd = open(SOUNDTRIGGER_DAT, O_RDONLY);
    LOGD("firmware_fd: %d! %d IN", firmware_fd, __LINE__);
	if(fstat(firmware_fd, &firmware_stat))
	{
		LOGE("can't get fstat! %d IN\n", __LINE__);
	}
	firmware_size = firmware_stat.st_size ;
	firmware_data = (unsigned char *)malloc(firmware_size);
	memset(firmware_data , 0 , firmware_size);

	ret = read(firmware_fd , firmware_data , firmware_size ) ;
	if(ret != firmware_size ){
		LOGE("read firmware file fail ret %d firmware_size %d\n" , ret , firmware_size );
	}

    LOGD("%s data size %d data %d - %d - %d - %d - %d - %d", __func__,
          firmware_size, firmware_data[0], firmware_data[1] , firmware_data[2],
          firmware_data[3], firmware_data[4], firmware_data[5]);

	ret = write(fd, (void*)firmware_data, firmware_size);
    if(ret == firmware_size){
        LOGD("load dsp firmware success! %d IN\n", __LINE__);
		ret = 1 ;
    } else{
		LOGE("load dsp firmware fail! %d IN\n", __LINE__);
	}

	free(firmware_data) ;
	close(firmware_fd);
	return ret ;

}

static void *callback_thread_loop(void)
{
	unsigned char capture_val = 0;

	LOGD("%s mmitest: soundtrigger thread sleep %d \n", __func__, capture_val);
	do {
		read(fd, &capture_val, 1);
		LOGD("%s mmitest: soundtrigger thread wake up %d \n", __func__, capture_val);
		if(capture_val == 1) {
			LOGD("mmitest: soundtrigger:wake up happened \n");
			ui_push_result(RL_PASS);
		} else {
			LOGD("mmitest: soundtrigger abort %d \n" , capture_val);
			break ;
		}
	} while(1) ;

	return NULL;

}

static void *tik_tok(void)
{
	time_t start_time,now_time;
	start_time=time(NULL);

	do{
		now_time = time(NULL);
		if (now_time - start_time > TRI_TIMEOUT ){
			LOGE("mmitest tel test failed");
			ui_set_color(CL_RED);
			cur_row = ui_show_text(cur_row, 0, TEXT_SOUNDTRIGGER_OPER3);
			gr_flip();
			sleep(1);
			ui_push_result(RL_FAIL);
			break;
		}
	}while(thread_run);

	return NULL;
}

int soundtrigger_test_start_recognition()
{
		LOGD("mmitest: init! %d IN\n", __LINE__);

	mixer_ctl_set_value(ctl_mic, 0, 1);
		LOGD("mmitest: init! %d IN\n", __LINE__);

	write(fd, "1" , 1);

		LOGD("mmitest: init! %d IN\n", __LINE__);

   pthread_create(&callback_thread, NULL, (void*)callback_thread_loop, NULL);
		LOGD("mmitest: init! %d IN\n", __LINE__);

	return 1 ;
}

int test_soundtrigger_start(void)
{
	int ret = -1, res = -1;
	char write_buf[1024] = {0};
	char *filepath = SOUNDTRIGGER_FILE;
	pthread_t tik_thread;
	cur_row = 2;

	ui_fill_locked();
	ui_show_title(MENU_TEST_SOUNDTRIGGER);
	ui_set_color(CL_WHITE);
	ui_show_text(cur_row, 0, TEXT_SOUNDTRIGGER_OPER);
	gr_flip();

	LOGD("mmitest: init! %d IN\n", __LINE__);
	soundtrigger_test_init();
	LOGD("mmitest: init! %d IN\n", __LINE__);

	res = soundtrigger_test_load_module();
	LOGD("mmitest: init! %d IN\n", __LINE__);

	if(res == 1){
		ui_clear_rows(cur_row, 1);
		ui_set_color(CL_GREEN);
		ui_show_text(cur_row, 0, TEXT_SOUNDTRIGGER_OPER1);
		gr_flip();

		do{
			res = ui_handle_button(NULL, TEXT_START, NULL);
		}while(RL_PASS == res);
		ui_clear_rows(cur_row, 1);
		if(RL_NEXT_PAGE == res){
			ui_set_color(CL_GREEN);
			cur_row = ui_show_text(cur_row, 0, TEXT_SOUNDTRIGGER_OPER4);
			gr_flip();
			snprintf(write_buf, sizeof(write_buf) - 1,"out_devices_test=%d;test_stream_route=0x1;samplerate=48000;channels=2;filepath=%s;", AUDIO_DEVICE_OUT_EARPIECE, filepath);
			LOGD("write:%s", write_buf);
			SendAudioTestCmd((const uchar *)write_buf,strlen(write_buf));
			LOGD("mmitest: init! %d IN\n", __LINE__);
			soundtrigger_test_start_recognition();
		} else {
			ret = RL_FAIL;
			goto end;
		}
	} else {
	   LOGE("load dsp firmware fail! %d IN\n", __LINE__);
	   ui_clear_rows(cur_row, 1);
	   ui_set_color(CL_RED);
	   cur_row = ui_show_text(cur_row, 0, TEXT_SOUNDTRIGGER_OPER2);
	   gr_flip();
	   sleep(1);
	   ret = RL_FAIL;
	   goto end;
	}

	thread_run = 1;
	pthread_create(&tik_thread, NULL, (void*)tik_tok, NULL);
	ret = ui_handle_button(TEXT_PASS,NULL,TEXT_FAIL);//, TEXT_GOBACK
	thread_run = 0;

	res = ioctl(fd, SOUND_TRIGGER_STOPPING_CAPTURING, (void *)NULL);
	if(!res)
		LOGD("stop recogniton success! %d IN\n", __LINE__);
	memset(write_buf,0,sizeof(write_buf));
	snprintf(write_buf,sizeof(write_buf) - 1,"out_devices_test=0");
	LOGD("write:%s", write_buf);
	SendAudioTestCmd((const uchar *)write_buf,strlen(write_buf));
	pthread_join(callback_thread, (void **) NULL);
	pthread_join(tik_thread, (void **) NULL);

end:
	if(ret == RL_PASS) {
		ui_set_color(CL_GREEN);
		ui_show_text(cur_row, 0, TEXT_TEST_PASS);
	} else {
		ui_set_color(CL_RED);
		ui_show_text(cur_row, 0, TEXT_TEST_FAIL);
	}
	gr_flip();
	sleep(1);
	mixer_ctl_set_value(ctl_mic, 0, 0);
	write(fd, "0" , 1);
	mixer_close(mixer);
	close(fd);
	save_result(CASE_TEST_SOUNDTRIGGER,ret);
	return ret;

}

