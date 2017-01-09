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
#include <camera/Camera.h>
#include <semaphore.h>
#include "cmr_oem.h"
#include "SprdOEMCamera.h"
#include "isp_cali_interface.h"
#include "sensor_drv_u.h"
#include "ion_sprd.h"

using namespace android;

#define ERR(x...) fprintf(stderr, ##x)
#define INFO(x...) fprintf(stdout, ##x)

#define UTEST_PARAM_NUM 11
#define UTEST_PREVIEW_BUF_NUM 12
#define UTEST_PREVIEW_WIDTH 640
#define UTEST_PREVIEW_HEIGHT 480
#define UTEST_MAX_MISCHEAP_NUM 10

enum utest_sensor_id {
	UTEST_SENSOR_MAIN = 0,
	UTEST_SENSOR_SUB,
	UTEST_SENSOR_ID_MAX
};

enum utest_calibration_cmd_id {
	UTEST_CALIBRATION_AWB= 0,
	UTEST_CALIBRATION_LSC,
	UTEST_CALIBRATION_FLASHLIGHT,
	UTEST_CALIBRATION_CAP_JPG,
	UTEST_CALIBRATION_MAX
};

enum utest_cali_sub_cmd_id {
	UTEST_CALI_SUB_CMD_GOLDEN= 0,
	UTEST_CALI_SUB_CMD_RANDOM,
	UTEST_CALI_SUB_CMD_MAX,
};

struct utest_cmr_context {
	uint32_t sensor_id;
	uint32_t cmd;
	uint32_t sub_cmd;
	uintptr_t capture_raw_vir_addr;
	uint32_t capture_width;
	uint32_t capture_height;

	sp<MemoryHeapIon> cap_pmem_hp;
	uint32_t cap_pmemory_size;
	int cap_physical_addr ;
	unsigned char* cap_virtual_addr;

	sp<MemoryHeapIon> misc_heap_array[UTEST_MAX_MISCHEAP_NUM];
	uint32_t misc_heap_num;

	sp<MemoryHeapIon> preview_pmem_hp[UTEST_PREVIEW_BUF_NUM];
	unsigned long preview_pmemory_size[UTEST_PREVIEW_BUF_NUM];
	unsigned long preview_physical_addr[UTEST_PREVIEW_BUF_NUM];
	unsigned char* preview_virtual_addr[UTEST_PREVIEW_BUF_NUM];

	sem_t sem_cap_raw_done;
	sem_t sem_cap_jpg_done;

	void * jpg_buffer;
	uint32_t jpg_size;
};

static char calibration_awb_file[] = "/data/sensor_%s_awb_%s.dat";
static char calibration_flashlight_file[] = "/data/sensor_%s_flashlight_%s.dat";
static char calibration_lsc_file[] = "/data/sensor_%s_lnc_%d_%d_%d_%s.dat";
static char calibration_raw_data_file[] = "/data/sensor_%s_raw_%d_%d_%s.raw";
static char calibration_cap_jpg_file[] = "/data/sensor_cali_cap_jpg.jpg";

static struct utest_cmr_context utest_cmr_cxt;
static struct utest_cmr_context *g_utest_cmr_cxt_ptr = &utest_cmr_cxt;

static void utest_dcam_usage()
{
	INFO("utest_dcam_usage:\n");
	INFO("utest_camera -maincmd cali_main_cmd -subcmd cali_sub_cmd -id sensor_id -w capture_width -h capture_height \n");
	INFO("-cmd	: calibration command\n");
	INFO("-id	: select sensor id(0: main sensor / 1: sub sensor)\n");
	INFO("-w	: captured picture size width\n");
	INFO("-h	: captured picture size width\n");
	INFO("-help	: show this help message\n");
}

static int utest_dcam_param_set(int argc, char **argv)
{
	int i = 0;
	struct utest_cmr_context *cmr_cxt_ptr = g_utest_cmr_cxt_ptr;

	if (argc < UTEST_PARAM_NUM) {
		utest_dcam_usage();
		return -1;
	}

	memset(cmr_cxt_ptr, 0, sizeof(utest_cmr_context));

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-maincmd") == 0 && (i < argc-1)) {
			cmr_cxt_ptr->cmd = atoi(argv[++i]);
			if (UTEST_CALIBRATION_MAX <= cmr_cxt_ptr->cmd) {
				ERR("the calibration command is invalid.\n");
				return -1;
			}
		} else if (strcmp(argv[i], "-subcmd") == 0 && (i < argc-1)) {
			cmr_cxt_ptr->sub_cmd= atoi(argv[++i]);
			if (UTEST_CALI_SUB_CMD_MAX <= cmr_cxt_ptr->sub_cmd) {
				ERR("the calibration sub-command is invalid.\n");
				return -1;
			}
		}else if (strcmp(argv[i], "-id") == 0 && (i < argc-1)) {
			cmr_cxt_ptr->sensor_id = atoi(argv[++i]);
			if (UTEST_SENSOR_SUB < cmr_cxt_ptr->sensor_id) {
				ERR("the sensor id must be 0: main_sesor 1: sub_sensor.\n");
				return -1;
			}
		} else if (strcmp(argv[i], "-w") == 0 && (i < argc-1)) {
			cmr_cxt_ptr->capture_width = atoi(argv[++i]);
			if((0 >= cmr_cxt_ptr->capture_width) ||
				(cmr_cxt_ptr->capture_width % 2)) {
				ERR("the width of captured picture is invalid.\n");
				return -1;
			}
		} else if (strcmp(argv[i], "-h") == 0 && (i < argc-1)) {
			cmr_cxt_ptr->capture_height = atoi(argv[++i]);
			if((0 >= cmr_cxt_ptr->capture_height) ||
				(cmr_cxt_ptr->capture_height % 2)) {
				ERR("the height of captured picture is invalid.\n");
				return -1;
			}
		} else if (strcmp(argv[i], "-help") == 0) {
			utest_dcam_usage();
			return -1;
		} else {
			utest_dcam_usage();
			return -1;
		}
	}

	if (sem_init(&(cmr_cxt_ptr->sem_cap_raw_done), 0, 0))
		return -1;

	if (sem_init(&(cmr_cxt_ptr->sem_cap_jpg_done), 0, 0))
		return -1;

	return Sensor_set_calibration(1);
}

