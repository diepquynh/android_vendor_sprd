/*
 * memtester version 4
 *
 * Very simple but very effective user-space memory tester.
 * Originally by Simon Kirby <sim@stormix.com> <sim@neato.org>
 * Version 2 by Charles Cazabon <charlesc-memtester@pyropus.ca>
 * Version 3 not publicly released.
 * Version 4 rewrite:
 * Copyright (C) 2004-2012 Charles Cazabon <charlesc-memtester@pyropus.ca>
 * Licensed under the terms of the GNU General Public License version 2 (only).
 * See the file COPYING for details.
 *
 */

#define __version__ "4.3.0"

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#define _GNU_SOURCE
#include<sched.h>
#include<ctype.h>
#include<string.h>

#include <pthread.h>

#include "types.h"
#include "sizes.h"
#include "tests.h"
#include "log.h"

#define SUPPORT_LINUX_L 0

#define EXIT_FAIL_NONSTARTER    0x01
#define EXIT_FAIL_ADDRESSLINES  0x02
#define EXIT_FAIL_OTHERTEST     0x04

struct test tests[] = {
    { "Random Value", test_random_value },
    { "Compare XOR", test_xor_comparison },
    { "Compare SUB", test_sub_comparison },
    { "Compare MUL", test_mul_comparison },
    { "Compare DIV",test_div_comparison },
    { "Compare OR", test_or_comparison },
    { "Compare AND", test_and_comparison },
    { "Sequential Increment", test_seqinc_comparison },
    { "Solid Bits", test_solidbits_comparison },
    { "Block Sequential", test_blockseq_comparison },
    { "Checkerboard", test_checkerboard_comparison },
    { "Bit Spread", test_bitspread_comparison },
    { "Bit Flip", test_bitflip_comparison },
    { "Walking Ones", test_walkbits1_comparison },
    { "Walking Zeroes", test_walkbits0_comparison },
#ifdef TEST_NARROW_WRITES    
    { "8-bit Writes", test_8bit_wide_random },
    { "16-bit Writes", test_16bit_wide_random },
#endif
    { "Bit Flip for Tiger", test_bitflip_comparison_tiger},
    { NULL, NULL }
};

/* Sanity checks and portability helper macros. */
#ifdef _SC_VERSION
void check_posix_system(void) {
    if (sysconf(_SC_VERSION) < 198808L) {
        fprintf(stdout, "A POSIX system is required.  Don't be surprised if "
            "this craps out.\n");
        fprintf(stdout, "_SC_VERSION is %lu\n", sysconf(_SC_VERSION));
    }
}
#else
#define check_posix_system()
#endif

#ifdef _SC_PAGE_SIZE
int memtester_pagesize(void) {
    int pagesize = sysconf(_SC_PAGE_SIZE);
    if (pagesize == -1) {
        perror("get page size failed");
        exit(EXIT_FAIL_NONSTARTER);
    }
    printf("pagesize is %ld\n", (long) pagesize);
    return pagesize;
}
#else
int memtester_pagesize(void) {
    printf("sysconf(_SC_PAGE_SIZE) not supported; using pagesize of 8192\n");
    return 8192;
}
#endif

/* Some systems don't define MAP_LOCKED.  Define it to 0 here
   so it's just a no-op when ORed with other constants. */
#ifndef MAP_LOCKED
  #define MAP_LOCKED 0
#endif

/* Function declarations */
void usage(char *me);

/* Global vars - so tests have access to this information */
int use_phys = 0;
off_t physaddrbase = 0;
int compare_20_times = 0;
int mode = 0; //0:normal, 1:refresh, 2:normal+refresh
int coreIndex = -1;
int use_refresh = 0;
int reftime = 0;
int refsize = 0;
int volatile bfinished = 0;
refresh_t ref = {0};
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* Function definitions */
void usage(char *me) {
    fprintf(stdout, "\n"
            "Usage: %s [-p physaddrbase [-d device]] <mem>[B|K|M|G] [loops] [mode] [coreindex] [refsize] [reftime]\n",
            me);
    exit(EXIT_FAIL_NONSTARTER);
}


#include <stdio.h>

#define FMT "%10d "

#include <sys/resource.h>

#define doit(name) pr_limits(#name, name)

