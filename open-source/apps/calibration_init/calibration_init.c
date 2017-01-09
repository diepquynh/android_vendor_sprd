#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

extern int property_set(const char *key, const char *value);

void klog_destory(void);
extern void klog_init(void);
extern void klog_write(int level, const char *fmt, ...);

#define PRINT(x...) klog_write(6,x)

/* for calibration. */
static char calibration[32];


static void parse(char *name)
{
    char *value = strchr(name, '=');

    if (value == 0) return;
    *value++ = 0;
    if (*name == 0) return;

	/* on a real device, white-list the kernel options */
	if (!strcmp(name,"calibration")) {
		/* calibration mode. */
		strlcpy(calibration, value, sizeof(calibration));
	}
}

static int isCalibration(){
	char cmdline[1024];
	char *ptr;
	int fd;

	fd = open("/proc/cmdline", O_RDONLY);
	if (fd >= 0) {
		int n = read(fd, cmdline, 1023);
		if (n < 0) n = 0;

		/* get rid of trailing newline, it happens */
		if (n > 0 && cmdline[n-1] == '\n') n--;

		cmdline[n] = 0;
		close(fd);
	} else {
		cmdline[0] = 0;
	}

	ptr = cmdline;
	while (ptr && *ptr) {
		char *x = strchr(ptr, ' ');
		if (x != 0) *x++ = 0;
		parse(ptr);
		ptr = x;
	}
	return calibration[0] != 0;
}

static void setCalibration_pro(const char * cali){
	if (cali[0]) {
        PRINT("###: set ro.calibration 1.\n");
        property_set("ro.calibration", "1");
    }
    else {
        PRINT("###: set ro.calibration 0.\n");
        property_set("ro.calibration", "0");
    }
}


#define WAKELOCK_PATH "/sys/power/wake_lock"
#define WAKELOCK_PATH_OLD "/sys/android_power/acquire_partial_wake_lock"
#define WAKELOCK_NAME "calibration"

static void calibration_wakelock_hold(){
        int fd = 0;

       /* while(1){*/
                PRINT("##### in calibration mode!!!\n");
                fd = open(WAKELOCK_PATH, O_RDWR);
                if(fd < 0){
                        PRINT("can not open %s in calibration mode errno = %d fd = %d\n", WAKELOCK_PATH, errno, fd);
                        fd = open(WAKELOCK_PATH_OLD, O_RDWR);
                        if(fd < 0)
                                PRINT("can not open %s in calibration mode errno = %d fd = %d\n", WAKELOCK_PATH_OLD, errno, fd);
                }
                if(fd < 0){
                        PRINT("open wakelock file failed!!!\n");
                }else{
                        write(fd, WAKELOCK_NAME, strlen(WAKELOCK_NAME));
                        close(fd);
                        PRINT("hold wakelock calibration\n");
                }
       /*         sleep(10);
        }*/
        return ;
}

int main(int argc, char **argv){
	klog_init();

	if(isCalibration()){
		PRINT("in calibration mode!!!\n");
		calibration_wakelock_hold();
	}else{
		PRINT("not in calibration mode!!!\n");
	}

	setCalibration_pro(calibration);
	klog_destory();
	return 0;
}

