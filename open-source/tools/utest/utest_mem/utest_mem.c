#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>

#define ERR(x...)	fprintf(stderr, ##x)
#define INFO(x...)	fprintf(stdout, ##x)

static int mem_bandwidth(size_t size, size_t count, int nproc)
{
	int p, n;
	char *src, *dst;
	struct timespec ts_start, ts_end;
	int pipe_barrier[2];
	int pipe_results[2];
	char junk;
	double duration;

	if (pipe(pipe_barrier) < 0) {
		return -EPIPE;
	}
	if (pipe(pipe_results) < 0) {
		return -EPIPE;
	}

	for (p = nproc; p > 1; p--) {
		switch (fork()) {
			case -1:
				return -EIO;
			case 0:
				/* child */
				break;
			default:
				/* parent: continue forking */
				continue;
		}
		/* child */
		break;
	}

	src = malloc(size);
	if (src == NULL) {
		return -ENOMEM;
	}
	dst = malloc (size);
	if (dst == NULL) {
		return -ENOMEM;
	}

	/* initialize and break copy on write */
	memset(src, 1, size);
	memset(dst, 0, size);

	if (close(pipe_barrier[1]) < 0) {
		return -EPIPE;
	}
	/* this read will get EOF when all the processes have started */
	if (read(pipe_barrier[0], &junk, 1) < 0) {
		return -EPIPE;
	}

	/* test start */
	if (clock_gettime(CLOCK_MONOTONIC, &ts_start) < 0) {
		return -EIO;
	}

	/* this is the stuff being measured */
	for (n = count;  n > 0;  n--)
		memcpy(dst, src, size);

	/* test end */
	if (clock_gettime(CLOCK_MONOTONIC, &ts_end) < 0) {
		return -EIO;
	}

	free(src);
	free(dst);

	duration = ts_end.tv_sec - ts_start.tv_sec +
		(ts_end.tv_nsec - ts_start.tv_nsec) / 1000000000.0;

	INFO("%2d Memory Speed: %.3f MB/s\n", p,
		1.0 * size * count / duration / (1024 * 1024));

	if (close(pipe_results[1]) < 0) {
		return -EPIPE;
	}

	if (p == 1) {
		/* this read will get EOF when all the processes have stopped */
		if (read(pipe_results[0], &junk, 1) < 0) {
			return -EPIPE;
		}
	}

	return 0;
}

static int do_bandwidth(int argc, char **argv)
{
	size_t size = 16 * 1024 * 1024;
	size_t count = 16;
	int nproc = 1;
	int opt;

	while ((opt = getopt (argc, argv, "s:n:p:")) != -1) {
		switch (opt) {
		case 's':
			size = atoi(optarg);
			break;
		case 'n':
			count = atoi(optarg);
			break;
		case 'p':
			nproc = atoi(optarg);
			break;
		default:
			return -EINVAL;
		}
	}
	if (optind < argc) {
		return -EINVAL;
	}

	return mem_bandwidth(size, count, nproc);
}

static const uint64_t patterns[] = {
	0,
	0xffffffffffffffffULL,
	0x5555555555555555ULL,
	0xaaaaaaaaaaaaaaaaULL,
	0x5a5a5a5a5a5a5a5aULL,
	0xa5a5a5a5a5a5a5a5ULL,
	0x1111111111111111ULL,
	0x2222222222222222ULL,
	0x4444444444444444ULL,
	0x8888888888888888ULL,
	0x3333333333333333ULL,
	0x6666666666666666ULL,
	0x9999999999999999ULL,
	0xccccccccccccccccULL,
	0x7777777777777777ULL,
	0xbbbbbbbbbbbbbbbbULL,
	0xddddddddddddddddULL,
	0xeeeeeeeeeeeeeeeeULL,
	0x7a6c7258554e494cULL, /* yeah ;-) */
};

static const int pattern_num = sizeof(patterns) / sizeof(patterns[0]);

static int mem_verify(size_t size, size_t count)
{
	uint64_t *addr;
	int n, i, j, err;

	addr = mmap(NULL, size * sizeof(uint64_t), PROT_READ |
			PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
	if (addr == NULL) {
		ERR("Failed to allocated memory!\n");
		return -ENOMEM;
	}

	err = 0;
	for (n = 0; n < count; n++) {
		for (i = 0; i < pattern_num; i++) {
			/* assin the patterns */
			for (j = 0; j < size; j++) {
				addr[j] = patterns[i];
			}

			/* do barrier here */
			__asm__ __volatile__("": : :"memory");

			/* check the patterns */
			for (j = 0; j < size; j++) {
				if (addr[j] != patterns[i]) {
					INFO("0x%llx != 0x%llx\n",
						addr[j], patterns[i]);
					err++;
				}
			}
		}

		if (err) {
			break;
		}
	}

	if (err) {
		INFO("Failure\n");
	} else {
		INFO("Success\n");
	}

	munmap(addr, size * sizeof(uint64_t));

	return err;
}

static int do_verify(int argc, char **argv)
{
	size_t size = 16 * 1024 * 1024;
	size_t count = 16;
	int opt;

	while ((opt = getopt (argc, argv, "s:n:")) != -1) {
		switch (opt) {
		case 's':
			size = atoi(optarg);
			break;
		case 'n':
			count = atoi(optarg);
			break;
		default:
			return -EINVAL;
		}
	}

	if (optind < argc) {
		return -EINVAL;
	}

	return mem_verify(size, count);
}

static void usage(void)
{
	INFO("Usage:\n");
	INFO("  utest_mem bandwidth [-s size] [-n count] [-p processoes]\n");
	INFO("  utest_mem verify [-s size] [-n count]\n");
}

int main(int argc, char **argv)
{
	char *cmd;
	int rval = -EINVAL;

	if (argc < 2) {
		usage();
		return rval;
	}

	cmd = argv[1];
	argc--;
	argv++;
	INFO("utest_mem -- %s\n", cmd);
	if (strcmp(cmd, "bandwidth") == 0) {
		rval = do_bandwidth(argc, argv);
	} else if (strcmp(cmd, "verify") == 0) {
		rval = do_verify(argc, argv);
	}

	if (rval == -EINVAL) {
		usage();
	}

	return rval;
}
