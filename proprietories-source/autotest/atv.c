/*
 * File:         atv.c
 * Based on:
 * Author:       NewportMedia Inc.
 *
 * Created:	  2011-05-19
 *  
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <linux/videodev2.h>
#include <linux/fb.h>
#include <fcntl.h>             
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <linux/delay.h>
#include <string.h>
#include <sys/mman.h>
#include <linux/slab.h>
#include <pthread.h>
#include "minui.h"
#include <linux/android_pmem.h>
#include "rotation_sc8800g2.h"
#include "sprd_engtest.h"
#include "sprd_engcommon.h"
#include "sprd_engcommon.h"
#include "scale_sc8800g2.h"
#include "sprd_engui.h"

#define SPRD_ATV_DEV				"/dev/video0"
#define SPRD_ATV_FB_DEV					"/dev/graphics/fb0"
#define SPRD_PMEM_DEV				"/dev/pmem_adsp"
#define SPRD_ATV_DISPLAY_WIDTH		320
#define SPRD_ATV_DISPLAY_HEIGHT	240
#define SPRD_MAX_PREVIEW_BUF		2
#define SPRD_ATV_TYPE					2
#define SPRD_PREVIEW_BUF_SIZE		(SPRD_ATV_DISPLAY_WIDTH*SPRD_ATV_DISPLAY_HEIGHT*2)


#define HW_ROTATION_DEV              "/dev/sc8800g_rotation"
#define SPRD_ATVTEST_PASS			"Pass: atv"

struct frame_buffer_t {   
    uint32_t phys_addr;
	uint32_t virt_addr;
    uint32_t length;								 //buffer's length is different from cap_image_size   
}; 

static int v4l2_fd;
static int fb_fd;
static int pmem_fd;
static int g_camera_id = -1;
static int eng_camera_start=0;

static pthread_t g_preview_thr; 						//the thread pointer for preview processor.
static uint32_t g_releasebuff_index = 0; 				//store the rellease buffer index.
static struct v4l2_framebuffer v4l2fb;
static struct frame_buffer_t fb_buf[SPRD_MAX_PREVIEW_BUF+1];
static struct frame_buffer_t preview_buf[SPRD_MAX_PREVIEW_BUF + 1];
static struct fb_fix_screeninfo fix;
static struct fb_var_screeninfo var;
static struct pmem_region sprd_pmem_region;
static uint8_t tmpbuf[480*800*4];
static void *sprd_pmem_base;
typedef struct zoom_trim_rect{
	uint32_t x;
	uint32_t y;
	uint32_t w;
	uint32_t h;
}ZOOM_TRIM_RECT_T;


/*----------------------- NMI ATV APIs ---------------------------------------*/
extern unsigned int atv_nmi600_poweron_init(void);
extern void         atv_nmi600_set_region(unsigned int region,unsigned int *schannel,unsigned int *echannel);
extern unsigned int atv_nmi600_scan_one_channel(unsigned int channelID);
extern unsigned int atv_nmi600_fast_set_channel(unsigned int channelID);
extern unsigned int atv_nmi600_poweroff_deinit(void);


static void eng_atvtest_setaudio(int on_off)
{
    if(on_off == 1) {
        system("alsa_amixer cset -c sprdphone name=\"BypassFM Playback Switch\" 1");
        //system("alsa_amixer cset -c sprdphone name=\"Headset Playback Switch\" 1");
        system("alsa_amixer cset -c sprdphone name=\"Speaker Playback Switch\" 1");
        system("alsa_amixer sset 'LineinFM' on");
        system("alsa_amixer -c sprdphone cset name='Power Codec' 1");
    }else{
        system("alsa_amixer -c sprdphone cset name='Power Codec' 4");
        system("alsa_amixer cset -c sprdphone name=\"BypassFM Playback Switch\" 0");
        system("alsa_amixer cset -c sprdphone name=\"Speaker Playback Switch\" 0");
        system("alsa_amixer sset 'LineinFM' off");

    }
}

static int xioctl(int v4l2_fd, int request, void * arg) {   
    int r;   
    do   
        r = ioctl(v4l2_fd, request, arg);   
    while (-1 == r && EINTR == errno);   
    return r;   
}

static int camera_rotation(uint32_t agree, uint32_t width, uint32_t height, uint32_t in_addr, uint32_t out_addr)
{
	int fd = -1; 
	ROTATION_PARAM_T rot_params;
	
	rot_params.data_format = ROTATION_YUV420;
	switch(agree){
		case 90:
			rot_params.rotation_dir = ROTATION_90;
			break;
		case 180:
			rot_params.rotation_dir = ROTATION_180;
			break;
		case 270:
			rot_params.rotation_dir = ROTATION_270;
			break;
		default: 
			rot_params.rotation_dir = ROTATION_DIR_MAX;
			break;			
	}
	rot_params.img_size.w = width;
	rot_params.img_size.h = height;
	rot_params.src_addr.y_addr = in_addr;
	rot_params.src_addr.uv_addr = rot_params.src_addr.y_addr + rot_params.img_size.w * rot_params.img_size.h;
	rot_params.src_addr.v_addr = rot_params.src_addr.uv_addr;
	rot_params.dst_addr.y_addr = out_addr;
	rot_params.dst_addr.uv_addr = rot_params.dst_addr.y_addr + rot_params.img_size.w * rot_params.img_size.h;
	rot_params.dst_addr.v_addr = rot_params.dst_addr.uv_addr;
	
	fd = open("/dev/sc8800g_rotation", O_RDWR /* required */, 0); 
	if (-1 == fd) 
	{   
		LOGE("Fail to open rotation device.");
        	return -1;   
   	} 
	
	//done	 
	if (-1 == ioctl(fd, SC8800G_ROTATION_DONE, &rot_params))   
	{
		LOGE("Fail to SC8800G_ROTATION_DONE");
		return -1;
	} 

	if(-1 == close(fd))   
	{   
		LOGE("Fail to close rotation device.");
        		return -1;   
   	 } 
    	fd = -1;  
	return 0;
}

