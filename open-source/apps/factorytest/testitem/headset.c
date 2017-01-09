#include "testitem.h"

static int headset_status = -1;
static int headset_key = -1;
static int thread_run;
extern int usbin_state;
/*
 * Read key code, check if pressed
 */
static int headset_keypressed(struct input_event *event)
{
	int ret = -1;
	LOGD("type=%d; value=%d; code=%d\n",event->type, event->value, event->code);

	if(event->type == EV_KEY) {
		if((event->code == SPRD_HEADSET_KEY)||(event->code==SPRD_HEADSET_KEYLONGPRESS)) {
			ret = event->value;
		}
	}

	LOGD("ret=%d", ret);
	return ret;
}

/*
 * Read headset switch node
 */
void* headset_check_thread(void)
{
	int fd, ret, input_fd=-1;
	int tmp = 0;
	fd_set rfds;
	char buffer[8];
	char str[128];
	struct timeval timeout;
	struct input_event event;
	int headset_key_count=0;
	int first_row = 2;
	int last_row = 3;
	int key_row = 3;
	headset_key = -1;
	headset_status = -1;

	memset(&timeout, 0, sizeof(timeout));
	timeout.tv_sec = 0;
	timeout.tv_usec = 500*1000;

	fd = open(SPRD_HEADSET_SWITCH_DEV, O_RDONLY);
	if(fd < 0) {
		LOGE("open %s fail", SPRD_HEADSET_SWITCH_DEV);
		goto err;
	}

	input_fd = get_sensor_name(SPRD_HEASETKEY_DEV);
	//input_fd = find_input_dev(O_RDONLY, SPRD_HEASETKEY_DEV);
	if(input_fd < 0) {
		LOGD("open %s fail", SPRD_HEASETKEY_DEV);
		goto err;
	}

	while(thread_run==1) {

		//check headset
		memset(buffer, 0, sizeof(buffer));
		lseek(fd, 0, SEEK_SET);
		ret = read(fd, buffer, sizeof(buffer));
		if(ret < 0) {
			LOGE("read fd fail error");
		} else {
			tmp = atoi(buffer);
		}
		LOGD("%d-%d",  tmp, headset_status);
		if(tmp != headset_status) {
			ui_clear_rows(first_row, (last_row - first_row + 1));
			ui_set_color(CL_WHITE);
			headset_status = tmp;
			//LOGD("[%s]: headset_status = %d\n", __func__, headset_status);
			switch(headset_status) {
				case 0:
					usbin_state=1;
					ui_set_color(CL_RED);
					last_row = ui_show_text(first_row, 0, TEXT_HD_UNINSERT);
					gr_flip();
					at_cmd_audio_loop(0,0,0,0,0,0);
					break;
				case 1:
					usbin_state=0;
					last_row = ui_show_text(first_row, 0, TEXT_HD_INSERTED);
					last_row = ui_show_text(last_row, 0, TEXT_HD_HAS_MIC);
					last_row = ui_show_text(last_row, 0, TEXT_HD_MICHD);
					sprintf(str, "%s%s", TEXT_HD_KEY_STATE, TEXT_HD_KEY_RELEASE);
					key_row = last_row;
					ui_show_text(key_row, 0, str);
					gr_flip();
					at_cmd_audio_loop(1,2,8,2,3,0);
					at_cmd_change_outindevice(4,4);
					break;
				case 2:
					usbin_state=0;
					last_row = ui_show_text(first_row, 0, TEXT_HD_INSERTED);
					last_row = ui_show_text(last_row, 0, TEXT_HD_NO_MIC);
					last_row = ui_show_text(last_row, 0, TEXT_HD_MICHD);
					sprintf(str, "%s%s", TEXT_HD_KEY_STATE, TEXT_HD_KEY_RELEASE);
					key_row = last_row;
					ui_show_text(key_row, 0, str);
					gr_flip();
					at_cmd_audio_loop(1,4,8,2,3,0);
					at_cmd_change_outindevice(4,1);
					break;
			}
			gr_flip();
		}

		//check headset key
		if(headset_status > 0) {
			FD_ZERO(&rfds);
			FD_SET(input_fd, &rfds);
			timeout.tv_sec = 0;
			timeout.tv_usec = 500*1000;
			ret = select(input_fd+1, &rfds, NULL, NULL, &timeout);

			if(ret > 0 && FD_ISSET(input_fd, &rfds)) {
				//read input event
				ret = read(input_fd, &event, sizeof(event));
				if (ret == sizeof(event)) {
					//handle key pressed
					tmp = headset_keypressed(&event);
					if(tmp >= 0 && tmp != headset_key) {
						headset_key = tmp;
						switch(headset_key) {
							case 0:
								sprintf(str, "%s%s", TEXT_HD_KEY_STATE, TEXT_HD_KEY_RELEASE);
								ui_clear_rows(key_row, 1);
								ui_set_color(CL_WHITE);
								ui_show_text(key_row, 0, str);
								headset_key_count++;
								break;
							case 1:
								sprintf(str, "%s%s", TEXT_HD_KEY_STATE, TEXT_HD_KEY_PRESSED);
								ui_clear_rows(key_row, 1);
								ui_set_color(CL_WHITE);
								ui_show_text(key_row, 0, str);
								headset_key_count++;
								break;
						}
						gr_flip();
					}
				}else {
					LOGD("read event too small %d",ret);
				}
			} else {
				//LOGD("%s: fd is not set", __func__);
			}
		} else {
			if(headset_key < 1) {
				headset_key = -1;
			}
			usleep(200*1000);
		}
		if(headset_key_count>=3)break;

		//show
		//headset_show();
	}

err:
//	at_cmd_audio_loop(0,0,0,0,0,0);
    if(headset_status==2)
        at_cmd_audio_loop(0,4,8,2,3,0);
    else
        at_cmd_audio_loop(0,2,8,2,3,0);
	if(fd > 0) close(fd);
	if(input_fd > 0) close(input_fd);
	ui_push_result(RL_PASS);
	return NULL;
}

int test_headset_start(void)
{
	int ret = 0;
	pthread_t t1, t2;
	ui_fill_locked();
	ui_show_title(MENU_TEST_HEADSET);
	gr_flip();

	thread_run=1;
	pthread_create(&t1, NULL, (void*)headset_check_thread, NULL);
	usleep(10*1000);
	ret = ui_handle_button(TEXT_PASS,NULL,TEXT_FAIL);//, TEXT_GOBACK
	thread_run=0;

	pthread_join(t1, NULL); /* wait "handle key" thread exit. */

	save_result(CASE_TEST_HEADSET,ret);
	return ret;
}
