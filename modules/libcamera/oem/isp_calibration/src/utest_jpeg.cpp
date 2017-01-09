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
#include <utils/Log.h>
#include <utils/String16.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <cutils/properties.h>
#include <linux/ion.h>
#include "MemoryHeapIon.h"
#include <semaphore.h>
#include "ion_sprd.h"
#include "jpegenc_api.h"

using namespace android;

#define ERR(x...) fprintf(stderr, ##x)
#define INFO(x...) fprintf(stdout, ##x)

#define UTEST_PARAM_NUM 9

struct utest_jpeg_context {
	uint32_t width;
	uint32_t height;

	sp<MemoryHeapIon> input_y_pmem_hp;
	size_t input_y_pmemory_size;
	unsigned long input_y_physical_addr ;
	unsigned char* input_y_virtual_addr;

	sp<MemoryHeapIon> input_uv_pmem_hp;
	size_t input_uv_pmemory_size;
	unsigned long input_uv_physical_addr ;
	unsigned char* input_uv_virtual_addr;

	sp<MemoryHeapIon> jpg_pmem_hp;
	size_t jpg_pmemory_size;
	unsigned long jpg_physical_addr ;
	unsigned char* jpg_virtual_addr;

	uint32_t quality_level;
	uint32_t number;
};

static char utest_jpeg_enc_src_y[] = "/data/misc/media/utest_src_y.raw";
static char utest_jpeg_enc_src_uv[] = "/data/misc/media/utest_src_uv.raw";
static char utest_jpg_enc_dst_file[] = "/data/misc/media/utest_jpg_file.jpg";

static struct utest_jpeg_context utest_jpeg_cxt;
static struct utest_jpeg_context *g_utest_jpeg_cxt_ptr = &utest_jpeg_cxt;

static void utest_jpeg_usage()
{
	INFO("utest_jpeg_usage:\n");
	INFO("utest_jpeg_enc -w width -h height -q quality -n number \n");
	INFO("-w	: picture  width\n");
	INFO("-h	: picture  height\n");
	INFO("-q      : quality of jpeg\n");
	INFO("-cnt    : number of jpeg encoding\n");
	INFO("-help	: show this help message\n");
}

static int utest_jpeg_mm_iommu_is_enabled(void)
{
	return MemoryHeapIon::IOMMU_is_enabled(ION_MM);
}

static JPEGENC_QUALITY_E utest_jpg_quality_covert(uint32_t quality)
{
	JPEGENC_QUALITY_E jq = JPEGENC_QUALITY_HIGH;

	if (quality <= 70) {
		jq = JPEGENC_QUALITY_LOW;
	} else if (quality <= 80) {
		jq = JPEGENC_QUALITY_MIDDLE_LOW;
	} else if (quality <= 85) {
		jq = JPEGENC_QUALITY_MIDDLE;
	} else if (quality <= 90) {
		jq = JPEGENC_QUALITY_MIDDLE_HIGH;
	} else {
		jq = JPEGENC_QUALITY_HIGH;
	}

	return jq;
}

static int utest_jpeg_param_set(int argc, char **argv)
{
	int i = 0;
	struct utest_jpeg_context *jpeg_cxt_ptr = g_utest_jpeg_cxt_ptr;

	if (argc < UTEST_PARAM_NUM) {
		utest_jpeg_usage();
		return -1;
	}

	memset(jpeg_cxt_ptr, 0, sizeof(utest_jpeg_context));

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-w") == 0 && (i < argc-1)) {
			jpeg_cxt_ptr->width = atoi(argv[++i]);
			if(0 >= jpeg_cxt_ptr->width) {
				ERR("the width of jpeg is invalid.\n");
				return -1;
			}
		} else if (strcmp(argv[i], "-h") == 0 && (i < argc-1)) {
			jpeg_cxt_ptr->height = atoi(argv[++i]);
			if(0 >= jpeg_cxt_ptr->height) {
				ERR("the height of jpeg is invalid.\n");
				return -1;
			}
		} else if (strcmp(argv[i], "-q") == 0 && (i < argc-1)) {
			jpeg_cxt_ptr->quality_level= atoi(argv[++i]);
		} else if (strcmp(argv[i], "-n") == 0 && (i < argc-1)) {
			jpeg_cxt_ptr->number= atoi(argv[++i]);
			if(0 >= jpeg_cxt_ptr->number) {
				ERR("the number of jpeg enc is invalid.\n");
				return -1;
			}
		} else if (strcmp(argv[i], "-help") == 0) {
			utest_jpeg_usage();
			return -1;
		} else {
			utest_jpeg_usage();
			return -1;
		}
	}

	return 0;
}

