#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>

static const uint64_t patterns[] = {
	0,
	0xffffffffffffffffULL,
	0x5555555555555555ULL,
	0xaaaaaaaaaaaaaaaaULL,
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

int do_one_pass(uint64_t pattern, uint64_t *p, int size)
{
	int i, err;

	err = 0;
	for (i = 0; i < size; i++) {
		p[i] = pattern;
	}
	for (i = 0; i < size; i++) {
		if (pattern != p[i]) {
			fprintf(stdout, "0x%llx != 0x%llx\n", pattern, p[i]);
			err++;
		}
	}

	return err;
}

int main(int argc, char **argv)
{
	int i, n, length, count, err;
	uint64_t *addr;

	if (argc != 3) {
		fprintf(stderr, "Usage: memverify <length> <count>\n");
		return -1;
	}

	length = atoi(argv[1]);
	count = atoi(argv[2]);
	err = 0;

	addr = mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
	if (addr == NULL) {
		fprintf(stderr, "Failed to allocated memory!\n");
		return -1;
	}

	for (n = 0; n < count; n++) {
		for (i = 0; i < pattern_num; i++) {
			err += do_one_pass(patterns[i], addr, length / sizeof(uint64_t));
		}

		if (err) {
			fprintf(stdout, "Error %d !\n", err);
			break;
		}
	}

	munmap(addr, length);

	return err;
}
