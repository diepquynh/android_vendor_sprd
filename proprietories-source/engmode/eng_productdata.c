#include <fcntl.h>
#include "engopt.h"
#include <pthread.h>
#include "eng_productdata.h"

#ifdef CONFIG_NAND
#include <sys/ioctl.h>
#include <ubi-user.h>
#endif

#ifdef CONFIG_NAND
#define PRODUCTINFO_FILE "/dev/ubi0_miscdata"
#else
#define PRODUCTINFO_FILE "/dev/block/platform/sdio_emmc/by-name/miscdata"
#endif

int eng_read_productnvdata(char *databuf, int data_len) {
  int ret = 0;
  int len;

  int fd = open(PRODUCTINFO_FILE, O_RDONLY);
  if (fd >= 0) {
    ENG_LOG("%s open Ok PRODUCTINFO_FILE = %s ", __FUNCTION__,
            PRODUCTINFO_FILE);
    len = read(fd, databuf, data_len);

    if (len <= 0) {
      ret = 1;
      ENG_LOG("%s read fail PRODUCTINFO_FILE = %s ", __FUNCTION__,
              PRODUCTINFO_FILE);
    }
    close(fd);
  } else {
    ENG_LOG("%s open fail PRODUCTINFO_FILE = %s ", __FUNCTION__,
            PRODUCTINFO_FILE);
    ret = 1;
  }
  return ret;
}

int eng_write_productnvdata(char *databuf, int data_len) {
  int ret = 0;
  int len;

  int fd = open(PRODUCTINFO_FILE, O_WRONLY);
  if (fd >= 0) {
    ENG_LOG("%s open Ok PRODUCTINFO_FILE = %s ", __FUNCTION__,
            PRODUCTINFO_FILE);
#ifdef CONFIG_NAND
    __s64 up_sz = data_len;
    ioctl(fd, UBI_IOCVOLUP, &up_sz);
#endif
    len = write(fd, databuf, data_len);

    if (len <= 0) {
      ret = 1;
      ENG_LOG("%s read fail PRODUCTINFO_FILE = %s ", __FUNCTION__,
              PRODUCTINFO_FILE);
    }
    fsync(fd);
    close(fd);
  } else {
    ENG_LOG("%s open fail PRODUCTINFO_FILE = %s ", __FUNCTION__,
            PRODUCTINFO_FILE);
    ret = 1;
  }
  return ret;
}