static void utest_dcam_wait_isp_ae_stab(void)
{
	struct camera_context *cxt = camera_get_cxt();

	camera_isp_ae_stab_set(1);

	sem_wait(&cxt->cmr_set.isp_ae_stab_sem);
}

static void utest_dcam_preview_mem_release(void)
{
	uint32_t i =0;
	struct utest_cmr_context *cmr_cxt_ptr = g_utest_cmr_cxt_ptr;

	for (i = 0; i < UTEST_PREVIEW_BUF_NUM; i++) {
		if (cmr_cxt_ptr->preview_physical_addr[i])
			cmr_cxt_ptr->preview_pmem_hp[i].clear();
	}
}

static int utest_callback_cap_mem_alloc(void* handle, unsigned int size, unsigned long *addr_phy, unsigned long *addr_vir)
{
	utest_cmr_context* camera = g_utest_cmr_cxt_ptr;
	if (camera == NULL) {
		return -1;
	}

	if (camera->misc_heap_num >= UTEST_MAX_MISCHEAP_NUM) {
		return -1;
	}

	sp<MemoryHeapIon> pHeapIon = new MemoryHeapIon("/dev/ion", size, MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_MM);
	if (pHeapIon == NULL) {
		return -1;
	}
	if (pHeapIon->getHeapID() < 0) {
		return -1;
	}

	pHeapIon->get_phy_addr_from_ion(addr_phy, &size);
	*addr_vir = (unsigned long)(pHeapIon->getBase());
	camera->misc_heap_array[camera->misc_heap_num++] = pHeapIon;

	return 0;
}

static int utest_callback_cap_mem_release(void* handle)
{
	utest_cmr_context* camera = g_utest_cmr_cxt_ptr;
	if (camera == NULL) {
		return -1;
	}

	uint32_t i;
	for (i=0; i<camera->misc_heap_num; i++) {
		sp<MemoryHeapIon> pHeapIon = camera->misc_heap_array[i];
		if (pHeapIon != NULL) {
			pHeapIon.clear();
		}
		camera->misc_heap_array[i] = NULL;
	}
	camera->misc_heap_num = 0;

	return 0;
}

static int utest_dcam_preview_mem_alloc(void)
{
	uint32_t i =0;
	uint32_t buf_size =0;

	struct utest_cmr_context *cmr_cxt_ptr = g_utest_cmr_cxt_ptr;

	buf_size = (UTEST_PREVIEW_WIDTH * UTEST_PREVIEW_HEIGHT * 3) >> 1;
	buf_size = camera_get_size_align_page(buf_size);

	for (i = 0; i < UTEST_PREVIEW_BUF_NUM; i++) {
		cmr_cxt_ptr->preview_pmem_hp[i] = new MemoryHeapIon("/dev/ion", buf_size,
			MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_MM);
		if (cmr_cxt_ptr->preview_pmem_hp[i]->getHeapID() < 0) {
			ERR("failed to alloc preview pmem buffer.\n");
			return -1;
		}
		cmr_cxt_ptr->preview_pmem_hp[i]->get_phy_addr_from_ion((int *)(&cmr_cxt_ptr->preview_physical_addr[i]),
			(int *)(&cmr_cxt_ptr->preview_pmemory_size[i]));
		cmr_cxt_ptr->preview_virtual_addr[i] = (unsigned char*)cmr_cxt_ptr->preview_pmem_hp[i]->getBase();
		if (!cmr_cxt_ptr->preview_physical_addr[i]) {
			ERR("failed to alloc preview pmem buffer:addr is null.\n");
			return -1;
		}
	}

	if (camera_set_preview_mem(cmr_cxt_ptr->preview_physical_addr,
								cmr_cxt_ptr->preview_virtual_addr,
								buf_size,
								UTEST_PREVIEW_BUF_NUM))
		return -1;

	return 0;
}

static void utest_dcam_cap_memory_release(void)
{
	struct utest_cmr_context *cmr_cxt_ptr = g_utest_cmr_cxt_ptr;

	if (cmr_cxt_ptr->cap_physical_addr)
		cmr_cxt_ptr->cap_pmem_hp.clear();

}

static int utest_dcam_cap_memory_alloc(void)
{
	uint32_t local_width, local_height;
	uint32_t mem_size;
	uint32_t buffer_size = 0;
	struct utest_cmr_context *cmr_cxt_ptr = g_utest_cmr_cxt_ptr;

	if (camera_capture_max_img_size(&local_width, &local_height))
		return -1;

	if (camera_capture_get_buffer_size(cmr_cxt_ptr->sensor_id, local_width,
		local_height, &mem_size))
		return -1;

	buffer_size = camera_get_size_align_page(mem_size);
	cmr_cxt_ptr->cap_pmem_hp = new MemoryHeapIon("/dev/ion", buffer_size,
		MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_MM);
	if (cmr_cxt_ptr->cap_pmem_hp->getHeapID() < 0) {
		ERR("failed to alloc capture pmem buffer.\n");
		return -1;
	}
	cmr_cxt_ptr->cap_pmem_hp->get_phy_addr_from_ion((int *)(&cmr_cxt_ptr->cap_physical_addr),
		(int *)(&cmr_cxt_ptr->cap_pmemory_size));
	cmr_cxt_ptr->cap_virtual_addr = (unsigned char*)cmr_cxt_ptr->cap_pmem_hp->getBase();
	if (!cmr_cxt_ptr->cap_physical_addr) {
		ERR("failed to alloc capture pmem buffer:addr is null.\n");
		return -1;
	}

	if (camera_set_capture_mem(0,
		(uint32_t)cmr_cxt_ptr->cap_physical_addr,
		(uint32_t)cmr_cxt_ptr->cap_virtual_addr,
		(uint32_t)cmr_cxt_ptr->cap_pmemory_size,
		(uint32_t)utest_callback_cap_mem_alloc,
		(uint32_t)utest_callback_cap_mem_release,
		0)) {
			utest_dcam_cap_memory_release();
			return -1;
	}

	return 0;
}

