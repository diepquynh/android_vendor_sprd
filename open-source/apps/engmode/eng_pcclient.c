#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <semaphore.h>
#include "cutils/sockets.h"
#include "cutils/properties.h"
#include <private/android_filesystem_config.h>
#include "eng_pcclient.h"
#include "eng_diag.h"
#include "engopt.h"
#include "eng_at.h"
#include "eng_sqlite.h"
#include "eng_uevent.h"
#include "eng_debug.h"
#include "eng_cmd4linuxhdlr.h"

#define VLOG_PRI  -20
#define USB_CONFIG_VSER  "vser"
#define SYS_CLASS_ANDUSB_ENABLE "/sys/class/android_usb/android0/enable"
#define SYS_CLASS_ANDUSB_STATE "/sys/class/android_usb/f_gser/device/state"

sem_t g_armlog_sem;
eng_dev_info_t *g_dev_info = 0;
int g_ap_cali_flag = 0;
extern int g_armlog_enable;
extern int	disconnect_vbus_charger(void);
extern int turnoff_calibration_backlight(void);
static int eng_iqfeed_start(int num);
static void eng_check_whether_iqfeed(void);

static struct eng_param cmdparam = {
    .califlag = 0,
    .engtest = 0,
    .cp_type = "t",
    .connect_type = CONNECT_USB,
    .nativeflag = 0,
    .normal_cali = 0
};

static void set_vlog_priority(void)
{
    int inc = VLOG_PRI;
    int res = 0;

    errno = 0;
    res = nice(inc);
    if (res < 0){
        printf("cannot set vlog priority, res:%d ,%s\n", res,
                strerror(errno));
        return;
    }
    int pri = getpriority(PRIO_PROCESS, getpid());
    printf("now vlog priority is %d\n", pri);
    return;
}

/* Parse one parameter which is before a special char for string.
 * buf:[IN], string data to be parsed.
 * gap:[IN], char, get value before this charater.
 * value:[OUT] parameter value
 * return length of parameter
 */
static int cali_parse_one_para(char * buf, char gap, int* value)
{
    int len = 0;
    char *ch = NULL;
    char str[10] = {0};

    if(buf != NULL && value  != NULL){
        ch = strchr(buf, gap);
        if(ch != NULL){
            len = ch - buf ;
            strncpy(str, buf, len);
            *value = atoi(str);
        }
    }
    return len;
}

