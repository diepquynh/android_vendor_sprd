#include "testitem.h"

#define AUDCTL "/dev/pipe/mmi.audio.ctrl"
static sem_t g_sem;

enum aud_outdev_e {
    AUDIO_DEVICE_NONE  = 0,
    AUDIO_DEVICE_OUT_EARPIECE                  = 0x1,
    AUDIO_DEVICE_OUT_SPEAKER                   = 0x2,
    AUDIO_DEVICE_OUT_WIRED_HEADSET             = 0x4,
    AUDIO_DEVICE_OUT_WIRED_HEADPHONE           = 0x8,
};

int SendAudioTestCmd(const uchar * cmd,int bytes)
{
    int fd = -1;
    int ret=-1;
    int bytes_to_read = bytes;

    if(cmd==NULL){
        return -1;
    }

    if(fd < 0) {
        fd = open(AUDCTL, O_WRONLY | O_NONBLOCK);
    }

    if(fd < 0) {
        return -1;
    }else{
        do {
            ret = write(fd, cmd, bytes);
            if( ret > 0) {
                if(ret <= bytes) {
                    bytes -= ret;
                }
            }else if((!((errno == EAGAIN) || (errno == EINTR))) || (0 == ret)) {
                LOGE("pipe write error ,bytes read is %d",bytes_to_read - bytes);
                break;
            }else {
                LOGD("pipe_write_warning ret is %d",ret);
            }
        }while(bytes);
    }

    if(fd > 0) {
        close(fd);
    }

    if(bytes == bytes_to_read)
        return ret ;
    else
        return (bytes_to_read - bytes);
}

void* receiver_thread(char *t_mode)
{
    char write_buf[1024] = {0};
    char filepath[1024] = SPRD_AUDIO_FILE;

    if(AUDIO_DEVICE_OUT_EARPIECE == *t_mode ){
        snprintf(write_buf,
                    sizeof(write_buf) - 1,"out_devices_test=1;test_stream_route=%d;samplerate=44100;channels=2;filepath=%s;",AUDIO_DEVICE_OUT_EARPIECE,filepath);
        LOGD("write:%s", write_buf);
        SendAudioTestCmd((const uchar *)write_buf,strlen(write_buf));
    }else{
        LOGD("not receiver type t_mode=%d",*t_mode);
	  return NULL;
    }

    sem_wait(&g_sem);
    memset(write_buf,0,sizeof(write_buf));
    snprintf(write_buf,sizeof(write_buf) - 1,"out_devices_test=0");
    LOGD("write:%s", write_buf);
    SendAudioTestCmd((const uchar *)write_buf,strlen(write_buf));

    return NULL;
}

int test_receiver_start(void)
{
	int ret = 0;
	pthread_t t1;
	int row = 2;
	char t_mode = AUDIO_DEVICE_OUT_EARPIECE;

	sem_init(&g_sem, 0, 0);
	ui_fill_locked();
	ui_show_title(MENU_TEST_RECEIVER);
	ui_set_color(CL_WHITE);
	ui_show_text(row, 0, TEXT_RECV_PLAYING);
	gr_flip();
	pthread_create(&t1, NULL, (void*)receiver_thread, &t_mode);
	ret = ui_handle_button(TEXT_PASS, NULL,TEXT_FAIL);//, TEXT_GOBACK
	sem_post(&g_sem);

	pthread_join(t1, NULL); /* wait "handle key" thread exit. */
	save_result(CASE_TEST_RECEIVER,ret);
	return ret;
}