static void utest_dcam_close(void)
{
	Sensor_set_calibration(0);
	utest_dcam_preview_mem_release();
	utest_dcam_cap_memory_release();
	camera_stop(NULL, NULL);
}

static int32_t utest_dcam_save_raw_data(void)
{
	int32_t rtn = 0;
	struct utest_cmr_context *cmr_cxt_ptr = g_utest_cmr_cxt_ptr;
	SENSOR_EXP_INFO_T *sensor_ptr = (SENSOR_EXP_INFO_T*)Sensor_GetInfo();
	char file_name[128] = "capture_raw_tool_temp.raw";
	FILE* fp = 0;

	switch(cmr_cxt_ptr->cmd) {
		case UTEST_CALIBRATION_AWB:
			sprintf(file_name, calibration_raw_data_file, sensor_ptr->name, cmr_cxt_ptr->capture_width,
					cmr_cxt_ptr->capture_height, "awb");
			break;
		case UTEST_CALIBRATION_LSC:
			sprintf(file_name, calibration_raw_data_file, sensor_ptr->name, cmr_cxt_ptr->capture_width,
					cmr_cxt_ptr->capture_height, "lsc");
			break;
		case UTEST_CALIBRATION_FLASHLIGHT:
			sprintf(file_name, calibration_raw_data_file, sensor_ptr->name, cmr_cxt_ptr->capture_width,
					cmr_cxt_ptr->capture_height, "flashlight");
			break;
		default:
			break;
	}

	fp = fopen(file_name, "wb");
	if (fp != NULL) {
		fwrite((void *)cmr_cxt_ptr->capture_raw_vir_addr , 1, cmr_cxt_ptr->capture_width
		* cmr_cxt_ptr->capture_height * 2, fp);
		fclose(fp);
	} else {
		ERR("utest_dcam_save_raw_data: failed to open save_file.\n");
		rtn = -1;
	}
	return rtn;
}

static int32_t utest_save_raw_data(char* file_name, void* buf, uint32_t len)
{
	int32_t rtn = 0;
	FILE* fp = 0;

	fp = fopen(file_name, "wb");
	if (fp != NULL) {
		fwrite((void *)buf, 1, len, fp);
		fclose(fp);
	} else {
		ERR("utest_save_raw_data: failed to open save_file.\n");
		rtn = -1;
	}

	return rtn;
}

static int32_t utest_dcam_save_jpg(void)
{
	int32_t rtn = 0;
	struct utest_cmr_context *cmr_cxt_ptr = g_utest_cmr_cxt_ptr;
	FILE* fp = 0;

	fp = fopen(calibration_cap_jpg_file, "wb");
	if (fp != NULL) {
		fwrite(cmr_cxt_ptr->jpg_buffer , 1, cmr_cxt_ptr->jpg_size , fp);
		fclose(fp);
	} else {
		ERR("utest_dcam_save_cap_jpg: failed to open save_file.\n");
		rtn = -1;
	}
	return rtn;
}

static int32_t utest_dcam_find_param_index(struct utest_cmr_context *cmr_cxt_ptr )
{
	uint32_t i = 0, index = 0;
	uint32_t height1 = 0;
	uint32_t height2 = 0;
	SENSOR_TRIM_T *trim_ptr = 0;
	struct sensor_raw_fix_info *raw_fix_info_ptr = 0;
	SENSOR_EXP_INFO_T *sensor_info_ptr = Sensor_GetInfo();

	height2 = cmr_cxt_ptr->capture_height;

	trim_ptr = (SENSOR_TRIM_T *)(sensor_info_ptr->ioctl_func_ptr->get_trim(0));

	i = 1;
	while(1) {

		height1 =  trim_ptr[i].trim_height;

		INFO("trim: index:%d, width:%d height:%d\n", (int32_t)i, (int32_t)trim_ptr[i].trim_width, (int32_t)trim_ptr[i].trim_height);

		if (height2 == trim_ptr[i].trim_height)
		{
			index = i -1;
			INFO("find the param i = %d, width = %d, height = %d\n", i, (int32_t)trim_ptr[i].trim_width, (int32_t)trim_ptr[i].trim_height);
			break;

		}

		if  (0 == trim_ptr[i].trim_height) {
			index = -1;
			INFO("do not find the param: i = %d \n", (int32_t)i);
			break;
		}

		i++;
	}

	return index;
}

