#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cutils/properties.h>

#include "hardware_legacy/wifi.h"
#include "engopt.h"
#include "eng_at.h"

#define LCD_TEST        "LCD"
#define TP_TEST         "TP"
#define CAMERAF_TEST    "CAMERAF"
#define CAMERAB_TEST    "CAMERAB"
#define SD_TEST         "SD"
#define SIM1_TEST       "SIM1"
#define SIM2_TEST       "SIM2"
#define HEADSET_TEST    "HEADSET"
#define FM_TEST         "FM"
#define ATV_TEST        "ATV"
#define BT_TEST         "BT"
#define WIFI_TEST       "WIFI"
#define GPS_TEST        "GPS"
#define GSENSOR_TEST    "GSENSOR"
#define MSENSOR_TEST    "MSENSOR"
#define PSENSOR_TEST    "PSENSOR"

#define FILE_NAME           "/data/wifi_test"
#define RETURN_TO_MODEM     "AT+SPBTWIFICALI:AT+DEVT="
#define GSENSOR_IOCTL_BASE  77
#define TP_DEV              "/sys/devices/platform/sc8810-i2c.2/i2c-2/2-005c/calibrate"
#define SPRD_FM_DEV         "/dev/KT0812G_FM"
#define SPRD_SD_DEV         "/dev/block/mmcblk0"
#define msensor_dev         "/dev/akm8975_dev"
#define PSENSOR_DEV         "/sys/devices/platform/sc8810-i2c.0/i2c-0/0-001c/version"
#define SPRD_GSENSOR_DEV    "/dev/lis3dh_acc"

#define GSENSOR_IOCTL_SET_ENABLE      _IOW(GSENSOR_IOCTL_BASE, 2, int)
#define LIS3DH_ACC_IOCTL_GET_CHIP_ID  _IOR(GSENSOR_IOCTL_BASE, 255, char[32])
#define KT0812G_FM_IOCTL_BASE         'R'
#define KT0812G_FM_IOCTL_ENABLE       _IOW(KT0812G_FM_IOCTL_BASE, 0, int)
#define MSENSOR_DEVICE_IOCTL_READ     _IOWR(0xA1, 0x01, char*)

void parse_space(char *buf);
int get_son_str(char *buf, char *ret_buf, char a, char b);
int test_dev(char *cmd, char *ret_buf);
int write_result_to_modem(char *dev, int item, char *result);

static void eng_simcard_poweron(int index)
{
#if 0 //@alvin: FIX ME
    int fd, ret;
    char cmd[32];

    ENG_LOG("%s", __FUNCTION__);

    //open sim1
    fd = engapi_open(index);

    ENG_LOG("%s: SIM%d fd=%d", __FUNCTION__, index, fd);
    if (fd < 0)
        return;

    do {
        memset(cmd, 0, sizeof(cmd));
        sprintf(cmd, "%d,%d,%s", ENG_AT_NOHANDLE_CMD, 1, "AT+SFUN=2");
        ENG_LOG("%s: send %s", __FUNCTION__, cmd);
        engapi_write(fd, cmd, strlen(cmd));
        memset(cmd, 0, sizeof(cmd));
        engapi_read(fd, cmd, sizeof(cmd));
        ENG_LOG("%s: response=%s\n", __FUNCTION__, cmd);
        sleep(1);
    } while (strstr(cmd, "OK") == NULL);

    engapi_close(fd);
#endif
}

//pre power sim on
static void eng_pre_powersimon(void)
{
    char simtype[PROPERTY_VALUE_MAX];
    int sim;
    memset(simtype, 0, sizeof(simtype));
    property_get("persist.msms.phone_count", simtype, "");
    ENG_LOG("%s: persist.msms.phone_count = %s", __FUNCTION__, simtype);

    sim = atoi(simtype);

    eng_simcard_poweron(0);
    if (sim == 2)
        eng_simcard_poweron(1);
}

