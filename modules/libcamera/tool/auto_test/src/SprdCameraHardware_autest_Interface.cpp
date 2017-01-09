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
#include "sensor_cfg.h"
#include "ion_sprd.h"



using namespace android;

#define ERR(x...) fprintf(stderr, ##x)
#define INFO(x...) fprintf(stdout, ##x)

#define AUTO_TEST_PARAM_NUM 11
#define AUTO_TEST_PREVIEW_BUF_NUM 6
#define AUTO_TEST_CAPTURE_BUF_NUM 4
#define AUTO_TEST_PREVIEW_WIDTH 640
#define AUTO_TEST_PREVIEW_HEIGHT 480
#define MAX_MISCHEAP_NUM 10

enum auto_test_sensor_id {
	AUTO_TEST_SENSOR_MAIN = 0,
	AUTO_TEST_SENSOR_SUB,
	AUTO_TEST_SENSOR_ID_MAX
};

enum auto_test_calibration_cmd_id {
	AUTO_TEST_CALIBRATION_AWB= 0,
	AUTO_TEST_CALIBRATION_LSC,
	AUTO_TEST_CALIBRATION_FLASHLIGHT,
	AUTO_TEST_CALIBRATION_CAP_JPG,
	AUTO_TEST_CALIBRATION_CAP_YUV,
	AUTO_TEST_CALIBRATION_MAX
};

enum auto_test_cali_sub_cmd_id {
	AUTO_TEST_CALI_SUB_CMD_GOLDEN= 0,
	AUTO_TEST_CALI_SUB_CMD_RANDOM,
	AUTO_TEST_CALI_SUB_CMD_MAX,
};

struct auto_test_cmr_context {
	cmr_handle camera_handle;
	cmr_u32 sensor_id;
	cmr_u32 cmd;
	cmr_u32 sub_cmd;
	cmr_uint capture_raw_vir_addr;
	cmr_uint capture_yuv_vir_addr;
	cmr_u32 capture_width;
	cmr_u32 capture_height;
	size_t cap_pmemory_size[AUTO_TEST_CAPTURE_BUF_NUM];
	sp<MemoryHeapIon> cap_pmem_hp[AUTO_TEST_CAPTURE_BUF_NUM];
	cmr_uint cap_physical_addr[AUTO_TEST_CAPTURE_BUF_NUM];
	cmr_u8 *cap_virtual_addr[AUTO_TEST_CAPTURE_BUF_NUM];

	sp<MemoryHeapIon> misc_pmem_hp;
	cmr_uint misc_physical_addr;
	cmr_u8* misc_virtual_addr;

	sp<MemoryHeapIon> misc_heap_array[MAX_MISCHEAP_NUM];
	cmr_u32 misc_heap_num;
	cmr_uint misc_pmemory_size;

	sp<MemoryHeapIon> preview_pmem_hp[AUTO_TEST_PREVIEW_BUF_NUM];
	size_t preview_pmemory_size[AUTO_TEST_PREVIEW_BUF_NUM];
	cmr_uint preview_physical_addr[AUTO_TEST_PREVIEW_BUF_NUM];
	cmr_u8* preview_virtual_addr[AUTO_TEST_PREVIEW_BUF_NUM];

	sem_t sem_cap_raw_done;
	sem_t sem_cap_jpg_done;

	void * jpg_buffer;
	cmr_u32 jpg_size;
	cmr_u32 reserved;
};

static char calibration_awb_file[] = "/data/sensor_%s_awb_%s.dat";
static char calibration_flashlight_file[] = "/data/sensor_%s_flashlight_%s.dat";
static char calibration_lsc_file[] = "/data/sensor_%s_lnc_%d_%d_%d_%s.dat";
static char calibration_raw_data_file[] = "/data/sensor_%s_raw_%d_%d_%s.raw";
static char calibration_cap_jpg_file[] = "/data/sensor_%s_jpg.jpg";
static char calibration_cap_yuv_file[] = "/data/sensor_%s_yuv.yuv";

static struct auto_test_cmr_context auto_test_cmr_cxt;
static struct auto_test_cmr_context *g_auto_test_cmr_cxt_ptr = &auto_test_cmr_cxt;

uint32_t *autotest_camer_saveaddr = NULL;
int g_image_size=0;
int g_commandid=0;
int g_sensor_opened=0;

#define USE_PHYSICAL_ADD 0
#define USE_IOMM_ADD 1
#define AUTOTEST_CLIENTDATA    0xffffffff

#define SET_PARM(x,y,z) do {\
			INFO("%s: set camera param: %s, %d", __func__, #x, y);\
			camera_set_param (x, y, z);\
		} while(0)

static int g_mem_method = USE_PHYSICAL_ADD;/*0: physical address, 1: iommu  address*/
cmr_int read_data_from_sensor();
static int auto_test_dcam_preview_mem_alloc(cmr_u32 size, cmr_u32 sum, cmr_uint *phy_addr, cmr_uint *vir_addr);
static int auto_test_dcam_cap_memory_alloc(cmr_u32 size, cmr_u32 sum, cmr_uint *addr_phy, cmr_uint *addr_vir);

static void auto_test_dcam_usage()
{
	INFO("auto_test_dcam_usage:\n");
	INFO("auto_test_camera -maincmd cali_main_cmd -subcmd cali_sub_cmd -id sensor_id -w capture_width -h capture_height \n");
	INFO("-cmd	: calibration command\n");
	INFO("-id	: select sensor id(0: main sensor / 1: sub sensor)\n");
	INFO("-w	: captured picture size width\n");
	INFO("-h	: captured picture size height\n");
	INFO("-help	: show this help message\n");
}
/*
* parse paramet for sensor setttings
* and inital globale  g_auto_test_cmr_cxt_ptr
*
*/
static int auto_test_dcam_param_set(int maincmd ,int subcmd,int cameraid,int width,int height)
{
	int i = 0;
	int test=0;
	struct auto_test_cmr_context *cmr_cxt_ptr = g_auto_test_cmr_cxt_ptr;

	INFO("debug %s %d E ",__func__,__LINE__);
	memset(cmr_cxt_ptr, 0, sizeof(auto_test_cmr_context));
	cmr_cxt_ptr->cmd             = maincmd;
	cmr_cxt_ptr->sub_cmd         = subcmd;
	cmr_cxt_ptr->sensor_id       = cameraid;
	cmr_cxt_ptr->capture_width   = width;
	cmr_cxt_ptr->capture_height  = height;

	INFO("debug %d \n",__LINE__);

	if (sem_init(&(cmr_cxt_ptr->sem_cap_raw_done), 0, 0))
		return -1;

	if (sem_init(&(cmr_cxt_ptr->sem_cap_jpg_done), 0, 0))
		return -1;

	INFO("debug %s %d X \n",__func__,__LINE__);

	// test=Sensor_set_calibration(1);
	 INFO("debug %s  test =%d LINE=%d  X \n",__func__,test,__LINE__);
	 return 0;
}

