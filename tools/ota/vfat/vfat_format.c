
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <linux/fs.h>
#include <sys/ioctl.h>

#include <linux/kdev_t.h>

#define CARD_SIZE_2G   (2*1024*1024*1024LL)
#define CARD_SIZE_128M (128*1024*1024LL)

extern int newfs_msdos_main(int argc, const char *argv[]);

static long long get_blk_device_size(const char *blk_device) {
    int fd_dev;
    long long card_size;

    if (0 > (fd_dev=open(blk_device, O_RDONLY))) {
        fprintf(stderr, "open blk_device :%s error\n", blk_device);
        return -1;
    }
    if(-1 == ioctl(fd_dev, BLKGETSIZE64, &card_size)) {
        fprintf(stderr, "ioctl error\n");
        close(fd_dev);
        return -1;
    }
    printf("blk_dev[%s]: size is :%lld \n", blk_device, card_size);
    close(fd_dev);
    return card_size;
}

static int fill_args(const char *args[], const char *blk_device, unsigned int num_sectors) {
    long long blk_dev_size;
    int argc = 0;

    args[0] = "newfs_msdos";
    args[1] = "-O";
    args[2] = "android";
    argc = 3;

    printf("blk_dev[%s]: num_sectors:%u\n", blk_device, num_sectors);
    if (num_sectors) {
        char tmp[32];
        snprintf(tmp, sizeof(tmp), "%u", num_sectors);
        const char *size = tmp;
        args[argc++] = "-s";
        args[argc++] = size;
    }

    if((blk_dev_size=get_blk_device_size(blk_device))< 0) {
        fprintf(stderr, "get_blk_device_size() error !\n");
        return -1;
    } else {
        args[argc++] = "-F";
        if(blk_dev_size <= CARD_SIZE_128M) {
            args[argc++] = "16";
            args[argc++] = "-c";
            args[argc++] = "32";
        } else if(blk_dev_size <= CARD_SIZE_2G) {
            args[argc++] = "16";
            args[argc++] = "-c";
            args[argc++] = "64";
        } else {
            args[argc++] = "32";
        }
    }

    args[argc++] = blk_device;
    return argc;
}

static void dump_args(const char *args[], int argc) {
    char buf[512], *p=buf;
    int i;

    memset(buf, 0, sizeof(buf));
    for (i = 0; i < argc; i++) {
        sprintf(p, "%s ", args[i]);
        p += strlen(p);
    }
    printf("format Args:%s\n", buf);
}

int format_vfat(const char *blk_device, unsigned int num_sectors, int wipe) {
    int result = -1;
    const char *args[10];
    int argc = 0;

    printf("Format vfat filesystem for %s.\n", blk_device);

    argc = fill_args(args, blk_device, num_sectors);
    dump_args(args, argc);

    if (argc < 0) {
        goto ret;
    }

    result = newfs_msdos_main(argc, args);

ret:
    if (result == 0) {
        printf("Vfat filesystem formatted OK\n");
    } else {
        fprintf(stderr, "Vfat format failed (result is %d)\n", result);
        errno = EIO;
    }

    return result;
}