static int en_simtest_checksim(int type)
{
#if 0  // FIX ME : @alvin
    static int i = 0;
    int fd, ret = 0;
    char cmd[32];
    char simstatus;

    fd = engapi_open(type);

    ENG_LOG("%s: type=%d\n", __FUNCTION__, type);

    //power on sim only once
    if (i == 0) {
        ENG_LOG("ttt: is is: %d\n", i);
        eng_pre_powersimon();
        i = 1;
    }

    if (fd < 0)
        return 0;

    memset(cmd, 0, sizeof(cmd));
    sprintf(cmd, "%d,%d", ENG_AT_EUICC, 0);
    engapi_write(fd, cmd, strlen(cmd));
    memset(cmd, 0, sizeof(cmd));
    engapi_read(fd, cmd, sizeof(cmd));
    simstatus = cmd[0];
    ENG_LOG("%s: response=%s, buffer=%c\n", __FUNCTION__, cmd, simstatus);

    if (simstatus == '2') {
        ret = 0;
    }
    else if ((simstatus == '0') || (simstatus == '1')) {
        ret = 1;
    }

    engapi_close(fd);

    return ret;
#else
    return 0;
#endif
}

static int eng_gsensortest_open(void)
{
    int fd;
    int enable = 1;

    fd = open(SPRD_GSENSOR_DEV, O_RDWR);
    ENG_LOG("Open %s fd:%d", SPRD_GSENSOR_DEV, fd);

    if(fd >= 0){
       if (ioctl(fd, GSENSOR_IOCTL_SET_ENABLE, &enable)) {
        ENG_LOG("Set G-sensor enable in: %s", __FUNCTION__);
        close(fd);
        return -1;
       }
    }
    return fd;
}

void parse_space(char *buf)
{
    char *p = buf;
    int len = 0;

    ENG_LOG("ttt: enter parse\n");
    if (buf == NULL)
        return;

    while (*p != '\0') {
        if (*p == ' ') {
            len = strlen(p);
            memmove(p, p + 1, len);
            *(p + len - 1) = '\0';
        }
        p++;
    }
}

int get_son_str(char *buf, char *ret_buf, char a, char b)
{
    char *cur = NULL;
    char *p = NULL;
    char *end = buf;
    int copy_size = 0;

    ENG_LOG("ttt: get_son str\n");

    if (buf == NULL || ret_buf == NULL)
        return -1;
    while (*end != '\0')
        end++;

    cur = strchr(buf, a);
    if (cur == NULL)
        return -1;

    if (b != 0)
        end = strchr(cur, b);
    else {
        p = strchr(cur, 13);
        if (p != NULL) {
            ENG_LOG("tttt: p is not null\n");
            end = p;
        }
    }

    copy_size = end - cur - 1;

    memcpy(ret_buf, cur + 1, copy_size);

    return 1;
}

static int test_lcd(int test_item, char *ret_buf)
{
    ENG_LOG("xlcd\n");
    strcpy(ret_buf, "PASS");
    return 0;
}

static int test_tp(int test_item, char *ret_buf)
{
    ENG_LOG("tp\n");

    int ret = -1;
    ret = access(TP_DEV, F_OK);
    if (ret < 0) {
        strcpy(ret_buf, "FAIL");
        return -1;
    }

    strcpy(ret_buf, "PASS");

    return 0;
}

static int test_cameraf(int test_item, char *ret_buf)
{
    ENG_LOG("cameraf\n");
    strcpy(ret_buf, "PASS");
    return 0;
}

static int test_camerab(int test_item, char *ret_buf)
{
    ENG_LOG("camerab\n");
    strcpy(ret_buf, "PASS");
    return 0;
}

static int test_sd(int test_item, char *ret_buf)
{
    int ret = 0;

    ENG_LOG("sd\n");
    ret = access(SPRD_SD_DEV, F_OK);
    if (ret < 0) {
        strcpy(ret_buf, "FAIL");
        return -1;
    }

    strcpy(ret_buf, "PASS");

    return 0;
}

static int test_sim1(int test_item, char *ret_buf)
{
    int sim1_in = 0;
    ENG_LOG("sim1\n");

    sim1_in = en_simtest_checksim(0);
    ENG_LOG("ttt: sim1_in= %d\n", sim1_in);
    if (sim1_in == 0) {
        strcpy(ret_buf, "FAIL");
        return -1;
    }

    strcpy(ret_buf, "PASS");
    return 0;
}