/*static void auto_test_dcam_wait_isp_ae_stab(void)
{
	struct camera_context *cxt = camera_get_cxt();

	camera_isp_ae_stab_set(1);

	sem_wait(&cxt->cmr_set.isp_ae_stab_sem);
}*/

static void auto_test_dcam_preview_mem_release(void)
{
	uint32_t i =0;
	INFO("%s g_mem_method %d line = %d\n ",__func__,g_mem_method,__LINE__);
 	struct auto_test_cmr_context *cmr_cxt_ptr = g_auto_test_cmr_cxt_ptr;
	for (i = 0; i < AUTO_TEST_PREVIEW_BUF_NUM; i++) {
		if(g_mem_method == USE_PHYSICAL_ADD) {
			if (cmr_cxt_ptr->preview_physical_addr[i])
				cmr_cxt_ptr->preview_pmem_hp[i].clear();
		} else {
			cmr_cxt_ptr->preview_pmem_hp[i]->free_iova(ION_MM, cmr_cxt_ptr->preview_physical_addr[i], cmr_cxt_ptr->preview_pmemory_size[i]);
		}
	}
}

cmr_int Callback_Malloc(enum camera_mem_cb_type type, cmr_u32 *size_ptr, cmr_u32 *sum_ptr, cmr_uint *phy_addr, cmr_uint *vir_addr, void* private_data)
{
	cmr_int            ret = 0;
	cmr_u32            size, sum;

	if (!phy_addr || !vir_addr || !size_ptr || !sum_ptr || (0 == *size_ptr) || (0 == *sum_ptr)) {
		INFO("alloc param error 0x%lx 0x%lx 0x%lx", (cmr_uint)phy_addr, (cmr_uint)vir_addr, (cmr_uint)size_ptr);
		return -1;
	}
	size = *size_ptr;
	sum = *sum_ptr;
	if (CAMERA_PREVIEW == type) {
		ret = auto_test_dcam_preview_mem_alloc(size, sum, phy_addr, vir_addr);
	} else {
		ret = auto_test_dcam_cap_memory_alloc(size, sum, phy_addr, vir_addr);
	}
	return ret;
}


cmr_int Callback_Free(enum camera_mem_cb_type type, cmr_uint *phy_addr, cmr_uint *vir_addr, cmr_u32 sum, void* private_data)
{
	cmr_int            ret = 0;

	return ret;
}
#if 0
static int auto_test_callback_cap_mem_alloc(void* handle, unsigned int size, unsigned int *addr_phy, unsigned int *addr_vir)
{
	auto_test_cmr_context* camera = g_auto_test_cmr_cxt_ptr;
	if (camera == NULL) {
		return -1;
	}
	INFO("debug %s %d E \n",__func__,__LINE__);
	INFO("%s g_mem_method %d line = %d\n ",__func__,g_mem_method,__LINE__);

	if (camera->misc_heap_num >= MAX_MISCHEAP_NUM) {
		return -1;
	}

	sp<MemoryHeapIon> pHeapIon = NULL;

	if (g_mem_method == USE_PHYSICAL_ADD)
		 pHeapIon = new MemoryHeapIon("/dev/ion", size ,0 , (1<<31) | ION_HEAP_ID_MASK_MM);
	else
		 pHeapIon = new MemoryHeapIon("/dev/ion", size ,0 , (1<<31) | ION_HEAP_ID_MASK_SYSTEM);

	if (pHeapIon == NULL) {
		return -1;
	}
	if (pHeapIon->getHeapID() < 0) {
		return -1;
	}
	if (g_mem_method == USE_PHYSICAL_ADD) {
		pHeapIon->get_phy_addr_from_ion((int*)addr_phy, (int*)&size);
	} else {
		pHeapIon->get_mm_iova((int*)addr_phy, (int*)&size);
	}
	*addr_vir = (int)(pHeapIon->base());
	camera->misc_heap_array[camera->misc_heap_num++] = pHeapIon;

	INFO("debug %s %d X \n",__func__,__LINE__);

	return 0;
}
#endif

int auto_test_dcam_preview_mem_alloc(cmr_u32 size, cmr_u32 sum, cmr_uint *phy_addr, cmr_uint *vir_addr)
{
	uint32_t i =0;
	uint32_t buf_size =0;
	cmr_uint *vir_tmp_addr = vir_addr;
	cmr_uint *phy_tmp_addr = phy_addr;
	INFO("debug %s g_mem_method =%d %d E \n",__func__,g_mem_method,__LINE__);

	struct auto_test_cmr_context *cmr_cxt_ptr = g_auto_test_cmr_cxt_ptr;

	buf_size = size;
	if (sum > AUTO_TEST_PREVIEW_BUF_NUM) {
		ERR("alloc param error %d", sum);
		return -1;
	}

	for (i = 0; i < sum; i++) {

		if (g_mem_method == USE_PHYSICAL_ADD)
			cmr_cxt_ptr->preview_pmem_hp[i] = new MemoryHeapIon("/dev/ion", buf_size , MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_MM);
		else
			cmr_cxt_ptr->preview_pmem_hp[i] = new MemoryHeapIon("/dev/ion", buf_size ,MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);

		if (cmr_cxt_ptr->preview_pmem_hp[i]->getHeapID() < 0) {
			ERR("failed to alloc preview pmem buffer.\n");
			return -1;
		}

		if (g_mem_method == USE_PHYSICAL_ADD) {
			cmr_cxt_ptr->preview_pmem_hp[i]->get_phy_addr_from_ion(&cmr_cxt_ptr->preview_physical_addr[i],
				&cmr_cxt_ptr->preview_pmemory_size[i]);
		} else {
			cmr_cxt_ptr->preview_pmem_hp[i]->get_iova(ION_MM, &cmr_cxt_ptr->preview_physical_addr[i],
				&cmr_cxt_ptr->preview_pmemory_size[i]);
		}

		cmr_cxt_ptr->preview_virtual_addr[i] = (unsigned char*)cmr_cxt_ptr->preview_pmem_hp[i]->getBase();
		if (!cmr_cxt_ptr->preview_physical_addr[i]) {
			ERR("failed to alloc preview pmem buffer:addr is null.\n");
			return -1;
		} else {
			*vir_tmp_addr++ = (cmr_uint)cmr_cxt_ptr->preview_virtual_addr[i];
			*phy_tmp_addr++ = (cmr_uint)cmr_cxt_ptr->preview_physical_addr[i];
		}
	}

	INFO("debug %s %d X \n",__func__,__LINE__);

	return 0;
}

