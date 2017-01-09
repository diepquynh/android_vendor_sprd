#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include<time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "eng_sqlite.h"
#include <utils/Log.h>
#include "eng_attok.h"
#include "engopt.h"
#include "cutils/properties.h"
#include <string.h>
#include <errno.h>
#include "eng_btwifiaddr.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG 			"BTWIFIMAC"

#define MAC_ERROR_EX			":::::"
#define MAC_ERROR			"FF:FF:FF:FF:FF:FF"
#define WIFI_MAC_FILE		"/productinfo/wifimac.txt"
#define WIFI_ROOT_MAC_FILE	"/data/misc/wifi/wifimac.txt"
#define BT_MAC_FILE		"/productinfo/btmac.txt"
#define BT_ROOT_MAC_FILE	"/data/misc/bluedroid/btmac.txt"
#define MAC_RAND_FILE		"/productinfo/rand_mac.txt"

typedef enum {
    BT_MAC_ADDR=0,
    WIFI_MAC_ADDR
}MAC_ADDR;


static void write_to_randmacfile(char *btmac, char *wifimac)
{
    int fd;
    char buf[80];

    fd = open(MAC_RAND_FILE, O_RDWR|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
    if( fd >= 0) {
        memset(buf, 0, sizeof(buf));
        sprintf(buf, "%s;%s",btmac, wifimac);
        write(fd, buf, sizeof(buf));
        close(fd);
    } else {
        ALOGD("%s: errno=%d, errstr=%s",__FUNCTION__, errno, strerror(errno));
    }
    ALOGD("%s: %s fd=%d, data=%s",__FUNCTION__, MAC_RAND_FILE, fd,buf);
}

// realtek_add_start
static int get_urandom(unsigned int *buf, size_t len){
    int fd;
    size_t rc;

    ALOGD("+%s+",__FUNCTION__);
    fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0){
        ALOGD("%s: Open urandom fail", __FUNCTION__);
        return -1;
    }
    rc = read(fd, buf, len);
    close(fd);
    ALOGD("-%s: rc: %d-",__FUNCTION__, rc);
    return rc;
}
// realtek_add_end

static void mac_rand(char *btmac, char *wifimac)
{
    int fd=0,i, j, k;
    off_t pos;
    char buf[80];
    char *ptr;
    int ret = 0;
    unsigned int randseed;
    // realtek_add_start
    int rc;
    struct timeval tt;
    // realtek_add_end

    memset(buf, 0, sizeof(buf));

    // realtek_add_start
    ALOGD("+%s+",__FUNCTION__);
    // realtek_add_end
    if(access(MAC_RAND_FILE, F_OK) == 0) {
        ALOGD("%s: %s exists",__FUNCTION__, MAC_RAND_FILE);
        fd = open(MAC_RAND_FILE, O_RDWR);
        if(fd>=0) {
	    ret = read(fd, buf, sizeof(buf)-1);
	    if(ret > 0){
		ALOGD("%s: read %s %s",__FUNCTION__, MAC_RAND_FILE, buf);
		ptr = strchr(buf, ';');
		if(ptr != NULL) {

		    if((strstr(wifimac, MAC_ERROR)!=NULL)||(strstr(wifimac, MAC_ERROR_EX)!=NULL)||(strlen(wifimac)==0))
			strcpy(wifimac, ptr+1);

		    *ptr = '\0';

		    if((strstr(btmac, MAC_ERROR)!=NULL)||(strstr(btmac, MAC_ERROR_EX)!=NULL)||(strlen(btmac)==0))
			strcpy(btmac, buf);

		    ALOGD("%s: read btmac=%s, wifimac=%s",__FUNCTION__, btmac, wifimac);
		    close(fd);
		    return;
		}
	    }else{
		ALOGD("%s: read failed",__FUNCTION__);
	    }
            // realtek_add_start
            close(fd);
            // realtek_add_end
        }
    }

    rc = get_urandom(&randseed, sizeof(randseed));
    if (rc > 0) {
        ALOGD("urandom:%u", randseed);
    } else {
        if (gettimeofday(&tt, (struct timezone *)0) > 0)
            randseed = (unsigned int) tt.tv_usec;
        else
            randseed = (unsigned int) time(NULL);

        ALOGD("urandom fail, using system time for randseed");
    }
    // realtek_add_end
    ALOGD("%s: randseed=%u",__FUNCTION__, randseed);
    srand(randseed);
    ALOGD("%s: mac=%s, fd[%s]=%d",__FUNCTION__, btmac, BT_MAC_FILE, fd);

    //FOR BT
    i=rand(); j=rand();
    ALOGD("%s:  rand i=0x%x, j=0x%x",__FUNCTION__, i,j);
    sprintf(btmac, "00:%02x:%02x:%02x:%02x:%02x", \
            (unsigned char)((i>>8)&0xFF), \
            (unsigned char)((i>>16)&0xFF), \
            (unsigned char)((j)&0xFF), \
            (unsigned char)((j>>8)&0xFF), \
            (unsigned char)((j>>16)&0xFF));

    //FOR WIFI
    i=rand(); j=rand();
    ALOGD("%s:  rand i=0x%x, j=0x%x",__FUNCTION__, i,j);
    sprintf(wifimac, "00:%02x:%02x:%02x:%02x:%02x", \
            (unsigned char)((i>>8)&0xFF), \
            (unsigned char)((i>>16)&0xFF), \
            (unsigned char)((j)&0xFF), \
            (unsigned char)((j>>8)&0xFF), \
            (unsigned char)((j>>16)&0xFF));

    ALOGD("%s: bt mac=%s, wifi mac=%s",__FUNCTION__, btmac, wifimac);

    //create rand file
    //write_to_randmacfile(btmac, wifimac);
}