static int test_sim2(int test_item, char *ret_buf)
{
    int sim2_in = 0;
    ENG_LOG("sim2\n");

    sim2_in = en_simtest_checksim(1);
    ENG_LOG("ttt: sim2_in= %d\n", sim2_in);
    if (sim2_in == 0) {
        strcpy(ret_buf, "FAIL");
        return -1;
    }

    strcpy(ret_buf, "PASS");

    return 0;
}

static int test_headset(int test_item, char *ret_buf)
{
    ENG_LOG("headset\n");
    strcpy(ret_buf, "PASS");
    return 0;
}

static int test_fm(int test_item, char *ret_buf)
{
    int fd = 0;
    int on_off = 0;

    ENG_LOG("fm\n");
    fd = open(SPRD_FM_DEV, O_RDWR);

    if (fd < 0) {
        strcpy(ret_buf, "FAIL");
        return -1;
    }

    if (ioctl(fd, KT0812G_FM_IOCTL_ENABLE, &on_off) < 0) {
        ENG_LOG("%s: ioctl open fm fail ", __FUNCTION__);
        strcpy(ret_buf, "FAIL");
        close(fd);
        return -1;
    }

    strcpy(ret_buf, "PASS");

    close(fd);

    return 0;
}

static int test_atv(int test_item, char *ret_buf)
{
    ENG_LOG("atv\n");
    strcpy(ret_buf, "PASS");
    return 0;
}

static int test_bt(int test_item, char *ret_buf)
{
    int ret = 0;
    char buf[PROPERTY_VALUE_MAX] = "";

    ENG_LOG("bt\n");

    //ret = property_get("ro.mac.bluetooth",mac,"");
    ret = property_get("bccmd.download.status", buf, "");
    ENG_LOG("ttt: result is: %s\n", buf);
    if (ret < 0) {
        strcpy(ret_buf, "FAIL");
        return -1;
    }
    strcpy(ret_buf, "PASS");
    return 0;
}

static int test_wifi(int test_item, char *ret_buf)
{
    char buff[PROPERTY_VALUE_MAX] = { 0 };
    ENG_LOG("wifi");

    property_get("wifi.init", buff, "");

    if (strstr(buff, "PASS") != NULL) {
        strcpy(ret_buf, "PASS");
    }
    else {
        strcpy(ret_buf, "FAIL");
    }

    ENG_LOG("ttt: wifi property is %s\n", buff);

    return 0;
}

static int test_gps(int test_item, char *ret_buf)
{
    char buff[PROPERTY_VALUE_MAX] = { 0 };
    ENG_LOG("gps");

    property_get("gps.init", buff, "");

    if (strstr(buff, "PASS") != NULL) {
        strcpy(ret_buf, "PASS");
    }
    else {
        strcpy(ret_buf, "FAIL");
    }

    ENG_LOG("ttt: gps property is %s\n", buff);

    return 0;
}

static int test_gsensor(int test_item, char *ret_buf)
{
    int fd = -1;
    int ret = 0;
    char device_info[32] = { 0 };

    ENG_LOG("gsensor\n");

    fd = eng_gsensortest_open();

    if(fd >= 0){
        if (ioctl(fd, LIS3DH_ACC_IOCTL_GET_CHIP_ID, device_info)) {
            ENG_LOG("%s: Get device info error", __FUNCTION__);
            strcpy(ret_buf, "FAIL");
            close(fd);
            ret = -1;
        }else{
            strcpy(ret_buf, "PASS");
            close(fd);
        }
    }else{
        ret = -1;
    }

    return ret;
}

static int test_msensor(int test_item, char *ret_buf)
{
    int fd = 0;
    char reg_cmd[2] = { 0 };

    ENG_LOG("msensor\n");

    fd = open(msensor_dev, O_RDWR);
    if (fd < 0) {
        ENG_LOG("ttt: open filed\n");
        strcpy(ret_buf, "FAIL");
        return -1;
    }

    reg_cmd[0] = 1; /* command length */
    reg_cmd[1] = 0x0; /* register "who am i" */

    if (ioctl(fd, MSENSOR_DEVICE_IOCTL_READ, reg_cmd)) {
        strcpy(ret_buf, "FAIL");
        ENG_LOG("%s: Get device info error", __FUNCTION__);

        close(fd);
        return -1;
    }

    strcpy(ret_buf, "PASS");

    close(fd);
    return 0;
}