static int camera_cap_zoom_colorformat(SCALE_DATA_FORMAT_E output_fmt, uint32_t output_width, uint32_t output_height, uint32_t output_addr, ZOOM_TRIM_RECT_T *trim_rect, uint32_t input_addr, SCALE_DATA_FORMAT_E input_fmt)
{
	static int fd = -1; 	
	SCALE_CONFIG_T scale_config;
	SCALE_SIZE_T scale_size;
	SCALE_RECT_T scale_rect;
	SCALE_ADDRESS_T scale_address;
	SCALE_DATA_FORMAT_E data_format;
	//uint32_t sub_sample_en;
	SCALE_MODE_E scale_mode;
	uint32_t enable = 0, mode;
	uint32_t slice_height = 0;

	fd = open("/dev/sc8800g_scale", O_RDONLY);//O_RDWR /* required */, 0);  
	if (-1 == fd) 
	{   
		LOGE("Fail to open scale device.");
        	return -1;   
   	 }
    	
	//set mode
	scale_config.id = SCALE_PATH_MODE;	
	scale_mode = SCALE_MODE_SCALE;
	scale_config.param = &scale_mode;	 
	if (-1 == ioctl(fd, SCALE_IOC_CONFIG, &scale_config))   
	{
		LOGE("Fail to SCALE_IOC_CONFIG: id=%d", scale_config.id);
		return -1;
	}
	//set input data format
	scale_config.id = SCALE_PATH_INPUT_FORMAT; 
	data_format = input_fmt;
	scale_config.param = &data_format;	 
	if (-1 == ioctl(fd, SCALE_IOC_CONFIG, &scale_config))   
	{
		LOGE("Fail to SCALE_IOC_CONFIG: id=%d", scale_config.id);
		return -1;
	}
	//set output data format
	scale_config.id = SCALE_PATH_OUTPUT_FORMAT;
	data_format = output_fmt;
	scale_config.param = &data_format;	 
	if (-1 == xioctl(fd, SCALE_IOC_CONFIG, &scale_config))   
	{
		LOGE("Fail to SCALE_IOC_CONFIG: id=%d", scale_config.id);
		return -1;
	}
	//set input size
	scale_config.id = SCALE_PATH_INPUT_SIZE;
	scale_size.w = trim_rect->w;
	scale_size.h = trim_rect->h;	 
	scale_config.param = &scale_size;	 
	if (-1 == xioctl(fd, SCALE_IOC_CONFIG, &scale_config))   
	{
		LOGE("Fail to SCALE_IOC_CONFIG: id=%d", scale_config.id);
		return -1;
	}
	//set output size
	scale_config.id = SCALE_PATH_OUTPUT_SIZE;
	scale_size.w = output_width;
	scale_size.h = output_height;
	scale_config.param = &scale_size;	 
	if (-1 == xioctl(fd, SCALE_IOC_CONFIG, &scale_config))   
	{
		LOGE("Fail to SCALE_IOC_CONFIG: id=%d", scale_config.id);
		return -1;
	}	
	//set input size
	scale_config.id = SCALE_PATH_INPUT_RECT;
	scale_rect.x = 0;//trim_rect->x;
	scale_rect.y = 0;//trim_rect->y;
	scale_rect.w = trim_rect->w;
	scale_rect.h = trim_rect->h;
	scale_config.param = &scale_rect;	 
	if (-1 == xioctl(fd, SCALE_IOC_CONFIG, &scale_config))   
	{
		LOGE("Fail to SCALE_IOC_CONFIG: id=%d", scale_config.id);
		return -1;
	}
	//set input address
	scale_config.id = SCALE_PATH_INPUT_ADDR;	 
	scale_address.yaddr = input_addr; 
	scale_address.uaddr = input_addr + trim_rect->w * trim_rect->h;	 
	scale_address.vaddr = scale_address.uaddr;
	scale_config.param = &scale_address;	 
	LOGV("INTERPOLATION:scale input y addr:0x%x,uv addr:0x%x.",scale_address.yaddr,scale_address.uaddr);
	if (-1 == xioctl(fd, SCALE_IOC_CONFIG, &scale_config))   
	{
		LOGE("Fail to SCALE_IOC_CONFIG: id=%d", scale_config.id);
		return -1;
	}
	//set output address
	scale_config.id = SCALE_PATH_OUTPUT_ADDR;
	scale_address.yaddr = output_addr; 
	scale_address.uaddr = output_addr + output_width * output_height;
	scale_address.vaddr = scale_address.uaddr; 
	scale_config.param = &scale_address;	 
	LOGV("INTERPOLATION:scale out  y addr:0x%x,uv addr:0x%x.",scale_address.yaddr,scale_address.uaddr);;
	if (-1 == xioctl(fd, SCALE_IOC_CONFIG, &scale_config))   
	{
		LOGE("Fail to SCALE_IOC_CONFIG: id=%d", scale_config.id);
		return -1;
	}
	//set input endian
	scale_config.id = SCALE_PATH_INPUT_ENDIAN;
	mode = 1;
	scale_config.param = &mode;	 
	if (-1 == xioctl(fd, SCALE_IOC_CONFIG, &scale_config))   
	{
		LOGE("Fail to SCALE_IOC_CONFIG: id=%d", scale_config.id);
		return -1;
	}
	//set output endian
	scale_config.id = SCALE_PATH_OUTPUT_ENDIAN;
	mode = 1;
	scale_config.param = &mode;	 
	if (-1 == xioctl(fd, SCALE_IOC_CONFIG, &scale_config))   
	{
		LOGE("Fail to SCALE_IOC_CONFIG: id=%d", scale_config.id);
		return -1;
	}	
	
	//done	 
	if (-1 == xioctl(fd, SCALE_IOC_DONE, 0))   
	{
		LOGE("Fail to SCALE_IOC_DONE");
		return -1;
	}

	if(-1 == close(fd))   
	{   
		LOGE("Fail to close scale device.");
        	return -1;   
   	 } 
    	fd = -1;   

	return 0;
}