void eng_check_factorymode(int normal_cali)
{
    int ret;
    int fd;
    int status = eng_sql_string2int_get(ENG_TESTMODE);
    char status_buf[8];
    char modem_diag_value[PROPERTY_VALUE_MAX];
    char usb_config_value[PROPERTY_VALUE_MAX];
    char gser_config[] = {",gser"};
    char build_type[PROPERTY_VALUE_MAX];
    int usb_diag_set = 0;
    int i;
    int property_get_count=0;
#ifdef USE_BOOT_AT_DIAG
    fd=open(ENG_FACOTRYMODE_FILE, O_RDWR|O_CREAT|O_TRUNC, 0660);

    if(fd >= 0){
        ENG_LOG("%s: status=%x\n",__func__, status);
        if(chmod(ENG_FACOTRYMODE_FILE, 0660) < 0){
            ENG_LOG("%s: chmod %s to 666 failed ", __FUNCTION__, ENG_FACOTRYMODE_FILE);
        }
        property_get("ro.build.type",build_type,"not_find");
        ENG_LOG("%s: build_type: %s", __FUNCTION__, build_type);
        property_get("persist.sys.modem.diag",modem_diag_value,"not_find");
        ENG_LOG("%s: modem_diag_value: %s\n", __FUNCTION__, modem_diag_value);
        if((status==1)||(status == ENG_SQLSTR2INT_ERR)) {
            sprintf(status_buf, "%s", "1");
            if(strcmp(modem_diag_value,",none") == 0) {
                usb_diag_set = 1;
            }
        }else {
            sprintf(status_buf, "%s", "0");
            if(strcmp(modem_diag_value,",none") != 0) {
                usb_diag_set = 1;
                memcpy(gser_config, ",none", 5);
            }
        }

	ENG_LOG("%s: normal_cali: %d\n", __FUNCTION__, normal_cali);

	if((usb_diag_set && (0 == strcmp(build_type, "userdebug"))) || normal_cali){
            do{
                property_get("sys.usb.config",usb_config_value,"not_find");
                if(strcmp(usb_config_value,"not_find") == 0){
                    usleep(200*1000);
                    ENG_LOG("%s: can not find sys.usb.config\n",__FUNCTION__);
                    continue;
                }else{
		    if(normal_cali){
                        do{
                            usleep(100*1000);
                            property_get("sys.usb.config",usb_config_value,"not_find");
                            property_get_count++;
                        }while(0 == strcmp(usb_config_value, "not_find"));
                        ENG_LOG("%s: get sys.usb.config: %s,%d\n",__FUNCTION__,usb_config_value,property_get_count);
                        property_set("sys.usb.config", USB_CONFIG_VSER);
                        property_get("sys.usb.config",usb_config_value,"not_find");
                        ENG_LOG("%s: get sys.usb.config: %s\n",__FUNCTION__,usb_config_value);
                    }else{
                        property_set("persist.sys.modem.diag", gser_config);
                        ENG_LOG("%s: set usb property mass_storage,adb,vser,gser\n",__FUNCTION__);
                    }
                    break;
                }
            }while(1);
        }
        ret = write(fd, status_buf, strlen(status_buf)+1);
        ENG_LOG("%s: write %d bytes to %s",__FUNCTION__, ret, ENG_FACOTRYMODE_FILE);

        close(fd);
    }else{
        ENG_LOG("%s: fd: %d, status: %d\n", __FUNCTION__, fd, status);
    }
#endif

    fd=open(ENG_FACOTRYSYNC_FILE, O_RDWR|O_CREAT|O_TRUNC, 0660);
    if(fd >= 0)
        close(fd);
}

