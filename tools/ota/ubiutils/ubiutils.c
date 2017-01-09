/*
 * Created by Spreadst
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mount.h>  // for _IOW, _IOR, mount()
#include <sys/stat.h>
#include "ubi-user.h"
#undef NDEBUG
#include <assert.h>
#include "mtdutils/mtdutils.h"


#define UIB_DEV_PREFIX "/dev/ubi0_"
#define UBI_CTRL_DEV "/dev/ubi_ctrl"

struct MtdPartition {
    int device_index;
    unsigned int size;
    unsigned int erase_size;
    char *name;
};

char *ubi_get_devname(const char *volume_name) {
    char *devname = malloc(sizeof(UIB_DEV_PREFIX) + strlen(volume_name) + 1);

    sprintf(devname, UIB_DEV_PREFIX"%s", volume_name);

    return devname;
}

int ubi_mount(const char *volume_name, const char *mount_point,
                        const char *filesystem, int read_only)
{
    const unsigned long flags = MS_NOATIME | MS_NODEV | MS_NODIRATIME;
    int rv = -1;
    char *devname = ubi_get_devname(volume_name);

    if (!read_only) {
        rv = mount(devname, mount_point, filesystem, flags, NULL);
    }
    if (read_only || rv < 0) {
        rv = mount(devname, mount_point, filesystem, flags | MS_RDONLY, NULL);
        if (rv < 0) {
            printf("Failed to mount %s on %s: %s\n",
                   devname, mount_point, strerror(errno));
        } else {
            printf("Mount %s on %s read-only\n", devname, mount_point);
        }
    }
    if (rv >= 0) {
        /* For some reason, the x bits sometimes aren't set on the root
         * of mounted volumes.
         */
        struct stat st;
        rv = stat(mount_point, &st);
        if (rv < 0) {
            goto done;
        }
        mode_t new_mode = st.st_mode | S_IXUSR | S_IXGRP | S_IXOTH;
        if (new_mode != st.st_mode) {
            printf("Fixing execute permissions for %s\n", mount_point);
            rv = chmod(mount_point, new_mode);
            if (rv < 0) {
                printf("Couldn't fix permissions for %s: %s\n",
                       mount_point, strerror(errno));
            }
        }
    }

done:
    free(devname);
    return rv;
}

int ubi_update(const char *volume_name, long long len, const char *mountpoint)
{
    char *devname = ubi_get_devname(volume_name);
    int rv = -1;

    int fd = open(devname, O_RDWR);
    if (fd < 0) {
        printf("Update ubi volume, open device %s error: %s\n",
               devname, strerror(errno));
        goto done;
    }
    rv = ioctl(fd, UBI_IOCVOLUP, &len);
    close(fd);

done:
    free(devname);
    return rv;
}

int ubi_fupdate(int fd, long long len) {
    return ioctl(fd, UBI_IOCVOLUP, &len);
}

int ubi_attach(int ubi_num, const char *mtd_part_name) {
    size_t write_size;
    struct ubi_attach_req req;
    int fd;
    int ret;

    mtd_scan_partitions();
    const MtdPartition *part = mtd_find_partition_by_name(mtd_part_name);
    if (part == NULL) {
        printf("Can't find mtd partition %s\n", mtd_part_name);
        return -1;
    }

    memset(&req, 0, sizeof(struct ubi_attach_req));
    req.ubi_num =(typeof(req.ubi_num))ubi_num;
    if(-1 == req.ubi_num){
        req.ubi_num = UBI_DEV_NUM_AUTO;
    }
    req.mtd_num =(typeof(req.mtd_num))part->device_index;

    fd = open(UBI_CTRL_DEV, O_RDONLY);
    if(-1 == fd){
        printf("Can't open ubi control device %s:%s\n",
               UBI_CTRL_DEV, strerror(errno));
        return -1;
    }
    ret = ioctl(fd, UBI_IOCATT, &req);
    close(fd);
    if(-1 == ret){
        printf("ioctl UBI_IOCATT error:%s\n", strerror(errno));
        return -1;
    }
    return 0;
}

int ubi_open(const char *volume_name, int flags) {
    char *devname = ubi_get_devname(volume_name);
    int fd = open(devname, flags);
    free(devname);
    return fd;
}
