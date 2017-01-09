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

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <libgen.h>
#include <time.h>
#include <unistd.h>

#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#define _GNU_SOURCE
#include<sched.h>
#define SUPPORT_LINUX_L 0
 extern "C" int set_cpu_affinity(unsigned int cpu);
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
