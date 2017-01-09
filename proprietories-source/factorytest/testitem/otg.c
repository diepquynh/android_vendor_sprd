#include "testitem.h"

static int thread_run;
extern int usbin_state;

#define Y_TYPEA    1
#define Y_TYPEC    2
#define Y_USB3     3
#define NO_OTG    -1

int cur_row;
int usb_type;

int is_support_otg(){
    int fp;
    char support[10] = {0};
    if(access(OTG_FILE_PATH, F_OK) != -1){
        fp = open(OTG_FILE_PATH,O_RDONLY);
        read(fp, &support, sizeof(char));
        close(fp);
        if(support[0] == '1'){
            LOGD("support type_A OTG");
            usb_type = Y_TYPEA;
            return Y_TYPEA;
        } else {
            return NO_OTG;
        }
    }
    if (access(USB3_OTG_ENABLE_PATH, F_OK) != -1){
        fp = open(USB3_OTG_ENABLE_PATH,O_RDONLY);
        read(fp, &support, sizeof(support));
        close(fp);
        if (NULL != strstr(support, "enable")){
            LOGD("support USB3 OTG");
            usb_type = Y_USB3;
            return Y_USB3;
        } else {
            return NO_OTG;
        }
    }
    if (access(TPYEC_OTG_ENABLE_PATH, F_OK) != -1){
        fp = open(TPYEC_OTG_ENABLE_PATH,O_RDONLY);
        read(fp, &support, sizeof(support));
        close(fp);
        if (NULL != strstr(support, "enable")){
            LOGD("support type_C OTG");
            usb_type = Y_TYPEC;
            return Y_TYPEC;
        } else {
            return NO_OTG;
        }
    }
    return NO_OTG;
}

int check_typeC_otg_status(){
    //LOGD("Do you poke the otg device into phone");
    char otg_status[10] = {0};
    int fd, j = 30;
    char temp[64];
    while (j){
        cur_row = 3;
        fd = open(OTG_TPYEC_STATUS_PATH, O_RDONLY);
        if (fd < 0){
            LOGE("open OTG_TPYEC_STATUS_PATH failed");
        }
        read(fd, &otg_status, sizeof(otg_status));
        close(fd);
        if (NULL != strstr(otg_status, "none")){
            ui_clear_rows(cur_row, 4);
            ui_set_color(CL_RED);
            if(j > 1){
                sprintf(temp, "%s, %ds", OTG_DEVICE_INSERT, j);
                cur_row = ui_show_text(cur_row, 0, OTG_UNSERT);
                cur_row = ui_show_text(cur_row, 0, temp);
                gr_flip();
                sleep(1);
                usbin_state = 1;
                ui_show_button(NULL, NULL,TEXT_FAIL);
                j--;
            }else{
                cur_row = ui_show_text(cur_row, 0, TEXT_OTG_UDISK_NO);
                gr_flip();
                sleep(1);
                ui_push_result(RL_FAIL);
                thread_run = 0;
                return 0;
            }
        }
        if (NULL != strstr(otg_status, "device")){
            LOGD("insert as device");
            return 2;
        }
        if (NULL != strstr(otg_status, "host")){
            LOGD("insert as host");
            return 1;
        }
        if (thread_run == 0){
            return -1;
        }
    }
    return 0;
}

