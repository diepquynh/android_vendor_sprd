/**
 * This file is used for query tasks & fds under /proc/
 * task_threshold = 30
 * fd_threshold = 100
 * */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <malloc.h>
#include <unistd.h>


#if 0
/*definition description*/
struct dirent {
	ino_t          d_ino;       /* inode number: long*/
	off_t          d_off;       /* offset to the next dirent */
	unsigned short d_reclen;    /* length of this record */
	unsigned char  d_type;      /* type of file; not supported
								   by all file system types : int*/
	char           d_name[256]; /* filename */
};
#endif


#define TASK_THRESHOLD	30
#define FD_THRESHOLD	100
#define PATH_LEN	1024
#define PARAM_LEN	32
#define READ_BYTES	63

extern int open(const char *pathname, int oflag , ...);

static int task_threshold = TASK_THRESHOLD;
static int fd_threshold = FD_THRESHOLD;
enum subdir_id {
	TASK = 0,
	FD = 1,
	SUBDIR_COUNTS
};


static void* arealloc(void *ptr, size_t size) {

	ptr = realloc(ptr, size);
	if(!ptr && !size) printf("ERROR! Fail to realloc %p, %d\n", ptr, size);

	return ptr;

}

static char* get_link(const char *path) {

	enum { GROWBY = 80 };

	char *buf = NULL;
	int bufsize = 0, readsize = 0;

	do {
		bufsize += GROWBY;
		buf = arealloc(buf, bufsize);
		readsize = readlink(path, buf, bufsize);
		if(readsize == -1) {
			printf("ERROR! Fail to readlink!\n");
			goto err;
		}
	} while(bufsize < readsize + 1);
	buf[readsize] = '\0';

	return buf;

err:
	if(buf) free(buf);
	return NULL;

}

static char* get_taskname(const char *path) {

	/*definition*/
	char *buf = NULL;
	char buffer[READ_BYTES + 1] = {0};
	int i = 0, bufsize = 0, start = 0, end = 0;
	int fd = open(path, 0);

	/*read a few charactors from "/proc/pid/stat" including process name*/
	if(0 > fd) {
		printf("ERROR! Fail to open %s\n", path);
		goto err2;
	}
	if(0 > read(fd, buffer, READ_BYTES)) {
		printf("ERROR! Fail to read from %s: %d\n", path, fd);
		goto err1;
	}
	buffer[READ_BYTES] = '\0';

	/*pick process name*/
	for(i = 0; i < READ_BYTES; i++) {
		if('(' == *(buffer + i)) {
			start = i + 1;
			continue;
		}
		if(')' == *(buffer + i)) {
			end = i;
			break;
		}
	}
	bufsize = end - start;
	if((0 == start) || (0 >= bufsize)) {
		printf("ERROR! Not found process name %s\n", path);
		goto err1;
	}
	buf = arealloc(buf, (bufsize + 1));
	if(!buf) {
		printf("ERROR! Got process name but fail to alloc buffer!\n");
		goto err1;
	}
	strncpy(buf, (buffer + start), bufsize);
	buf[bufsize] = '\0';

	/*fini*/
	close(fd);
	return buf;

err1:
	close(fd);
err2:
	return NULL;

}

static DIR* get_dir(const char *path) {

	/*definition*/
	DIR *dir = opendir(path);

	/*dir*/
	if(!dir) printf("ERROR! Fail to opendir %s\n", (path ? path : "NULL"));

	return dir;

}

static int get_subdir_counts(const char *path) {

	/*definition*/
	int i = 0;
	struct dirent *dir_entry = NULL;
	DIR *dir = get_dir(path);

	/*count, excluding . && ..*/
	while (dir && (NULL != (dir_entry = readdir(dir)))) {
		if(!strcmp(".", dir_entry->d_name) || !strcmp("..", dir_entry->d_name)) {
			continue;
		}
		i++;
	}
	if(dir) closedir(dir);

	return i;

}

static void show_subdir_info(const char *subdir_path, enum subdir_id flag) {

	/*definition*/
	DIR *subdir = NULL;
	struct dirent *direntry = NULL;
	char *buf = NULL;
	char path[PATH_LEN] = {0};

	/*get names*/
	subdir = get_dir(subdir_path);
	while (subdir && (NULL != (direntry = readdir(subdir)))) {
		if(!strcmp(".", direntry->d_name) || !strcmp("..", direntry->d_name)) {
			continue;
		}

		buf = NULL;
		memset(path, 0, sizeof(path));

		switch(flag) {
			case TASK:
				snprintf(path, PATH_LEN, "%s/%s/stat", subdir_path, direntry->d_name);
				buf = get_taskname(path);
				break;
			case FD:
				snprintf(path, PATH_LEN, "%s/%s", subdir_path, direntry->d_name);
				buf = get_link(path);
				break;
			default:
				break;
		}

		if(!buf) {
			printf("%s/%s -> %s\n", subdir_path, direntry->d_name, "Unknown!");
		} else {
			printf("%s/%s -> %s\n", subdir_path, direntry->d_name, buf);
			free(buf);
		}
	}

	/*fini*/
	if(subdir) closedir(subdir);

}

