#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <termios.h>
#include <fcntl.h>
#include <linux/watchdog.h>

static void usage(void)
{
    printf("Usage:\n");
    printf("utest_wdt [-m margin] [-p feed_period] [-f]\n");
}

int main(int argc, char **argv)
{
    int rval = -EINVAL;
    int opt, wt_fd = -1;
    int margin = 0, feed_period = 0;
    int tem_margin = 0, tem_period = 0;
    printf("--------------------utest_wdt begin-----------------------\n");

    wt_fd = open("/dev/sprd-watchdog", O_WRONLY);
    if (wt_fd == -1) {
        printf("fail to open watchdog device\n");
        return -EINVAL;
    }

    while ((opt = getopt(argc, argv, "m:p:f")) != -1) {
        switch (opt)
        {
        case 'm':
            margin = atoi(optarg);
            printf("set marin:%d\n", margin);
            ioctl(wt_fd, WDIOC_SETTIMEOUT, &margin);
            break;
        case 'p':
            feed_period = atoi(optarg);
            printf("set feed_period:%d\n", feed_period);
            ioctl(wt_fd, WDIOC_SETPRETIMEOUT, &feed_period);
            break;
        case 'f':
            printf("feed dog\n");
            ioctl(wt_fd, WDIOC_KEEPALIVE, 0);
            break;
        default:
            printf("utest_wdt set invalid option -%c\n", opt);
            usage();
            break;
        }
    }

    ioctl(wt_fd, WDIOC_GETTIMEOUT, &tem_margin);
    printf("marin:%d\n", tem_margin);

    ioctl(wt_fd,  WDIOC_GETPRETIMEOUT, &tem_period);
    printf("feed_period:%d\n", tem_period);

    printf("-------------------utest_wdt end-------------------------\n");
    return rval;
}