static int32_t utest_dcam_awb(uint8_t sub_cmd)
{
	struct utest_cmr_context *cmr_cxt_ptr = g_utest_cmr_cxt_ptr;
	SENSOR_EXP_INFO_T *sensor_ptr = (SENSOR_EXP_INFO_T*)Sensor_GetInfo();
	struct sensor_raw_fix_info *fix_ptr = 0;
	struct isp_addr_t img_addr = {0, 0, 0};
	struct isp_rect_t rect = {0, 0, 0,0};
	struct isp_size_t img_size = {0, 0};
	struct isp_bayer_ptn_stat_t stat_param;
	struct isp_addr_t dst_addr = {0, 0, 0};
	struct isp_addr_t tmp_addr = {0, 0, 0};
	int32_t index = 0;;
#if 0
	uint32_t *dst_temp_addr = NULL;
#endif
	uint32_t *img_blc_addr = NULL;
	uint32_t *img_tmp_addr = NULL;
	uint16_t *len_tab_addr = NULL;
	uint32_t len_tab_size = 0;
	uint32_t img_len = 0;
	char file_name[128] = {0};
	FILE *fp = NULL;
	int32_t rtn = 0;
	uint32_t lsc_grid =32;

	memset(&stat_param, 0, sizeof(isp_bayer_ptn_stat_t));
	img_addr.y_addr = cmr_cxt_ptr->capture_raw_vir_addr;
	rect.x = 0;
	rect.y = 0;
	rect.width = cmr_cxt_ptr->capture_width;
	rect.height = cmr_cxt_ptr->capture_height;
	img_size.width = cmr_cxt_ptr->capture_width;
	img_size.height = cmr_cxt_ptr->capture_height;
	img_len = img_size.width * img_size.height * 2;
#if 0
	dst_temp_addr = (uint32_t *)malloc(cmr_cxt_ptr->capture_width * cmr_cxt_ptr->capture_height * 2);

	if(dst_temp_addr) {

		dst_addr.y_addr = (uint32_t)dst_temp_addr;
		if (ISP_Cali_UnCompressedPacket(img_addr, dst_addr,img_size, 1)) {
			ERR("utest_dcam_awb: failed to unCompresse.\n");
			rtn = -1;
			goto awb_exit;
		}
	} else {
		ERR("utest_dcam_awb: failed to alloc buffer.\n");
		return -1;
	}
#else
	if (0 == (sensor_ptr->raw_info_ptr->tune_ptr->blc_bypass)) {
		uint32_t bayer_mod;
		struct isp_rect_t blc_rect = { 0, 0, 0, 0};
		struct isp_size_t blc_size = {0, 0};
		struct isp_bayer_ptn_stat_t blc_stat = {0, 0, 0, 0};
		struct sensor_blc_param *blc_ptr = NULL;
		char file_name1[64] = "/data/sensor_before_blc.raw";

		img_blc_addr = (uint32_t *)malloc(cmr_cxt_ptr->capture_width * cmr_cxt_ptr->capture_height * 2);
		if(img_blc_addr) {

			memset((void*)img_blc_addr, 0x00, cmr_cxt_ptr->capture_width * cmr_cxt_ptr->capture_height * 2);

			blc_size.width= cmr_cxt_ptr->capture_width;
			blc_size.height = cmr_cxt_ptr->capture_height;
			blc_rect.x = 0;
			blc_rect.y = 0;
			blc_rect.width = cmr_cxt_ptr->capture_width;
			blc_rect.height = cmr_cxt_ptr->capture_height;
			bayer_mod = sensor_ptr->image_pattern;
			tmp_addr.y_addr = (uintptr_t)img_blc_addr;
			blc_ptr = &(sensor_ptr->raw_info_ptr->tune_ptr->blc);
			blc_stat.r_stat = blc_ptr->offset[0].r;
			blc_stat.b_stat = blc_ptr->offset[0].b;
			blc_stat.gr_stat = blc_ptr->offset[0].gr;
			blc_stat.gb_stat = blc_ptr->offset[0].gb;
			rtn = ISP_Cali_BlackLevelCorrection(&img_addr,
												&tmp_addr,
												&blc_rect,
												&blc_size,
												bayer_mod,
												&blc_stat);

		} else {
			ERR("utest_dcam_awb: failed to alloc buffer.\n");
			rtn = -1;
			goto awb_exit;
		}
	} else {
		tmp_addr.y_addr = img_addr.y_addr;
		tmp_addr.uv_addr = img_addr.uv_addr;
		tmp_addr.v_addr = img_addr.v_addr;
	}

#endif

	ISP_Cali_GetLensTabSize(img_size, lsc_grid, &len_tab_size);

	len_tab_addr = (uint16_t *)malloc(len_tab_size);
	if (NULL == len_tab_addr) {
		rtn = -1;
		ERR("utest_dcam_awb: failed to alloc buffer.\n");
		goto awb_exit;
	}
	memset(len_tab_addr, 0x00, len_tab_size);

	img_tmp_addr = (uint32_t*)malloc(img_len);
	if (0 == img_tmp_addr) {
		rtn = -1;
		ERR("utest_dcam_awb: failed to alloc buffer.\n");
		goto awb_exit;
	}
	memset(img_tmp_addr, 0x00, img_len);

	index = utest_dcam_find_param_index(cmr_cxt_ptr);
	if (index < 0) {
		rtn = -1;
		ERR("utest_dcam_awb: do not find param index.\n");
		goto awb_exit;
	}

	if (0 == sub_cmd) {

		uint16_t* tmp_lnc_ptr = 0;
		uint16_t len = 0;
		fix_ptr = (struct sensor_raw_fix_info*)sensor_ptr->raw_info_ptr->fix_ptr;
		tmp_lnc_ptr = (uint16_t*)(fix_ptr->lnc.map[0][0].param_addr);
		len = fix_ptr->lnc.map[0][0].len;
		memcpy((void*)len_tab_addr, (void*)tmp_lnc_ptr, len);

	} else {
		if (ISP_Cali_GetLensTabs(tmp_addr, lsc_grid, img_size, (uint32_t*)len_tab_addr, index, 0, sub_cmd)) {
			rtn = -1;
			ERR("utest_dcam_awb: fail to isp_cali_getlenstab.\n");
			goto awb_exit;
		}
	}

	dst_addr.y_addr = (uintptr_t)img_tmp_addr;
	dst_addr.v_addr = 0;
	dst_addr.uv_addr = 0;

	ISP_Cali_LensCorrection(&tmp_addr,
							&dst_addr,
							img_size,
							lsc_grid,
							len_tab_addr);

	if (ISP_Cali_RawRGBStat(&dst_addr, &rect, &img_size, &stat_param)) {
		ERR("utest_dcam_awb: failed to isp_cali_raw_rgb_status.\n");
		rtn = -1;
		goto awb_exit;
	}

	switch (sub_cmd) {
		case UTEST_CALI_SUB_CMD_GOLDEN:
		{
			sprintf(file_name, calibration_awb_file, sensor_ptr->name,"gldn");
		}
		break;
		case UTEST_CALI_SUB_CMD_RANDOM:
		{
			sprintf(file_name, calibration_awb_file, sensor_ptr->name,"rdm");
		}
		break;
		default:
		{
			ERR("utest_dcam_awb: sub cmd is invalid\n");
			goto awb_exit;
		}
		break;
	}

	fp = fopen(file_name, "wb");
	if (fp != NULL) {
		fwrite((void *)(&stat_param), 1, sizeof(stat_param), fp);
		fclose(fp);
	} else {
		ERR("utest_dcam_awb: failed to open calibration_awb_file.\n");
		rtn = -1;
		goto awb_exit;
	}

	if (utest_dcam_save_raw_data())
			goto awb_exit;

awb_exit:
#if 0
	if (dst_temp_addr) {
		free(dst_temp_addr);
	}
#endif

	if (img_blc_addr) {
		free(img_blc_addr);
		img_blc_addr = NULL;
	}

	if (img_tmp_addr) {
		free(img_tmp_addr);
		img_tmp_addr = NULL;
	}

	if (len_tab_addr) {
		free(len_tab_addr);
		len_tab_addr = NULL;
	}

	return rtn;
}


