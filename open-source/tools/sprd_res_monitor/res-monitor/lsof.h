#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUF_MAX 1024
#define CMD_DISPLAY_MAX 10

struct pid_info_t {
    pid_t pid;

    char cmdline[CMD_DISPLAY_MAX];

    char path[PATH_MAX];
    ssize_t parent_length;
};

void print_header(FILE *fp);
void lsof_dumpinfo(pid_t pid,FILE *fp);
