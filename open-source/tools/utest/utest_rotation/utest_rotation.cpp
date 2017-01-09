/*
 * Copyright (C) 2008 The Android Open Source Project
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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cutils/log.h>
#include "sprd_rot_k.h"
#include "cmr_common.h"
#include <semaphore.h>
#include "ion_sprd.h"
#include <binder/MemoryHeapIon.h>

using namespace android;

#define UTEST_ROTATION_COUNTER  0XFFFFFFFF
#define SAVE_ROTATION_OUTPUT_DATA 0
#define ERR(x...) fprintf(stderr, x)
#define INFO(x...) fprintf(stdout, x)

#define UTEST_ROTATION_EXIT_IF_ERR(n)                      \
	do {                                                                 \
		if (n) {                                           \
			ERR("utest rotate  error  . Line:%d ", __LINE__);\
			goto exit;                   \
		}                                                    \
	} while(0)

enum file_number {
	NO_FILE = 0,
	ONE_FILE,
	TWO_FLIE,
	MAX_FILE_NUMBER
};

struct utest_rotation_context {
	sp<MemoryHeapIon> input_y_pmem_hp;
	uint32_t input_y_pmemory_size;
	int input_y_physical_addr ;
	unsigned char* input_y_virtual_addr;

	sp<MemoryHeapIon> input_uv_pmem_hp;
	uint32_t input_uv_pmemory_size;
	int input_uv_physical_addr ;
	unsigned char* input_uv_virtual_addr;

	sp<MemoryHeapIon> output_y_pmem_hp;
	uint32_t output_y_pmemory_size;
	int output_y_physical_addr ;
	unsigned char* output_y_virtual_addr;

	sp<MemoryHeapIon> output_uv_pmem_hp;
	uint32_t output_uv_pmemory_size;
	int output_uv_physical_addr ;
	unsigned char* output_uv_virtual_addr;

	uint32_t width;
	uint32_t height;
	int angle;
	enum file_number filenum;
};

static char utest_rotation_src_y_file[] = "/data/utest_rotation_src_y.raw";
static char utest_rotation_src_uv_file[] = "/data/utest_rotation_src_uv.raw";
static char utest_rotation_src_yuv_file[] = "/data/utest_rotation_src_yuv.raw";
static char utest_rotation_dst_y_file[] = "/data/utest_rotation_dst_y_%dx%d-angle%d.raw";
static char utest_rotation_dst_uv_file[] = "/data/utest_rotation_dst_uv_%dx%d-angle%d.raw";


static int utest_do_rotation(int fd,uint32_t width,uint32_t height,
							uint32_t input_yaddr,uint32_t input_uvaddr,
							uint32_t output_yaddr,uint32_t output_uvaddr,
							ROT_DATA_FORMAT_E format,enum img_rot_angle angle)
{
	int ret = 0;
	struct _rot_cfg_tag rot_cfg;

	memset((void *)&rot_cfg,0x00,sizeof(struct _rot_cfg_tag));
	rot_cfg.angle = (ROT_ANGLE_E)(angle - IMG_ROT_90 + ROT_90);
	rot_cfg.format = format;
	rot_cfg.img_size.w = (uint16_t)width;
	rot_cfg.img_size.h =(uint16_t)height;
	rot_cfg.src_addr.y_addr = input_yaddr;
	rot_cfg.src_addr.u_addr = input_uvaddr;
	rot_cfg.src_addr.v_addr = input_uvaddr;
	rot_cfg.dst_addr.y_addr = output_yaddr;
	rot_cfg.dst_addr.u_addr = output_uvaddr;
	rot_cfg.dst_addr.v_addr = output_uvaddr;

	ret = ioctl(fd, ROT_IO_START, &rot_cfg);
	if (ret) {
		ERR("utest_rotation camera_rotation fail. Line:%d", __LINE__);
		ret = -1;
	} else {
		ret = 0;
	}
	return ret;
}

static void usage(void)
{
	INFO("Usage:\n");
	INFO("utest_rotation -iw width -ih height -ia angle(90 180 270 -1) -if filenum(1 2)\n");
}

static enum img_rot_angle utest_rotation_angle_cvt(int angle)
{
	enum img_rot_angle tmp_angle;
	if(angle == 90)
	{
		tmp_angle = IMG_ROT_90;
	}
	else if(angle == 270)
	{
		tmp_angle = IMG_ROT_270;
	}
	else if(angle == 180)
	{
		tmp_angle = IMG_ROT_180;
	}
	else  if(angle == -1)
	{
		tmp_angle = IMG_ROT_MIRROR;
	}
	else
	{
		ERR("utest rotate  error  . Line:%d ", __LINE__);
	}
//	INFO("utest_rotation_angle_cvt %d \n",tmp_angle);
	return tmp_angle;
}


static int utest_rotation_param_set(struct utest_rotation_context *g_utest_rotation_cxt_ptr,int argc, char **argv)
{
	int i = 0;
	struct utest_rotation_context *rotation_cxt_ptr = g_utest_rotation_cxt_ptr;

	if (argc < 9) {
		usage();
		return -1;
	}

	memset(rotation_cxt_ptr, 0, sizeof(utest_rotation_context));

	for (i=1; i<argc; i++) {
		if (strcmp(argv[i], "-iw") == 0 && (i < argc-1)) {
			rotation_cxt_ptr->width = atoi(argv[++i]);
		} else if (strcmp(argv[i], "-ih") == 0 && (i < argc-1)) {
			rotation_cxt_ptr->height = atoi(argv[++i]);
		} else if (strcmp(argv[i], "-ia") == 0 && (i < argc-1)) {
			rotation_cxt_ptr->angle= atoi(argv[++i]);
		}else if (strcmp(argv[i], "-if") == 0 && (i < argc-1)) {
			rotation_cxt_ptr->filenum= (enum file_number)(atoi(argv[++i]));
		}else {
			usage();
			return -1;
		}
	}


	return 0;
}

static int utest_mm_iommu_is_enabled(void)
{
	return MemoryHeapIon::Mm_iommu_is_enabled();
}

static int utest_rotation_mem_alloc(struct utest_rotation_context *g_utest_rotation_cxt_ptr)
{
	struct utest_rotation_context *rotation_cxt_ptr = g_utest_rotation_cxt_ptr;
	int s_mem_method = utest_mm_iommu_is_enabled();

//	INFO("utest_rotation_mem_alloc %d\n",s_mem_method);
	/* alloc input y buffer */
	if (0 == s_mem_method) {
		rotation_cxt_ptr->input_y_pmem_hp = new MemoryHeapIon("/dev/ion",
											rotation_cxt_ptr->width * rotation_cxt_ptr->height,
											MemoryHeapBase::NO_CACHING, ION_HEAP_ID_MASK_MM);
	} else {
		rotation_cxt_ptr->input_y_pmem_hp = new MemoryHeapIon("/dev/ion",
										rotation_cxt_ptr->width * rotation_cxt_ptr->height,
										MemoryHeapBase::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
	}
	if (rotation_cxt_ptr->input_y_pmem_hp->getHeapID() < 0) {
		ERR("failed to alloc input_y pmem buffer.\n");
		return -1;
	}

	if (0 == s_mem_method) {
		rotation_cxt_ptr->input_y_pmem_hp->get_phy_addr_from_ion((int *)(&rotation_cxt_ptr->input_y_physical_addr),
		(int *)(&rotation_cxt_ptr->input_y_pmemory_size));
	} else {
		rotation_cxt_ptr->input_y_pmem_hp->get_mm_iova((int *)(&rotation_cxt_ptr->input_y_physical_addr),
		(int *)(&rotation_cxt_ptr->input_y_pmemory_size));
	}
	rotation_cxt_ptr->input_y_virtual_addr = (unsigned char*)rotation_cxt_ptr->input_y_pmem_hp->base();
	if (!rotation_cxt_ptr->input_y_physical_addr) {
		ERR("failed to alloc input_y pmem buffer:addr is null.\n");
		return -1;
	}
	memset(rotation_cxt_ptr->input_y_virtual_addr, 0x80, rotation_cxt_ptr->width * rotation_cxt_ptr->height);


	/* alloc input uv buffer */
	if (0 == s_mem_method) {
		rotation_cxt_ptr->input_uv_pmem_hp = new MemoryHeapIon("/dev/ion",
											rotation_cxt_ptr->width* rotation_cxt_ptr->height/ 2,
											MemoryHeapBase::NO_CACHING, ION_HEAP_ID_MASK_MM);
	} else {
		rotation_cxt_ptr->input_uv_pmem_hp = new MemoryHeapIon("/dev/ion",
										rotation_cxt_ptr->width* rotation_cxt_ptr->height/ 2,
										MemoryHeapBase::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
	}
	if (rotation_cxt_ptr->input_uv_pmem_hp->getHeapID() < 0) {
		ERR("failed to alloc input_uv pmem buffer.\n");
		return -1;
	}

	if (0 == s_mem_method) {
		rotation_cxt_ptr->input_uv_pmem_hp->get_phy_addr_from_ion((int *)(&rotation_cxt_ptr->input_uv_physical_addr),
			(int *)(&rotation_cxt_ptr->input_uv_pmemory_size));
	} else {
		rotation_cxt_ptr->input_uv_pmem_hp->get_mm_iova((int *)(&rotation_cxt_ptr->input_uv_physical_addr),
			(int *)(&rotation_cxt_ptr->input_uv_pmemory_size));
	}
	rotation_cxt_ptr->input_uv_virtual_addr = (unsigned char*)rotation_cxt_ptr->input_uv_pmem_hp->base();
	if (!rotation_cxt_ptr->input_uv_physical_addr) {
		ERR("failed to alloc input_uv pmem buffer:addr is null.\n");
		return -1;
	}
	memset(rotation_cxt_ptr->input_uv_virtual_addr, 0x80, rotation_cxt_ptr->width * rotation_cxt_ptr->height/ 2);


	/* alloc outout y buffer */
	if (0 == s_mem_method) {
		rotation_cxt_ptr->output_y_pmem_hp = new MemoryHeapIon("/dev/ion",
											rotation_cxt_ptr->width* rotation_cxt_ptr->height,
											MemoryHeapBase::NO_CACHING, ION_HEAP_ID_MASK_MM);
	} else {
		rotation_cxt_ptr->output_y_pmem_hp = new MemoryHeapIon("/dev/ion",
										rotation_cxt_ptr->width* rotation_cxt_ptr->height,
										MemoryHeapBase::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
	}
	if (rotation_cxt_ptr->output_y_pmem_hp->getHeapID() < 0) {
		ERR("failed to alloc output_y pmem buffer.\n");
		return -1;
	}

	if (0 == s_mem_method) {
		rotation_cxt_ptr->output_y_pmem_hp->get_phy_addr_from_ion((int *)(&rotation_cxt_ptr->output_y_physical_addr),
			(int *)(&rotation_cxt_ptr->output_y_pmemory_size));
	} else {
		rotation_cxt_ptr->output_y_pmem_hp->get_mm_iova((int *)(&rotation_cxt_ptr->output_y_physical_addr),
		(int *)(&rotation_cxt_ptr->output_y_pmemory_size));
	}
	rotation_cxt_ptr->output_y_virtual_addr = (unsigned char*)rotation_cxt_ptr->output_y_pmem_hp->base();
	if (!rotation_cxt_ptr->output_y_physical_addr) {
		ERR("failed to alloc output_y pmem buffer:addr is null.\n");
		return -1;
	}

	/* alloc outout uv buffer */
	if (0 == s_mem_method) {
		rotation_cxt_ptr->output_uv_pmem_hp = new MemoryHeapIon("/dev/ion",
											rotation_cxt_ptr->width * rotation_cxt_ptr->height/ 2,
											MemoryHeapBase::NO_CACHING, ION_HEAP_ID_MASK_MM);
	} else {
		rotation_cxt_ptr->output_uv_pmem_hp = new MemoryHeapIon("/dev/ion",
										rotation_cxt_ptr->width * rotation_cxt_ptr->height/ 2,
										MemoryHeapBase::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
	}
	if (rotation_cxt_ptr->output_uv_pmem_hp->getHeapID() < 0) {
		ERR("failed to alloc output_uv pmem buffer.\n");
		return -1;
	}

	if (0 == s_mem_method) {
		rotation_cxt_ptr->output_uv_pmem_hp->get_phy_addr_from_ion((int *)(&rotation_cxt_ptr->output_uv_physical_addr),
			(int *)(&rotation_cxt_ptr->output_uv_pmemory_size));
	} else {
		rotation_cxt_ptr->output_uv_pmem_hp->get_mm_iova((int *)(&rotation_cxt_ptr->output_uv_physical_addr),
		(int *)(&rotation_cxt_ptr->output_uv_pmemory_size));
	}
	rotation_cxt_ptr->output_uv_virtual_addr = (unsigned char*)rotation_cxt_ptr->output_uv_pmem_hp->base();
	if (!rotation_cxt_ptr->output_uv_physical_addr) {
		ERR("failed to alloc output_uv pmem buffer:addr is null.\n");
		return -1;
	}
//	INFO("utest physical addr 0x%x  0x%x  0x%x  0x%x\n",rotation_cxt_ptr->input_y_physical_addr,rotation_cxt_ptr->input_uv_physical_addr,rotation_cxt_ptr->output_y_physical_addr,rotation_cxt_ptr->output_uv_physical_addr);

	return 0;
}

static int utest_rotation_mem_release(struct utest_rotation_context *g_utest_rotation_cxt_ptr)
{
	struct utest_rotation_context *rotation_cxt_ptr = g_utest_rotation_cxt_ptr;
	int s_mem_method = utest_mm_iommu_is_enabled();

	INFO("utest_rotation_mem_release %d\n",s_mem_method);
	if (rotation_cxt_ptr->input_y_physical_addr) {
		if (0 == s_mem_method) {
		rotation_cxt_ptr->input_y_pmem_hp.clear();
		} else {
		rotation_cxt_ptr->input_y_pmem_hp->free_mm_iova(rotation_cxt_ptr->input_y_physical_addr,rotation_cxt_ptr->input_y_pmemory_size);
		}
	}

	if (rotation_cxt_ptr->input_uv_physical_addr) {
		if (0 == s_mem_method) {
		rotation_cxt_ptr->input_uv_pmem_hp.clear();
		} else {
		rotation_cxt_ptr->input_uv_pmem_hp->free_mm_iova(rotation_cxt_ptr->input_uv_physical_addr,rotation_cxt_ptr->input_uv_pmemory_size);
		}
	}

	if (rotation_cxt_ptr->output_y_physical_addr) {
		if (0 == s_mem_method) {
		rotation_cxt_ptr->output_y_pmem_hp.clear();
		} else {
		rotation_cxt_ptr->output_y_pmem_hp->free_mm_iova(rotation_cxt_ptr->output_y_physical_addr,rotation_cxt_ptr->output_y_pmemory_size);
		}
	}

	if (rotation_cxt_ptr->output_uv_physical_addr) {
		if (0 == s_mem_method) {
		rotation_cxt_ptr->output_uv_pmem_hp.clear();
		} else {
		rotation_cxt_ptr->output_uv_pmem_hp->free_mm_iova(rotation_cxt_ptr->output_uv_physical_addr,rotation_cxt_ptr->output_uv_pmemory_size);
		}
	}
	return 0;
}

static int utest_rotation_src_cfg(struct utest_rotation_context *g_utest_rotation_cxt_ptr)
{
	FILE* fp = 0;
	struct utest_rotation_context *rotation_cxt_ptr = g_utest_rotation_cxt_ptr;

	if(rotation_cxt_ptr->filenum == ONE_FILE)
	{
		/* get input_yuv src */
		fp = fopen(utest_rotation_src_yuv_file, "r");
		if (fp != NULL) {
			fread((void *)rotation_cxt_ptr->input_y_virtual_addr, 1,
				rotation_cxt_ptr->width * rotation_cxt_ptr->height, fp);
			fread((void *)rotation_cxt_ptr->input_uv_virtual_addr, 1,
				rotation_cxt_ptr->width * rotation_cxt_ptr->height / 2, fp);
			fclose(fp);
		} else {
			ERR("utest_rotation_src_cfg fail : no input_yuv source file.\n");
			return -1;
		}
	}
	else if(rotation_cxt_ptr->filenum == TWO_FLIE)
	{
		/* get input_y src */
		fp = fopen(utest_rotation_src_y_file, "r");
		if (fp != NULL) {
			fread((void *)rotation_cxt_ptr->input_y_virtual_addr, 1,
				rotation_cxt_ptr->width * rotation_cxt_ptr->height, fp);
			fclose(fp);
		} else {
			ERR("utest_rotation_src_cfg fail : no input_y source file.\n");
			return -1;
		}

		/* get input_uv src */
		fp = fopen(utest_rotation_src_uv_file, "r");
		if (fp != NULL) {
			fread((void *)rotation_cxt_ptr->input_uv_virtual_addr, 1,
				rotation_cxt_ptr->width * rotation_cxt_ptr->height / 2, fp);
			fclose(fp);
		} else {
			ERR("utest_rotation_src_cfg fail : no input_uv source file.\n");
			return -1;
		}
	}
	else
	{
		ERR("utest_rotation_src_cfg fail :  file number is wrong.\n");
	}
	return 0;
}

static int utest_rotation_save_raw_data(struct utest_rotation_context *g_utest_rotation_cxt_ptr,int angle)
{
#if SAVE_ROTATION_OUTPUT_DATA
	FILE* fp = 0;
	char file_name[128] = "utest_rotation_output_temp.raw";
	struct utest_rotation_context *rotation_cxt_ptr = g_utest_rotation_cxt_ptr;

	sprintf(file_name, utest_rotation_dst_y_file, rotation_cxt_ptr->width,
			rotation_cxt_ptr->height,angle);
	fp = fopen(file_name, "wb");
	if (fp != NULL) {
		fwrite((void *)rotation_cxt_ptr->output_y_virtual_addr , 1,
				rotation_cxt_ptr->width * rotation_cxt_ptr->height, fp);
		fclose(fp);
	} else {
		ERR("utest_rotation_save_raw_data: failed to open save_file_y.\n");
		return -1;
	}

	sprintf(file_name, utest_rotation_dst_uv_file, rotation_cxt_ptr->width,
			rotation_cxt_ptr->height,angle);
	fp = fopen(file_name, "wb");
	if (fp != NULL) {
		fwrite((void *)rotation_cxt_ptr->output_uv_virtual_addr , 1,
				rotation_cxt_ptr->width * rotation_cxt_ptr->height / 2, fp);
		fclose(fp);
	} else {
		ERR("utest_rotation_save_raw_data: failed to open save_file_uv.\n");
		return -1;
	}
#endif
	return 0;
}

int main(int argc, char **argv)
{
	int fd = 0;
	int32_t i =0;
	int64_t time_start = 0, time_end = 0;
	static struct utest_rotation_context utest_rotation_cxt;
	static struct utest_rotation_context *g_utest_rotation_cxt_ptr = &utest_rotation_cxt;
	struct utest_rotation_context *rotation_cxt_ptr = g_utest_rotation_cxt_ptr;
	enum img_rot_angle tmp_angle;

	if (utest_rotation_param_set(&utest_rotation_cxt,argc, argv))
		return -1;

	if (utest_rotation_mem_alloc(&utest_rotation_cxt))
		goto err;


	if (utest_rotation_src_cfg(&utest_rotation_cxt))
		goto err;

		for (i = 0; i < UTEST_ROTATION_COUNTER; i++) {

			fd = open( "/dev/sprd_rotation", O_RDONLY, 0);

			INFO("utest_rotation testing  start\n");
			time_start = systemTime();

			tmp_angle = utest_rotation_angle_cvt(rotation_cxt_ptr->angle);
			utest_do_rotation(fd, rotation_cxt_ptr->width, rotation_cxt_ptr->height,
							rotation_cxt_ptr->input_y_physical_addr,rotation_cxt_ptr->input_uv_physical_addr,
							rotation_cxt_ptr->output_y_physical_addr, rotation_cxt_ptr->output_uv_physical_addr,
							ROT_YUV420,tmp_angle);

			time_end = systemTime();
			INFO("utest_rotation testing  end time=%d, i = %x\n", (unsigned int)((time_end-time_start) / 1000000L), i);
			//usleep(30*1000);
			utest_rotation_save_raw_data(&utest_rotation_cxt,rotation_cxt_ptr->angle);

			close(fd);

		}

err:
	utest_rotation_mem_release(&utest_rotation_cxt);

	return 0;
}
