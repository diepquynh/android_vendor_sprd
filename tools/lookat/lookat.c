#include <stdio.h>
#include <sched.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#include <linux/kernel.h>
#include <linux/magic.h>

#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/vfs.h>
#include <sys/mman.h>

/* #define DEBUG */
#ifdef DEBUG
#define DEBUGSEE fprintf
#else
#define DEBUGSEE(...)
#endif

/* option flags */
#define LOOKAT_F_GET (1 << 0)
#define LOOKAT_F_GET_MUL (1 << 1)
#define LOOKAT_F_SET (1 << 2)

#define LOOKAT_F_GET_V (1 << 3)
#define LOOKAT_F_GET_MUL_V (1 << 4)
#define LOOKAT_F_SET_V (1 << 5)

static int debugfs_found = 0;
char debugfs_mountpoint[4096 + 1] = "/sys/kernel/debug";
static int debugfs_premounted;

#define SIZE_BUF   24
static const char *debugfs_known_mountpoints[] = {
    "/sys/kernel/debug/", "/debug/", "/d/", 0,
};

/* verify that a mountpoint is actually a debugfs instance */
static int debugfs_valid_mountpoint(const char *debugfs) {
  struct statfs st_fs;

  if (statfs(debugfs, &st_fs) < 0)
    return -2;
  else if (st_fs.f_type != (long)DEBUGFS_MAGIC)
    return -2;

  return 0;
}

/* find the path to the mounted debugfs */
static const char *debugfs_find_mountpoint(void) {
  const char **ptr;
  char type[100];
  FILE *fp;

  if (debugfs_found) return (const char *)debugfs_mountpoint;

  ptr = debugfs_known_mountpoints;
  while (*ptr) {
    if (debugfs_valid_mountpoint(*ptr) == 0) {
      debugfs_found = 1;
      strcpy(debugfs_mountpoint, *ptr);
      return debugfs_mountpoint;
    }
    ptr++;
  }

  /* give up and parse /proc/mounts */
  fp = fopen("/proc/mounts", "r");
  if (fp == NULL) return NULL;

  while (fscanf(fp, "%*s %4096s %99s %*s %*d %*d\n", debugfs_mountpoint,
                type) == 2) {
    if (strcmp(type, "debugfs") == 0) break;
  }
  fclose(fp);

  if (strcmp(type, "debugfs") != 0) return NULL;

  debugfs_found = 1;

  return debugfs_mountpoint;
}

/* mount the debugfs somewhere if it's not mounted */
static char *debugfs_mount(const char *mountpoint) {
  struct statfs st_fs;
  /* see if it's already mounted */
  if (debugfs_find_mountpoint()) {
    debugfs_premounted = 1;
    goto out;
  }
  /* if not mounted and no argument */
  if (mountpoint == NULL) {
    /* see if environment variable set */
    /* if no environment variable, use default */
    mountpoint = "/sys/kernel/debug/";
  }

  if (statfs("/debug_12345", &st_fs) < 0) {
    if (mkdir("/debug_12345/", 0644) < 0) return NULL;
  }
  if (mount(NULL, mountpoint, "debugfs", 0, NULL) < 0) return NULL;

  /* save the mountpoint */
  debugfs_found = 1;
  strncpy(debugfs_mountpoint, mountpoint, sizeof(debugfs_mountpoint));
out:

  return debugfs_mountpoint;
}