static void RGBRotate90_anticlockwise(uint8_t *des,uint8_t *src,int width,int height)
{
	if ((!des)||(!src))
	{
		return;
	}    

	int n = 0;
	int linesize = width*4;
	int i,j;

	for(j = 0;j < width ;j++)
	{
		for(i= height;i>0;i--)
		{ 
			memcpy(&des[n],&src[linesize*(i-1)+j*4],4);							
			n+=4;
		}
	}
}
static void add_pitch(uint8_t *des,uint8_t *src,int width,int height)
{
	if ((!des)||(!src))
	{
		return;
	}    

	int n = 0;
	int linesize = width*4;
	int i,j;

	for(j = 0;j < height ;j++)
	{
		memcpy(&des[n],&src[j*linesize],linesize);
		n += 480*4;
	}
}

static void scale_up(uint8_t *des,uint8_t *src,int width,int height)
{
	if ((!des)||(!src))
	{
		return;
	}    

	int n = 0;
	int linesize = width*4;
	int i,j;

	for(j = 0;j < height;j++)
	{
		for(i = 0; i < width; i++){
			memcpy(&des[n],&src[j*linesize + i * 4],4);
			memcpy(&des[n + 4],&src[j*linesize + i * 4],4);
			n += 8;
		}			
		memcpy(&des[n],&des[j*linesize*2],linesize * 2);
		n += linesize * 2;
	}
}
    

static void YUVRotate90(uint8_t *des,uint8_t *src,int width,int height)
{
int i=0,j=0,n=0;
int hw=width/2,hh=height/2;

	for(j=width;j>0;j--) 
		for(i=0;i<height;i++) 
		{
			des[n++] = src[width*i+j];
		}    
	unsigned char *ptmp = src+width*height;
	for(j=hw;j>0;j--)
		for(i=0;i<hh;i++) 
		{ 
			des[n++] = ptmp[hw*i+j];
		}

	ptmp = src+width*height*5/4;
	for(j=hw;j>0;j--)
		for(i=0;i<hh;i++) 
		{
			des[n++] = ptmp[hw*i+j];
		}     
}


static void yuv420_to_rgb565(int width, int height, unsigned char *src, unsigned int *dst)
{
    int frameSize = width * height;
	int j = 0, yp = 0, i = 0;

    unsigned char *yuv420sp = src;
    for (j = 0, yp = 0; j < height; j++) {
        int uvp = frameSize + (j >> 1) * width, u = 0, v = 0;
        for (i = 0; i < width; i++, yp++) {
            int y = (0xff & ((int) yuv420sp[yp])) - 16;
            if (y < 0) y = 0;
            if ((i & 1) == 0) {
                v = (0xff & yuv420sp[uvp++]) - 128;
                u = (0xff & yuv420sp[uvp++]) - 128;
            }

            int y1192 = 1192 * y;
            int r = (y1192 + 1634 * v);
            int g = (y1192 - 833 * v - 400 * u);
            int b = (y1192 + 2066 * u);

            if (r < 0) r = 0; else if (r > 262143) r = 262143;
            if (g < 0) g = 0; else if (g > 262143) g = 262143;
            if (b < 0) b = 0; else if (b > 262143) b = 262143;

            dst[yp] = ((((r << 6) & 0xff0000)>>16)<<16)|(((((g >> 2) & 0xff00)>>8))<<8)|((((b >> 10) & 0xff))<<0);
        }
    } 
}


static int eng_atvtest_camera_open(void)
{  
	int counter=5;
	int ret=0;
	SPRD_DBG("ATV: %s.", __func__);

	while((v4l2_fd<0)&&(counter>0)) {
		v4l2_fd = open(SPRD_ATV_DEV, O_RDWR /* required */, 0);
		if (v4l2_fd < 0) {  
	        SPRD_DBG("ATV: Cannot open '%s': %d, %s", SPRD_ATV_DEV, errno,  strerror(errno));   
			usleep(10000);
		}
	    else
	      SPRD_DBG("ATV: OK to open device.");
		counter--;
	}

	if(counter <= 0)
		ret = -1;


	SPRD_DBG("ATV: %s, ret=%d", __func__,ret);
	return ret;
}

static void *eng_atvtest_pmem(int size, struct pmem_region *region)
{
	void *pmem_base=NULL;

	if(pmem_fd == -1)
		pmem_fd = open(SPRD_PMEM_DEV, O_RDWR, 0);
	
	if (pmem_fd >= 0)
	{

		if (xioctl(pmem_fd, PMEM_GET_TOTAL_SIZE, region) < 0) 
		{
			SPRD_DBG("ATV: PMEM_GET_TOTAL_SIZE failed\n");
		}


		pmem_base = mmap(0, size, PROT_READ|PROT_WRITE, MAP_SHARED, pmem_fd, 0);
		if (pmem_base == MAP_FAILED)
		{
			pmem_base = 0;
			close(pmem_fd);
			pmem_fd = -1;
			SPRD_DBG("ATV: mmap pmem error!\n");
		}

		region->len = size;
		if(xioctl(pmem_fd, PMEM_GET_PHYS, region) < 0)
		{
			SPRD_DBG("ATV: PMEM_GET_PHYS failed\n");
		}

	}

	return pmem_base;
}