static int test_psensor(int test_item, char *ret_buf)
{
    int ret = -1;
    ENG_LOG("psensor\n");

    ret = access(PSENSOR_DEV, F_OK);

    if (ret < 0) {
        strcpy(ret_buf, "FAIL");
        return -1;
    }

    strcpy(ret_buf, "PASS");

    return 0;
}

int test_dev(char *cmd, char *ret_buf)
{
    int fd = 0;
    int ret = 0;
    char test_dev[20] = { 0 };
    char test_item = 0;
    char ptmp[50] = { 0 };

    ENG_LOG("ttt: %s\n", __FUNCTION__);

    parse_space(cmd);
    get_son_str(cmd, test_dev, ',', 0);

    get_son_str(cmd, &test_item, '=', ',');
    test_item -= '0';

    ENG_LOG("test_dev is: %s\n", test_dev);
    ENG_LOG("test_item is : %d\n", test_item);

    if (strstr(test_dev, TP_TEST) != NULL)
        ret = test_tp(test_item, ret_buf);

    else if (strstr(test_dev, LCD_TEST) != NULL)
        ret = test_lcd(test_item, ret_buf);

    else if (strstr(test_dev, CAMERAF_TEST) != NULL)
        ret = test_cameraf(test_item, ret_buf);

    else if (strstr(test_dev, CAMERAB_TEST) != NULL)
        ret = test_camerab(test_item, ret_buf);

    else if (strstr(test_dev, SD_TEST) != NULL)
        ret = test_sd(test_item, ret_buf);

    else if (strstr(test_dev, SIM1_TEST) != NULL)
        ret = test_sim1(test_item, ret_buf);

    else if (strstr(test_dev, SIM2_TEST) != NULL)
        ret = test_sim2(test_item, ret_buf);

    else if (strstr(test_dev, HEADSET_TEST) != NULL)
        ret = test_headset(test_item, ret_buf);

    else if (strstr(test_dev, FM_TEST) != NULL)
        ret = test_fm(test_item, ret_buf);

    else if (strstr(test_dev, ATV_TEST) != NULL)
        ret = test_atv(test_item, ret_buf);

    else if (strstr(test_dev, BT_TEST) != NULL)
        ret = test_bt(test_item, ret_buf);

    else if (strstr(test_dev, WIFI_TEST) != NULL)
        ret = test_wifi(test_item, ret_buf);

    else if (strstr(test_dev, GPS_TEST) != NULL)
        ret = test_gps(test_item, ret_buf);

    else if (strstr(test_dev, GSENSOR_TEST) != NULL)
        ret = test_gsensor(test_item, ret_buf);

    else if (strstr(test_dev, MSENSOR_TEST) != NULL)
        ret = test_msensor(test_item, ret_buf);

    else if (strstr(test_dev, PSENSOR_TEST) != NULL)
        ret = test_psensor(test_item, ret_buf);
    else {
        ENG_LOG("no test dev\n");
        strcpy(ret_buf, "havenot this device test");
    }

    ENG_LOG("ttt: result is: %s\n", ret_buf);

#if 0 // @alvin: FIX ME
    // why send to modem ? I think the result should send to pc directly.
    ret = write_result_to_modem(test_dev, test_item, ret_buf);
#endif
    return 0;
}

int write_result_to_modem(char *dev, int item, char *result)
{
#if 0 //@alvin: FIX ME
    char w_buff[100] = { 0 };

    w_buff[0] = item + '0';
    strcat(w_buff, ", ");
    strcat(w_buff, dev);
    strcat(w_buff, ", ");
    strcat(w_buff, result);

    eng_atdiag_rsp(eng_get_csclient_fd(), w_buff, strlen(w_buff));
#endif
    return 0;
}
