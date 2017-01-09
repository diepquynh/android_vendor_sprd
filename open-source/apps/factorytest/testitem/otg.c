#include "testitem.h"

static int thread_run;
extern int usbin_state;


void* otg_check_thread(void)
{
    int fp,fp1,fp2,fp3;
    int i,cur_row,read_len;
    int j = 30;
    unsigned long value=0;
    char support,flag = 0;
    char sta_device[12]={0};
    char sta_insert[4]={0};
    char buffer[64],temp[64];
    char *endptr;

    while(thread_run==1){
	cur_row=3;
	if(access(OTG_FILE_PATH,F_OK)!=-1){
		fp=open(OTG_FILE_PATH,O_RDONLY);
		read(fp,&support,sizeof(char));
		close(fp);
		if(support=='1'){
		      fp1=open(OTG_INSERT_STATUS, O_RDONLY);
		      read(fp1,&sta_insert,sizeof(sta_insert));
		      close(fp1);
			if(NULL!=strstr(sta_insert, "low")){
				j = 30;
				if(0 == flag){
					ui_clear_rows(cur_row, 2);
					ui_set_color(CL_GREEN);
					cur_row=ui_show_text(cur_row, 0, OTG_INSERT);
					//gr_flip();
					usleep(500*1000);
					fp2=open(OTG_DEVICE_HOST,O_RDONLY);
					read(fp2,&sta_device,sizeof(sta_device));
					close(fp2);
					if(NULL!=strstr(sta_device, "Mode = 0x1")){
						LOGD("mmitest %s  Mode = 0x1",__func__ );
						cur_row=ui_show_text(cur_row, 0, OTG_HOST);
						flag = 1;
					}else if(NULL != strstr(sta_device, "Mode = 0x0")){
						cur_row=ui_show_text(cur_row, 0, OTG_DEVICE);
					}
					gr_flip();
					i = 10;
				}else
					cur_row = cur_row + 2;
				if(1 == flag){
					ui_clear_rows(cur_row, 4);
					ui_set_color(CL_GREEN);
					//sleep(3);
					if (0 == access("/sys/block/sda",F_OK)){
						cur_row = ui_show_text(cur_row, 0, TEXT_OTG_UDISK_OK);
						//gr_flip();
						fp3 = open("/sys/block/sda/size",O_RDONLY);
						read_len = read(fp3,buffer,sizeof(buffer));
						if(read_len > 0)
							value = strtoul(buffer,&endptr,0);
						close(fp3);
						LOGD("%s /sys/block/sda/size value = %lu, read_len = %d \n",__FUNCTION__, value, read_len);
						sprintf(temp, "%s %ld MB", TEXT_OTG_CAPACITY,(value/2/1024));
						cur_row = ui_show_text(cur_row, 0, temp);
						gr_flip();
						usbin_state = 0;
						i = 10;
					}else{
						ui_set_color(CL_RED);
						if(i){
							sprintf(temp, "%s, %ds", TEXT_OTG_UDISK_SCANING, i);
							ui_show_text(cur_row, 0, temp);
							gr_flip();
							sleep(1);
							i--;
						}else{
							cur_row = ui_show_text(cur_row, 0, TEXT_OTG_UDISK_NO);
							gr_flip();
						}
						usbin_state = 1;
					}
				}
			} else if(NULL!=strstr(sta_insert, "hign")){
				flag = 0;
				ui_clear_rows(cur_row, 4);
				ui_set_color(CL_RED);
				if(j){
					sprintf(temp, "%s, %ds", OTG_DEVICE_INSERT, j);
					cur_row = ui_show_text(cur_row, 0, OTG_UNSERT);
					cur_row = ui_show_text(cur_row, 0, temp);
					gr_flip();
					sleep(1);
					usbin_state = 1;
					j--;
				}else{
					cur_row = ui_show_text(cur_row, 0, TEXT_OTG_UDISK_NO);
					gr_flip();
					sleep(1);
					ui_push_result(RL_FAIL);
					break;
				}
			}
		}else{
			ui_set_color(CL_RED);
			cur_row = ui_show_text(cur_row, 0, OTG_NOT_SUPPORT);
			gr_flip();
			usleep(500*1000);
			usbin_state = 1;
			ui_push_result(RL_NA);
		}
	}else{
		ui_set_color(CL_RED);
		cur_row = ui_show_text(cur_row, 0, OTG_NOT_SUPPORT);
		gr_flip();
		usleep(500*1000);
		usbin_state = 1;
		ui_push_result(RL_NA);
	}
	//sleep(1);
    }
  return NULL;
}

int test_otg_start(void)
{
    int cur_row=2;
    int ret;
    pthread_t t1;

    ui_fill_locked();
    ui_show_title(MENU_TEST_OTG);
    ui_set_color(CL_WHITE);
    ui_show_text(cur_row, 0, OTG_TEST_START);
    gr_flip();

    thread_run=1;
    pthread_create(&t1, NULL, (void*)otg_check_thread, NULL);
    usleep(10*1000);
    ret = ui_handle_button(TEXT_PASS, NULL,TEXT_FAIL);
    thread_run=0;
    pthread_join(t1, NULL);
    save_result(CASE_TEST_OTG,ret);
    return ret;
}