static int find_create_debugfs(char **p) {
  char *_p = 0;
  char *debugfs_dir = debugfs_mount("/debug_12345/");
  if (!debugfs_dir) return -1;
  _p = debugfs_dir;
  while (*_p) _p++;
  if (*(_p - 1) != '/') {
    *(_p) = '/';
    *(_p + 1) = 0;
  }

  DEBUGSEE("debugfs_dir = %s\n", debugfs_dir);
  *p = debugfs_dir;

  return 0;
}
int sub_main(int flags, unsigned long addr, int value, int nword) {
  int fd1, fd2;
  char *dir = 0;
  char strPath1[256] = {0};
  char strPath2[256] = {0};
  char c[12 + 12] = {0};
  int n = 0;
  ssize_t cnt = 0;

  if (find_create_debugfs(&dir)) return 0;

  sprintf(strPath1, "%s%s", dir, "lookat/addr_rwpv");
  sprintf(strPath2, "%s%s", dir, "lookat/data");

  if ((fd1 = open(strPath1, O_RDWR | O_SYNC)) < 0) {
    perror("open addr_rwpv");
    exit(EXIT_FAILURE);
    return -1;
  }
  if ((fd2 = open(strPath2, O_RDWR | O_SYNC)) < 0) {
    perror("open data");
    exit(EXIT_FAILURE);
    return -1;
  }

  if ((flags & LOOKAT_F_SET) || (flags & LOOKAT_F_SET_V)) {
    if (flags & LOOKAT_F_SET)
      addr |= 2;
    else
      addr |= 3;
    n = snprintf(c, SIZE_BUF, "0x%lx", addr);
    write(fd1, c, n);
    n = snprintf(c, SIZE_BUF, "0x%x", value);
    write(fd2, c, n);
  } else if ((flags & LOOKAT_F_GET) || (flags & LOOKAT_F_GET_V)) {
    if (flags & LOOKAT_F_GET_V) addr |= 1;
    n = snprintf(c, SIZE_BUF, "0x%lx", addr);
    write(fd1, c, n);
    memset(c, 0, SIZE_BUF);
    cnt = read(fd2, c, SIZE_BUF);
    if (cnt < 0) {
      perror("read data");
      exit(EXIT_FAILURE);
      return -1;
    }
    fprintf(stdout, "%s\n", c);
  } else if ((flags & LOOKAT_F_GET_MUL) || (flags & LOOKAT_F_GET_MUL_V)) {
    int i;
    /* print a serial of reg in a formated way */
    fprintf(stdout,
            "  ADDRESS  |   VALUE\n"
            "-----------+-----------\n");
    for (i = 0; i < nword; ++i) {
      if (flags & LOOKAT_F_GET_MUL_V) addr |= 1;
      n = snprintf(c, SIZE_BUF, "0x%lx", addr);
      write(fd1, c, n);
      memset(c, 0, SIZE_BUF);
      cnt = read(fd2, c,
                 SIZE_BUF); /*It is  simple file system don't support seek method
                                        just open again it is quickly.*/
      if (cnt < 0) {
        perror("read data");
        exit(EXIT_FAILURE);
        return -1;
      }
      close(fd2);
      if ((fd2 = open(strPath2, O_RDWR | O_SYNC)) < 0) {
        fprintf(stderr, "open %s error\n", strPath2);
        goto Exit; /*sad*/
      }
      fprintf(stdout, "0x%lx | %s\n", addr & ~0x3, c);
      addr += 4;
    }
  }
  close(fd2);
Exit:
  close(fd1);
  return 0;
}

void usage1(void) {
  fprintf(stderr,
          "\n"
          "Usage: lookat [-l nword] [-s value] [-h] addr_in_hex\n"
          "get/set the value of IO reg at addr_in_hex\n"
          "  -l nword     print values of nword continous regs start from \n"
          "               addr_in_hex in a formated way\n"
          "  -s value     set a reg at addr_in_hex to value\n"
          "  -h           print this message\n\n"
          " NOTE:\n"
          " * the addr_in_hex should be 4-byte aligned in hex form\n"
          " * Default is physical address, Using Virtual address If the \n"
          " * parameters are capital letter: Only the [-L],[-S],(Read/Write) \n"
          " * will use virtual address\n");
}

int main(int argc, char **argv) {
  int fd, opt;
  int flags = 0; /* option indicator */
  unsigned long paddr, pbase, offset;
  unsigned long value = 0, nword = 0;
  char *vbase;
  char *op = "s:l:h";
  void (*usage)(void);

  usage = usage1;
  while ((opt = getopt(argc, argv, op)) != -1) {
    switch (opt) {
      case 's':
        flags |= LOOKAT_F_SET;
        value = strtoul(optarg, NULL, 16);
        fprintf(stdout, "[I] value = 0x%lx\n", value);
        break;
      case 'l':
        flags |= LOOKAT_F_GET_MUL;
        nword = strtoul(optarg, NULL, 10);
        fprintf(stdout, "[I] nword = 0x%ld\n", nword);
        if (nword <= 0) nword = 1;
        break;
      case 'h':
        usage();
        exit(0);
      default:
          usage();
          exit(EXIT_FAILURE);
    }
  }

  if (optind == 1) flags |= LOOKAT_F_GET;

  if (optind >= argc) {
    fprintf(stderr, "Expected argument after options\n");
    exit(EXIT_FAILURE);
  }

  paddr = strtoul(argv[optind], NULL, 16);
  DEBUGSEE(stdout, "[I] paddr = 0x%lx\n", paddr);

  if (paddr & 0x3) {
    fprintf(stderr, "address should be 4-byte aligned\n");
    exit(EXIT_FAILURE);
  }

    sub_main(flags, paddr, value, nword);

  return 0;
}