static int32_t utest_dcam_flashlight(uint8_t sub_cmd)
{
	struct utest_cmr_context *cmr_cxt_ptr = g_utest_cmr_cxt_ptr;
	SENSOR_EXP_INFO_T *sensor_ptr = (SENSOR_EXP_INFO_T*)Sensor_GetInfo();
	struct sensor_raw_fix_info *fix_ptr = 0;
	struct isp_addr_t img_addr = {0, 0, 0};
	struct isp_rect_t rect = {0, 0, 0,0};
	struct isp_size_t img_size = {0, 0};
	struct isp_bayer_ptn_stat_t stat_param = {0, 0, 0, 0};
	struct isp_addr_t dst_addr = {0, 0, 0};
	struct isp_addr_t tmp_addr = {0,0,0};
	int32_t index = 0;
#if 0
	uint32_t *dst_temp_addr = NULL;
#endif
	uint32_t *img_blc_addr = NULL;
	uint32_t *img_tmp_addr = NULL;
	uint16_t *len_tab_addr = NULL;
	uint32_t len_tab_size = 0;
	uint32_t img_len = 0;
	char file_name[128] = {0};
	FILE *fp = NULL;
	int32_t rtn = 0;
	uint32_t lsc_grid =32;

	memset(&stat_param, 0, sizeof(isp_bayer_ptn_stat_t));
	img_addr.y_addr = cmr_cxt_ptr->capture_raw_vir_addr;
	rect.x = cmr_cxt_ptr->capture_width * 4 / 6;
	rect.y = cmr_cxt_ptr->capture_height * 4 / 6;
	rect.width = cmr_cxt_ptr->capture_width / 6;
	rect.height = cmr_cxt_ptr->capture_height / 6;
	img_size.width = cmr_cxt_ptr->capture_width;
	img_size.height = cmr_cxt_ptr->capture_height;
	img_len = img_size.width * img_size.height * 2;

#if 0
	dst_temp_addr = (uint32_t *)malloc(cmr_cxt_ptr->capture_width * cmr_cxt_ptr->capture_height * 2);

	if(dst_temp_addr) {
		dst_addr.y_addr = (uint32_t)dst_temp_addr;
		if (ISP_Cali_UnCompressedPacket(img_addr, dst_addr,img_size, 1)) {
			ERR("utest_dcam_flashlight: failed to unCompresse.\n");
			rtn = -1;
			goto awb_exit;
		}

	} else {
		ERR("utest_dcam_flashlight: failed to alloc buffer.\n");
		return -1;
	}
#else
	if (0 == (sensor_ptr->raw_info_ptr->tune_ptr->blc_bypass)) {
		uint32_t bayer_mod;
		struct isp_rect_t blc_rect = {0, 0, 0, 0};
		struct isp_size_t blc_size = {0, 0};
		struct isp_bayer_ptn_stat_t blc_stat = {0, 0, 0, 0};
		struct sensor_blc_param *blc_ptr = NULL;

		img_blc_addr = (uint32_t *)malloc(cmr_cxt_ptr->capture_width * cmr_cxt_ptr->capture_height * 2);
		if(img_blc_addr) {

			memset((void*)img_blc_addr, 0x00, cmr_cxt_ptr->capture_width * cmr_cxt_ptr->capture_height * 2);

			blc_size.width= cmr_cxt_ptr->capture_width;
			blc_size.height = cmr_cxt_ptr->capture_height;
			blc_rect.x = 0;
			blc_rect.y = 0;
			blc_rect.width = cmr_cxt_ptr->capture_width;
			blc_rect.height = cmr_cxt_ptr->capture_height;
			bayer_mod = sensor_ptr->image_pattern;
			tmp_addr.y_addr = (uintptr_t)img_blc_addr;
			blc_ptr = &(sensor_ptr->raw_info_ptr->tune_ptr->blc);
			blc_stat.r_stat = blc_ptr->offset[0].r;
			blc_stat.b_stat = blc_ptr->offset[0].b;
			blc_stat.gr_stat = blc_ptr->offset[0].gr;
			blc_stat.gb_stat = blc_ptr->offset[0].gb;
			rtn = ISP_Cali_BlackLevelCorrection(&img_addr,
												&tmp_addr,
												&blc_rect,
												&blc_size,
												bayer_mod,
												&blc_stat);

		} else {
			ERR("utest_dcam_flashlight: failed to alloc buffer.\n");
			rtn = -1;
			goto flashlight_exit;
		}
	} else {
		tmp_addr.y_addr = img_addr.y_addr;
		tmp_addr.uv_addr = img_addr.uv_addr;
		tmp_addr.v_addr = img_addr.v_addr;
	}
#endif

ISP_Cali_GetLensTabSize(img_size, lsc_grid, &len_tab_size);

	len_tab_addr = (uint16_t *)malloc(len_tab_size);
	if (NULL == len_tab_addr) {
		rtn = -1;
		ERR("utest_dcam_flashlight: failed to alloc buffer.\n");
		goto flashlight_exit;
	}
	memset(len_tab_addr, 0x00, len_tab_size);

	img_tmp_addr = (uint32_t*)malloc(img_len);
	if (0 == img_tmp_addr) {
		rtn = -1;
		ERR("utest_dcam_flashlight: failed to alloc buffer.\n");
		goto flashlight_exit;
	}
	memset(img_tmp_addr, 0x00, img_len);

	index = utest_dcam_find_param_index(cmr_cxt_ptr);
	if (index < 0) {
		rtn = -1;
		ERR("utest_dcam_flashlight: do not find param index.\n");
		goto flashlight_exit;
	}

	if (0 == sub_cmd) {
		uint16_t* tmp_lnc_ptr = 0;
		uint16_t len = 0;
		fix_ptr = (struct sensor_raw_fix_info*)sensor_ptr->raw_info_ptr->fix_ptr;
		tmp_lnc_ptr = (uint16_t*)fix_ptr->lnc.map[0][0].param_addr;
		len = fix_ptr->lnc.map[0][0].len;
		memcpy((void*)len_tab_addr, (void*)tmp_lnc_ptr, len);

	} else {

		if (ISP_Cali_GetLensTabs(tmp_addr, lsc_grid, img_size, (uint32_t*)len_tab_addr, index, 0, sub_cmd)) {
			rtn = -1;
			ERR("utest_dcam_lsc: fail to isp_cali_getlenstab.\n");
			goto flashlight_exit;
		}
	}

	dst_addr.y_addr = (uintptr_t)img_tmp_addr;
	dst_addr.v_addr = 0;
	dst_addr.uv_addr = 0;

	ISP_Cali_LensCorrection(&tmp_addr,
							&dst_addr,
							img_size,
							lsc_grid,
							len_tab_addr);

	if (ISP_Cali_RawRGBStat(&dst_addr, &rect, &img_size, &stat_param)) {
		ERR("utest_dcam_flashlight: failed to isp_cali_raw_rgb_status.\n");
		rtn = -1;
		goto flashlight_exit;
	}

	switch (sub_cmd) {
		case UTEST_CALI_SUB_CMD_GOLDEN:
		{
			sprintf(file_name, calibration_flashlight_file, sensor_ptr->name, "gldn");
		}
		break;
		case UTEST_CALI_SUB_CMD_RANDOM:
		{
			sprintf(file_name, calibration_flashlight_file, sensor_ptr->name, "rdm");
		}
		break;
		default:
		{
			goto flashlight_exit;
		}
		break;
	}

	fp = fopen(file_name, "wb");
	if (fp != NULL) {
		fwrite((void *)(&stat_param), 1, sizeof(stat_param), fp);
		fclose(fp);
	} else {
		ERR("utest_dcam_flashlight: failed to open calibration_awb_file.\n");
		rtn = -1;
		goto flashlight_exit;
	}

	if (utest_dcam_save_raw_data())
			goto flashlight_exit;

flashlight_exit:
#if 0
	if (dst_temp_addr) {
		free(dst_temp_addr);
	}
#endif

	if (img_blc_addr) {
		free (img_blc_addr);
		img_blc_addr = NULL;
	}

	if (img_tmp_addr) {
		free(img_tmp_addr);
		img_tmp_addr = NULL;
	}

	if (len_tab_addr) {
		free(len_tab_addr);
		len_tab_addr = NULL;
	}

	return rtn;
}

