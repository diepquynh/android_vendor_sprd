/*
 * Copyright (C) 2012 The Android Open Source Project
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
#define LOG_TAG "cmr_copy"

#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include "cmr_cvt.h"

static sem_t dma_copy_sem;
static char dma_copy_dev_name[50] = "/dev/sprd_dma_copy";
static int dma_copy_fd = -1;

int cmr_dma_copy_init(void)
{
	int                      ret = 0;

	dma_copy_fd = open(dma_copy_dev_name, O_RDWR, 0);

	if (-1 == dma_copy_fd) {
		CMR_LOGE("Fail to open dma copy device.");
		return -ENODEV;
	} else {
		CMR_LOGI("OK to open dma copy device.");
	}

	sem_init(&dma_copy_sem, 0, 1);

	return ret;
}

int cmr_dma_copy_deinit(void)
{
	int                      ret = 0;

	CMR_LOGI("Start to close dma copy device.");

	if (-1 == dma_copy_fd) {
		CMR_LOGE("Invalid dam copyfd");
		return -ENODEV;
	}

	sem_wait(&dma_copy_sem);
	sem_post(&dma_copy_sem);

	sem_destroy(&dma_copy_sem);

	if (-1 == close(dma_copy_fd)) {
		exit(EXIT_FAILURE);
	}

	dma_copy_fd = -1;

	CMR_LOGI("close device.");

	return 0;
}

int cmr_dma_cpy(struct _dma_copy_cfg_tag dma_copy_cfg)
{
	int                      ret = 0;

	if (-1 == dma_copy_fd) {
		CMR_LOGE("invalid dma copy fd");
		return -ENODEV;
	}

	if (DMA_COPY_YUV400 <= dma_copy_cfg.format ||
		(dma_copy_cfg.src_size.w & 0x01) || (dma_copy_cfg.src_size.h & 0x01) ||
		(dma_copy_cfg.src_rec.x & 0x01) || (dma_copy_cfg.src_rec.y & 0x01) ||
		(dma_copy_cfg.src_rec.w & 0x01) || (dma_copy_cfg.src_rec.h & 0x01) ||
		0 == dma_copy_cfg.src_addr.y_addr || 0 == dma_copy_cfg.src_addr.uv_addr ||
		0 == dma_copy_cfg.dst_addr.y_addr || 0 == dma_copy_cfg.dst_addr.uv_addr ||
		0 == dma_copy_cfg.src_rec.w || 0 == dma_copy_cfg.src_rec.h ||
		0 == dma_copy_cfg.src_size.w|| 0 == dma_copy_cfg.src_size.h ||
		(dma_copy_cfg.src_rec.x + dma_copy_cfg.src_rec.w > dma_copy_cfg.src_size.w) ||
		(dma_copy_cfg.src_rec.y + dma_copy_cfg.src_rec.h > dma_copy_cfg.src_size.h)) {

		CMR_LOGE("dma copy wrong parameter.");
		return -EINVAL;
	}

	sem_wait(&dma_copy_sem);

	ret = ioctl(dma_copy_fd, 0, &dma_copy_cfg);

	sem_post(&dma_copy_sem);

	return ret;
}
