/*
 * Copyright (C) 2012 Spreadtrum Communications Inc.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/uio.h>
#include <dirent.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/statfs.h>
#include <cutils/properties.h>

#define TARGET_FILE "/data/.boot_complete_file"
int data_available_size = 0; /*kb*/
int target_size = 4096; /* 4M */

/*
 *handler data size according to internal available space
 *
 */
static void handle_data_size()
{
	struct statfs diskInfo;

	statfs("/data", &diskInfo);
	unsigned int blocksize = diskInfo.f_bsize;
	unsigned int availabledisk = diskInfo.f_bavail * blocksize;

	/* default setting internal log size, half of available */
	data_available_size = availabledisk >> 10;
}

/*
 *
 */
static void handle_target_file()
{
	int fd_out;

	fd_out = open(TARGET_FILE, O_WRONLY | O_CREAT , S_IRUSR | S_IRGRP | S_IROTH);
	if(fd_out < 0)
		exit(0);

	lseek(fd_out, target_size *1024 ,SEEK_SET);

	write(fd_out, "shut down!", 12);
	close(fd_out);
}

/*
 * the main function
 */
int main(int argc, char *argv[])
{

	unlink(TARGET_FILE);
	sleep(30);

	handle_data_size();
	if(data_available_size < 8)
		return 0;

	if(data_available_size < target_size)
		target_size = data_available_size - 8;

	handle_target_file();

	return 0;
}