static int write_mac2file(char *wifimac, char *btmac)
{
    int fd;
    int ret = 0;

    //wifi mac
    fd = open(WIFI_MAC_FILE, O_CREAT|O_RDWR|O_TRUNC, 0666);
    ENG_LOG("%s: mac=%s, fd[%s]=%d,errno=%d, errstr=%s",__FUNCTION__, wifimac, WIFI_MAC_FILE, fd,errno, strerror(errno));
    if(fd >= 0) {
        if(-1 == chmod(WIFI_MAC_FILE, 0666))
	    ENG_LOG("%s chmod failed",__FUNCTION__);
        ret = write(fd, wifimac, strlen(wifimac));
        close(fd);
    }else{
        ret = -1;
        return ret;
    }

    //bt mac
    fd = open(BT_MAC_FILE, O_CREAT|O_RDWR|O_TRUNC, 0666);
    ENG_LOG("%s: mac=%s, fd[%s]=%d,errno=%d, errstr=%s",__FUNCTION__, btmac, BT_MAC_FILE, fd,errno, strerror(errno));
    if(fd >= 0) {
        if(-1 == chmod(BT_MAC_FILE, 0666))
	    ENG_LOG("%s chmod failed",__FUNCTION__);
        ret = write(fd, btmac, strlen(btmac));
        close(fd);
        if (0 == access(BT_ROOT_MAC_FILE,F_OK)){
            if (-1 == remove(BT_ROOT_MAC_FILE)) {
                ENG_LOG("%s remove  %s failed",__FUNCTION__,BT_ROOT_MAC_FILE);
            }
        }
    }else{
        ret = -1;
    }

    sync();

    return ret;
}

int eng_btwifimac_write(char* bt_mac, char* wifi_mac)
{
    int ret = 0;
    char bt_mac_rand[32] = {0};
    char wifi_mac_rand[32] = {0};

    ENG_LOG("set BT/WIFI mac");

    // If no bt_mac or wifi_mac, we can randomly generate them.
    if(!bt_mac || !wifi_mac) {
        mac_rand(bt_mac_rand, wifi_mac_rand);
        if(!bt_mac){
            eng_btwifimac_read(bt_mac_rand, ENG_BT_MAC);
            bt_mac = bt_mac_rand;
        }
        if(!wifi_mac){
            eng_btwifimac_read(wifi_mac_rand, ENG_WIFI_MAC);
            wifi_mac = wifi_mac_rand;
        }
    }

    ENG_LOG("property ro.mac.wifi=%s, ro.mac.bluetooth=%s",wifi_mac,bt_mac);
    ret = write_mac2file(wifi_mac,bt_mac);

    if(ret > 0){
        property_set("sys.mac.wifi" ,wifi_mac);
        property_set("sys.mac.bluetooth",bt_mac);
        property_set("sys.bt.bdaddr_path",BT_MAC_FILE);
        property_set("ctl.start", "set_mac");
    }

    return ret;
}

int eng_btwifimac_read(char* mac, MacType type)
{
    int fd, rcount;
    int ret = 0;

    if(!mac)
        return -1;

    if(ENG_WIFI_MAC == type) {
        // read wifi mac
        if(access(WIFI_MAC_FILE, F_OK) == 0) {
            ENG_LOG("%s: %s exists",__FUNCTION__, WIFI_MAC_FILE);
            fd = open(WIFI_MAC_FILE, O_RDONLY);
        }else{
            ENG_LOG("%s: %s not exists,read %s",__FUNCTION__, WIFI_MAC_FILE,WIFI_ROOT_MAC_FILE);
            fd = open(WIFI_ROOT_MAC_FILE, O_RDONLY);
        }
    }
    else {
        // read bt mac
        if(access(BT_MAC_FILE, F_OK) == 0) {
            ENG_LOG("%s: %s exists",__FUNCTION__, BT_MAC_FILE);
            fd = open(BT_MAC_FILE, O_RDONLY);
        }else{
            ENG_LOG("%s: %s not exists,read %s",__FUNCTION__, BT_MAC_FILE,BT_ROOT_MAC_FILE);
            fd = open(BT_ROOT_MAC_FILE, O_RDONLY);
        }
    }

    if(fd >= 0) {
        rcount = read(fd, mac, 31);
        if(rcount <= 0)
        {
            ret = -1;
        }

        ENG_LOG("%s: mac=%s, fd[%s]=%d",__FUNCTION__, mac, WIFI_MAC_FILE, fd);
        close(fd);
    }else{
        ENG_LOG("%s: fd=%d,errno=%d, strerror(errno)=%s",__FUNCTION__, fd, errno, strerror(errno));
        ret = -1;
    }

    return ret;
}