static int eng_parse_cmdline(struct eng_param * cmdvalue)
{
    int fd = 0;
    char cmdline[ENG_CMDLINE_LEN] = {0};
    char ssda_mode[PROPERTY_VALUE_MAX] = {0};
    char w_enable_mode[PROPERTY_VALUE_MAX] = {0};
    char *str = NULL;
    int mode =  0;
    int freq = 0;
    int device = 0;
    int len = -1;
    int ret = 0;

    if(cmdvalue == NULL)
        return -1;

    fd = open("/proc/cmdline", O_RDONLY);
    if (fd >= 0) {
        if ((ret = read(fd, cmdline, sizeof(cmdline)-1)) > 0){
            ALOGD("eng_pcclient: cmdline %s\n",cmdline);
            /*calibration*/
            str = strstr(cmdline, "calibration");
            if ( str  != NULL){
                cmdvalue->califlag = 1;
                disconnect_vbus_charger();
		turnoff_calibration_backlight();
                /*calibration= mode,freq, device. Example: calibration=8,10096,146*/
                str = strchr(str, '=');
                if(str != NULL){
                    str++;
                    /*get calibration mode*/
                    len = cali_parse_one_para(str, ',', &mode);
                    if(len > 0){
                        str = str + len + 1;
                        /*get calibration freq*/
                        len = cali_parse_one_para(str, ',', &freq);
                        /*get calibration device*/
                        str = str + len + 1;
                        len = cali_parse_one_para(str, ' ', &device);
                    }
                    switch(mode){
                        case 1:
                        case 5:
                            property_get("persist.modem.w.enable", w_enable_mode, "not_find");
                            if(0 == strcmp(w_enable_mode, "1")){
                                strcpy(cmdvalue->cp_type,"w");
                            }else{
                                strcpy(cmdvalue->cp_type,"gge");
                            }
                            break;
                        case 7:
                        case 8:
                            strcpy(cmdvalue->cp_type,"tl");
                            break;
                        case 11:
                        case 12:
                            strcpy(cmdvalue->cp_type,"w");
                            break;
                        case 14:
                        case 15:
			    property_get("persist.modem.w.enable", w_enable_mode, "not_find");
                            if(0 == strcmp(w_enable_mode, "1")){
                                strcpy(cmdvalue->cp_type,"w");
                            }else{
                                strcpy(cmdvalue->cp_type,"tl"); /*tddcsfb gsm cali and cali-post */
                            }
                            break;
                        case 16:
                        case 17:
                            strcpy(cmdvalue->cp_type,"lf");
                            break;
                        case 22:
                            g_ap_cali_flag = 1;
                            break;
                        default:
                            break;
                    }
                    property_get("persist.radio.ssda.mode", ssda_mode, "not_find");

                    if(0 == strcmp(ssda_mode, "tdd-csfb")){
                            strcpy(cmdvalue->cp_type,"tl");
                    }
                    else if(0 == strcmp(ssda_mode, "fdd-csfb")){
                            strcpy(cmdvalue->cp_type,"lf");
                    }
			else if(0 == strcmp(ssda_mode, "csfb")){
                            strcpy(cmdvalue->cp_type,"l");
                    }




                    /*Device[4:6] : device that AP uses;  0: UART 1:USB  2:SPIPE*/
                    cmdvalue->connect_type = (device >> 4) & 0x3;

                    if(device >>7)
                        cmdvalue->nativeflag = 1;
                    else
                        cmdvalue->nativeflag = 0;

                    ENG_LOG("eng_pcclient: cp_type=%s, connent_type(AP) =%d, is_native=%d, g_ap_cali_flag:%d\n",
                            cmdvalue->cp_type, cmdvalue->connect_type, cmdvalue->nativeflag, g_ap_cali_flag);
                }
            }else{
                /*if not in calibration mode, use default */
                strcpy(cmdvalue->cp_type,"t");
                cmdvalue->connect_type = CONNECT_USB;
            }

            str = strstr(cmdline, "androidboot.mode");
            ENG_LOG("%s: str: %s", __FUNCTION__, str);
            if(str != NULL){
                str = strchr(str, '=');
                ENG_LOG("%s: str: %s", __FUNCTION__, str);
                if(str != NULL){
                    str ++;
                    ENG_LOG("%s: str: %s", __FUNCTION__, str);
                    str = strstr(cmdline, "autotest");
                    ENG_LOG("%s: str: %s", __FUNCTION__, str);
                    if(str != NULL){
                        str = strchr(str, '=');
                        ENG_LOG("%s: str: %s", __FUNCTION__, str);
                        if(str != NULL){
                            str ++;
                            ENG_LOG("%s: str: %s", __FUNCTION__, str);
                            cali_parse_one_para(str, ' ', &(cmdvalue->normal_cali));
                            ENG_LOG("%s: cmdvalue->normal_cali: %d\n", __FUNCTION__, cmdvalue->normal_cali);
                            if(cmdvalue->normal_cali==1){//disable vbus charger in autotest mode
                                disconnect_vbus_charger();
                            }
                        }
                    }
                }
            }
            /*engtest*/
            if(strstr(cmdline,"engtest") != NULL)
                cmdvalue->engtest = 1;
        }
        close(fd);
    }

    ENG_LOG("eng_pcclient califlag=%d \n", cmdparam.califlag);

    return 0;
}

void eng_usb_enable(void)
{
    int fd  = -1;
    int ret = 0;

    fd = open(SYS_CLASS_ANDUSB_ENABLE, O_WRONLY);
    if(fd >= 0){
        ret = write(fd, "1", 1);
        ENG_LOG("%s: Write sys class androidusb enable file: %d\n", __FUNCTION__, ret);
        close(fd);
    }else{
        ENG_LOG("%s: Open sys class androidusb enable file failed!\n", __FUNCTION__);
    }
}