static int32_t utest_dcam_lsc(uint8_t sub_cmd)
{
	struct utest_cmr_context *cmr_cxt_ptr = g_utest_cmr_cxt_ptr;
	SENSOR_EXP_INFO_T *sensor_ptr = (SENSOR_EXP_INFO_T*)Sensor_GetInfo();
	struct isp_addr_t img_addr = {0, 0, 0};
	struct isp_rect_t rect = {0, 0, 0, 0};
	struct isp_size_t img_size = {0, 0};
	struct isp_addr_t dst_addr = {0, 0, 0};
#if 0
	uint32_t *dst_temp_addr = NULL;
#endif
	uint32_t *img_blc_addr = NULL;
	uint32_t *len_tab_addr = NULL;
	uint32_t len_tab_size = 0;
	int32_t rtn = 0;
	char file_name[128] = {0};
	FILE *fp = NULL;
	uint32_t lsc_grid = 32;/*grid: 16, 32, 64*/
	int32_t index = 0;

	img_addr.y_addr = cmr_cxt_ptr->capture_raw_vir_addr;
	rect.x = 0;
	rect.y = 0;
	rect.width = cmr_cxt_ptr->capture_width;
	rect.height = cmr_cxt_ptr->capture_height;
	img_size.width = cmr_cxt_ptr->capture_width;
	img_size.height = cmr_cxt_ptr->capture_height;

	ISP_Cali_GetLensTabSize(img_size, lsc_grid, &len_tab_size);

	len_tab_addr = (uint32_t *)malloc(len_tab_size);
	if (NULL == len_tab_addr) {
		rtn = -1;
		ERR("utest_dcam_lsc: failed to alloc buffer.\n");
		goto lsc_exit;
	}
#if 0
	dst_temp_addr = (uint32_t *)malloc(cmr_cxt_ptr->capture_width * cmr_cxt_ptr->capture_height * 2);

	if (NULL == len_tab_addr || NULL == dst_temp_addr) {
		rtn = -1;
		ERR("utest_dcam_lsc: failed to alloc buffer.\n");
		goto lsc_exit;
	}

	dst_addr.y_addr = (uint32_t)dst_temp_addr;

	if (ISP_Cali_UnCompressedPacket(img_addr, dst_addr,img_size, 1)) {
		rtn = -1;
		ERR("utest_dcam_lsc: failed to unCompresse.\n");
		goto lsc_exit;
	}
#else
	if (0 == (sensor_ptr->raw_info_ptr->tune_ptr->blc_bypass)) {
		uint32_t bayer_mod;
		struct isp_rect_t blc_rect = {0, 0, 0, 0};
		struct isp_size_t blc_size = {0, 0};
		struct isp_bayer_ptn_stat_t blc_stat = {0, 0, 0, 0};
		struct sensor_blc_param *blc_ptr = NULL;

		img_blc_addr = (uint32_t *)malloc(cmr_cxt_ptr->capture_width * cmr_cxt_ptr->capture_height * 2);
		if(img_blc_addr) {

			memset((void*)img_blc_addr, 0x00, cmr_cxt_ptr->capture_width * cmr_cxt_ptr->capture_height * 2);

			blc_size.width= cmr_cxt_ptr->capture_width;
			blc_size.height = cmr_cxt_ptr->capture_height;
			blc_rect.x = 0;
			blc_rect.y = 0;
			blc_rect.width = cmr_cxt_ptr->capture_width;
			blc_rect.height = cmr_cxt_ptr->capture_height;
			bayer_mod = sensor_ptr->image_pattern;
			dst_addr.y_addr = (uintptr_t)img_blc_addr;
			blc_ptr = &(sensor_ptr->raw_info_ptr->tune_ptr->blc);
			blc_stat.r_stat = blc_ptr->offset[0].r;
			blc_stat.b_stat = blc_ptr->offset[0].b;
			blc_stat.gr_stat = blc_ptr->offset[0].gr;
			blc_stat.gb_stat = blc_ptr->offset[0].gb;
			rtn = ISP_Cali_BlackLevelCorrection(&img_addr,
												&dst_addr,
												&blc_rect,
												&blc_size,
												bayer_mod,
												&blc_stat);
		} else {
			ERR("utest_dcam_flashlight: failed to alloc buffer.\n");
			rtn = -1;
			goto lsc_exit;
		}
	} else {
		dst_addr.y_addr = img_addr.y_addr;
		dst_addr.uv_addr = img_addr.uv_addr;
		dst_addr.v_addr = img_addr.v_addr;
	}
#endif

	index = utest_dcam_find_param_index(cmr_cxt_ptr);
	if (index < 0) {
		rtn = -1;
		ERR("utest_dcam_lsc: do not find param index.\n");
		goto lsc_exit;
	}

	if (ISP_Cali_GetLensTabs(dst_addr, lsc_grid, img_size, len_tab_addr, index, 0, sub_cmd)) {
		rtn = -1;
		ERR("utest_dcam_lsc: fail to isp_cali_getlenstab.\n");
		goto lsc_exit;
	}

	switch (sub_cmd) {
		case UTEST_CALI_SUB_CMD_GOLDEN:
		sprintf(file_name, calibration_lsc_file,sensor_ptr->name, cmr_cxt_ptr->capture_width,
				cmr_cxt_ptr->capture_height, 0, "gldn");
		break;

		case UTEST_CALI_SUB_CMD_RANDOM:
		sprintf(file_name, calibration_lsc_file,sensor_ptr->name, cmr_cxt_ptr->capture_width,
				cmr_cxt_ptr->capture_height, 0, "rdm");
		break;

		default:
		break;
	}

	fp = fopen(file_name, "wb");
	if (fp != NULL) {
		fwrite(len_tab_addr, 1, len_tab_size, fp);
		fclose(fp);
	} else {
		ERR("utest_dcam_lsc: failed to open calibration_lsc_file.\n");
		rtn = -1;
		goto lsc_exit;
	}

	if (utest_dcam_save_raw_data())
			goto lsc_exit;

lsc_exit:
	if (len_tab_addr) {
		free(len_tab_addr);
		len_tab_addr = NULL;
	}

	if (img_blc_addr) {
		free (img_blc_addr);
		img_blc_addr = NULL;
	}

#if 0
	if (dst_temp_addr)
		free(dst_temp_addr);
#endif
	return rtn;
}