#define ZeroArray(arr) memset(arr, 0, sizeof(arr));

static void pr_limits(char *, int);


#define fprintf my_fprint_f
#define printf my_printf

static void
pr_limits(char *name, int resource)
{
    struct rlimit limit;

    if (getrlimit(resource, &limit) < 0) {
        printf("getrlimit error for %s\n", name);
        exit(1);
    }
    printf("%-14s ", name);
    if (limit.rlim_cur == RLIM_INFINITY)
        printf("(infinite) ");
    else
        printf(FMT, limit.rlim_cur);
    if (limit.rlim_max == RLIM_INFINITY)
        printf("(infinite)");
    else
        printf(FMT, limit.rlim_max);
    putchar((int)'\n');
}

void volatile* malloc_lock_mem(size_t wantbytes, size_t wantbytes_orig, ptrdiff_t pagesizemask,
									size_t pagesize, int *do_mlock, size_t* bufsize) {
	void volatile *buf = NULL, *aligned = NULL;
	int  done_mem = 0;

	while (!done_mem) {
		while (!buf && wantbytes) {
			buf = (void volatile *) malloc(wantbytes);
			if (!buf)
				wantbytes -= pagesize;
		}
		*bufsize = wantbytes;
		printf("got  %lluMB (%llu bytes)", (ull) wantbytes >> 20,
				(ull) wantbytes);
		fflush(stdout);
		if (*do_mlock) {
			printf(", trying mlock ...");

			fprintf(stdout, ", trying mlock ...");
			fsync(stdout);
			fflush(stdout);
			if ((size_t) buf % pagesize) {
				/* printf("aligning to page -- was 0x%tx\n", buf); */
				aligned = (void volatile *) ((size_t) buf & pagesizemask)
						+ pagesize;
				/* printf("  now 0x%tx -- lost %d bytes\n", aligned,
				 *      (size_t) aligned - (size_t) buf);
				 */
				*bufsize -= ((size_t) aligned - (size_t) buf);
			} else {
				aligned = buf;
			}
			/* Try mlock */
			if (mlock((void *) aligned, *bufsize) < 0) {
				switch (errno) {
				case EAGAIN: /* BSDs */
					printf("over system/pre-process limit, reducing...\n");
					free((void *) buf);
					buf = NULL;
					wantbytes -= pagesize;
					break;
				case ENOMEM:
					printf("too many pages, reducing...\n");
					free((void *) buf);
					buf = NULL;
					wantbytes -= pagesize;
					break;
				case EPERM:
					printf("insufficient permission.\n");
					printf("Trying again, unlocked:\n");
					*do_mlock = 0;
					free((void *) buf);
					buf = NULL;
					wantbytes = wantbytes_orig;
					break;
				default:
					printf("failed for unknown reason.\n");
					*do_mlock = 0;
					done_mem = 1;
				}
			} else {
				printf("locked.\n");
				done_mem = 1;
			}
		} else {
			done_mem = 1;
			printf("\n");
		}
	}

	if (!(*do_mlock))
		fprintf(stdout, "Continuing with unlocked memory; testing "
				"will be slower and less reliable.\n");

	return aligned;
}

void test_refresh(ptrdiff_t pagesizemask, size_t pagesize)
{
	int ret;
	void volatile *buf, *aligned;
    pthread_t tid;
    size_t halflen, bufsize, wantbytes, wantbytes_orig;

    printf("test refresh\n");
    if(!use_refresh) {
    	return ;
    }

    wantbytes_orig = wantbytes = refsize<<10;
    ref.do_mlock = 1;
	aligned = malloc_lock_mem(wantbytes, wantbytes_orig, pagesizemask, pagesize, &(ref.do_mlock), &bufsize);
	if (aligned == NULL) {
		fprintf(stdout, "failed to malloc_lock_mem\n");
		exit(EXIT_FAIL_NONSTARTER);
	}

	printf("\n\n refresh: malloc_lock_mem: %x\n\n", aligned);

    halflen = bufsize / 2;
    ref.count = halflen / sizeof(ul);
    ref.bufa = (ulv *) aligned;
    ref.bufb = (ulv *) ((size_t) aligned + halflen);
    ref.use = use_refresh;
    ref.time = reftime;

    ret = pthread_create(&tid, NULL, (void*)test_bitRefresh_comparison, &ref);
    if(ret < 0)
        printf("test refresh: create thread fail!");
}

