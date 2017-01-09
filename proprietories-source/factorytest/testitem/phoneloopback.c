#include "testitem.h"

int at_cmd_audio_loop(int enable, int mode, int volume, int loopbacktype, int voiceformat, int delaytime)
{
    char buf[89];
    char *at_cmd = buf;
    if(volume >9) {
        volume = 9;
    }
    //AT+SPVLOOP=1,1,8,2,3,0 //mic+speaker;1,0 //mic + receiver;1,2 //headmic + head;1,4//main mic + headphone
    LOGD("mmitest en:%d,mode:%d,vol:%d,type:%d,format:%d,delay:%d",
            enable,mode,volume,loopbacktype,voiceformat,delaytime);

    snprintf(at_cmd, sizeof buf, "AT+SPVLOOP=%d,%d,%d,%d,%d,%d", enable,mode,volume,loopbacktype,voiceformat,delaytime);

    modem_send_at(-1,at_cmd,NULL,0,0);

    return 0;

}

int at_cmd_change_outindevice(int outdevice, int indevice)
{
    char buf[89];
    char *at_cmd = buf;

    LOGD("mmitest:outdevice:%d,indevice:%d", outdevice, indevice);
    //outdevice: 1.receiver 2.speaker 4.headphone; indevice: 1.mic0 2.mic1 4.headmic
    snprintf(at_cmd, sizeof buf, "AT+SPVLOOP=4,,,,,,%d,%d", outdevice, indevice);

    modem_send_at(-1, at_cmd, NULL, 0, 0);

    return 0;
}

#ifdef AUDIO_DRIVER_2
//used by whale2

static sem_t g_sem;

void *assisloopback_thread(void)
{
    char write_buf[1024] = {0};

//	snprintf(write_buf,sizeof(write_buf) - 1,"test_out_stream_route=2;test_in_stream_route=0x80000004;dsploop_delay=2500;dsploop_type=1;dsp_loop=1;");
    snprintf(write_buf,sizeof(write_buf) - 1,"test_out_stream_route=%d;test_in_stream_route=0x80000080;dsploop_type=1;dsp_loop=1;", AUDIO_DEVICE_OUT_SPEAKER);
    LOGD("write:%s", write_buf);
    SendAudioTestCmd((const uchar *)write_buf,strlen(write_buf));
    sem_wait(&g_sem);

    memset(write_buf,0,sizeof(write_buf));
    snprintf(write_buf,sizeof(write_buf) - 1,"dsp_loop=0");
    LOGD("write:%s", write_buf);
    SendAudioTestCmd((const uchar *)write_buf,strlen(write_buf));

    return NULL;
}

int test_assisloopback_start(void)
{
	int ret = 0;
	int row = 2;
	pthread_t t1;

	sem_init(&g_sem, 0, 0);
	ui_fill_locked();
	ui_show_title(MENU_TEST_ASSISLOOP);
	ui_clear_rows(row, 2);
	ui_set_color(CL_WHITE);
	row = ui_show_text(row, 0, TEXT_LB_MICRECEIVER);
	gr_flip();
	pthread_create(&t1, NULL, (void*)assisloopback_thread, NULL);
	ret = ui_handle_button(TEXT_PASS, NULL,TEXT_FAIL);//, TEXT_GOBACK
	sem_post(&g_sem);

	pthread_join(t1, NULL); /* wait "handle key" thread exit. */
	save_result(CASE_TEST_ASSISLOOP,ret);
	return ret;
}

void *mainloopback_thread(void)
{
    char write_buf[1024] = {0};

//	snprintf(write_buf,sizeof(write_buf) - 1,"test_out_stream_route=2;test_in_stream_route=0x80000004;dsploop_delay=2500;dsploop_type=1;dsp_loop=1;");
    snprintf(write_buf,sizeof(write_buf) - 1,"test_out_stream_route=%d;test_in_stream_route=0x80000004;dsploop_type=1;dsp_loop=1;", AUDIO_DEVICE_OUT_SPEAKER);
    LOGD("write:%s", write_buf);
    SendAudioTestCmd((const uchar *)write_buf,strlen(write_buf));
    sem_wait(&g_sem);

    memset(write_buf,0,sizeof(write_buf));
    snprintf(write_buf,sizeof(write_buf) - 1,"dsp_loop=0");
    LOGD("write:%s", write_buf);
    SendAudioTestCmd((const uchar *)write_buf,strlen(write_buf));

    return NULL;
}

int test_mainloopback_start(void)
{
	int ret = 0;
	int row = 2;
	pthread_t t1;

	sem_init(&g_sem, 0, 0);
	ui_fill_locked();
	ui_show_title(MENU_TEST_MAINLOOP);
	ui_set_color(CL_WHITE);

	ui_clear_rows(row, 2);
	ui_set_color(CL_WHITE);
	row = ui_show_text(row, 0, TEXT_LB_MICSPEAKER);
	gr_flip();
	pthread_create(&t1, NULL, (void*)mainloopback_thread, NULL);
	ret = ui_handle_button(TEXT_PASS, NULL,TEXT_FAIL);//, TEXT_GOBACK
	sem_post(&g_sem);

	pthread_join(t1, NULL); /* wait "handle key" thread exit. */
	save_result(CASE_TEST_MAINLOOP,ret);
	return ret;
}