static int32_t utest_dcam_preview_flash_eb(void)
{
	struct utest_cmr_context *cmr_cxt_ptr = g_utest_cmr_cxt_ptr;

	if (UTEST_CALIBRATION_FLASHLIGHT == cmr_cxt_ptr->cmd) {
		SENSOR_FLASH_LEVEL_T flash_level;
		struct camera_context *cxt = camera_get_cxt();
		struct isp_alg flash_param;

		if (Sensor_GetFlashLevel(&flash_level))
			return -1;

		flash_param.mode=ISP_AE_BYPASS;
		flash_param.flash_eb=0x01;
		if (isp_ioctl(NULL, ISP_CTRL_ALG, (void*)&flash_param))
			return -1;

		sem_wait(&cxt->cmr_set.isp_alg_sem);

		if (camera_set_flashdevice((uint32_t)FLASH_OPEN))
			return -1;

		flash_param.mode=ISP_ALG_FAST;
		flash_param.flash_eb=0x01;
		flash_param.flash_ratio=flash_level.high_light*256/flash_level.low_light;
		if (isp_ioctl(NULL, ISP_CTRL_ALG, (void*)&flash_param))
			return -1;

		utest_dcam_wait_isp_ae_stab();
	}
	return 0;
}

static int32_t utest_dcam_preview_flash_dis(void)
{
	struct utest_cmr_context *cmr_cxt_ptr = g_utest_cmr_cxt_ptr;

	if (UTEST_CALIBRATION_FLASHLIGHT == cmr_cxt_ptr->cmd) {
		struct camera_context *cxt = camera_get_cxt();

		sem_wait(&cxt->cmr_set.isp_alg_sem);

		if (camera_set_flashdevice((uint32_t)FLASH_CLOSE_AFTER_OPEN))
			return -1;
	}
	return 0;
}

static int32_t utest_dcam_capture_flash_eb(void)
{
	struct utest_cmr_context *cmr_cxt_ptr = g_utest_cmr_cxt_ptr;

	if (UTEST_CALIBRATION_FLASHLIGHT == cmr_cxt_ptr->cmd) {
		if (isp_ioctl(NULL, ISP_CTRL_FLASH_EG,0))
			return -1;

		if (camera_set_flashdevice((uint32_t)FLASH_HIGH_LIGHT))
			return -1;
	}
	return 0;
}

static int32_t utest_dcam_capture_flash_dis(void)
{
	struct utest_cmr_context *cmr_cxt_ptr = g_utest_cmr_cxt_ptr;

	if (UTEST_CALIBRATION_FLASHLIGHT == cmr_cxt_ptr->cmd) {
		if (camera_set_flashdevice((uint32_t)FLASH_CLOSE_AFTER_OPEN))
			return -1;
	}
	return 0;
}