static int32_t utest_jpeg_save_jpg(void)
{
	int32_t rtn = 0;
	FILE* fp = 0;
	struct utest_jpeg_context *jpeg_cxt_ptr = g_utest_jpeg_cxt_ptr;

	fp = fopen(utest_jpg_enc_dst_file, "wb");

	if (fp != NULL) {
		fwrite(jpeg_cxt_ptr->jpg_virtual_addr , 1,
			jpeg_cxt_ptr->width * jpeg_cxt_ptr->height, fp);
		fclose(fp);
	} else {
		ERR("utest_jpeg_save_jpg: failed to open save_file.\n");
		rtn = -1;
	}

	return rtn;
}

static int utest_jpeg_mem_alloc(void)
{
	struct utest_jpeg_context *jpeg_cxt_ptr = g_utest_jpeg_cxt_ptr;

	int s_mem_method = utest_jpeg_mm_iommu_is_enabled();

	INFO("utest_jpeg_mem_alloc: method=%d\n",s_mem_method);

	/* alloc input y buffer */
	if (0 == s_mem_method) {
		jpeg_cxt_ptr->input_y_pmem_hp = new MemoryHeapIon("/dev/ion",
											jpeg_cxt_ptr->width * jpeg_cxt_ptr->height,
											MemoryHeapBase::NO_CACHING, ION_HEAP_ID_MASK_MM);
	} else {
		jpeg_cxt_ptr->input_y_pmem_hp = new MemoryHeapIon("/dev/ion",
										jpeg_cxt_ptr->width * jpeg_cxt_ptr->height,
										MemoryHeapBase::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
	}
	if (jpeg_cxt_ptr->input_y_pmem_hp->getHeapID() < 0) {
		ERR("failed to alloc input_y pmem buffer.\n");
		return -1;
	}

	if (0 == s_mem_method) {
		jpeg_cxt_ptr->input_y_pmem_hp->get_phy_addr_from_ion(&jpeg_cxt_ptr->input_y_physical_addr,
		&jpeg_cxt_ptr->input_y_pmemory_size);
	} else {
		jpeg_cxt_ptr->input_y_pmem_hp->get_iova(ION_MM, &jpeg_cxt_ptr->input_y_physical_addr,
		&jpeg_cxt_ptr->input_y_pmemory_size);
	}
	jpeg_cxt_ptr->input_y_virtual_addr = (unsigned char*)jpeg_cxt_ptr->input_y_pmem_hp->base();
	if (!jpeg_cxt_ptr->input_y_physical_addr) {
		ERR("failed to alloc input_y pmem buffer:addr is null.\n");
		return -1;
	}
	memset(jpeg_cxt_ptr->input_y_virtual_addr, 0x80, jpeg_cxt_ptr->width * jpeg_cxt_ptr->height);


	/* alloc input uv buffer */
	if (0 == s_mem_method) {
		jpeg_cxt_ptr->input_uv_pmem_hp = new MemoryHeapIon("/dev/ion",
											jpeg_cxt_ptr->width * jpeg_cxt_ptr->height / 2,
											MemoryHeapBase::NO_CACHING, ION_HEAP_ID_MASK_MM);
	} else {
		jpeg_cxt_ptr->input_uv_pmem_hp = new MemoryHeapIon("/dev/ion",
										jpeg_cxt_ptr->width * jpeg_cxt_ptr->height / 2,
										MemoryHeapBase::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
	}
	if (jpeg_cxt_ptr->input_uv_pmem_hp->getHeapID() < 0) {
		ERR("failed to alloc input_uv pmem buffer.\n");
		return -1;
	}

	if (0 == s_mem_method) {
		jpeg_cxt_ptr->input_uv_pmem_hp->get_phy_addr_from_ion(&jpeg_cxt_ptr->input_uv_physical_addr,
			&jpeg_cxt_ptr->input_uv_pmemory_size);
	} else {
		jpeg_cxt_ptr->input_uv_pmem_hp->get_iova(ION_MM, &jpeg_cxt_ptr->input_uv_physical_addr,
			&jpeg_cxt_ptr->input_uv_pmemory_size);
	}
	jpeg_cxt_ptr->input_uv_virtual_addr = (unsigned char*)jpeg_cxt_ptr->input_uv_pmem_hp->base();
	if (!jpeg_cxt_ptr->input_uv_physical_addr) {
		ERR("failed to alloc input_uv pmem buffer:addr is null.\n");
		return -1;
	}
	memset(jpeg_cxt_ptr->input_uv_virtual_addr, 0x80, jpeg_cxt_ptr->width * jpeg_cxt_ptr->height/ 2);


	/* alloc output buffer */
	if (0 == s_mem_method) {
		jpeg_cxt_ptr->jpg_pmem_hp = new MemoryHeapIon("/dev/ion",
											jpeg_cxt_ptr->width * jpeg_cxt_ptr->height,
											MemoryHeapBase::NO_CACHING, ION_HEAP_ID_MASK_MM);
	} else {
		jpeg_cxt_ptr->jpg_pmem_hp = new MemoryHeapIon("/dev/ion",
										jpeg_cxt_ptr->width * jpeg_cxt_ptr->height,
										MemoryHeapBase::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
	}
	if (jpeg_cxt_ptr->jpg_pmem_hp->getHeapID() < 0) {
		ERR("failed to alloc output_y pmem buffer.\n");
		return -1;
	}

	if (0 == s_mem_method) {
		jpeg_cxt_ptr->jpg_pmem_hp->get_phy_addr_from_ion(&jpeg_cxt_ptr->jpg_physical_addr,
			&jpeg_cxt_ptr->jpg_pmemory_size);
	} else {
		jpeg_cxt_ptr->jpg_pmem_hp->get_iova(ION_MM, &jpeg_cxt_ptr->jpg_physical_addr,
		&jpeg_cxt_ptr->jpg_pmemory_size);
	}
	jpeg_cxt_ptr->jpg_virtual_addr = (unsigned char*)jpeg_cxt_ptr->jpg_pmem_hp->base();
	if (!jpeg_cxt_ptr->jpg_physical_addr) {
		ERR("failed to alloc output_y pmem buffer:addr is null.\n");
		return -1;
	}

	INFO("utest physical addr 0x%x  0x%x  0x%x\n", jpeg_cxt_ptr->input_y_physical_addr, jpeg_cxt_ptr->input_uv_physical_addr, jpeg_cxt_ptr->jpg_physical_addr);

	return 0;
}


static int utest_jpg_mem_release(void)
{
	int s_mem_method = utest_jpeg_mm_iommu_is_enabled();
	struct utest_jpeg_context *jpeg_cxt_ptr = g_utest_jpeg_cxt_ptr;

	INFO("utest_jpg_mem_release: method=%d\n",s_mem_method);

	if (jpeg_cxt_ptr->input_y_physical_addr) {
		if (0 == s_mem_method) {
			jpeg_cxt_ptr->input_y_pmem_hp.clear();
		} else {
			jpeg_cxt_ptr->input_y_pmem_hp->free_iova(ION_MM, jpeg_cxt_ptr->input_y_physical_addr, jpeg_cxt_ptr->input_y_pmemory_size);
		}
	}

	if (jpeg_cxt_ptr->input_uv_physical_addr) {
		if (0 == s_mem_method) {
			jpeg_cxt_ptr->input_uv_pmem_hp.clear();
		} else {
			jpeg_cxt_ptr->input_uv_pmem_hp->free_iova(ION_MM, jpeg_cxt_ptr->input_uv_physical_addr, jpeg_cxt_ptr->input_uv_pmemory_size);
		}
	}

	if (jpeg_cxt_ptr->jpg_physical_addr) {
		if (0 == s_mem_method) {
			jpeg_cxt_ptr->jpg_pmem_hp.clear();
		} else {
			jpeg_cxt_ptr->jpg_pmem_hp->free_iova(ION_MM, jpeg_cxt_ptr->jpg_physical_addr, jpeg_cxt_ptr->jpg_pmemory_size);
		}
	}

	return 0;
}


static int utest_jpg_src_cfg(void)
{
	FILE* fp = 0;
	struct utest_jpeg_context *jpeg_cxt_ptr = g_utest_jpeg_cxt_ptr;

	/* get input_y src */
	fp = fopen(utest_jpeg_enc_src_y, "r");
	if (fp != NULL) {
		fread((void *)jpeg_cxt_ptr->input_y_virtual_addr, 1,
			jpeg_cxt_ptr->width * jpeg_cxt_ptr->height, fp);
		fclose(fp);
	} else {
		ERR("utest_jpg_src_cfg fail : no input_y source file.\n");
		return -1;
	}

	/* get input_uv src */
	fp = fopen(utest_jpeg_enc_src_uv, "r");
	if (fp != NULL) {
		fread((void *)jpeg_cxt_ptr->input_uv_virtual_addr, 1,
			jpeg_cxt_ptr->width * jpeg_cxt_ptr->height / 2, fp);
		fclose(fp);
	} else {
		ERR("utest_jpg_src_cfg fail : no input_uv source file.\n");
		return -1;
	}

	return 0;
}

static void utest_jpg_start(void)
{
	uint32_t i = 0;
	int64_t time_start = 0, time_end = 0;
	JPEGENC_PARAMS_T jenc_parm_ptr;
	JPEGENC_SLICE_OUT_T slice_out;
	struct utest_jpeg_context *jpeg_cxt_ptr = g_utest_jpeg_cxt_ptr;

	memset((void*)&slice_out,0,sizeof(JPEGENC_SLICE_OUT_T));
	jenc_parm_ptr.set_slice_height = 4096;
	jenc_parm_ptr.format = JPEGENC_YUV_420;
	jenc_parm_ptr.quality = utest_jpg_quality_covert(jpeg_cxt_ptr->quality_level);
	jenc_parm_ptr.width = jpeg_cxt_ptr->width;
	jenc_parm_ptr.height = jpeg_cxt_ptr->height;
	jenc_parm_ptr.yuv_virt_buf = jpeg_cxt_ptr->input_y_virtual_addr;
	jenc_parm_ptr.yuv_phy_buf = jpeg_cxt_ptr->input_y_physical_addr;
	jenc_parm_ptr.yuv_u_virt_buf = jpeg_cxt_ptr->input_uv_virtual_addr;
	jenc_parm_ptr.yuv_u_phy_buf = jpeg_cxt_ptr->input_uv_physical_addr;
	jenc_parm_ptr.yuv_v_virt_buf = (void*)0;
	jenc_parm_ptr.yuv_v_phy_buf = 0;
	jenc_parm_ptr.stream_virt_buf[0] = jpeg_cxt_ptr->jpg_virtual_addr;
	jenc_parm_ptr.stream_phy_buf[0] = jpeg_cxt_ptr->jpg_physical_addr;
	jenc_parm_ptr.stream_buf_len = jpeg_cxt_ptr->width * jpeg_cxt_ptr->height;
	jenc_parm_ptr.stream_size = 0;

	for (i = 0; i < jpeg_cxt_ptr->number; i++) {
		time_start = systemTime();

		JPEGENC_Slice_Start(&jenc_parm_ptr, &slice_out);

		time_end = systemTime();

		INFO("utest_jpg index=%d, time=%d\n", i, (unsigned int)((time_end-time_start) / 1000000L));
	}

	utest_jpeg_save_jpg();

}

int main(int argc, char **argv)
{
	int32_t rtn = 0, i=0;

	if (utest_jpeg_param_set(argc, argv))
		return -1;

	if (0 ==utest_jpeg_mem_alloc()) {
		if (0 == utest_jpg_src_cfg()) {
			utest_jpg_start();
		}
	}

	utest_jpg_mem_release();

	return rtn;
}
