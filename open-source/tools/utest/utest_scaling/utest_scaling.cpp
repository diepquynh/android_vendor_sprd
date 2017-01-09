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
#include "cmr_common.h"
#include <semaphore.h>
#include <linux/ion.h>
#include "ion_sprd.h"
#include <binder/MemoryHeapIon.h>
#include <linux/types.h>
#include <asm/ioctl.h>
#include "sprd_scale_k.h"

using namespace android;

#define UTEST_SCALING_COUNTER 0xFFFFFFFF
#define SAVE_SCALING_OUTPUT_DATA 0
#define ERR(x...) fprintf(stderr, x)
#define INFO(x...) fprintf(stdout, x)

#define UTEST_SCALING_EXIT_IF_ERR(n)                      \
	do {                                                                 \
		if (n) {                                           \
			ERR("utest scale  error  . Line:%d ", __LINE__);\
			goto exit;                   \
		}                                                    \
	} while(0)

typedef enum {
	HW_SCALE_DATA_YUV422 = 0,
	HW_SCALE_DATA_YUV420,
	HW_SCALE_DATA_YUV400,
	HW_SCALE_DATA_YUV420_3FRAME,
	HW_SCALE_DATA_RGB565,
	HW_SCALE_DATA_RGB888,
	HW_SCALE_DATA_FMT_MAX
}HW_SCALE_DATA_FORMAT_E;

struct utest_scaling_context {
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

	uint32_t input_width;
	uint32_t input_height;
	uint32_t output_width;
	uint32_t output_height;
	uint32_t cnt;
};

static char utest_scaling_src_y_file[] = "/data/utest_scaling_src_y.raw";
static char utest_scaling_src_uv_file[] = "/data/utest_scaling_src_uv.raw";
static char utest_scaling_dst_y_file[] = "/data/utest_scaling_dst_y_%dx%d.raw";
static char utest_scaling_dst_uv_file[] = "/data/utest_scaling_dst_uv_%dx%d.raw";

static struct utest_scaling_context utest_scaling_cxt;
static struct utest_scaling_context *g_utest_scaling_cxt_ptr = &utest_scaling_cxt;

static int utest_do_scaling(int fd,
			uint32_t output_width, uint32_t output_height,
			uint32_t output_yaddr,uint32_t output_uvaddr,
			HW_SCALE_DATA_FORMAT_E input_fmt,
			uint32_t input_width,uint32_t input_height,
			uint32_t input_yaddr, uint32_t intput_uvaddr)
{
	int ret = 0;
	struct img_frm src_img, dst_img;
	struct img_rect src_rect;

//	struct scale_frame scale_frm;

	struct scale_frame_param_t frame_params;

	memset((void*)&frame_params, 0x00, sizeof(struct scale_frame_param_t));

	//input params
	frame_params.input_size.w = input_width;
	frame_params.input_size.h = input_height;
	frame_params.input_rect.x = 0;
	frame_params.input_rect.y = 0;
	frame_params.input_rect.w = input_width;
	frame_params.input_rect.h = input_height;
	frame_params.input_format = (enum scale_fmt_e)input_fmt;
	frame_params.input_addr.yaddr = input_yaddr;
	frame_params.input_addr.uaddr = intput_uvaddr;
	frame_params.input_addr.vaddr = intput_uvaddr;
	frame_params.input_endian.y_endian = 1;
	frame_params.input_endian.uv_endian = 1;

	//output params
	frame_params.output_size.w = output_width;
	frame_params.output_size.h = output_height;
	frame_params.output_format = (enum scale_fmt_e)HW_SCALE_DATA_YUV420;
	frame_params.output_addr.yaddr = output_yaddr;
	frame_params.output_addr.uaddr = output_uvaddr;
	frame_params.output_addr.vaddr = output_uvaddr;
	frame_params.output_endian.y_endian = 1;
	frame_params.output_endian.uv_endian = 1;

	frame_params.scale_mode = SCALE_MODE_NORMAL;

	ret = ioctl(fd, SCALE_IO_START, &frame_params);

	if (ret) {
		ERR("utes_scaling camera_scaling fail. Line:%d", __LINE__);
		ret = -1;
	} else {
		ret = 0;
	}

	return ret;
}