static void auto_test_dcam_cap_memory_release(void)
{
	struct auto_test_cmr_context *cmr_cxt_ptr = g_auto_test_cmr_cxt_ptr;
	uint32_t  i = 0;
	for (i=0 ; i<AUTO_TEST_CAPTURE_BUF_NUM ; i++) {
		if (g_mem_method == USE_PHYSICAL_ADD) {
			if (cmr_cxt_ptr->cap_physical_addr[i])
				cmr_cxt_ptr->cap_pmem_hp[i].clear();
		/*	if (cmr_cxt_ptr->misc_physical_addr)
				cmr_cxt_ptr->misc_pmem_hp.clear();*/
		} else {
			if(cmr_cxt_ptr->cap_physical_addr[i])
				cmr_cxt_ptr->cap_pmem_hp[i]->free_iova(ION_MM, cmr_cxt_ptr->cap_physical_addr[i], cmr_cxt_ptr->cap_pmemory_size[i]);
		}
	}
	INFO("debug %s %d X \n",__func__,__LINE__);
}

int auto_test_dcam_cap_memory_alloc(cmr_u32 size, cmr_u32 sum, cmr_uint *addr_phy, cmr_uint *addr_vir)
{
	uint32_t local_width, local_height;
	uint32_t mem_size;
	uint32_t buffer_size = 0;
	uint32_t i = 0;
	struct auto_test_cmr_context *cmr_cxt_ptr = g_auto_test_cmr_cxt_ptr;


	buffer_size = size;
	if (sum > AUTO_TEST_CAPTURE_BUF_NUM) {
		CMR_LOGE("param error, sum %d", sum);
		return -1;
	}
	for (i=0 ; i< sum ; i++) {
		if(g_mem_method == USE_PHYSICAL_ADD)
			cmr_cxt_ptr->cap_pmem_hp[i] = new MemoryHeapIon("/dev/ion", buffer_size , MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_MM);
		else
			cmr_cxt_ptr->cap_pmem_hp[i] = new MemoryHeapIon("/dev/ion", buffer_size ,MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);

		if (cmr_cxt_ptr->cap_pmem_hp[i]->getHeapID() < 0) {
			ERR("failed to alloc capture pmem buffer.\n");
			return -1;
		}

		if (g_mem_method == USE_PHYSICAL_ADD) {
			cmr_cxt_ptr->cap_pmem_hp[i]->get_phy_addr_from_ion(&cmr_cxt_ptr->cap_physical_addr[i],
				&cmr_cxt_ptr->cap_pmemory_size[i]);
		} else {
			cmr_cxt_ptr->cap_pmem_hp[i]->get_iova(ION_MM, &cmr_cxt_ptr->cap_physical_addr[i],
				&cmr_cxt_ptr->cap_pmemory_size[i]);
		}

		cmr_cxt_ptr->cap_virtual_addr[i] = (unsigned char*)cmr_cxt_ptr->cap_pmem_hp[i]->getBase();
		if (!cmr_cxt_ptr->cap_physical_addr[i]) {
			ERR("failed to alloc capture pmem buffer:addr is null.\n");
			return -1;
		} else {
			*addr_phy++ = (cmr_uint)cmr_cxt_ptr->cap_physical_addr[i];
			*addr_vir++ = (cmr_uint)cmr_cxt_ptr->cap_virtual_addr[i];
		}
	}
	INFO("debug %s %d X \n",__func__,__LINE__);

	return 0;
}

static void auto_test_dcam_close(void)
{
	struct auto_test_cmr_context *cmr_cxt_ptr = g_auto_test_cmr_cxt_ptr;
/*	Sensor_set_calibration(0);*/
	INFO("auto_test_dcam_close:start");
	/*auto_test_dcam_preview_mem_release();*/
	auto_test_dcam_cap_memory_release();
	camera_cancel_takepicture(cmr_cxt_ptr->camera_handle);
	camera_deinit(cmr_cxt_ptr->camera_handle);
	cmr_cxt_ptr->camera_handle = 0;
	INFO("auto_test_dcam_close:end");
}

/*
image_type : 0 is raw   1 is  jpeg ,2 is yuv
*/
int autotest_came_save_to_buf(uint32_t **ppbuf_addr,struct auto_test_cmr_context *save_cmr_cxt_ptr,int image_type)
{
	int ret_val=-1;
	cmr_uint capture_addr=0;
	INFO(" %s %d E \n",__func__,__LINE__);
	g_commandid=image_type;

	if ((0 == image_type) || (2 == image_type)) {
		g_image_size = save_cmr_cxt_ptr->capture_width* save_cmr_cxt_ptr->capture_height * 2;
		capture_addr = save_cmr_cxt_ptr->capture_raw_vir_addr;
	} else if(1 == image_type) {
		g_image_size = save_cmr_cxt_ptr->jpg_size;
		capture_addr = (cmr_uint)save_cmr_cxt_ptr->jpg_buffer;
	}
	if ((ppbuf_addr != NULL) && (save_cmr_cxt_ptr != NULL)) {
		*ppbuf_addr = (uint32_t *)malloc(g_image_size);
		if (!*ppbuf_addr) {
	         ERR("No mem!");
	         return ret_val;
		}
		memset((void*)*ppbuf_addr, 0x00, g_image_size);
		memcpy(*ppbuf_addr,(void *)capture_addr,g_image_size);
		ret_val = 0;
	} else {
		ERR("%s poiter of  ppbuf_addr or save_cmr_cxt_ptr is null ,line=%d \n",__func__,__LINE__);
	}

return ret_val;

}

int autotest_cam_from_buf(void**pp_image_addr,int bufsize,int *out_size )
{
	read_data_from_sensor();
	int tempsize=0;
	if ((pp_image_addr!= NULL) && (*pp_image_addr!=NULL) && (bufsize>0)) {
		tempsize=(bufsize<=g_image_size)?(bufsize):(g_image_size);
		memcpy(*pp_image_addr,autotest_camer_saveaddr,tempsize);
		*out_size=g_image_size;
		return 0;
	} else {
		ERR("%s pp_image_addr is null !!\n",__func__);
		return -1;
	}
}