static void utest_dcam_preview_cb(camera_cb_type cb,
			const void *client_data,
			camera_func_type func,
			int32_t parm4)
{
	if (CAMERA_FUNC_START_PREVIEW == func) {

		switch(cb) {
		case CAMERA_EVT_CB_FRAME:
			{
				camera_frame_type *frame = (camera_frame_type *)parm4;

				camera_release_frame(frame->buf_id);
			}
			break;
		default:
			break;
		}
	}
}

static void utest_dcam_cap_cb(camera_cb_type cb,
			const void *client_data,
			camera_func_type func,
			int32_t parm4)
{
	struct utest_cmr_context *cmr_cxt_ptr = g_utest_cmr_cxt_ptr;

	if (CAMERA_FUNC_TAKE_PICTURE == func) {
		switch(cb) {
		case CAMERA_RSP_CB_SUCCESS:
			break;
		case CAMERA_EVT_CB_SNAPSHOT_DONE:
			break;
		case CAMERA_EXIT_CB_DONE:
			if (UTEST_CALIBRATION_AWB == cmr_cxt_ptr->cmd ||
				UTEST_CALIBRATION_LSC == cmr_cxt_ptr->cmd  ||
				UTEST_CALIBRATION_FLASHLIGHT == cmr_cxt_ptr->cmd ) {
				camera_frame_type *frame = (camera_frame_type *)parm4;
				cmr_cxt_ptr->capture_raw_vir_addr = (uintptr_t)frame->buf_Virt_Addr;
				cmr_cxt_ptr->capture_width = frame->captured_dx;
				cmr_cxt_ptr->capture_height= frame->captured_dy;
				sem_post(&(cmr_cxt_ptr->sem_cap_raw_done));
			}
			break;
		default:
			break;
		}
	} else if (CAMERA_FUNC_ENCODE_PICTURE ==  func) {
		switch (cb) {
		case CAMERA_RSP_CB_SUCCESS:
			break;
		case CAMERA_EXIT_CB_DONE:
			{
				JPEGENC_CBrtnType *encInfo = (JPEGENC_CBrtnType *)parm4;
				camera_encode_mem_type *enc = (camera_encode_mem_type *)encInfo->outPtr;
				cmr_cxt_ptr->jpg_size = encInfo->size;
				cmr_cxt_ptr->jpg_buffer = enc->buffer;
				sem_post(&(cmr_cxt_ptr->sem_cap_jpg_done));
			}
			break;
		default:
			break;
		}
	}
}

static int32_t utest_dcam_preview(void)
{
	if (camerea_set_preview_format(1))
		return -1;

	if (utest_dcam_preview_mem_alloc())
		return -1;

	if (camera_start_preview(utest_dcam_preview_cb, NULL,CAMERA_NORMAL_MODE))
		return -1;

	utest_dcam_wait_isp_ae_stab();

	if (utest_dcam_preview_flash_eb())
		return -1;

	if (utest_dcam_preview_flash_dis())
		return -1;

	if (camera_stop_preview())
		return -1;

	return 0;
}

static int32_t utest_dcam_capture(void)
{
	int32_t rtn = 0;
	struct timespec ts;
	struct utest_cmr_context *cmr_cxt_ptr = g_utest_cmr_cxt_ptr;

	if (utest_dcam_cap_memory_alloc())
		return -1;

	if (utest_dcam_capture_flash_eb())
		return -1;

	if (UTEST_CALIBRATION_CAP_JPG == cmr_cxt_ptr->cmd) {
		if (CAMERA_SUCCESS != camera_take_picture(utest_dcam_cap_cb,
			NULL, CAMERA_NORMAL_MODE)) {
			rtn = -1;
			goto cap_exit;
		}
	} else {
		if (CAMERA_SUCCESS != camera_take_picture_raw(utest_dcam_cap_cb,
			NULL, CAMERA_TOOL_RAW_MODE)) {
			rtn = -1;
			goto cap_exit;
		}
	}

	if (clock_gettime(CLOCK_REALTIME, &ts)) {
		rtn = -1;
		goto cap_exit;
	}

	ts.tv_sec += 3;


	if (UTEST_CALIBRATION_CAP_JPG == cmr_cxt_ptr->cmd) {
		if (sem_timedwait(&(cmr_cxt_ptr->sem_cap_jpg_done), &ts)) {
			rtn = -1;
			goto cap_exit;
		}
	} else {
		if (sem_timedwait(&(cmr_cxt_ptr->sem_cap_raw_done), &ts)) {
			rtn = -1;
			goto cap_exit;
		}
	}

cap_exit:

	utest_dcam_capture_flash_dis();
	return rtn;
}

int main(int argc, char **argv)
{
	int32_t rtn = 0;
	struct utest_cmr_context *cmr_cxt_ptr = g_utest_cmr_cxt_ptr;

	if (utest_dcam_param_set(argc, argv))
		return -1;

	if (CAMERA_SUCCESS != camera_init(cmr_cxt_ptr->sensor_id))
		return -1;

	camera_set_dimensions(cmr_cxt_ptr->capture_width,
					cmr_cxt_ptr->capture_height,
					UTEST_PREVIEW_WIDTH,/*cmr_cxt_ptr->capture_width,*/
					UTEST_PREVIEW_HEIGHT,/*cmr_cxt_ptr->capture_height,*/
					NULL,
					NULL,
					0);

	if (utest_dcam_preview()) {
		rtn = -1;
		goto cali_exit;
	}

	if (utest_dcam_capture()) {
		rtn = -1;
		goto cali_exit;
	} else {
		switch (cmr_cxt_ptr->cmd) {
		case UTEST_CALIBRATION_AWB:
			utest_dcam_awb(cmr_cxt_ptr->sub_cmd);
			break;
		case UTEST_CALIBRATION_LSC:
			utest_dcam_lsc(cmr_cxt_ptr->sub_cmd);
			break;
		case UTEST_CALIBRATION_FLASHLIGHT:
			utest_dcam_flashlight(cmr_cxt_ptr->sub_cmd);
			break;
		case UTEST_CALIBRATION_CAP_JPG:
			utest_dcam_save_jpg();
			break;
		default:
			rtn = -1;
			break;
		}
	}

cali_exit:

	utest_dcam_close();
	return rtn;
}