int check_typeA_otg_status(){
    char otg_status[10] = {0};
    char sta_insert[4] = {0};
    char sta_device[12]={0};
    char temp[64];
    int fd, j = 30;
    while (j){
        cur_row = 3;
        fd = open(OTG_INSERT_STATUS, O_RDONLY);
        read(fd, &sta_insert, sizeof(sta_insert));
        close(fd);
        if(NULL != strstr(sta_insert, "low")){
            fd = open(OTG_DEVICE_HOST, O_RDONLY);
            read(fd,&sta_device,sizeof(sta_device));
            close(fd);
            if(NULL != strstr(sta_device, "Mode = 0x1")){
                LOGD("mmitest %s  Mode = 0x1",__func__ );
                return 1;
            }else if(NULL != strstr(sta_device, "Mode = 0x0")){
                LOGD("insert as host");
                return 2;
            }
        } else if (NULL!=strstr(sta_insert, "high")){
            ui_clear_rows(cur_row, 4);
            ui_set_color(CL_RED);
            if(j > 1){
                sprintf(temp, "%s, %ds", OTG_DEVICE_INSERT, j);
                cur_row = ui_show_text(cur_row, 0, OTG_UNSERT);
                cur_row = ui_show_text(cur_row, 0, temp);
                gr_flip();
                sleep(1);
                usbin_state = 1;
                ui_show_button(NULL, NULL,TEXT_FAIL);
                j--;
            }else{
                cur_row = ui_show_text(cur_row, 0, TEXT_OTG_UDISK_NO);
                gr_flip();
                sleep(1);
                ui_push_result(RL_FAIL);
                thread_run = 0;
                return 0;
            }
        }
        if (thread_run == 0){
            return -1;
        }
    }
    return 0;
}

int check_usb3_otg_status(){
    char otg_status[10] = {0};
    char sta_insert[4] = {0};
    char sta_device[12]={0};
    char temp[64];
    int fd, j = 30;
    while (j){
        cur_row = 3;
        fd = open(OTG_USB3_STATUS_PATH, O_RDONLY);
        read(fd, &sta_insert, sizeof(sta_insert));
        close(fd);
        if(NULL != strstr(sta_insert, "low")){
            LOGD("insert as host");
            return 1;
        } else if (NULL!=strstr(sta_insert, "high")){
            fd = open(USB_STATUS_PATH, O_RDONLY);
            read(fd,&sta_device,sizeof(sta_device));
            close(fd);
            if(NULL != strstr(sta_device, "CONFIGURED")){
                LOGD("insert as device");
                return 2;
            }

            ui_clear_rows(cur_row, 4);
            ui_set_color(CL_RED);
            if(j > 1){
                sprintf(temp, "%s, %ds", OTG_DEVICE_INSERT, j);
                cur_row = ui_show_text(cur_row, 0, OTG_UNSERT);
                cur_row = ui_show_text(cur_row, 0, temp);
                gr_flip();
                sleep(1);
                usbin_state = 1;
                ui_show_button(NULL, NULL,TEXT_FAIL);
                j--;
            }else{
                cur_row = ui_show_text(cur_row, 0, TEXT_OTG_UDISK_NO);
                gr_flip();
                sleep(1);
                ui_push_result(RL_FAIL);
                thread_run = 0;
                return 0;
            }
        }
        if (thread_run == 0){
            return -1;
        }
    }
    return 0;
}

void get_udisk_size(){
    int fd, i = 10;
    int read_len;
    unsigned long value=0;
    char buffer[64],temp[64];
    char *endptr;
    int row_temp = cur_row;
    while (i){
        cur_row = row_temp;
        ui_clear_rows(cur_row, 2);
        if (0 == access("/sys/block/sda",F_OK)){
            ui_set_color(CL_GREEN);
            cur_row = ui_show_text(cur_row, 0, TEXT_OTG_UDISK_OK);
            //gr_flip();
            fd = open("/sys/block/sda/size",O_RDONLY);
            read_len = read(fd,buffer,sizeof(buffer));
            if(read_len > 0)
                value = strtoul(buffer,&endptr,0);
            close(fd);
            LOGD("%s /sys/block/sda/size value = %lu, read_len = %d \n",__FUNCTION__, value, read_len);
            sprintf(temp, "%s %ld MB", TEXT_OTG_CAPACITY,(value/2/1024));
            cur_row = ui_show_text(cur_row, 0, temp);
            gr_flip();
            usbin_state = 0;
            ui_show_button(TEXT_PASS, NULL, TEXT_FAIL);
            return;
        }else{
            if (Y_TYPEA == usb_type)
                if (0 == check_typeA_otg_status())
                    return;
            if (Y_TYPEC == usb_type)
                if (0 == check_typeC_otg_status())
                    return;

            cur_row = row_temp;
            ui_set_color(CL_RED);
            if(i > 1){
                sprintf(temp, "%s, %ds", TEXT_OTG_UDISK_SCANING, i);
                ui_show_text(cur_row, 0, temp);
                gr_flip();
                sleep(1);
                i--;
            }else{
                cur_row = ui_show_text(cur_row, 0, TEXT_OTG_UDISK_NO);
                gr_flip();
                ui_push_result(RL_FAIL);
                thread_run = 0;
            }
            usbin_state = 1;
            ui_show_button(NULL, NULL,TEXT_FAIL);
        }
        if (thread_run == 0){
            return;
        }
    }
}