static int32_t auto_test_dcam_save_raw_data(void)
{
	int32_t rtn = 0;
	struct auto_test_cmr_context *cmr_cxt_ptr = g_auto_test_cmr_cxt_ptr;
	SENSOR_EXP_INFO_T *sensor_ptr = (SENSOR_EXP_INFO_T*)Sensor_GetInfo();
	char file_name[128] = "capture_raw_tool_temp.raw";
	FILE* fp = 0;
	INFO("debug %s cmr_cxt_ptr->cmd=%d %d E \n",__func__,cmr_cxt_ptr->cmd,__LINE__);
	switch(cmr_cxt_ptr->cmd) {
	case AUTO_TEST_CALIBRATION_AWB:
		sprintf(file_name, calibration_raw_data_file, sensor_ptr->name, cmr_cxt_ptr->capture_width,
			cmr_cxt_ptr->capture_height, "awb");
		break;
	case AUTO_TEST_CALIBRATION_LSC:
		sprintf(file_name, calibration_raw_data_file, sensor_ptr->name, cmr_cxt_ptr->capture_width,
			cmr_cxt_ptr->capture_height, "lsc");
		break;
	case AUTO_TEST_CALIBRATION_FLASHLIGHT:
		sprintf(file_name, calibration_raw_data_file, sensor_ptr->name, cmr_cxt_ptr->capture_width,
			cmr_cxt_ptr->capture_height, "flashlight");
		break;
	default:
		break;
	}
	INFO("debug %s file_name =%s  %d E \n",__func__,file_name,__LINE__);

	autotest_came_save_to_buf(&autotest_camer_saveaddr,cmr_cxt_ptr,0);
	fp = fopen(file_name, "wb");
	if (fp != NULL) {
		fwrite((void *)cmr_cxt_ptr->capture_raw_vir_addr , 1, cmr_cxt_ptr->capture_width
		* cmr_cxt_ptr->capture_height * 2, fp);
		fclose(fp);
		ERR("auto_test_dcam_save_raw_data: successed to open save_file.\n");
	} else {
		ERR("auto_test_dcam_save_raw_data: failed to open save_file.\n");
		rtn = -1;
	}
	INFO("debug %s %d X ",__func__,__LINE__);
	return rtn;
}

static int32_t auto_test_save_raw_data(char* file_name, void* buf, uint32_t len)
{
	int32_t rtn = 0;
	FILE* fp = 0;
	INFO("debug %s %d E \n",__func__,__LINE__);
	fp = fopen(file_name, "wb");
	if (fp != NULL) {
		fwrite((void *)buf, 1, len, fp);
		fclose(fp);
		ERR("auto_test_dcam_save_raw_data: successed to open save_file.\n");
	} else {
		ERR("auto_test_save_raw_data: failed to open save_file.\n");
		rtn = -1;
	}
	INFO("debug %s %d X \n",__func__,__LINE__);
	return rtn;
}

static int32_t auto_test_dcam_save_jpg(int image_type)
{
	int32_t rtn = 0;
	struct auto_test_cmr_context *cmr_cxt_ptr = g_auto_test_cmr_cxt_ptr;

	SENSOR_EXP_INFO_T *sensor_ptr = (SENSOR_EXP_INFO_T*)Sensor_GetInfo();
	FILE* fp = 0;
	char file_name[250] = "capture_raw_tool_temp.raw";
	sprintf(file_name, calibration_cap_jpg_file,sensor_ptr->name);
	INFO("debug  %s file_name %s %d X \n",__func__,file_name,__LINE__);

	autotest_came_save_to_buf(&autotest_camer_saveaddr,cmr_cxt_ptr,image_type);
	fp = fopen(file_name, "wb");
	if (fp != NULL) {
		fwrite(cmr_cxt_ptr->jpg_buffer , 1, cmr_cxt_ptr->jpg_size , fp);
		fclose(fp);
		ERR("auto_test_dcam_save_raw_data: successed to open save_file.\n");
	} else {
		ERR("auto_test_dcam_save_cap_jpg: failed to open save_file.\n");
		rtn = -1;
	}
	INFO("debug %s %d X \n",__func__,__LINE__);
	return rtn;
}

static int32_t auto_test_dcam_save_yuv(int image_type)
{
	int32_t rtn = 0;
	struct auto_test_cmr_context *cmr_cxt_ptr = g_auto_test_cmr_cxt_ptr;

	SENSOR_EXP_INFO_T *sensor_ptr = (SENSOR_EXP_INFO_T*)Sensor_GetInfo();
	FILE* fp = 0;
	char file_name[250] = "\0";
	INFO("debug %s %d E \n",__func__,__LINE__);
	sprintf(file_name, calibration_cap_yuv_file,sensor_ptr->name);
	INFO("debug  %s file_name %s %d X \n",__func__,file_name,__LINE__);

	autotest_came_save_to_buf(&autotest_camer_saveaddr,cmr_cxt_ptr,image_type);
	fp = fopen(file_name, "wb");
	if (fp != NULL) {
		fwrite((void*)cmr_cxt_ptr->capture_raw_vir_addr , 1, cmr_cxt_ptr->capture_width*cmr_cxt_ptr->capture_height*2 , fp);
		fclose(fp);
		ERR("%s : successed to  save_file. line=%d \n",__func__,__LINE__);
	} else {
		ERR("%s: failed to open save_file.\n",__func__);
		rtn = -1;
	}
	INFO("debug %s %d X \n",__func__,__LINE__);
	return rtn;
}

static int32_t auto_test_dcam_find_param_index(struct auto_test_cmr_context *cmr_cxt_ptr )
{
	uint32_t i = 0, index = 0;
	uint32_t height1 = 0;
	uint32_t height2 = 0;
	SENSOR_TRIM_T *trim_ptr = 0;
	struct sensor_raw_fix_info *raw_fix_info_ptr = 0;
	SENSOR_EXP_INFO_T *sensor_info_ptr = Sensor_GetInfo();

	height2 = cmr_cxt_ptr->capture_height;

	trim_ptr = (SENSOR_TRIM_T *)(sensor_info_ptr->ioctl_func_ptr->get_trim(0));
	INFO("debug %s %d E \n",__func__,__LINE__);

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
		if (0 == trim_ptr[i].trim_height) {
			index = -1;
			INFO("do not find the param: i = %d \n", (int32_t)i);
			break;
		}
		i++;
	}
	INFO("debug %s loop(i)=%d  %d X \n",__func__,i,__LINE__);

	return index;
}