int eng_usb_state(void)
{
    int fd = -1;
    int ret = 0;
    char usb_state[32]= {0};

    fd = open(SYS_CLASS_ANDUSB_STATE, O_RDONLY);
    if(fd >= 0){
        ret = read(fd, usb_state, 32);
        if(ret > 0){
            if(0 == strncmp(usb_state, "CONFIGURED", 10)){
                ret = 1;
            }else{
                ENG_LOG("%s: usb state: %s\n", __FUNCTION__, usb_state);
                ret = 0;
            }
        }else{
            ret = 0;
            ENG_LOG("%s: Read sys class androidusb state file failed, read:%d\n", __FUNCTION__, ret);
        }

        close(fd);
    }else{
        ret = 0;
        ENG_LOG("%s: Open sys class androidusb state file failed, err: %s!\n", __FUNCTION__, strerror(errno));
    }

    return ret;
}

static void eng_get_usb_int(int argc, char** argv, char* at_dev, char* diag_dev, char* log_dev, char* type)
{
    int opt  = -1;

    do {
        opt = getopt(argc, argv, "p:a:d:l:");
        if(-1 == opt)
            continue;

        switch(opt){
            case 'p':
                strcpy(type, optarg);
                break;
            case 'a':
                strcpy(at_dev, optarg);
                break;
            case 'd':
                strcpy(diag_dev, optarg);
                break;
            case 'l':
                strcpy(log_dev, optarg);
                break;
            default:
                break;
        }
    }while(-1 != opt);

    return type;
}

static void eng_get_modem_int(char* type, char* at_chan, char* diag_chan, char* log_chan)
{
    char property_name[32] = {0};

    sprintf(property_name,"%s%s%s","ro.modem.",type,".diag");
    property_get(property_name, diag_chan, "not_find");
    ENG_LOG("%s %s diag_chan:%s", __FUNCTION__,property_name,diag_chan);

    sprintf(property_name,"%s%s%s","ro.modem.",type,".tty");
    property_get(property_name, at_chan, "not_find");
    ENG_LOG("%s %s at_chan:%s", __FUNCTION__,property_name,at_chan);

    sprintf(property_name,"%s%s%s","ro.modem.",type,".log");
    property_get(property_name, log_chan, "not_find");
    ENG_LOG("%s %s log_chan:%s", __FUNCTION__,property_name,log_chan);

    if(strcmp(type,"wcn") != 0 && 0 != strcmp(at_chan, "not_find")){
        strcat(at_chan, "31"); // channel31 is reserved for eng at
    }
}

static int eng_iqfeed_start(int num)
{
	int ret = -1;
	char status[PROPERTY_VALUE_MAX] = {0};
	while(num--)
	{
		memset(status, 0, PROPERTY_VALUE_MAX);
		property_set("ctl.start", "iqfeed");
		usleep(100 * 1000);

		property_get("init.svc.iqfeed", status, NULL);
		ENG_LOG("%s: svc iqfeed status = %s\n", __FUNCTION__, status);

		if( 0 == strcmp(status, "running") ) {
			ret = 0;
			break;
		}
	}
	return ret;
}

static void eng_check_whether_iqfeed(void)
{
	int ret, err;
	if (0 == access(IQMODE_FLAG_PATH, F_OK)) {
		err = remove(IQMODE_FLAG_PATH);
		if(err < 0) {
			ENG_LOG("%s: delete iqfeed flag file fail, %s\n", __FUNCTION__, strerror(errno));
			return NULL;
		}
		ret = eng_iqfeed_start(5);//start 5 times
		if(ret < 0) {
			ENG_LOG("%s: iqfeed start fail\n", __FUNCTION__);
			return NULL;
		}
	}
}