#else
#ifdef AUTO_AUDIOLOOP
static int loop_result = 0;

void* loopback_thread(int inrouth)
{
    char write_buf[1024] = {0};
    char value[PROPERTY_VALUE_MAX] = {0};
    int re = 0;
    time_t start_time,now_time;

    snprintf(write_buf,sizeof(write_buf) - 1,"huaweilaohua_audio_loop_test=1;test_in_stream_route=0x%x;test_stream_route=2;samplerate=8000;channels=1", inrouth);
    LOGD("write:%s", write_buf);
    SendAudioTestCmd((const uchar *)write_buf,strlen(write_buf));
    start_time=time(NULL);
    sleep(18);
    do{
        memset(value, 0, sizeof(value));
        property_get("audioloop.test.result", value, "0");
        re = atoi(value);
        LOGD("audioloop.test.result calue is: %s, atoi re is :%d!", value, re);
        if(1 == re)
            loop_result = RL_PASS;
        else
            loop_result = RL_FAIL;
        now_time=time(NULL);
        if((now_time-start_time)>=GSENSOR_TIMEOUT) break;
        sleep(1);
    } while(0 == re);

    if(loop_result == RL_PASS){
        property_set("audioloop.test.result", "0");
        ui_set_color(CL_GREEN);
        ui_show_text(4, 0, TEXT_TEST_PASS);
//        memset(write_buf, 0, sizeof(write_buf));
//        snprintf(write_buf,sizeof(write_buf) - 1,"huaweilaohua_audio_loop_test=0");
//        LOGD("write:%s", write_buf);
//        SendAudioTestCmd((const uchar *)write_buf,strlen(write_buf));
    }else{
        ui_set_color(CL_RED);
        ui_show_text(4, 0, TEXT_TEST_FAIL);
    }
    gr_flip();
    sleep(1);
    return NULL;
}

int test_assisloopback_start(void)
{
    int ret = 0;
    int row = 2;
    pthread_t t1;

    ui_fill_locked();
    ui_show_title(MENU_TEST_ASSISLOOP);
    ui_clear_rows(row, 2);
    ui_set_color(CL_WHITE);
    row = ui_show_text(row, 0, TEXT_LB_MICRECEIVER);
    gr_flip();
    pthread_create(&t1, NULL, loopback_thread, (void*)0x80000080);
    pthread_join(t1, NULL); /* wait "handle key" thread exit. */
    save_result(CASE_TEST_ASSISLOOP, loop_result);
    return ret = loop_result;
}

int test_mainloopback_start(void)
{
    int ret = 0;
    int row = 2;
    pthread_t t1;

    ui_fill_locked();
    ui_show_title(MENU_TEST_MAINLOOP);
    ui_set_color(CL_WHITE);

    ui_clear_rows(row, 2);
    ui_set_color(CL_WHITE);
    row = ui_show_text(row, 0, TEXT_LB_MICSPEAKER);
    gr_flip();
    pthread_create(&t1, NULL, (void*)loopback_thread, (void*)0x80000004);
    pthread_join(t1, NULL); /* wait "handle key" thread exit. */
    save_result(CASE_TEST_MAINLOOP, loop_result);
    return ret = loop_result;
}

#else

int test_assisloopback_start(void)
{
    int ret = 0;
    int row = 2;

    ui_fill_locked();
    ui_show_title(MENU_TEST_ASSISLOOP);
    ui_clear_rows(row, 2);
    ui_set_color(CL_WHITE);
    row = ui_show_text(row, 0, TEXT_LB_MICRECEIVER);
    gr_flip();
    at_cmd_audio_loop(0,0,0,0,0,0);
    usleep(5*1000);
    at_cmd_audio_loop(1,3,8,2,3,0);
    at_cmd_change_outindevice(2,2);
    ret = ui_handle_button(TEXT_PASS,NULL,TEXT_FAIL);//, TEXT_GOBACK
    at_cmd_audio_loop(0,3,8,2,3,0);
    save_result(CASE_TEST_ASSISLOOP,ret);
    return ret;
}

int test_mainloopback_start(void)
{
    int ret = 0;
    int row = 2;

    ui_fill_locked();
    ui_show_title(MENU_TEST_MAINLOOP);
    ui_set_color(CL_WHITE);

    ui_clear_rows(row, 2);
    ui_set_color(CL_WHITE);
    row = ui_show_text(row, 0, TEXT_LB_MICSPEAKER);
    gr_flip();
    at_cmd_audio_loop(0,0,0,0,0,0);
    usleep(5*1000);
    at_cmd_audio_loop(1,1,8,2,3,0);
    at_cmd_change_outindevice(2,1);
    ret = ui_handle_button(TEXT_PASS, NULL,TEXT_FAIL);//, TEXT_GOBACK
    at_cmd_audio_loop(0,1,8,2,3,0);

    save_result(CASE_TEST_MAINLOOP,ret);
    return ret;
}

#endif

#endif