int main(int argc, char **argv) {
    ul loops, loop, i;
    size_t pagesize, wantraw, wantmb, wantbytes, wantbytes_orig, bufsize,
         halflen, count;
    char *memsuffix, *addrsuffix, *loopsuffix, *modesuffix, *coreIndexsuffix, *reftimesuffix, *refsizesuffix;
    ptrdiff_t pagesizemask;
    void volatile *buf, *aligned;
    ulv *bufa, *bufb;
    int do_mlock = 1, done_mem = 0;
    int exit_code = 0;
    int memfd, opt, memshift;
    size_t maxbytes = -1; /* addressable memory, in bytes */
    size_t maxmb = (maxbytes >> 20) + 1; /* addressable memory, in MB */
    /* Device to mmap memory from with -p, default is normal core */
    char *device_name = "/dev/mem";
    struct stat statbuf;
    int device_specified = 0;
    char *env_testmask = 0;
    ul testmask = 0;

	ConnectSrv();

    printf("pid=%d\n",getpid());

    printf("memtester version " __version__ " (%d-bit)\n", UL_LEN);
    printf("Copyright (C) 2001-2012 Charles Cazabon.\n");

    doit(RLIMIT_MEMLOCK);

    printf("Licensed under the GNU General Public License version 2 (only).\n");

    printf("\n");
    check_posix_system();
    pagesize = memtester_pagesize();
    pagesizemask = (ptrdiff_t) ~(pagesize - 1);
    printf("pagesizemask is 0x%tx\n", pagesizemask);
    
    /* If MEMTESTER_TEST_MASK is set, we use its value as a mask of which
       tests we run.
     */
    if (env_testmask = getenv("MEMTESTER_TEST_MASK")) {
        errno = 0;
        testmask = strtoul(env_testmask, 0, 0);
        if (errno) {
            fprintf(stdout, "error parsing MEMTESTER_TEST_MASK %s: %s\n",
                    env_testmask, strerror(errno));
            usage(argv[0]); /* doesn't return */
        }
        printf("using testmask 0x%lx\n", testmask);
    }

    while ((opt = getopt(argc, argv, "p:d:")) != -1) {
        switch (opt) {
            case 'p':
                errno = 0;
                physaddrbase = (off_t) strtoull(optarg, &addrsuffix, 16);
                if (errno != 0) {
                    fprintf(stdout,
                            "failed to parse physaddrbase arg; should be hex "
                            "address (0x123...)\n");
                    usage(argv[0]); /* doesn't return */
                }
                if (*addrsuffix != '\0') {
                    /* got an invalid character in the address */
                    fprintf(stdout,
                            "failed to parse physaddrbase arg; should be hex "
                            "address (0x123...)\n");
                    usage(argv[0]); /* doesn't return */
                }
                if (physaddrbase & (pagesize - 1)) {
                    fprintf(stdout,
                            "bad physaddrbase arg; does not start on page "
                            "boundary\n");
                    usage(argv[0]); /* doesn't return */
                }
                /* okay, got address */
                use_phys = 1;
                break;
            case 'd':
                if (stat(optarg,&statbuf)) {
                    fprintf(stdout, "can not use %s as device: %s\n", optarg,
                            strerror(errno));
                    usage(argv[0]); /* doesn't return */
                } else {
                    if (!S_ISCHR(statbuf.st_mode)) {
                        fprintf(stdout, "can not mmap non-char device %s\n",
                                optarg);
                        usage(argv[0]); /* doesn't return */
                    } else {
                        device_name = optarg;
                        device_specified = 1;
                    }
                }
                break;              
            default: /* '?' */
                usage(argv[0]); /* doesn't return */
        }
    }

    if (device_specified && !use_phys) {
        fprintf(stdout,
                "for mem device, physaddrbase (-p) must be specified\n");
        usage(argv[0]); /* doesn't return */
    }
    
    if (optind >= argc) {
        fprintf(stdout, "need memory argument, in MB\n");
        usage(argv[0]); /* doesn't return */
    }

    errno = 0;
    wantraw = (size_t) strtoul(argv[optind], &memsuffix, 0);
    if (errno != 0) {
        fprintf(stdout, "failed to parse memory argument");
        usage(argv[0]); /* doesn't return */
    }
    switch (*memsuffix) {
        case 'G':
        case 'g':
            memshift = 30; /* gigabytes */
            break;
        case 'M':
        case 'm':
            memshift = 20; /* megabytes */
            break;
        case 'K':
        case 'k':
            memshift = 10; /* kilobytes */
            break;
        case 'B':
        case 'b':
            memshift = 0; /* bytes*/
            break;
        case '\0':  /* no suffix */
            memshift = 20; /* megabytes */
            break;
        default:
            /* bad suffix */
            usage(argv[0]); /* doesn't return */
    }
    wantbytes_orig = wantbytes = ((size_t) wantraw << memshift);
    wantmb = (wantbytes_orig >> 20);
    optind++;
    if (wantmb > maxmb) {
        fprintf(stdout, "This system can only address %llu MB.\n", (ull) maxmb);
        exit(EXIT_FAIL_NONSTARTER);
    }
    if (wantbytes < pagesize) {
        fprintf(stdout, "bytes %ld < pagesize %ld -- memory argument too large?\n",
                wantbytes, pagesize);
        exit(EXIT_FAIL_NONSTARTER);
    }

    if (optind >= argc) {
        loops = 0;
    } else {
        errno = 0;
        loops = strtoul(argv[optind], &loopsuffix, 0);
        if (errno != 0) {
            fprintf(stdout, "failed to parse number of loops");
            usage(argv[0]); /* doesn't return */
        }
        if (*loopsuffix != '\0') {
            fprintf(stdout, "loop suffix %c\n", *loopsuffix);
            usage(argv[0]); /* doesn't return */
        }
    }

    optind++;
    if (optind >= argc) {
        mode = 0;
    } else {
        errno = 0;
        mode = strtoul(argv[optind], &modesuffix, 0);
        if (errno != 0) {
            fprintf(stdout, "failed to parse number of mode");
            usage(argv[0]); /* doesn't return */
        }
        if (*modesuffix != '\0') {
            fprintf(stdout, "mode suffix %c\n", *modesuffix);
            usage(argv[0]); /* doesn't return */
        }
    }

    compare_20_times = mode;
    fprintf(stdout, "mode = %d\n", mode);

    optind++;
    if (optind >= argc) {
        coreIndex = -1;
    } else {
        errno = 0;
        coreIndex = strtoul(argv[optind], &coreIndexsuffix, 0);
        if (errno != 0) {
            fprintf(stdout, "failed to parse number of coreIndex");
            usage(argv[0]); /* doesn't return */
        }
        if (*modesuffix != '\0') {
            fprintf(stdout, "mode suffix %c\n", *modesuffix);
            usage(argv[0]); /* doesn't return */
        }
    }
    fprintf(stdout, "coreIndex = %d\n", coreIndex);
    if (coreIndex >= 0 && coreIndex < 4) {
    	set_cpu_affinity(coreIndex);
    }

    optind++;
    if (optind >= argc) {
    	refsize = 0;
    } else {
        errno = 0;
        refsize = strtoul(argv[optind], &refsizesuffix, 0);
        if (errno != 0) {
            fprintf(stdout, "failed to parse number of coreIndex");
            usage(argv[0]); /* doesn't return */
        }
        if (*refsizesuffix != '\0') {
            fprintf(stdout, "mode suffix %c\n", *refsizesuffix);
            usage(argv[0]); /* doesn't return */
        }
    }
    fprintf(stdout, "refsize = %d\n", refsize);

    optind++;
    if (optind >= argc) {
    	reftime = 0;
    } else {
        errno = 0;
        reftime = strtoul(argv[optind], &reftimesuffix, 0);
        if (errno != 0) {
            fprintf(stdout, "failed to parse number of reftime");
            usage(argv[0]); /* doesn't return */
        }
        if (*reftimesuffix != '\0') {
            fprintf(stdout, "mode suffix %c\n", *reftimesuffix);
            usage(argv[0]); /* doesn't return */
        }
    }
    fprintf(stdout, "reftime = %d\n", reftime);

    if(refsize == 0||reftime == 0) {
    	use_refresh = 0;
    }else {
    	use_refresh = 1;
    }

    printf("want %lluMB (%llu bytes)\n", (ull) wantmb, (ull) wantbytes);
    buf = NULL;

    if (use_phys) {
        memfd = open(device_name, O_RDWR | O_SYNC);
        if (memfd == -1) {
            fprintf(stdout, "failed to open %s for physical memory: %s\n",
                    device_name, strerror(errno));
            exit(EXIT_FAIL_NONSTARTER);
        }
        buf = (void volatile *) mmap(0, wantbytes, PROT_READ | PROT_WRITE,
                                     MAP_SHARED | MAP_LOCKED, memfd,
                                     physaddrbase);
        if (buf == MAP_FAILED) {
            fprintf(stdout, "failed to mmap %s for physical memory: %s\n",
                    device_name, strerror(errno));
            exit(EXIT_FAIL_NONSTARTER);
        }

        if (mlock((void *) buf, wantbytes) < 0) {
            fprintf(stdout, "failed to mlock mmap'ed space\n");
            do_mlock = 0;
        }

        bufsize = wantbytes; /* accept no less */
        aligned = buf;
        done_mem = 1;
    }

    if (use_refresh && (mode&MODE_REFRESH) ) {
    	test_refresh(pagesizemask, pagesize);
    }

    if (mode == MODE_REFRESH) {

    	do{
    		sleep(100);
    	}while(1);

    }else if (mode&MODE_NORMAL){
    	pthread_mutex_lock(&mutex);
    	aligned = malloc_lock_mem(wantbytes, wantbytes_orig, pagesizemask, pagesize, &do_mlock, &bufsize);
    	if (aligned == NULL) {
    		fprintf(stdout, "failed to malloc_lock_mem\n");
    		exit(EXIT_FAIL_NONSTARTER);
    	}
    	pthread_mutex_unlock(&mutex);

    	printf("\n\n malloc_lock_mem: %x\n\n", aligned);

        halflen = bufsize / 2;
        count = halflen / sizeof(ul);
        bufa = (ulv *) aligned;
        bufb = (ulv *) ((size_t) aligned + halflen);

        for(loop=1; ((!loops) || loop <= loops); loop++) {
            printf("Loop %lu", loop);
            if (loops) {
                printf("/%lu", loops);
            }
            printf(":\n");
            fflush(stdout);
            if (!test_stuck_address(aligned, bufsize / sizeof(ul))) {
 		 printf("  %-20s: ok\n", "Stuck Address");
            	 fflush(stdout);
            } else {
                exit_code |= EXIT_FAIL_ADDRESSLINES;
            }
            for (i=0;;i++) {
                if (!tests[i].name) break;
                /* If using a custom testmask, only run this test if the
                   bit corresponding to this test was set by the user.
                 */
                if (testmask && (!((1 << i) & testmask))) {
                    continue;
                }
                if (!tests[i].fp(bufa, bufb, count)) {
                	printf("  %-20s: ok\n", tests[i].name);
                } else {
                    exit_code |= EXIT_FAIL_OTHERTEST;
                }
                fflush(stdout);
            }
            printf("\n");
            fflush(stdout);
        }
        if (do_mlock) munlock((void *) aligned, bufsize);
        printf("Done.\n");
        fflush(stdout);
    }else {
    }

	DisconnectSrv();

	exit(exit_code);
}

#if 0
int set_cpu_affinity(unsigned int cpu)
{
#if !SUPPORT_LINUX_L

	int num = sysconf(_SC_NPROCESSORS_CONF);

        cpu_set_t cpuset;

        fprintf(stdout, "system has %d processor(s), cpu = %d \n", num, cpu);

        if (sched_getaffinity(getpid(), sizeof(cpuset), &cpuset) == 0) {
                //CPU_ZERO(&cpuset);
                CPU_SET(cpu, &cpuset);
                if (sched_setaffinity(getpid(), sizeof(cpuset), &cpuset) < 0) {
                	fprintf(stdout, "errno = %d \n", errno);
                    fprintf(stdout, "warning: unable to set cpu affinity \n");
                    return -1;
                }
        }
#endif
        return 1;
}
#endif