static void usage(void)
{
	INFO("Usage:\n");
	INFO("utest_scaling -iw input_width -ih input_height -ow out_width -oh out_height -cnt count\n");
}

static int utest_scaling_param_set(int argc, char **argv)
{
	int i = 0;
	struct utest_scaling_context *scaling_cxt_ptr = g_utest_scaling_cxt_ptr;

	if (argc < 9) {
		usage();
		return -1;
	}

	memset(scaling_cxt_ptr, 0, sizeof(utest_scaling_context));

	for (i=1; i<argc; i++) {
		if (strcmp(argv[i], "-iw") == 0 && (i < argc-1)) {
			scaling_cxt_ptr->input_width = atoi(argv[++i]);
		} else if (strcmp(argv[i], "-ih") == 0 && (i < argc-1)) {
			scaling_cxt_ptr->input_height = atoi(argv[++i]);
		} else if (strcmp(argv[i], "-ow") == 0 && (i < argc-1)) {
			scaling_cxt_ptr->output_width = atoi(argv[++i]);
		} else if (strcmp(argv[i], "-oh") == 0 && (i < argc-1)) {
			scaling_cxt_ptr->output_height = atoi(argv[++i]);
		} else if (strcmp(argv[i], "-cnt") == 0 && (i < argc-1)) {
			scaling_cxt_ptr->cnt = atoi(argv[++i]);
		} else {
			usage();
			return -1;
		}
	}

	if (((scaling_cxt_ptr->input_width << 2) < scaling_cxt_ptr->output_width) ||
		((scaling_cxt_ptr->input_height << 2) < scaling_cxt_ptr->output_height) ||
		((scaling_cxt_ptr->input_width >> 6) > scaling_cxt_ptr->output_width) ||
		((scaling_cxt_ptr->input_height >> 6) > scaling_cxt_ptr->output_height)) {
		ERR("errro: out of Scaling range.\n");
		return -1;
	}

	return 0;
}

static int utest_mm_iommu_is_enabled(void)
{
	return MemoryHeapIon::Mm_iommu_is_enabled();
}

