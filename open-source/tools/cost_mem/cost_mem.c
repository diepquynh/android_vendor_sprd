#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

unsigned int block_size = 64;
unsigned int cost_size = 1024 * 1024;
int oom_adj = 15;

static void usage(void)
{
	printf("Usage:\n");
	printf("  costmem [-ccost_size(KB) -bblock_size(KB) -oOom_adj(-16 to 15)]\n");
	printf("  such as: costmem -c2048 -b128 -o15\n");
}

void process_options(int argc, char **argv)
{
	int opt = 0;
	while ((opt = getopt (argc, argv, "c:b:o:")) != -1) {
		switch (opt) {
		case 'c':
			cost_size = (unsigned int)atoi(optarg);
			break;
		case 'b':
			block_size = (unsigned int)atoi(optarg);
			break;
		case 'o':
			oom_adj = atoi(optarg);
			break;
		default:
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	int i, max;
	char *memory;
	size_t j;
	size_t page_size;
	int rval = -EINVAL;
	char text[128] = {0};
	int fd;
	pid_t pid = getpid();

	if (argc < 2) {
		usage();
		return rval;
	} else if (argc == 2) {
		if (strstr(argv[1], "help"))
			usage();
		return rval;
	}

	process_options(argc, argv);
	if (oom_adj < -16 || oom_adj > 15) {
		printf("Oom_adj must between -16 to 15\n");
		return rval;
	}

	sprintf(text, "/proc/%d/oom_adj", pid);

	fd = open(text, O_WRONLY);

	if (-1 == fd) {
		perror("open");
		return rval;
	} else {
		sprintf(text, "%d", oom_adj);
		if (write(fd, text, strlen(text)) == -1)
			perror("write");

		close(fd);
	}

	printf("Cost mem %d KB, %d KB per Block, oom_adj %d\n", cost_size, block_size, oom_adj);

	max = cost_size / block_size;

	for(i = 1; i < max + 1; i++) {
		memory = malloc(block_size * 1024);
		if(NULL == memory){
			perror("malloc");
			return rval;
		}

		if(mlock(memory, block_size * 1024) == -1) {
			perror("mlock");
			return rval;
		}

		memset(memory, 0, block_size * 1024);

		printf("%dKB,", (int)(block_size * i));
		if(9 == i % 10)
			printf("\n");
	}

	printf("Have malloc and mlock %d KB mem\n", block_size * i);
	printf("Have malloc and mlock %d KB mem\n", block_size * i);
	printf("Have malloc and mlock %d KB mem\n", block_size * i);

	i = 0;
	while(1){
		sleep(20);
		i++;
		printf(".");
		if(9 == i % 10)
			printf("Please Ctrl+c to kill this APP\n");
	}
	return 0;
}
