/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#include <hardware/memtrack.h>

#include "memtrack_sprd.h"

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))
#define min(x, y) ((x) < (y) ? (x) : (y))

struct memtrack_record record_templates[] = {
    {
        .flags = MEMTRACK_FLAG_SMAPS_ACCOUNTED |
                 MEMTRACK_FLAG_PRIVATE |
                 MEMTRACK_FLAG_NONSECURE,
    },
    {
        .flags = MEMTRACK_FLAG_SMAPS_UNACCOUNTED |
                 MEMTRACK_FLAG_PRIVATE |
                 MEMTRACK_FLAG_NONSECURE,
    },
};


#define LOG_TAG "memtrack_sprd"

#include <log/log.h>


int sprd_check_memory(pid_t pid, enum memtrack_type type,
                             struct memtrack_record *records,
                             size_t *num_records)
{
	size_t allocated_records = min(*num_records, ARRAY_SIZE(record_templates));
	int i;
	FILE *fp;
	FILE *smaps_fp = NULL;
	char line[1024];
	char tmp[128];
	size_t accounted_size = 0;
	size_t unaccounted_size = 0;
	unsigned long smaps_addr = 0;
	unsigned int pid_gl;
	*num_records = ARRAY_SIZE(record_templates);

	ALOGD("sprd_check_memory begin,pid:%d, type:%d, num records:%d",(int)pid, type, *num_records);

	/* fastpath to return the necessary number of records */
	if (allocated_records == 0) {
		return 0;
	}

	memcpy(records, record_templates,
		   sizeof(struct memtrack_record) * allocated_records);

	sprintf(tmp, "/sys/kernel/debug/mali/memory_usage");
	fp = fopen(tmp, "r");
	if (fp == NULL) {
		ALOGD("sprd_check_memory ,cant open mali file");
		return -errno;
	}

	if (type == MEMTRACK_TYPE_GL) {
		sprintf(tmp, "/proc/%d/smaps", pid);
		smaps_fp = fopen(tmp, "r");
		if (smaps_fp == NULL) {
			ALOGD("sprd_check_memory ,can't open pid %d smaps",(int)pid);
			fclose(fp);
			return -errno;
		}
	}

	while (1) {
		unsigned long uaddr;
		unsigned long size;
		char line_type[7];
		int ret;

		if (fgets(line, sizeof(line), fp) == NULL) {
			break;
		}

		/* Format:
		 */
		ret = sscanf(line, "%u %*[0] %*[xX] %lx\n", &pid_gl, &size);
		if (ret != 2) {
			continue;
		}
		ALOGD("sprd_check_memory read line from gl ,pid:%d, size:%x",pid_gl,size);
		if (pid_gl !=(unsigned int)pid)
			continue;

		if (type == MEMTRACK_TYPE_GL ) {
			bool accounted = false;
			/*
			 * We need to cross reference the user address against smaps,
			 *	luckily both are sorted.
			 */

			/* uaddr is not provided in mali node, so ignore accounted type. */
			if (!accounted) {
				unaccounted_size += size;
			}
		} else if (type == MEMTRACK_TYPE_GRAPHICS ) {
		//get from sys node of ion
			unaccounted_size += size;
		}
	}

	if (allocated_records > 0) {
		records[0].size_in_bytes = accounted_size;
	}
	if (allocated_records > 1) {
		records[1].size_in_bytes = unaccounted_size;
	}

	if (smaps_fp)
		fclose(smaps_fp);
	fclose(fp);

	ALOGD("sprd_check_memory end,accounted_size:%d,unaccounted_size:%d",accounted_size,unaccounted_size);

	return 0;
}