void* otg_check_thread(void){
    int flag = 0;
    int i;
    while(thread_run == 1){
        cur_row = 3;
        switch (is_support_otg()){
            case NO_OTG:{
                ui_set_color(CL_RED);
                cur_row = ui_show_text(cur_row, 0, OTG_NOT_SUPPORT);
                gr_flip();
                usleep(500*1000);
                usbin_state = 1;
                ui_show_button(NULL, NULL,TEXT_FAIL);
                ui_push_result(RL_NA);
                break;
            }
            case Y_TYPEA:{
                flag = check_typeA_otg_status();
                if (flag > 0){
                    cur_row = 3;
                    ui_clear_rows(cur_row, 2);
                    ui_set_color(CL_GREEN);
                    cur_row = ui_show_text(cur_row, 0, OTG_INSERT);
                }
                switch (flag){
                    case 0:{
                        break;
                    }
                    case 1:{
                        cur_row = ui_show_text(cur_row, 0, OTG_HOST);
                        get_udisk_size();
                        break;
                    }
                    case 2:{
                        cur_row = ui_show_text(cur_row, 0, OTG_DEVICE);
                        break;
                    }
                }
                gr_flip();
                break;
            }
            case Y_TYPEC:{
                flag = check_typeC_otg_status();
                if (flag > 0){
                    cur_row = 3;
                    ui_clear_rows(cur_row, 2);
                    ui_set_color(CL_GREEN);
                    cur_row = ui_show_text(cur_row, 0, OTG_INSERT);
                }
                switch (flag){
                    case 0:{
                        break;
                    }
                    case 1:{
                        cur_row = ui_show_text(cur_row, 0, OTG_HOST);
                        get_udisk_size();
                        break;
                    }
                    case 2:{
                        cur_row = ui_show_text(cur_row, 0, OTG_DEVICE);
                        sleep(1);
                        break;
                    }
                }
                gr_flip();
                break;
            }
            case Y_USB3:{
                flag = check_usb3_otg_status();
                if (flag > 0){
                    cur_row = 3;
                    ui_clear_rows(cur_row, 2);
                    ui_set_color(CL_GREEN);
                    cur_row = ui_show_text(cur_row, 0, OTG_INSERT);
                }
                switch (flag){
                    case 0:{
                        break;
                    }
                    case 1:{
                        cur_row = ui_show_text(cur_row, 0, OTG_HOST);
                        get_udisk_size();
                        break;
                    }
                    case 2:{
                        cur_row = ui_show_text(cur_row, 0, OTG_DEVICE);
                        sleep(1);
                        break;
                    }
                }
                gr_flip();
                break;
            }
        }
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
    usbin_state=1;
    usleep(10*1000);
    ret = ui_handle_button(TEXT_PASS, NULL,TEXT_FAIL);
    thread_run=0;
    pthread_join(t1, NULL);
    save_result(CASE_TEST_OTG,ret);
    usbin_state=0;
    return ret;
}