static int32_t auto_test_dcam_awb(uint8_t sub_cmd)
{    
int32_t rtn = 0;

#if !defined(CONFIG_CAMERA_ISP_VERSION_V3) && !defined(CONFIG_CAMERA_ISP_VERSION_V4)
	struct auto_test_cmr_context *cmr_cxt_ptr = g_auto_test_cmr_cxt_ptr;
	SENSOR_EXP_INFO_T *sensor_ptr = (SENSOR_EXP_INFO_T*)Sensor_GetInfo();
	struct sensor_raw_fix_info *fix_ptr = 0;
	struct isp_addr_t img_addr = {0, 0, 0};
	struct isp_rect_t rect = {0, 0, 0,0};
	struct isp_size_t img_size = {0, 0};
	struct isp_bayer_ptn_stat_t stat_param;
	struct isp_addr_t dst_addr = {0, 0, 0};
	struct isp_addr_t tmp_addr = {0, 0, 0};
	int32_t index = 0;
	INFO("debug %s %d E \n",__func__,__LINE__);
	uint32_t *img_blc_addr = NULL;
	uint32_t *img_tmp_addr = NULL;
	uint16_t *len_tab_addr = NULL;
	uint32_t len_tab_size = 0;
	uint32_t img_len = 0;
	char file_name[128] = {0};
	FILE *fp = NULL;
	//int32_t rtn = 0;
	uint32_t lsc_grid =32;
	time_t awb_save_time;
	struct tm * time_stamp;
	struct timeval  awb_time_stampusec;
	char postfix_name[128] = "capture_raw_tool_temp.raw";
	time( &awb_save_time );
	gettimeofday(&awb_time_stampusec, NULL);
	time_stamp = gmtime( &awb_save_time );
	snprintf(postfix_name,128, "%d_%d_%d_%d_%d_%d_%lu",
		 (time_stamp->tm_year+1900),
		 time_stamp->tm_mon,
		 time_stamp->tm_mday,
		 (time_stamp->tm_hour+8),
		 time_stamp->tm_min,
		 time_stamp->tm_sec,
		 awb_time_stampusec.tv_usec);

	memset(&stat_param, 0, sizeof(isp_bayer_ptn_stat_t));
	img_addr.y_addr = cmr_cxt_ptr->capture_raw_vir_addr;
	rect.x = 0;
	rect.y = 0;
	rect.width = cmr_cxt_ptr->capture_width;
	rect.height = cmr_cxt_ptr->capture_height;
	img_size.width = cmr_cxt_ptr->capture_width;
	img_size.height = cmr_cxt_ptr->capture_height;
	img_len = img_size.width * img_size.height * 2;

	/*
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
			ERR("auto_test_dcam_awb: failed to alloc buffer.\n");
			rtn = -1;
			goto awb_exit;
		}
	} else  */
	{
		tmp_addr.y_addr = img_addr.y_addr;
		tmp_addr.uv_addr = img_addr.uv_addr;
		tmp_addr.v_addr = img_addr.v_addr;
	}
	ISP_Cali_GetLensTabSize(img_size, lsc_grid, &len_tab_size);
	len_tab_addr = (uint16_t *)malloc(len_tab_size);
	if (NULL == len_tab_addr) {
		rtn = -1;
		ERR("auto_test_dcam_awb: failed to alloc buffer.\n");
		goto awb_exit;
	}
	memset(len_tab_addr, 0x00, len_tab_size);
	img_tmp_addr = (uint32_t*)malloc(img_len);
	if (0 == img_tmp_addr) {
		rtn = -1;
		ERR("auto_test_dcam_awb: failed to alloc buffer.\n");
		goto awb_exit;
	}
	memset(img_tmp_addr, 0x00, img_len);

	index = auto_test_dcam_find_param_index(cmr_cxt_ptr);
	if (index < 0) {
		rtn = -1;
		ERR("auto_test_dcam_awb: do not find param index.\n");
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
			ERR("auto_test_dcam_awb: fail to isp_cali_getlenstab.\n");
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
		ERR("auto_test_dcam_awb: failed to isp_cali_raw_rgb_status.\n");
		rtn = -1;
		goto awb_exit;
	}
	switch (sub_cmd) {
	case AUTO_TEST_CALI_SUB_CMD_GOLDEN:
		sprintf(file_name, calibration_awb_file, sensor_ptr->name,"gldn",postfix_name);
		break;
	case AUTO_TEST_CALI_SUB_CMD_RANDOM:
		sprintf(file_name, calibration_awb_file, sensor_ptr->name,"rdm",postfix_name);
		break;
	default:
		ERR("AUTO_TEST_dcam_awb: sub cmd is invalid\n");
		goto awb_exit;
		break;
	}
	INFO("debug %s file_name =%s  %d X \n",__func__,file_name,__LINE__);
	fp = fopen(file_name, "wb");
	if (fp != NULL) {
		fwrite((void *)(&stat_param), 1, sizeof(stat_param), fp);
		fclose(fp);
		INFO("auto_test_dcam_awb ok open jiao zhun file. \n");
	} else {
		ERR("auto_test_dcam_awb: failed to open calibration_awb_file.\n");
		rtn = -1;
		goto awb_exit;
	}

	if (auto_test_dcam_save_raw_data())
		goto awb_exit;
awb_exit:
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
	INFO("debug %s %d X \n",__func__,__LINE__);
#endif
	return rtn;
}

static int32_t auto_test_dcam_flashlight(uint8_t sub_cmd)
{
int32_t rtn = 0;
#if !defined(CONFIG_CAMERA_ISP_VERSION_V3) && !defined(CONFIG_CAMERA_ISP_VERSION_V4)

	struct auto_test_cmr_context *cmr_cxt_ptr = g_auto_test_cmr_cxt_ptr;
	SENSOR_EXP_INFO_T *sensor_ptr = (SENSOR_EXP_INFO_T*)Sensor_GetInfo();
	struct sensor_raw_fix_info *fix_ptr = 0;
	struct isp_addr_t img_addr = {0, 0, 0};
	struct isp_rect_t rect = {0, 0, 0,0};
	struct isp_size_t img_size = {0, 0};
	struct isp_bayer_ptn_stat_t stat_param = {0, 0, 0, 0};
	struct isp_addr_t dst_addr = {0, 0, 0};
	struct isp_addr_t tmp_addr = {0,0,0};
	int32_t index = 0;
	INFO("debug %s %d E \n",__func__,__LINE__);
	uint32_t *img_blc_addr = NULL;
	uint32_t *img_tmp_addr = NULL;
	uint16_t *len_tab_addr = NULL;
	uint32_t len_tab_size = 0;
	uint32_t img_len = 0;
	char file_name[128] = {0};
	FILE *fp = NULL;
	//int32_t rtn = 0;
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
	/*
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
			ERR("auto_test_dcam_flashlight: failed to alloc buffer.\n");
			rtn = -1;
			goto flashlight_exit;
		}
	} else  */
	{
		tmp_addr.y_addr = img_addr.y_addr;
		tmp_addr.uv_addr = img_addr.uv_addr;
		tmp_addr.v_addr = img_addr.v_addr;
	}

	ISP_Cali_GetLensTabSize(img_size, lsc_grid, &len_tab_size);
	len_tab_addr = (uint16_t *)malloc(len_tab_size);
	if (NULL == len_tab_addr) {
		rtn = -1;
		ERR("auto_test_dcam_flashlight: failed to alloc buffer.\n");
		goto flashlight_exit;
	}
	memset(len_tab_addr, 0x00, len_tab_size);
	img_tmp_addr = (uint32_t*)malloc(img_len);
	if (0 == img_tmp_addr) {
		rtn = -1;
		ERR("auto_test_dcam_flashlight: failed to alloc buffer.\n");
		goto flashlight_exit;
	}
	memset(img_tmp_addr, 0x00, img_len);
	index = auto_test_dcam_find_param_index(cmr_cxt_ptr);
	if (index < 0) {
		rtn = -1;
		ERR("auto_test_dcam_flashlight: do not find param index.\n");
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
			ERR("auto_test_dcam_lsc: fail to isp_cali_getlenstab.\n");
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
		ERR("auto_test_dcam_flashlight: failed to isp_cali_raw_rgb_status.\n");
		rtn = -1;
		goto flashlight_exit;
	}
	switch (sub_cmd) {
	case AUTO_TEST_CALI_SUB_CMD_GOLDEN:
		sprintf(file_name, calibration_flashlight_file, sensor_ptr->name, "gldn");
		break;
	case AUTO_TEST_CALI_SUB_CMD_RANDOM:
		sprintf(file_name, calibration_flashlight_file, sensor_ptr->name, "rdm");
		break;
	default:
		goto flashlight_exit;
		break;
	}
	fp = fopen(file_name, "wb");
	INFO("debug %s  file_name =%s  %d ..\n ",__func__,file_name,__LINE__);
	if (fp != NULL) {
		fwrite((void *)(&stat_param), 1, sizeof(stat_param), fp);
		fclose(fp);
	} else {
		ERR("auto_test_dcam_flashlight: failed to open calibration_awb_file.\n");
		rtn = -1;
		goto flashlight_exit;
	}
	if (auto_test_dcam_save_raw_data())
		goto flashlight_exit;
flashlight_exit:
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
	INFO("debug %s %d X \n",__func__,__LINE__);
#endif
	return rtn;
}