static void show_dir_summery(const char *dirpath, const struct dirent *dir_entry, int task_count, int fd_count) {

	/*definitions*/
	char path[PATH_LEN] = {0};
	char *buf = NULL;

	/*summery*/
	snprintf(path, PATH_LEN, "%s/%s/stat", dirpath, dir_entry->d_name);
	buf = get_taskname(path);
	if(!buf) {
		memset(path, 0, sizeof(path));
		snprintf(path, PATH_LEN, "%s/%s/exe", dirpath, dir_entry->d_name);
		buf = get_link(path);
		if(!buf) printf("ERROR! Task readlink failed!\n");
	}

	if(buf) {
		printf("\n\n******************** dirpath : %s/%s ******************** \nSUMMERY:\n proc name	: %s\n task count	: %d	(threshold %d)\n fd count	: %d	(threshold %d)\n",
				dirpath, dir_entry->d_name, buf, task_count, task_threshold, fd_count, fd_threshold);
		free(buf);
	} else {
		printf("\n\n******************** dirpath : %s/%s ********************\nSUMMERY:\n proc name	: %s\n task count	: %d	(threshold %d)\n fd count	: %d	(threshold %d)\n",
				dirpath, dir_entry->d_name, "Unknown", task_count, task_threshold, fd_count, fd_threshold);
	}

}

static int read_dir(DIR *dir, const char *dirpath) {

	/*definition*/
    struct dirent *dir_entry = NULL;
	char dir_task_path[PATH_LEN] = {0};
	char dir_fd_path[PATH_LEN] = {0};
	int task_count = 0, fd_count = 0;

	/*task	&  fd	&	prints*/
    while (dir && (NULL != (dir_entry = readdir(dir)))) {
		if((DT_DIR != dir_entry->d_type) || !strcmp(".", dir_entry->d_name) || !strcmp("..", dir_entry->d_name) || !atoi(dir_entry->d_name)) {
			continue;
		}

		/*useful subdir counts*/
		snprintf(dir_task_path, PATH_LEN, "%s/%s/task", dirpath, dir_entry->d_name);
		snprintf(dir_fd_path, PATH_LEN, "%s/%s/fd", dirpath, dir_entry->d_name);
		task_count = get_subdir_counts(dir_task_path);
		fd_count = get_subdir_counts(dir_fd_path);

		if((task_threshold > task_count) && (fd_threshold > fd_count)) continue;

		/*current dir : summery*/
		show_dir_summery(dirpath, dir_entry, task_count, fd_count);

		/*subdir : task names*/
		printf("\nTASK:\n");
		show_subdir_info(dir_task_path, TASK);

		/*subdir : fd links*/
		printf("\nFD:\n");
		show_subdir_info(dir_fd_path, FD);

	}

	return 0;
}

static void helper() {
	printf("\nUsage:\n ./query_task_fd --task_threshold=30 --fd_threshold=50\nParams:\n --help\n --task_threshold: task count threshold (default 30)\n --fd_threshold: fd count threshold (default 50)\n\n");
}

static int parse_cmdline(char **buf, int size) {

	/*definitions*/
	int i = 0;
	int errorno = 1;

	/*start from 1, because argv[0] is cmd name*/
	for(i = 1; i < size; i++) {

		/*definitions*/
		int j = 0, start = 0, end = 0;
		char param[PARAM_LEN] = {0};
		char value[PARAM_LEN] = {0};
		int length = 0;
		length = strlen(*(buf + i));

		/*ignore -- && check param format*/
		if(('-' == *(*(buf + i))) && ('-' == *(*(buf + i) + 1))) {
			start = 2;
			j += 2;
		} else {
			printf("Unknown param: %s\n", buf[i]);
			errorno++;
			continue;
		}

		/*process param & it's value*/
		while((j < length) && ('=' != *(*(buf + i) + j))) j++;
		if(j == length) {
			/*check --help*/
			if(!strcmp("help", (*(buf + i) + start))) goto err;
			printf("Param without a value: %s\n", buf[i]);
			errorno++;
			continue;
		} else {
			end = j;
			strncpy(param, (*(buf + i) + start), (end - start));
			param[end - start] = '\0';
			start = end + 1;
			end = length;
			strncpy(value, (*(buf + i) + start), (end - start));
			value[end - start] = '\0';
		}

		/*check if param exist && set task_threshold fd_threshold*/
		if(!strcmp(param, "task_threshold")) {
			task_threshold = atoi(value);
		} else if (!strcmp(param, "fd_threshold")) {
			fd_threshold = atoi(value);
		} else {
			printf("Param not support yet: %s\n", buf[i]);
			errorno++;
			continue;
		}

	}

	if(errorno == size) goto err;

	return 0;

err:
	return -1;
}

int main(int argc, char *argv[]) {

	/*definitions*/
    DIR *dir = NULL;
	char dir_path[PATH_LEN] = {0};
	snprintf(dir_path, PATH_LEN, "/proc");

	/*proceding arguments*/
	if(1 == argc) {
		printf("Use default value: task_threshold %d; fd_threshold %d\n", task_threshold, fd_threshold);
	} else if(parse_cmdline(argv, argc)) {
		helper();
		return 0;
	}

	/*readdirs*/
	dir = get_dir(dir_path);
	if(!dir) return 0;

	if(!read_dir(dir, dir_path)) {
		printf("\n == SUCCESS! ==\n");
	}

	/*fini*/
	if(dir) closedir(dir);
	return 0;
}