int main (int argc, char** argv)
{
    char cmdline[ENG_CMDLINE_LEN];
    char run_type[32] = {'t'};
    eng_thread_t t0,t1,t2,t3,t4;
    int fd;
    char set_propvalue[] = {"1"};
    char get_propvalue[PROPERTY_VALUE_MAX] = {0};
    eng_dev_info_t dev_info = {{"/dev/ttyGS0", "/dev/vser", 0, 1}, {0, 0, 0}};

    g_dev_info = &dev_info;

    eng_get_usb_int(argc, argv, dev_info.host_int.dev_at,
            dev_info.host_int.dev_diag, dev_info.host_int.dev_log,run_type);
    ENG_LOG("engpcclient runtype:%s, atPath:%s, diagPath:%s, logPath:%s, type: %d\n", run_type,
            dev_info.host_int.dev_at, dev_info.host_int.dev_diag,
            dev_info.host_int.dev_log, dev_info.host_int.dev_type);

    // Get the status of calibration mode & device type.
    eng_parse_cmdline(&cmdparam);
    // Correct diag path and run type by cmdline.
    if(1 == cmdparam.califlag){
        strcpy(run_type,cmdparam.cp_type);
        dev_info.host_int.cali_flag = cmdparam.califlag;
        dev_info.host_int.dev_type = cmdparam.connect_type;
        if(CONNECT_UART == cmdparam.connect_type){
            strcpy(dev_info.host_int.dev_diag, "/dev/ttyS1");
            dev_info.host_int.dev_type = CONNECT_UART;
        }
        if(CONNECT_USB == cmdparam.connect_type && g_ap_cali_flag){
	    eng_usb_enable();
	}
    }
    else
    {
        ENG_LOG("cmdparam.connect_type=%d\n", cmdparam.connect_type);
        dev_info.host_int.dev_type = cmdparam.connect_type;
    }

    eng_get_modem_int(run_type, dev_info.modem_int.at_chan, dev_info.modem_int.diag_chan,
            dev_info.modem_int.log_chan);
    ENG_LOG("eng_pcclient: modem at chan: %s, modem diag chan: %s, modem log chan: %s\n",
            dev_info.modem_int.at_chan, dev_info.modem_int.diag_chan, dev_info.modem_int.log_chan);

    set_vlog_priority();

    // Semaphore & log state initialization
    sem_init(&g_armlog_sem, 0, 0);

    if(0 != strcmp(run_type, "wcn")){
        fd = eng_file_lock();
        if(fd >= 0){
            property_get("sys.onemodem.start.enable", get_propvalue, "not_find");
            ENG_LOG("sys.onemodem.start.enable = %s",get_propvalue);
            if(0 == strcmp(get_propvalue, "not_find") || 0 != strcmp(get_propvalue, set_propvalue)){
                property_set("sys.onemodem.start.enable", set_propvalue);
                eng_file_unlock(fd);
                eng_init_test_file();
                eng_sqlite_create();
                if(cmdparam.califlag != 1){
                    if(cmdparam.normal_cali){
                    //Change gser port
                    memcpy(dev_info.host_int.dev_diag, "/dev/vser", sizeof("/dev/vser"));
                }
                // Check factory mode and switch device mode.
                eng_check_factorymode(cmdparam.normal_cali);
                if(cmdparam.normal_cali)
                    eng_autotestStart();
                }else{
                    // Initialize file for ADC
                    initialize_ctrl_file();
                }
                if(1 == cmdparam.califlag) {
                    if(0 != eng_thread_create(&t4, eng_printlog_thread, NULL)){
                        ENG_LOG("printlog thread start error");
                    }
                }
            }else{
                eng_file_unlock(fd);
            }
        }
    }

	/* Check whether the iqfeed shall be started. */
	eng_check_whether_iqfeed();
    if(0 != eng_thread_create(&t0, eng_uevt_thread, NULL)){
        ENG_LOG("uevent thread start error");
    }

    // Create vlog thread for reading diag data from modem and send it to PC.
    if (0 != eng_thread_create( &t1, eng_vlog_thread, &dev_info)){
        ENG_LOG("vlog thread start error");
    }

    // Create vdiag thread for reading diag data from PC, some data will be
    // processed by ENG/AP, and some will be pass to modem transparently.
    if (0 != eng_thread_create( &t2, eng_vdiag_wthread, &dev_info)){
        ENG_LOG("vdiag wthread start error");
    }

    if (0 != eng_thread_create( &t3, eng_vdiag_rthread, &dev_info)){
        ENG_LOG("vdiag rthread start error");
    }

    if(cmdparam.califlag != 1 || cmdparam.nativeflag != 1){
        eng_at_pcmodem(&dev_info);
    }

    while(1){
        sleep(10000);
    }

    return 0;
}