static int32_t auto_test_dcam_lsc(uint8_t sub_cmd)
{
int32_t rtn = 0;
#if !defined(CONFIG_CAMERA_ISP_VERSION_V3) && !defined(CONFIG_CAMERA_ISP_VERSION_V4)
	struct auto_test_cmr_context *cmr_cxt_ptr = g_auto_test_cmr_cxt_ptr;
	SENSOR_EXP_INFO_T *sensor_ptr = (SENSOR_EXP_INFO_T*)Sensor_GetInfo();
	struct isp_addr_t img_addr = {0, 0, 0};
	struct isp_rect_t rect = {0, 0, 0, 0};
	struct isp_size_t img_size = {0, 0};
	struct isp_addr_t dst_addr = {0, 0, 0};
	uint32_t *img_blc_addr = NULL;
	uint32_t *len_tab_addr = NULL;
	uint32_t len_tab_size = 0;
	//int32_t rtn = 0;
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

	INFO("debug %s %d E \n",__func__,__LINE__);

	ISP_Cali_GetLensTabSize(img_size, lsc_grid, &len_tab_size);

	len_tab_addr = (uint32_t *)malloc(len_tab_size);
	if (NULL == len_tab_addr) {
		rtn = -1;
		ERR("auto_test_dcam_lsc: failed to alloc buffer.\n");
		goto lsc_exit;
	}
	/*
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
			ERR("auto_test_dcam_flashlight: failed to alloc buffer.\n");
			rtn = -1;
			goto lsc_exit;
		}
	} else   */
	{
		dst_addr.y_addr = img_addr.y_addr;
		dst_addr.uv_addr = img_addr.uv_addr;
		dst_addr.v_addr = img_addr.v_addr;
	}
	index = auto_test_dcam_find_param_index(cmr_cxt_ptr);
	if (index < 0) {
		rtn = -1;
		ERR("auto_test_dcam_lsc: do not find param index.\n");
		goto lsc_exit;
	}
	if (ISP_Cali_GetLensTabs(dst_addr, lsc_grid, img_size, len_tab_addr, index, 0, sub_cmd)) {
		rtn = -1;
		ERR("auto_test_dcam_lsc: fail to isp_cali_getlenstab.\n");
		goto lsc_exit;
	}
	switch (sub_cmd) {
	case AUTO_TEST_CALI_SUB_CMD_GOLDEN:
		sprintf(file_name, calibration_lsc_file,sensor_ptr->name, cmr_cxt_ptr->capture_width,
				cmr_cxt_ptr->capture_height, 0, "gldn");
		break;
	case AUTO_TEST_CALI_SUB_CMD_RANDOM:
		sprintf(file_name, calibration_lsc_file,sensor_ptr->name, cmr_cxt_ptr->capture_width,
				cmr_cxt_ptr->capture_height, 0, "rdm");
		break;
	default:
		break;
	}
	INFO("debug %s file_name =%s %d X \n",__func__,file_name,__LINE__);
	fp = fopen(file_name, "wb");
	if (fp != NULL) {
		fwrite(len_tab_addr, 1, len_tab_size, fp);
		fclose(fp);
	} else {
		ERR("AUTO_TEST_dcam_lsc: failed to open calibration_lsc_file.\n");
		rtn = -1;
		goto lsc_exit;
	}

	if (auto_test_dcam_save_raw_data())
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
	INFO("debug %s %d X \n",__func__,__LINE__);
#endif
	return rtn;
}
#if 0
static int32_t auto_test_dcam_preview_flash_eb(void)
{
	struct auto_test_cmr_context *cmr_cxt_ptr = g_auto_test_cmr_cxt_ptr;
	INFO("debug %s cmr_cxt_ptr->cmd =%d line=%d E \n",__func__,cmr_cxt_ptr->cmd,__LINE__);
	if (AUTO_TEST_CALIBRATION_FLASHLIGHT == cmr_cxt_ptr->cmd) {
		SENSOR_FLASH_LEVEL_T flash_level;
		struct camera_context *cxt = camera_get_cxt();
		struct isp_alg flash_param;
		if (Sensor_GetFlashLevel(&flash_level))
			return -1;
		flash_param.mode=ISP_AE_BYPASS;
		flash_param.flash_eb=0x01;
		if (isp_ioctl(NULL, ISP_CTRL_ALG, (void*)&flash_param))
			return -1;
		INFO("debug %s cmr_cxt_ptr->cmd =%d line=%d E \n",__func__,cmr_cxt_ptr->cmd,__LINE__);

		if (camera_set_flashdevice((uint32_t)FLASH_OPEN))
			return -1;
		flash_param.mode=ISP_ALG_FAST;
		flash_param.flash_eb=0x01;
		flash_param.flash_ratio=flash_level.high_light*256/flash_level.low_light;
		if (isp_ioctl(NULL, ISP_CTRL_ALG, (void*)&flash_param))
			return -1;
	}
	INFO("debug %s %d X \n",__func__,__LINE__);
	return 0;
}