static int utest_scaling_mem_alloc(void)
{
	struct utest_scaling_context *scaling_cxt_ptr = g_utest_scaling_cxt_ptr;
	int mem_method = utest_mm_iommu_is_enabled();

//	INFO("utest_scaling_mem_alloc %d\n",mem_method);
	/* alloc input y buffer */
	if (0 == mem_method ) {
		scaling_cxt_ptr->input_y_pmem_hp = new MemoryHeapIon("/dev/ion",
										scaling_cxt_ptr->input_width * scaling_cxt_ptr->input_height,
										MemoryHeapBase::NO_CACHING, ION_HEAP_CARVEOUT_MASK);
	} else {
		scaling_cxt_ptr->input_y_pmem_hp = new MemoryHeapIon("/dev/ion",
										scaling_cxt_ptr->input_width * scaling_cxt_ptr->input_height,
										MemoryHeapBase::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
	}

	if (scaling_cxt_ptr->input_y_pmem_hp->getHeapID() < 0) {
		ERR("failed to alloc input_y pmem buffer.\n");
		return -1;
	}

	if ( 0 == mem_method ) {
		scaling_cxt_ptr->input_y_pmem_hp->get_phy_addr_from_ion((int *)(&scaling_cxt_ptr->input_y_physical_addr),
			(int *)(&scaling_cxt_ptr->input_y_pmemory_size));
	} else {
		scaling_cxt_ptr->input_y_pmem_hp->get_mm_iova((int *)(&scaling_cxt_ptr->input_y_physical_addr),
		(int *)(&scaling_cxt_ptr->input_y_pmemory_size));
	}

	scaling_cxt_ptr->input_y_virtual_addr = (unsigned char*)scaling_cxt_ptr->input_y_pmem_hp->base();
	if (!scaling_cxt_ptr->input_y_physical_addr) {
		ERR("failed to alloc input_y pmem buffer:addr is null.\n");
		return -1;
	}
	memset(scaling_cxt_ptr->input_y_virtual_addr, 0x80, scaling_cxt_ptr->input_width * scaling_cxt_ptr->input_height);

	/* alloc input uv buffer */
	if (0 == mem_method ) {
		scaling_cxt_ptr->input_uv_pmem_hp = new MemoryHeapIon("/dev/ion",
											scaling_cxt_ptr->input_width * scaling_cxt_ptr->input_height / 2,
											MemoryHeapBase::NO_CACHING, ION_HEAP_CARVEOUT_MASK);
	} else {
		scaling_cxt_ptr->input_uv_pmem_hp = new MemoryHeapIon("/dev/ion",
										scaling_cxt_ptr->input_width * scaling_cxt_ptr->input_height / 2,
										MemoryHeapBase::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
	}

	if (scaling_cxt_ptr->input_uv_pmem_hp->getHeapID() < 0) {
		ERR("failed to alloc input_uv pmem buffer.\n");
		return -1;
	}

	if (0 == mem_method ) {
		scaling_cxt_ptr->input_uv_pmem_hp->get_phy_addr_from_ion((int *)(&scaling_cxt_ptr->input_uv_physical_addr),
				(int *)(&scaling_cxt_ptr->input_uv_pmemory_size));
	} else {
		scaling_cxt_ptr->input_uv_pmem_hp->get_mm_iova((int *)(&scaling_cxt_ptr->input_uv_physical_addr),
			(int *)(&scaling_cxt_ptr->input_uv_pmemory_size));
	}

	scaling_cxt_ptr->input_uv_virtual_addr = (unsigned char*)scaling_cxt_ptr->input_uv_pmem_hp->base();
	if (!scaling_cxt_ptr->input_uv_physical_addr) {
		ERR("failed to alloc input_uv pmem buffer:addr is null.\n");
		return -1;
	}
	memset(scaling_cxt_ptr->input_uv_virtual_addr, 0x80, scaling_cxt_ptr->input_width * scaling_cxt_ptr->input_height / 2);

	/* alloc outout y buffer */
	if ( 0 == mem_method ) {
		scaling_cxt_ptr->output_y_pmem_hp = new MemoryHeapIon("/dev/ion",
											scaling_cxt_ptr->output_width * scaling_cxt_ptr->output_height,
											MemoryHeapBase::NO_CACHING, ION_HEAP_CARVEOUT_MASK);
	} else {
		scaling_cxt_ptr->output_y_pmem_hp = new MemoryHeapIon("/dev/ion",
										scaling_cxt_ptr->output_width * scaling_cxt_ptr->output_height,
										MemoryHeapBase::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
	}

	if (scaling_cxt_ptr->output_y_pmem_hp->getHeapID() < 0) {
		ERR("failed to alloc output_y pmem buffer.\n");
		return -1;
	}

	if ( 0 == mem_method ) {
		scaling_cxt_ptr->output_y_pmem_hp->get_phy_addr_from_ion((int *)(&scaling_cxt_ptr->output_y_physical_addr),
			(int *)(&scaling_cxt_ptr->output_y_pmemory_size));
	} else {
		scaling_cxt_ptr->output_y_pmem_hp->get_mm_iova((int *)(&scaling_cxt_ptr->output_y_physical_addr),
			(int *)(&scaling_cxt_ptr->output_y_pmemory_size));
	}

	scaling_cxt_ptr->output_y_virtual_addr = (unsigned char*)scaling_cxt_ptr->output_y_pmem_hp->base();
	if (!scaling_cxt_ptr->output_y_physical_addr) {
		ERR("failed to alloc output_y pmem buffer:addr is null.\n");
		return -1;
	}

	/* alloc outout uv buffer */
	if ( 0 == mem_method ) {
		scaling_cxt_ptr->output_uv_pmem_hp = new MemoryHeapIon("/dev/ion",
											scaling_cxt_ptr->output_width * scaling_cxt_ptr->output_height / 2,
											MemoryHeapBase::NO_CACHING, ION_HEAP_CARVEOUT_MASK);
	} else {
		scaling_cxt_ptr->output_uv_pmem_hp = new MemoryHeapIon("/dev/ion",
											scaling_cxt_ptr->output_width * scaling_cxt_ptr->output_height / 2,
											MemoryHeapBase::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
	}
	if (scaling_cxt_ptr->output_uv_pmem_hp->getHeapID() < 0) {
		ERR("failed to alloc output_uv pmem buffer.\n");
		return -1;
	}

	if ( 0 == mem_method ) {
		scaling_cxt_ptr->output_uv_pmem_hp->get_phy_addr_from_ion((int *)(&scaling_cxt_ptr->output_uv_physical_addr),
			(int *)(&scaling_cxt_ptr->output_uv_pmemory_size));
	} else {
		scaling_cxt_ptr->output_uv_pmem_hp->get_mm_iova((int *)(&scaling_cxt_ptr->output_uv_physical_addr),
			(int *)(&scaling_cxt_ptr->output_uv_pmemory_size));
	}

	scaling_cxt_ptr->output_uv_virtual_addr = (unsigned char*)scaling_cxt_ptr->output_uv_pmem_hp->base();
	if (!scaling_cxt_ptr->output_uv_physical_addr) {
		ERR("failed to alloc output_uv pmem buffer:addr is null.\n");
		return -1;
	}

	return 0;
}

static int utest_scaling_mem_release(void)
{
	struct utest_scaling_context *scaling_cxt_ptr = g_utest_scaling_cxt_ptr;
	int mem_method = utest_mm_iommu_is_enabled();

	if (scaling_cxt_ptr->input_y_physical_addr) {
		if( 0 == mem_method ) {
			scaling_cxt_ptr->input_y_pmem_hp.clear();
		} else {
			scaling_cxt_ptr->input_y_pmem_hp->free_mm_iova(scaling_cxt_ptr->input_y_physical_addr,
				scaling_cxt_ptr->input_y_pmemory_size);
		}
	}

	if (scaling_cxt_ptr->input_uv_physical_addr) {
		if ( 0 == mem_method ) {
			scaling_cxt_ptr->input_uv_pmem_hp.clear();
		} else {
			scaling_cxt_ptr->input_uv_pmem_hp->free_mm_iova(scaling_cxt_ptr->input_uv_physical_addr,
				scaling_cxt_ptr->input_uv_pmemory_size);
		}
	}

	if (scaling_cxt_ptr->output_y_physical_addr) {
		if ( 0 == mem_method ) {
			scaling_cxt_ptr->output_y_pmem_hp.clear();
		} else {
			scaling_cxt_ptr->output_y_pmem_hp->free_mm_iova(scaling_cxt_ptr->output_y_physical_addr,
				scaling_cxt_ptr->output_y_pmemory_size);
		}
	}

	if (scaling_cxt_ptr->output_uv_physical_addr) {
		if ( 0 == mem_method ) {
			scaling_cxt_ptr->output_uv_pmem_hp.clear();
		} else {
			scaling_cxt_ptr->output_uv_pmem_hp->free_mm_iova(scaling_cxt_ptr->output_uv_physical_addr,
				scaling_cxt_ptr->output_uv_pmemory_size);
		}
	}

	return 0;
}

static int utest_scaling_src_cfg(void)
{
	FILE* fp = 0;
	struct utest_scaling_context *scaling_cxt_ptr = g_utest_scaling_cxt_ptr;

	/* get input_y src */
	fp = fopen(utest_scaling_src_y_file, "r");
	if (fp != NULL) {
		fread((void *)scaling_cxt_ptr->input_y_virtual_addr, 1,
			scaling_cxt_ptr->input_width * scaling_cxt_ptr->input_height, fp);
		fclose(fp);
	} else {
		ERR("utest_scaling_src_cfg fail : no input_y source file.\n");
		return -1;
	}

	/* get input_uv src */
	fp = fopen(utest_scaling_src_uv_file, "r");
	if (fp != NULL) {
		fread((void *)scaling_cxt_ptr->input_uv_virtual_addr, 1,
			scaling_cxt_ptr->input_width * scaling_cxt_ptr->input_height / 2, fp);
		fclose(fp);
	} else {
		ERR("utest_scaling_src_cfg fail : no input_uv source file.\n");
		return -1;
	}

	return 0;
}

static int utest_scaling_save_raw_data(void)
{
#if SAVE_SCALING_OUTPUT_DATA
	FILE* fp = 0;
	char file_name[128] = "utest_scaling_output_temp.raw";
	struct utest_scaling_context *scaling_cxt_ptr = g_utest_scaling_cxt_ptr;

	sprintf(file_name, utest_scaling_dst_y_file, scaling_cxt_ptr->output_width,
			scaling_cxt_ptr->output_height);
	fp = fopen(file_name, "wb");
	if (fp != NULL) {
		fwrite((void *)scaling_cxt_ptr->output_y_virtual_addr , 1,
				scaling_cxt_ptr->output_width * scaling_cxt_ptr->output_height, fp);
		fclose(fp);
	} else {
		ERR("utest_scaling_save_raw_data: failed to open save_file_y.\n");
		return -1;
	}

	sprintf(file_name, utest_scaling_dst_uv_file, scaling_cxt_ptr->output_width,
			scaling_cxt_ptr->output_height);
	fp = fopen(file_name, "wb");
	if (fp != NULL) {
		fwrite((void *)scaling_cxt_ptr->output_uv_virtual_addr , 1,
				scaling_cxt_ptr->output_width * scaling_cxt_ptr->output_height / 2, fp);
		fclose(fp);
	} else {
		ERR("utest_scaling_save_raw_data: failed to open save_file_uv.\n");
		return -1;
	}
#endif
	return 0;
}

int main(int argc, char **argv)
{
	int fd = 0, i = 0;
	int64_t time_start = 0, time_end = 0;
	struct utest_scaling_context *scaling_cxt_ptr = g_utest_scaling_cxt_ptr;

	if (utest_scaling_param_set(argc, argv))
		return -1;

	if (utest_scaling_mem_alloc())
		goto err;


	if (utest_scaling_src_cfg())
		goto err;

		for (i = 0; i < UTEST_SCALING_COUNTER; i++) {

			fd = open("/dev/sprd_scale", O_RDONLY);

			INFO("utest_scaling testing  start\n");
			time_start = systemTime();

			utest_do_scaling(fd, scaling_cxt_ptr->output_width, scaling_cxt_ptr->output_height,
							scaling_cxt_ptr->output_y_physical_addr,scaling_cxt_ptr->output_uv_physical_addr,
							HW_SCALE_DATA_YUV420,scaling_cxt_ptr->input_width, scaling_cxt_ptr->input_height,
							scaling_cxt_ptr->input_y_physical_addr, scaling_cxt_ptr->input_uv_physical_addr);

			time_end = systemTime();
			INFO("utest_scaling testing  end time=%d, i = %x\n", (unsigned int)((time_end-time_start) / 1000000L),i);

			//usleep(10*1000);
			utest_scaling_save_raw_data();

			close(fd);
		}

err:

	utest_scaling_mem_release();

	return 0;
}