static int eng_atvtest_preview_scale_colorformat(SCALE_DATA_FORMAT_E output_fmt, uint32_t output_width, uint32_t output_height, uint32_t output_addr, SCALE_DATA_FORMAT_E input_fmt,uint32_t input_width, uint32_t input_height, uint32_t input_addr)
{
	static int fd = -1; 	
	SCALE_CONFIG_T scale_config;
	SCALE_SIZE_T scale_size;
	SCALE_RECT_T scale_rect;
	SCALE_ADDRESS_T scale_address;
	SCALE_DATA_FORMAT_E data_format;	 
	SCALE_MODE_E scale_mode;
	uint32_t enable = 0, mode;
	uint32_t slice_height = 0;

	SPRD_DBG("ATV: %s.1", __func__);

	fd = open("/dev/sc8800g_scale", O_RDONLY);
	if (-1 == fd) 
	{   
		LOGE("[SPRD OEM ERR]:camera_interpolation,Fail to open scale device.\n");
        	return -1;   
   	 }
    	
	//set mode
	scale_config.id = SCALE_PATH_MODE;	
	scale_mode = SCALE_MODE_SCALE;
	scale_config.param = &scale_mode;	 
	if (-1 == xioctl(fd, SCALE_IOC_CONFIG, &scale_config))   
	{
		LOGE("[SPRD OEM ERR]:camera_interpolation,Fail to SCALE_IOC_CONFIG: id=%d", scale_config.id);
		return -1;
	}
	//set input data format
	scale_config.id = SCALE_PATH_INPUT_FORMAT;	
	data_format = input_fmt;
	scale_config.param = &data_format;	 
	if (-1 == xioctl(fd, SCALE_IOC_CONFIG, &scale_config))   
	{
		LOGE("Fail to SCALE_IOC_CONFIG: id=%d", scale_config.id);
		return -1;
	}
	SPRD_DBG("ATV: %s.2", __func__);
	//set output data format
	scale_config.id = SCALE_PATH_OUTPUT_FORMAT;
	data_format = output_fmt;
	scale_config.param = &data_format;	 
	if (-1 == xioctl(fd, SCALE_IOC_CONFIG, &scale_config))   
	{
		LOGE("[SPRD OEM ERR]:camera_interpolation,Fail to SCALE_IOC_CONFIG: id=%d", scale_config.id);
		return -1;
	}
	//set input size
	scale_config.id = SCALE_PATH_INPUT_SIZE;
	scale_size.w = input_width;// trim_rect->w;
	scale_size.h = input_height;//trim_rect->h;	
	scale_config.param = &scale_size;	 
	if (-1 == xioctl(fd, SCALE_IOC_CONFIG, &scale_config))   
	{
		LOGE("[SPRD OEM ERR]:camera_interpolation,Fail to SCALE_IOC_CONFIG: id=%d", scale_config.id);
		return -1;
	}
	SPRD_DBG("ATV: %s.3", __func__);
	//set output size
	scale_config.id = SCALE_PATH_OUTPUT_SIZE;
	scale_size.w = output_width;
	scale_size.h = output_height;
	scale_config.param = &scale_size;	 
	if (-1 == xioctl(fd, SCALE_IOC_CONFIG, &scale_config))   
	{
		LOGE("[SPRD OEM ERR]:camera_interpolation,Fail to SCALE_IOC_CONFIG: id=%d", scale_config.id);
		return -1;
	}	
	//set input size
	scale_config.id = SCALE_PATH_INPUT_RECT;
	scale_rect.x = 0;//trim_rect->x;
	scale_rect.y = 0;//trim_rect->y;
	scale_rect.w = input_width;//trim_rect->w;
	scale_rect.h = input_height;//trim_rect->h;
	scale_config.param = &scale_rect;	 
	if (-1 == xioctl(fd, SCALE_IOC_CONFIG, &scale_config))   
	{
		LOGE("[SPRD OEM ERR]:camera_interpolation,Fail to SCALE_IOC_CONFIG: id=%d", scale_config.id);
		return -1;
	}
	//set input address
	scale_config.id = SCALE_PATH_INPUT_ADDR; 
	scale_address.yaddr = input_addr; 
	scale_address.uaddr = input_addr + input_width * input_height; 
	scale_address.vaddr = scale_address.uaddr;
	scale_config.param = &scale_address;	 
	//LOGV("[SPRD OEM]:camera_interpolation,scale input y addr:0x%x,uv addr:0x%x.",scale_address.yaddr,scale_address.uaddr);
	if (-1 == xioctl(fd, SCALE_IOC_CONFIG, &scale_config))   
	{
		LOGE("[SPRD OEM ERR]:camera_interpolation,Fail to SCALE_IOC_CONFIG: id=%d", scale_config.id);
		return -1;
	}
	//set output address
	scale_config.id = SCALE_PATH_OUTPUT_ADDR;
	scale_address.yaddr = output_addr; 
	scale_address.uaddr = output_addr + output_width * output_height;
	scale_address.vaddr = scale_address.uaddr; 
	scale_config.param = &scale_address;	 
	//LOGV("[SPRD OEM]:camera_interpolation,scale out  y addr:0x%x,uv addr:0x%x.",scale_address.yaddr,scale_address.uaddr);;
	if (-1 == xioctl(fd, SCALE_IOC_CONFIG, &scale_config))   
	{
		LOGE("[SPRD OEM ERR]:camera_interpolation,Fail to SCALE_IOC_CONFIG: id=%d", scale_config.id);
		return -1;
	}

	//set input endian
	scale_config.id = SCALE_PATH_INPUT_ENDIAN;
	mode = 1;
	scale_config.param = &mode;	 
	if (-1 == xioctl(fd, SCALE_IOC_CONFIG, &scale_config))   
	{
		LOGE("[SPRD OEM ERR]:camera_interpolation,Fail to SCALE_IOC_CONFIG: id=%d", scale_config.id);
		return -1;
	}
	//set output endian
	scale_config.id = SCALE_PATH_OUTPUT_ENDIAN;
	mode = 1;
	scale_config.param = &mode;	 
	if (-1 == xioctl(fd, SCALE_IOC_CONFIG, &scale_config))   
	{
		LOGE("[SPRD OEM ERR]:camera_interpolation,Fail to SCALE_IOC_CONFIG: id=%d", scale_config.id);
		return -1;
	}	
	SPRD_DBG("ATV: %s.6", __func__);
	//done	 
	if (-1 == xioctl(fd, SCALE_IOC_DONE, 0))   
	{
		LOGE("[SPRD OEM ERR]:camera_interpolation,Fail to SCALE_IOC_DONE");
		return -1;
	}
	SPRD_DBG("ATV: %s.7", __func__);
	if(-1 == close(fd))   
	{   
		LOGE("[SPRD OEM ERR]:camera_interpolation,Fail to close scale device.");
        		return -1;   
   	 } 
	SPRD_DBG("ATV: %s.8", __func__);
    	fd = -1;   

	return 0;
}
static int eng_atvtest_align_page(int size)
{
	int buffer_size, page_size;
	page_size = getpagesize();   
	buffer_size = size;
	buffer_size = (buffer_size + page_size - 1) & ~(page_size - 1); 
	SPRD_DBG("ATV: page_size=%d; buffer_size=%d\n", page_size, buffer_size);
	return buffer_size;
}