static int32_t auto_test_dcam_preview_flash_dis(void)
{
	struct auto_test_cmr_context *cmr_cxt_ptr = g_auto_test_cmr_cxt_ptr;

	if (AUTO_TEST_CALIBRATION_FLASHLIGHT == cmr_cxt_ptr->cmd) {
		struct camera_context *cxt = camera_get_cxt();

		sem_wait(&cxt->cmr_set.isp_alg_sem);

		if (camera_set_flashdevice((uint32_t)FLASH_CLOSE_AFTER_OPEN))
			return -1;
	}
	return 0;
}

static int32_t auto_test_dcam_capture_flash_eb(void)
{
	struct auto_test_cmr_context *cmr_cxt_ptr = g_auto_test_cmr_cxt_ptr;
	INFO("debug %s %d E \n",__func__,__LINE__);
	if (AUTO_TEST_CALIBRATION_FLASHLIGHT == cmr_cxt_ptr->cmd) {
		if (isp_ioctl(NULL, ISP_CTRL_FLASH_EG,0))
			return -1;

		if (camera_set_flashdevice((uint32_t)FLASH_HIGH_LIGHT))
			return -1;
	}
	INFO("debug %s %d X \n",__func__,__LINE__);
	return 0;
}

static int32_t auto_test_dcam_capture_flash_dis(void)
{
	struct auto_test_cmr_context *cmr_cxt_ptr = g_auto_test_cmr_cxt_ptr;
	if (AUTO_TEST_CALIBRATION_FLASHLIGHT == cmr_cxt_ptr->cmd) {
		if (camera_set_flashdevice((uint32_t)FLASH_CLOSE_AFTER_OPEN))
			return -1;
	}
	return 0;
}
#endif
static void auto_test_dcam_preview_cb(camera_cb_type cb,
			const void *client_data,
			camera_func_type func,
			int32_t parm4)
{
#if 0
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
#endif
}

static void auto_test_dcam_cap_cb(camera_cb_type cb,
			const void *client_data,
			camera_func_type func,
			void* parm4)
{
	struct auto_test_cmr_context *cmr_cxt_ptr = g_auto_test_cmr_cxt_ptr;
	INFO("debug %s func= %d cb= %d line=%d  E \n",__func__,func,cb,__LINE__);
	FILE * fp =NULL;
	if (CAMERA_FUNC_TAKE_PICTURE == func) {
		switch(cb) {
		case CAMERA_RSP_CB_SUCCESS:
			break;
		case CAMERA_EVT_CB_SNAPSHOT_DONE:
			INFO("debug %s %d X \n",__func__,__LINE__);
			{
				camera_frame_type *frame_snap = (camera_frame_type *)parm4;
				cmr_cxt_ptr->capture_raw_vir_addr = (uintptr_t)frame_snap->y_vir_addr;
				cmr_cxt_ptr->capture_width = frame_snap->width;
				cmr_cxt_ptr->capture_height= frame_snap->height;
			}
			break;
		case CAMERA_EXIT_CB_DONE:
			INFO("debug %s %d X \n",__func__,__LINE__);
			if (AUTO_TEST_CALIBRATION_AWB == cmr_cxt_ptr->cmd ||
				AUTO_TEST_CALIBRATION_LSC == cmr_cxt_ptr->cmd ||
				AUTO_TEST_CALIBRATION_FLASHLIGHT == cmr_cxt_ptr->cmd ||
				AUTO_TEST_CALIBRATION_CAP_YUV==cmr_cxt_ptr->cmd ) {
				INFO("debug %s %d X \n",__func__,__LINE__);
				camera_frame_type *frame = (camera_frame_type *)parm4;
				cmr_cxt_ptr->capture_raw_vir_addr = (uintptr_t)frame->y_vir_addr;
				cmr_cxt_ptr->capture_width = frame->width;
				cmr_cxt_ptr->capture_height= frame->height;
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
				INFO("debug %s %d X \n",__func__,__LINE__);
				struct camera_jpeg_param *encInfo = &((struct camera_frame_type *)parm4)->jpeg_param;
				cmr_cxt_ptr->jpg_size = encInfo->size;
				cmr_cxt_ptr->jpg_buffer = encInfo->outPtr;
				sem_post(&(cmr_cxt_ptr->sem_cap_jpg_done));
			}
			break;
		default:
			break;
		}
	}
	INFO("debug %s %d X \n",__func__,__LINE__);
}

static int32_t auto_test_dcam_preview(void)
{
#if 0
	if (camerea_set_preview_format(1)) {
		INFO("error %s %d . \n",__func__,__LINE__);
		return -1;
	}
	if (auto_test_dcam_preview_mem_alloc()) {
		INFO("error %s %d . \n",__func__,__LINE__);
		return -1;
	}
	if (camera_start_preview(auto_test_dcam_preview_cb, NULL,CAMERA_NORMAL_MODE)){
		INFO("error %s %d . \n",__func__,__LINE__);
		return -1;
	}
/*	if (auto_test_dcam_preview_flash_eb()) {
		INFO("error %s %d . \n",__func__,__LINE__);
		return -1;
	}
	if (auto_test_dcam_preview_flash_dis()) {
		INFO("error %s %d . \n",__func__,__LINE__);
		return -1;
	}*/
	if (camera_stop_preview()) {
		INFO("error %s %d . \n",__func__,__LINE__);
		return -1;
	}
#endif
	INFO("debug %s %d X \n",__func__,__LINE__);
	return 0;
}

static int32_t auto_test_dcam_capture(void)
{
	int32_t rtn = 0;
	struct timespec ts;
	struct auto_test_cmr_context *cmr_cxt_ptr = g_auto_test_cmr_cxt_ptr;

	INFO("debug %s cmr_cxt_ptr->cmd=%d line=%d E \n",__func__,cmr_cxt_ptr->cmd,__LINE__);

/*	if (auto_test_dcam_cap_memory_alloc())
		return -1;

	if (auto_test_dcam_capture_flash_eb())
		return -1;
*/
	if (CMR_CAMERA_SUCCESS != camera_set_mem_func(cmr_cxt_ptr->camera_handle, (void*)Callback_Malloc, (void*)Callback_Free, (void*)AUTOTEST_CLIENTDATA)) {
		ERR("auto_test_dcam_capture: failed to camera_set_mem_func");
		return -1;
	}
	if ((AUTO_TEST_CALIBRATION_CAP_JPG == cmr_cxt_ptr->cmd) ) {
		if (CMR_CAMERA_SUCCESS != camera_take_picture(cmr_cxt_ptr->camera_handle,CAMERA_NORMAL_MODE)) {
			rtn = -1;
			INFO("debug %s  line=%d  \n",__func__,__LINE__);
			goto cap_exit;
		}
	} else {
		INFO("debug %s  line=%d  \n",__func__,__LINE__);
		if (CMR_CAMERA_SUCCESS != camera_take_picture(cmr_cxt_ptr->camera_handle,CAMERA_AUTOTEST_MODE)) {
			rtn = -1;
			goto cap_exit;
		}
	}
	INFO("debug %s  line=%d  \n",__func__,__LINE__);

	if (clock_gettime(CLOCK_REALTIME, &ts)) {
		rtn = -1;
		goto cap_exit;
	}

	ts.tv_sec += 6;
	INFO("debug %s	line=%d  \n",__func__,__LINE__);
	if ((AUTO_TEST_CALIBRATION_CAP_JPG == cmr_cxt_ptr->cmd)) {
		if (sem_timedwait(&(cmr_cxt_ptr->sem_cap_jpg_done), &ts)) {
			rtn = -1;
			INFO("debug %s cmr_cxt_ptr->cmd=%d  line=%d E \n",__func__,cmr_cxt_ptr->cmd,__LINE__);
			goto cap_exit;
		}
	} else {
		INFO("debug %s cmr_cxt_ptr->cmd=%d  line=%d E \n",__func__,cmr_cxt_ptr->cmd,__LINE__);
		if (sem_timedwait(&(cmr_cxt_ptr->sem_cap_raw_done), &ts)) {
			rtn = -1;
			INFO("debug %s Error!!!\n ",__func__);
			goto cap_exit;
		}
	}
cap_exit:
/*	auto_test_dcam_capture_flash_dis();*/
	INFO("debug %s rtn= %d line =%d X \n",__func__,rtn,__LINE__);
	return rtn;
}

cmr_int read_data_from_sensor()
{
	cmr_int rtn = 0;
	struct  img_size req_size;
	struct  auto_test_cmr_context *cmr_cxt_ptr = g_auto_test_cmr_cxt_ptr;
	INFO("debug %s %d . \n",__func__,__LINE__);

	req_size.width = (cmr_u32)AUTO_TEST_PREVIEW_WIDTH;
	req_size.height = (cmr_u32)AUTO_TEST_PREVIEW_HEIGHT;
	SET_PARM(cmr_cxt_ptr->camera_handle, CAMERA_PARAM_PREVIEW_SIZE, (cmr_uint)&req_size);

	req_size.width = (cmr_u32)cmr_cxt_ptr->capture_width;
	req_size.height = (cmr_u32)cmr_cxt_ptr->capture_height;
	SET_PARM(cmr_cxt_ptr->camera_handle, CAMERA_PARAM_CAPTURE_SIZE, (cmr_uint)&req_size);

	INFO("debug %s %d . ..\n",__func__,__LINE__);

	if ((g_sensor_opened==0) &&
		(CMR_CAMERA_SUCCESS != camera_init(cmr_cxt_ptr->sensor_id, auto_test_dcam_cap_cb, (void*)AUTOTEST_CLIENTDATA, 1, &cmr_cxt_ptr->camera_handle))) {
		INFO("debug %s %d . \n",__func__,__LINE__);
		rtn = -1;
		goto exit;
	}
	g_sensor_opened=1;

/*	if ( auto_test_dcam_preview() ) {
		goto exit;
	}*/

	INFO("debug  %s  cmr_cxt_ptr->cmd=%d   LINE=%d . \n ",__func__, cmr_cxt_ptr->cmd,__LINE__);
	if (auto_test_dcam_capture()) {
		goto exit;
	}

	switch (cmr_cxt_ptr->cmd) {
	case AUTO_TEST_CALIBRATION_AWB:
		auto_test_dcam_awb(cmr_cxt_ptr->sub_cmd);
		INFO("debug  %s  cmr_cxt_ptr->cmd=%d   LINE=%d . \n ",__func__,cmr_cxt_ptr->cmd,__LINE__);
		break;
	case AUTO_TEST_CALIBRATION_LSC:
		auto_test_dcam_lsc(cmr_cxt_ptr->sub_cmd);
		break;
	case AUTO_TEST_CALIBRATION_FLASHLIGHT:
		auto_test_dcam_flashlight(cmr_cxt_ptr->sub_cmd);
		break;
	case AUTO_TEST_CALIBRATION_CAP_JPG:
		auto_test_dcam_save_jpg(1);
		break;
	case AUTO_TEST_CALIBRATION_CAP_YUV:
		CMR_LOGE("autotest  %s %d E\n ",__func__,__LINE__);
		auto_test_dcam_save_yuv(2);
		break;
	default:
		rtn = -1;
		break;
	}
	return 0;

exit:
	g_sensor_opened=0;

	return -1;

}


int32_t autotest_set_testmode(int camerinterface,int maincmd ,int subcmd,int cameraid,int width,int height)
{
	int32_t ret = 0;
	struct auto_test_cmr_context *cmr_cxt_ptr = g_auto_test_cmr_cxt_ptr;
	cmr_int is_raw_sensor=(camerinterface) & (1<<2);

	if(g_sensor_opened==1)
		return 0;

	CMR_LOGV("sensor is %s ",is_raw_sensor?("raw"):("yuv"));

	g_mem_method = MemoryHeapIon::IOMMU_is_enabled(ION_MM);

	INFO("debug %s camerinterface =%d maincmd=%d subcmd= %d cameraid=%d width=%d height=%d line=%d E\n ",
		__func__,camerinterface,maincmd,subcmd,cameraid,width,height,__LINE__);

	if ((maincmd>=AUTO_TEST_CALIBRATION_MAX||maincmd<0)
		||(subcmd>=AUTO_TEST_CALI_SUB_CMD_MAX||subcmd<0)
		||(cameraid>1||cameraid<0)
		||(width%32||height%32)) {
		CMR_LOGV("autotest parameter error %s %d E\n ",__func__,__LINE__);
		ret = -1;
		goto cali_exit;
	} else {
		auto_test_dcam_param_set(maincmd ,subcmd,cameraid,width,height);
	}
	if (CMR_CAMERA_SUCCESS != camera_init(cmr_cxt_ptr->sensor_id, auto_test_dcam_cap_cb, (void*)AUTOTEST_CLIENTDATA, 1, &cmr_cxt_ptr->camera_handle)) {
		INFO("debug %s %d . \n",__func__,__LINE__);
		ret = -1;
		goto cali_exit;
	}
	g_sensor_opened=1;
	return 0;

cali_exit:
	g_sensor_opened=0;
	return ret;
}

int autotest_close_testmode(void)
{
	INFO("%s %d X \n",__func__,__LINE__);
	auto_test_dcam_close();
	INFO("%s %d X \n",__func__,__LINE__);
	g_sensor_opened=0;
	return 0;
}