static int eng_atvtest_fb_open(void)
{
	int i;
	void *bits;
	int offset_page_align;

	if(fb_fd==-1)	
		fb_fd = open(SPRD_ATV_FB_DEV,O_RDWR);
	
	if(fb_fd<0) {
        SPRD_DBG("ATV: %s Cannot open '%s': %d, %s", __func__, SPRD_ATV_FB_DEV, errno,  strerror(errno));   
		return -1;
	}

    if(ioctl(fb_fd, FBIOGET_FSCREENINFO,&fix))
    {
        SPRD_DBG("ATV: %s failed to get fix\n",__func__);
        close(fb_fd);
        return -1;
    }

    if(ioctl(fb_fd, FBIOGET_VSCREENINFO, &var))
    {
        SPRD_DBG("ATV: %s failed to get var\n",__func__);
        close(fb_fd);
        return -1;
    }	

	SPRD_DBG("%s: fix.smem_len=%d\n",__func__, fix.smem_len);

	bits = mmap(0, fix.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
    if (bits == MAP_FAILED) {
        SPRD_DBG("ATV: failed to mmap framebuffer");
        return -1;
    }

	SPRD_DBG("%s: var.yres=%d; var.xres=%d\n",__func__, var.yres, var.xres);

	memset(&sprd_pmem_region, 0, sizeof(sprd_pmem_region));
	sprd_pmem_base=eng_atvtest_pmem(var.yres*var.xres*SPRD_ATV_TYPE*3, &sprd_pmem_region);
	
#if 1
	offset_page_align = eng_atvtest_align_page(var.yres * var.xres * SPRD_ATV_TYPE);
	memset(&preview_buf, 0, sizeof(preview_buf));
	preview_buf[0].virt_addr= (uint32_t)sprd_pmem_base;
	preview_buf[0].phys_addr = sprd_pmem_region.offset;
	preview_buf[0].length = offset_page_align;
	
	preview_buf[1].virt_addr= (uint32_t)(((unsigned) sprd_pmem_base) + offset_page_align);
	preview_buf[1].phys_addr = sprd_pmem_region.offset + offset_page_align;
	preview_buf[1].length = offset_page_align;

	preview_buf[2].virt_addr= (uint32_t)(((unsigned) sprd_pmem_base) + offset_page_align * 2);
	preview_buf[2].phys_addr = sprd_pmem_region.offset + offset_page_align * 2;
	preview_buf[2].length = offset_page_align;


	for(i=0; i<2; i++){
		SPRD_DBG("ATV: preview_buf[%d] virt_addr=0x%x, phys_addr=0x%x, length=%d", \
			i, preview_buf[i].virt_addr,preview_buf[i].phys_addr,preview_buf[i].length);
	}
#endif

	//set framebuffer address
	memset(&fb_buf, 0, sizeof(fb_buf));
	fb_buf[0].virt_addr = (uint32_t)bits;
    fb_buf[0].phys_addr = fix.smem_start;
	fb_buf[0].length = var.yres * var.xres * 4;
	
    fb_buf[1].virt_addr = (uint32_t)(((unsigned) bits) + var.yres * var.xres * 4);
	fb_buf[1].phys_addr = fix.smem_start+ var.yres * var.xres * 4;
	fb_buf[1].length = var.yres * var.xres * 4;

    fb_buf[2].virt_addr = tmpbuf;
	fb_buf[2].length = var.yres * var.xres * 4;

	
	for(i=0; i<3; i++){
		SPRD_DBG("ATV: buf[%d] virt_addr=0x%x, phys_addr=0x%x, length=%d", \
			i, fb_buf[i].virt_addr,fb_buf[i].phys_addr,fb_buf[i].length);
	}
	
	return 0;

}

static void eng_atvtest_set_framebuffer(unsigned n)
{
	//SPRD_DBG("ATV: active framebuffer[%d], bits_per_pixel=%d",n, var.bits_per_pixel);
    if (n > 1) return;
	
    var.yres_virtual = var.yres * 2;
    var.yoffset = n * var.yres;
  //  var.bits_per_pixel = 16;
    if (ioctl(fb_fd, FBIOPUT_VSCREENINFO, &var) < 0) {
        SPRD_DBG("ATV: active fb swap failed");
    }
}

static int eng_atvtest_streamon(void)
{	
	enum v4l2_buf_type type;
	
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;   
	if (-1 == xioctl(v4l2_fd, VIDIOC_STREAMON, &type))   
	{
		SPRD_DBG("ATV: Fail to VIDIOC_STREAMON.");
		return -1;
	}

	return 0;
}


static int eng_atvtest_streamoff(void)
{
	enum v4l2_buf_type type;
	
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;   
	if (-1 == xioctl(v4l2_fd, VIDIOC_STREAMOFF, &type))   
	{
		SPRD_DBG("ATV: Fail to VIDIOC_STREAMOFF.");
		return -1;
	}

	return 0;
}


static int eng_atvtest_camera_init(int front_back) 
{   
    struct v4l2_capability cap;   
    struct v4l2_cropcap cropcap;   
    struct v4l2_crop crop;   
    struct v4l2_format fmt;   
	struct v4l2_streamparm streamparm;	
	struct v4l2_requestbuffers req;
	enum v4l2_buf_type type;
    unsigned int min;  
	int i;

	/***********************VIDIOC_S_PARM*******************/		
	memset(&streamparm, 0, sizeof(streamparm));   
	streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	streamparm.parm.capture.capturemode = 0;

	streamparm.parm.raw_data[199] = 1;
	streamparm.parm.raw_data[198] = 5;

	if (-1 == xioctl(v4l2_fd, VIDIOC_S_PARM, &streamparm)) 
	{
		SPRD_DBG("ATV: Fail to VIDIOC_S_PARM.");
		return -1;
	}



	/***********************VIDIOC_S_FMT*******************/	
	memset(&fmt, 0, sizeof(fmt));   
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;   
	fmt.fmt.pix.width = 320;//SPRD_ATV_DISPLAY_WIDTH;   
    fmt.fmt.pix.height = 240;//SPRD_ATV_DISPLAY_HEIGHT; 
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420; 
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;   
       
	if (-1 == xioctl(v4l2_fd, VIDIOC_S_FMT, &fmt))   
	{
		SPRD_DBG("ATV: Fail to VIDIOC_S_FMT.");
		return -1;
	}
	
    SPRD_DBG("ATV: fmt.fmt.pix.width = %d", fmt.fmt.pix.width);   
    SPRD_DBG("ATV: fmt.fmt.pix.height = %d", fmt.fmt.pix.height);   
    SPRD_DBG("ATV: fmt.fmt.pix.sizeimage = %d", fmt.fmt.pix.sizeimage);   
    SPRD_DBG("ATV: fmt.fmt.pix.bytesperline = %d", fmt.fmt.pix.bytesperline);   


	/***********************VIDIOC_REQBUFS*******************/	
	memset(&req, 0, sizeof(req));
    req.count = SPRD_MAX_PREVIEW_BUF;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_USERPTR;

    if (-1 == xioctl(v4l2_fd, VIDIOC_REQBUFS, &req))
    {
		SPRD_DBG("ATV: Fail to VIDIOC_REQBUFS");
		return -1;
    }


	/***********************VIDIOC_CROPCAP*******************/	
    memset(&cropcap, 0, sizeof(cropcap));  
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;   
    if (0 == xioctl(v4l2_fd, VIDIOC_CROPCAP, &cropcap)) {   
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;   
        crop.c.left = 0;   
        crop.c.top = 0;   
        crop.c.width = 320;//SPRD_ATV_DISPLAY_WIDTH;   //wxz: 320x480 ???
        crop.c.height = 240;//SPRD_ATV_DISPLAY_HEIGHT;   
		
        SPRD_DBG("ATV: has ability to crop!!");   
		
        if (-1 == xioctl(v4l2_fd, VIDIOC_S_CROP, &crop)) {   
            switch (errno) {   
            case EINVAL:   
                /* Cropping not supported. */   
                break;   
            default:   
                /* Errors ignored. */   
                break;   
            }   
            SPRD_DBG("ATV: but crop to (%d, %d, %d, %d) Failed!!",   
                    crop.c.left, crop.c.top, crop.c.width, crop.c.height);   
        } else {   
            SPRD_DBG("ATV: sussess crop to (%d, %d, %d, %d)", crop.c.left,   
                    crop.c.top, crop.c.width, crop.c.height);   
        }   
    } else {   
        /* Errors ignored. */   
        SPRD_DBG("ATV: !! has no ability to crop!!");   
    }   


#if 0
	/***********************VIDIOC_QBUF*******************/	
	for (i=0; i<SPRD_MAX_PREVIEW_BUF; i++) 
	{   
		struct v4l2_buffer buf;   
		memset(&buf, 0, sizeof(buf));   
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;   
		buf.memory = V4L2_MEMORY_USERPTR;   
		buf.index = i;   
		buf.m.userptr = preview_buf[i].phys_addr;               
		buf.length = preview_buf[i].length;
		SPRD_DBG("ATV: preview QBuf: buffers[%d].start: %lx, %x\n", buf.index, buf.m.userptr,buf.length);	
		if (-1 == xioctl(v4l2_fd, VIDIOC_QBUF, &buf))   
		{
			SPRD_DBG("ATV: Fail to VIDIOC_QBUF from camera_preview_qbuffers.");
			return -1;
		} 
     } 	

	/***********************VIDIOC_STREAMON*******************/	
	eng_atvtest_streamon();
#endif
	SPRD_DBG("ATV: %s OK",__func__);
    ////////////crop finished!

	return 0;
}


static int eng_atvtest_camera_start(void) 
{
	int i;

	/***********************VIDIOC_QBUF*******************/	
	for (i=0; i<SPRD_MAX_PREVIEW_BUF; i++) 
	{   
		struct v4l2_buffer buf;   
		memset(&buf, 0, sizeof(buf));   
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;   
		buf.memory = V4L2_MEMORY_USERPTR;   
		buf.index = i;   
		buf.m.userptr = preview_buf[i].phys_addr;               
		buf.length = preview_buf[i].length;
		SPRD_DBG("ATV: preview QBuf: buffers[%d].start: %lx, %x\n", buf.index, buf.m.userptr,buf.length);	
		if (-1 == xioctl(v4l2_fd, VIDIOC_QBUF, &buf))   
		{
			SPRD_DBG("ATV: Fail to VIDIOC_QBUF from camera_preview_qbuffers.");
			return -1;
		} 
     } 	

	/***********************VIDIOC_STREAMON*******************/	
	eng_atvtest_streamon();

	SPRD_DBG("ATV: %s OK",__func__);
    ////////////crop finished!

	return 0;

}

static int eng_atvtest_init(int front_back)
{	
	if(eng_atvtest_camera_open()<0)
		return -1;

	eng_atvtest_camera_init(front_back);

	SPRD_DBG("ATV: %s: OK",__func__);
	
	return 0;
}


static int eng_atvtest_preview_rotation(ROTATION_PARAM_T *rotation_para)
{
	int fd_temp = -1;
	
	if(fd_temp == -1)
    	fd_temp = open(HW_ROTATION_DEV, O_RDWR /* required */, 0);      

	SPRD_DBG("%s -- 1",__func__);

	if (fd_temp < 0) {   
        SPRD_DBG("ATV: Cannot open '%s': %d, %s", HW_ROTATION_DEV, errno,  strerror(errno));   
        return -1;  
    }
    else
      SPRD_DBG("ATV: OK to open rotation device.");  

	SPRD_DBG("%s -- 2",__func__);
	if (-1 == xioctl(fd_temp, SC8800G_ROTATION_DONE, rotation_para)) 
	{
		SPRD_DBG("ATV: Fail to VIDIOC_DQBUF.");
		return -1;
	} 	

	SPRD_DBG("%s -- 3",__func__);
	if(-1 == close(fd_temp))   
	{   
		SPRD_DBG("Fail to close rotation device.");
        	return -1;   
   	 } 
    	fd_temp = -1;

	SPRD_DBG("%s -- 4",__func__);
	return 0;
}

static int eng_atvtest_preview(void)
{
	struct v4l2_buffer buf;
	uint32_t i, counter;
	ROTATION_PARAM_T rotation_dir;
	
	SPRD_DBG("ATV: %s.", __func__);
	while(eng_camera_start == 1){
		/***********************VIDIOC_DQBUF*******************/	
		//SPRD_DBG("ATV: %s.1", __func__);
		memset(&buf, 0, sizeof(buf));   
    	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;   
    	buf.memory = V4L2_MEMORY_USERPTR;   	
        if (-1 == xioctl(v4l2_fd, VIDIOC_DQBUF, &buf)) 
		{
			SPRD_DBG("ATV: Fail to VIDIOC_DQBUF.");
			return -1;
		}  
		//SPRD_DBG("ATV: %s.2", __func__);

		for (i=0; i < SPRD_MAX_PREVIEW_BUF; i++)   
		{
			if (buf.m.userptr == preview_buf[i].phys_addr)
			{	
				g_releasebuff_index = i;
				break;   
			}
		}
		
		usleep(100000);

#if 0
		yuv420_to_rgb565(SPRD_ATV_DISPLAY_WIDTH,SPRD_ATV_DISPLAY_HEIGHT, preview_buf[g_releasebuff_index].virt_addr, \
			fb_buf[2].virt_addr);

		RGBRotate90_anticlockwise(fb_buf[g_releasebuff_index].virt_addr, fb_buf[2].virt_addr,
			SPRD_ATV_DISPLAY_WIDTH, SPRD_ATV_DISPLAY_HEIGHT);
#else
#if 0
		yuv420_to_rgb565(320,240, preview_buf[g_releasebuff_index].virt_addr, \
			fb_buf[2].virt_addr);

		RGBRotate90_anticlockwise(fb_buf[g_releasebuff_index].virt_addr, fb_buf[2].virt_addr,
			320, 240);

		add_pitch(fb_buf[g_releasebuff_index].virt_addr, fb_buf[2].virt_addr,
			320, 240);

#endif		
		yuv420_to_rgb565(320,240, preview_buf[g_releasebuff_index].virt_addr, \
			fb_buf[g_releasebuff_index].virt_addr);

		RGBRotate90_anticlockwise(fb_buf[2].virt_addr, fb_buf[g_releasebuff_index].virt_addr,
			320, 240);

		scale_up(fb_buf[g_releasebuff_index].virt_addr, fb_buf[2].virt_addr,
			240, 320);

#endif
		//SPRD_DBG("ATV: %s.4", __func__);
		
		eng_atvtest_set_framebuffer(g_releasebuff_index);
		/***********************VIDIOC_QBUF*******************/		
		memset(&buf, 0, sizeof(buf));   
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;   
		buf.memory = V4L2_MEMORY_USERPTR;   
		buf.index = g_releasebuff_index;   
		buf.m.userptr = preview_buf[g_releasebuff_index].phys_addr;               
		buf.length = preview_buf[g_releasebuff_index].length;
		//SPRD_DBG("ATV: preview QBuf: buffers[%d].start: %lx, %x\n", buf.index, buf.m.userptr,buf.length);	
		if (-1 == xioctl(v4l2_fd, VIDIOC_QBUF, &buf))   
		{
			SPRD_DBG("ATV: Fail to VIDIOC_QBUF from camera_preview_qbuffers.");
			return -1;
		} 
		//SPRD_DBG("ATV: %s.5", __func__);
    }
	munmap(sprd_pmem_base, var.yres*var.xres*SPRD_ATV_TYPE*3);
	munmap(fb_buf[0].virt_addr, fix.smem_len);
	
	SPRD_DBG("ATV: Stop preview");

	return 0;
}


static void eng_atvtest_show_result(void)
{
	int start_x = gr_fb_width()/2 - 100;
	int start_y = gr_fb_height()/2;
	
	SPRD_DBG("%s", __func__);
	
    sprd_ui_fill_locked();

	//title
	gr_color(0, 255, 0, 255);
	sprd_ui_draw_text(start_x, start_y, SPRD_ATVTEST_PASS);

	//update
	gr_flip();
}


static void eng_atvtest_close(void)
{
	SPRD_DBG("%s: %d,%d,%d\n",__func__, v4l2_fd, fb_fd ,pmem_fd);

	if(v4l2_fd >= 0){
		close(v4l2_fd);
		v4l2_fd = -1;
	}
	
	if(fb_fd >= 0) {
		close(fb_fd);
		fb_fd = -1;
	}
	
	if(pmem_fd >= 0) {
		close(pmem_fd);
		pmem_fd = -1;
	}
	
}

static void eng_atvtest_entry(void *par)
{	
	struct timeval time;
	unsigned int schannel=0;
	unsigned int echannel=0;
	int i;
	
	time.tv_sec = 1;
	time.tv_usec = 0;
	
	select(1, NULL, NULL, NULL, &time);
	
	eng_atvtest_fb_open();


	if(eng_atvtest_init(5)<0)
		return NULL;


	atv_nmi600_poweron_init();

	atv_nmi600_set_region(1, &schannel, &echannel);

	for(i=schannel;i<=echannel;i++)
	{
		if(1 == atv_nmi600_scan_one_channel(i))
		{
			break;
		}
	}

	SPRD_DBG("ATV: Lock channel is:%d", i);

	atv_nmi600_fast_set_channel(i);

	eng_atvtest_camera_start();

	eng_atvtest_setaudio(1);
	eng_atvtest_preview();

	atv_nmi600_poweroff_deinit();
	eng_atvtest_setaudio(0);

	eng_atvtest_streamoff();

	eng_atvtest_close();

	return NULL;
}

extern int eng_stop_service(char *name); 

int eng_atvtest_start(void)
{
	int ret;
	struct timeval time;
	pthread_t t;
	
	time.tv_sec = 1;
	time.tv_usec =0;
	
	v4l2_fd = -1;
	fb_fd = -1;
	pmem_fd = -1;

    eng_stop_service("media");

	eng_camera_start = 1;
	pthread_create(&t, NULL, (void*)eng_atvtest_entry, NULL);
	eng_wait_anykey();
	eng_camera_start = 0;
	pthread_join(t, NULL);  
	/*wait "handle key" thread exit. */
	eng_atvtest_show_result();

	eng_draw_handle_softkey(ENG_ITEM_ATV);

	return 0;	
}


/* atv区域对应表
China<中国>=1,
Usa<美国>,
Canada<加拿大>,
Korea<韩国>,
Taiwan<中国台湾>,
Mexico<墨西哥>,
Chile<智利>,
Venezuela<委内瑞拉>,
Philippines<菲律宾>,
Vietnam<越南>=10,
Japan<日本>,
Uk<英国>,
HongKong<中国香港>,
SAfrica<南非>,
Brazil<巴西>,
ChinaSZ<中国深圳>,
UsCable<UsCable>,
WesternEurope<西欧>,
Argentina<阿根廷>,
Turkey<土耳其>=20,
UAE<阿联酋>,
Afghanistan<阿富汗>,
Singapore<新加坡>,
Thailand<泰国>,
Cambodia<柬埔寨>,
Indonesia<印尼>,
Malaysia<马来西亚>,
India<印度>,
Pakistan<巴基斯坦>,
Portugal<葡萄牙>=30,
Spain<西班牙>,
Laos<老挝>,
Egypt<埃及>,
Poland<波兰>,
Russia_d_k<俄罗斯_d_k>,
Russia_b_g<俄罗斯_b_g>,
Israel<以色列>,
Holland<荷兰>,
Azerbaijan<阿塞拜疆>,
Tanzania<坦桑尼亚>=40,
Namibia<纳米比亚>,
Colombia<哥伦比亚>,
Bolivia<玻利维亚>,
Peru<秘鲁>,
Brunei<文莱>,
Kuwait<科威特>,
Oman<阿曼>,
Yemen<也门>,
Jordan<约旦>,
Cyprus<塞浦路斯>=50,
Gambia<冈比亚>,
SierraLeone<塞拉利昂>,
Ghana<加纳>,
Nigeria<尼日利亚>,
Cameroon<喀麦隆>,
Somali<索马里>,
Kenya<肯尼亚>,
Uganda<乌干达>,
Zambia<赞比亚>,
Swaziland<斯威士兰>=60,
Bangladesh<孟加拉国>,
Algeria<阿尔及利亚>,
Liberia<利比里亚>,
SriLanka<斯里兰卡>,
Qatar<卡塔尔>,
Sudan<苏丹>,
Paraguay<巴拉圭>,
Uruguay<乌拉圭>,
Angola<安哥拉>,
Congo<刚果>=70,
Myanmar<缅甸>,
EastTimor<东帝汶>,
Iran<伊朗>,
Maldives<马尔代夫>,
Mongolia<蒙古>,
Syria<叙利亚>,
SaudiArabia<沙特阿拉伯>,
Iraq<伊拉克>,
Lebanon<黎巴嫩>,
Macao<中国澳门>=80,
Libya<利比亚>,
Zimbabwe<津巴布韦>,
Mali<马里>,
Madagascar<马达加斯加>,
Senegal<塞内加尔>,
Morocco<摩洛哥>,
Tunisia<突尼斯>,
Guyana<圭亚那>,
Surinam<苏里南>,
Ecuador<厄瓜多尔>=90,
Ireland<爱尔兰>,
Austria<奥地利>,
Belgium<比利时>,
Czech<捷克>,
Denmark<丹麦>,
Finland<芬兰>,
Georgia<格鲁吉亚>,
Germany<德国>,
Greece<希腊>,
Hungary<匈牙利>=100,
Iceland<冰岛>,
Italy<意大利>,
Macedonia<马其顿>,
Madeira<马德拉>,
Malta<马耳他>,
Norway< 挪威>,
Ukraine<乌克兰>,
Romania<罗马尼亚>,
Switzerland<瑞士>,
Sweden<瑞典>=110,
Costarica<哥斯达黎加>,
Cuba<古巴>,
Dominica<多米尼加岛>,
ElSalvador<厄瓜多尔>,
Guatemala<危地马拉>,
Haiti<海地>,
Jamaica<牙买加>,
PuertoRico<波多黎各>,
Montserrat<蒙特色拉特岛>,
Nicaragua<尼加拉瓜>=120,
Panama<巴拿马>,
Australia<澳大利亚>,
Fiji<斐济>,
NewZealand<新西兰>,
Solomon<所罗门>=125,
*/
static void atcommand_atvtest_entry(unsigned int region, unsigned int channel)
{	
	struct timeval time;
	unsigned int schannel=0;
	unsigned int echannel=0;
	int i;
	
	time.tv_sec = 1;
	time.tv_usec = 0;
	
	select(1, NULL, NULL, NULL, &time);
	
	eng_atvtest_fb_open();


	if(eng_atvtest_init(5)<0)
		return NULL;


	atv_nmi600_poweron_init();

    if(region < 1) region = 1;
    if(region > 125) region = 125;
    
	atv_nmi600_set_region(region, &schannel, &echannel);
#if 0
	for(i=schannel;i<=echannel;i++)
	{
		if(1 == atv_nmi600_scan_one_channel(i))
		{
			break;
		}
	}


	SPRD_DBG("ATV: Lock channel is:%d", i);
#endif
	atv_nmi600_fast_set_channel(channel);

	eng_atvtest_camera_start();

	eng_atvtest_setaudio(1);
	eng_atvtest_preview();

	atv_nmi600_poweroff_deinit();
	eng_atvtest_setaudio(0);

	eng_atvtest_streamoff();

	eng_atvtest_close();

	return NULL;
}