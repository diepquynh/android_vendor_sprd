#include <camera/Camera.h>
#include <utils/Log.h>
#include "SprdCamera3OEMIf.h"
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
#include <media/hardware/MetadataBufferType.h>
#include "cmr_common.h"
#include "ion_sprd.h"
#include "SprdCamera3Setting.h"

#include <linux/fb.h>
#include "sprd_rot_k.h"
#include <dlfcn.h>
using namespace sprdcamera;

#include <dlfcn.h>

typedef struct {
    int width;
    int height;
    int row_bytes;
    int pixel_bytes;
    unsigned char* data;
} GRSurface;

typedef GRSurface* gr_surface;

typedef struct minui_backend {
    // Initializes the backend and returns a gr_surface to draw into.
    gr_surface (*init)(struct minui_backend*);

    // Causes the current drawing surface (returned by the most recent
    // call to flip() or init()) to be displayed, and returns a new
    // drawing surface.
    gr_surface (*flip)(struct minui_backend*);

    // Blank (or unblank) the screen.
    void (*blank)(struct minui_backend*, bool);

    // Device cleanup when drawing is done.
    void (*exit)(struct minui_backend*);
} minui_backend;

static minui_backend* gr_backend = NULL;
static GRSurface* gr_draw = NULL;

/*yuv->rgb*/
#define RGB565(r,g,b) \
    ((unsigned short)((((unsigned char)(r)>>3)|((unsigned short)(((unsigned char)(g)>>2))<<5))|(((unsigned short)((unsigned char)(b>>3)))<<11)))

/*rotation device nod*/
static int rot_fd = -1;
#define ROT_DEV "/dev/sprd_rotation"

/*process control*/
Mutex previewLock; /*preview lock*/
int previewvalid = 0; /*preview flag*/
static int s_mem_method = 1;/*0: physical address, 1: iommu  address*/
static unsigned char camera_id = 0; /*camera id: fore=1,back=0*/

/*data processing useful*/
//#define PREVIEW_WIDTH       960//640//1280//
//#define PREVIEW_HIGHT       720//480//960//
#define PREVIEW_BUFF_NUM    4  /*preview buffer*/
#define SPRD_MAX_PREVIEW_BUF    PREVIEW_BUFF_NUM
#define TEST_MAX_MISCHEAP_NUM 10
#define TEST_PREVIEW_BUF_NUM 12
struct frame_buffer_t {
    size_t phys_addr;
	size_t virt_addr;
    size_t length; //buffer's length is different from cap_image_size
};

typedef struct{
    uint32_t sensor_id;
    uint32_t cmd;
    uint32_t sub_cmd;
    uintptr_t capture_raw_vir_addr;
    uint32_t capture_width;
    uint32_t capture_height;

    sp<MemoryHeapIon> cap_pmen_hp;
    uint32_t cap_pmemory_size;
    int cap_physical_addr;
    unsigned char* cap_virtual_addr;

    sp<MemoryHeapIon> misc_heap_array[TEST_MAX_MISCHEAP_NUM];
    uint32_t misc_heap_num;

    sp<MemoryHeapIon> preview_pmen_hp[TEST_PREVIEW_BUF_NUM];
    unsigned long preview_pmemory_size[TEST_PREVIEW_BUF_NUM];
    unsigned long preview_physical_addr[TEST_PREVIEW_BUF_NUM];
    unsigned long preview_virtual_addr[TEST_PREVIEW_BUF_NUM];

    sem_t sem_cap_raw_done;
    sem_t sem_cap_jpg_done;

    void* jpg_buffer;
    uint32_t jpg_size;
}TEST_CMR_CONTEXT_T,*TEST_CMR_CONTEXT_T_PTR;

static TEST_CMR_CONTEXT_T eng_test_cmr_cxt;
static TEST_CMR_CONTEXT_T_PTR g_eng_test_cmr_cxt_ptr = &eng_test_cmr_cxt;
static bool state_in_preview = false;
static uint32_t* test_camer_saveaddr = NULL;
static int test_commandid = 0;
static int test_image_size = 0;

static struct frame_buffer_t fb_buf[SPRD_MAX_PREVIEW_BUF+1];
static uint8_t *tmpbuf2,*tmpbuf3;//*tmpbuf1, *tmpbuf,
static uint32_t post_preview_buf[PREVIEW_WIDTH*PREVIEW_HIGHT];
static struct fb_var_screeninfo var;
static uint32_t frame_num=0; /*record frame number*/
static unsigned int mPreviewHeapNum = 0; /*allocated preview buffer number*/
static sprd_camera_memory_t* mPreviewHeapReserved = NULL;
static sprd_camera_memory_t* mIspLscHeapReserved = NULL;
static sprd_camera_memory_t* mIspAFLHeapReserved = NULL;
static const int kISPB4awbCount = 16;
sprd_camera_memory_t* mIspB4awbHeapReserved[kISPB4awbCount];
sprd_camera_memory_t* mIspRawAemHeapReserved[kISPB4awbCount];
sprd_camera_memory_t* mIspRawAfmHeapReserved[kISPB4awbCount];

static sprd_camera_memory_t* previewHeapArray[PREVIEW_BUFF_NUM]; /*preview heap arrary*/

static oem_module_t   *mHalOem;

struct client_t
{
    int reserved;
};
static struct client_t client_data;
static cmr_handle oem_handle = 0;

uint32_t lcd_w = 0, lcd_h = 0;

bool getLcdSize(uint32_t *width, uint32_t *height)
{
	*width = (uint32_t)gr_draw->width;
	*height = (uint32_t)gr_draw->height;
	return true;
}

static void RGBRotate90_anticlockwise(void* pDest, int nDestWidth, int nDestHeight, int nDestBits, void* pSrc, int nSrcWidth, int nSrcHeight, int nSrcBits)
{

	int i,j;
	int m = nSrcBits >> 3;

	unsigned int *des32 = (unsigned int *)pDest;
	unsigned int *src32 = (unsigned int *)pSrc;
	unsigned short *des16 = (unsigned short *)pDest;
	unsigned short *src16 = (unsigned short *)pSrc;

    if ((nDestWidth < nSrcHeight)||(nDestHeight < nSrcWidth)){
        int s_offsetX = 0;
        int s_offsetY = 0;
        int d_offsetX = s_offsetY;
        int d_offsetY = s_offsetX;

        if ((!pDest)||(!pSrc)) return;
        if(m == 4){
            for(j = 0; j < nDestHeight; j++) {
                for(i = 0; i < nDestWidth; i++) {
                    *(des32 + (j+d_offsetY) * nDestWidth + nDestWidth - 1 - (j + d_offsetX)) = *(src32 + (j+s_offsetY) * nDestWidth + i + s_offsetX);
                }
            }
        }else{
            for(j = 0; j < nDestHeight; j++) {
                for(i = 0; i < nDestWidth; i++) {
                    *(des16 + (j+d_offsetY) * nDestWidth + nDestWidth - 1 - (j + d_offsetX)) = *(src16 + (j+s_offsetY) * nDestWidth + i + s_offsetX);
                }
            }
        }
    } else {
        int d_offsetX = ((int)((nDestWidth - nSrcHeight)/2));
        int d_offsetY = ((int)((nDestHeight - nSrcWidth)/2));
        int s_offsetX = d_offsetY;
        int s_offsetY = d_offsetX;
        if ((!pDest)||(!pSrc)) return;
        if(m == 4){
            for(j = 0; j < nSrcHeight; j++) {
                for(i = 0; i < nSrcWidth; i++) {
                    *(des32 + (i+d_offsetY) * nDestWidth + nDestWidth - 1 - (j + d_offsetX)) = *(src32 + (j+s_offsetY) * nDestHeight + i + s_offsetX);
                }
            }
        }else{
            for(j = 0; j < nSrcHeight; j++) {
                for(i = 0; i < nSrcWidth; i++) {
                    *(des16 + (i+d_offsetY) * nDestWidth + nDestWidth - 1 - (j + d_offsetX)) = *(src16 + (j+s_offsetY) * nDestHeight + i + s_offsetX);
                }
            }
        }
    }
}

static void RGBRotate90_clockwise(void* pDest, int nDestWidth, int nDestHeight, int nDestBits, void* pSrc, int nSrcWidth, int nSrcHeight, int nSrcBits)
{

    int i,j;
    int m = nSrcBits >> 3;

    unsigned int *des32 = (unsigned int *)pDest;
    unsigned int *src32 = (unsigned int *)pSrc;
    unsigned short *des16 = (unsigned short *)pDest;
    unsigned short *src16 = (unsigned short *)pSrc;

    if ((nDestWidth < nSrcHeight)||(nDestHeight < nSrcWidth)){
        int s_offsetX = 0;
        int s_offsetY = 0;
        int d_offsetX = s_offsetY;
        int d_offsetY = s_offsetX;

        if ((!pDest)||(!pSrc)) return;
        if(m == 4){
            for(j = 0; j < nDestHeight; j++) {
                for(i = 0; i < nDestWidth; i++) {
                    *(des32 + (nDestHeight - i - d_offsetY) * nDestWidth + nDestWidth - 1 - (j + d_offsetX)) = *(src32 + (j+s_offsetY) * nDestWidth + i + s_offsetX);
                }
            }
        }else{
            for(j = 0; j < nDestHeight; j++) {
                for(i = 0; i < nDestWidth; i++) {
                    *(des16 + (nDestHeight - i - d_offsetY) * nDestWidth + nDestWidth - 1 - (j + d_offsetX)) = *(src16 + (j+s_offsetY) * nDestWidth + i + s_offsetX);
                }
            }
        }
    } else {
        int d_offsetX = ((int)((nDestWidth - nSrcHeight)/2));
        int d_offsetY = ((int)((nDestHeight - nSrcWidth)/2));
        int s_offsetX = d_offsetY;
        int s_offsetY = d_offsetX;
        if ((!pDest)||(!pSrc)) return;
        if(m == 4){
            for(j = 0; j < nSrcHeight; j++) {
                for(i = 0; i < nSrcWidth; i++) {
                    *(des32 + (nDestHeight - i - d_offsetY) * nDestWidth + nDestWidth - 1 - (j + d_offsetX)) = *(src32 + (j+s_offsetY) * nDestHeight + i + s_offsetX);
                }
            }
        }else{
            for(j = 0; j < nSrcHeight; j++) {
                for(i = 0; i < nSrcWidth; i++) {
                    *(des16 + (nDestHeight - i - d_offsetY) * nDestWidth + nDestWidth - 1 - (j + d_offsetX)) = *(src16 + (j+s_offsetY) * nDestHeight + i + s_offsetX);
                }
            }
        }
    }
}

static void YUVRotate90(uint8_t *des,uint8_t *src,int width,int height)
{
    int i=0,j=0,n=0;
    int hw=width/2,hh=height/2;

    for(j=width;j>0;j--) {
        for(i=0;i<height;i++) {
            des[n++] = src[width*i+j];
        }
    }

    unsigned char *ptmp = src+width*height;
    for(j=hw;j>0;j--) {
        for(i=0;i<hh;i++) {
            des[n++] = ptmp[hw*i+j];
        }
    }

    ptmp = src+width*height*5/4;
    for(j=hw;j>0;j--) {
        for(i=0;i<hh;i++) {
            des[n++] = ptmp[hw*i+j];
        }
    }
}

static void  StretchColors(void* pDest, int nDestWidth, int nDestHeight, int nDestBits, void* pSrc, int nSrcWidth, int nSrcHeight, int nSrcBits)
{
    if ((nDestWidth < nSrcWidth)||(nDestHeight<nSrcHeight)){
        double dfAmplificationX = ((double)nDestWidth)/nSrcWidth;
        double dfAmplificationY = ((double)nDestHeight)/nSrcHeight;
        double stretch = 1/(dfAmplificationY > dfAmplificationX ? dfAmplificationY : dfAmplificationX);
        int offsetX = (dfAmplificationY > dfAmplificationX ? (int)(nSrcWidth - nDestWidth/dfAmplificationY) >> 1 : 0);
        int offsetY = (dfAmplificationX > dfAmplificationY ? (int)(nSrcHeight - nDestHeight/dfAmplificationX) >>1 : 0);
        int i = 0;
        int j = 0;
        double tmpY = 0;
        unsigned int *pSrcPos = (unsigned int*)pSrc;
        unsigned int *pDestPos = (unsigned int*)pDest;
        int linesize ;
        ALOGI("Native MMI Test: nDestWidth = %d, nDestHeight = %d, nSrcWidth = %d, nSrcHeight = %d, offsetX = %d, offsetY= %d \n", \
            nDestWidth, nDestHeight, nSrcWidth, nSrcHeight, offsetX, offsetY);

        for(i = 0; i<nDestHeight; i++) {
            double tmpX = 0;

            int nLine = (int)(tmpY+0.5) + offsetY;

            linesize = nLine * nSrcWidth;

            for(j = 0; j<nDestWidth; j++) {

                int nRow = (int)(tmpX+0.5) + offsetX;

                *pDestPos++ = *(pSrcPos + linesize + nRow);

                tmpX += stretch;
            }
            tmpY += stretch;
        }
    } else {
        int offsetX = (int)((nDestHeight - nSrcHeight)/2);
        int offsetY = (int)((nDestWidth - nSrcWidth)/2);
        int i = 0, j = 0;
        unsigned int *pSrcPos = (unsigned int*)pSrc;
        unsigned int *pDestPos = (unsigned int*)pDest;
        for (j = 0; j < nSrcHeight; j++){
            for (i = 0; i < nSrcWidth; i++){
                *(pDestPos + (j + offsetX) * nDestWidth + i + offsetY) = *(pSrcPos + j * nSrcWidth + i);
            }
        }

    }
}


static void yuv420_to_rgb(int width, int height, unsigned char *src, unsigned int *dst)
{
	int frameSize = width * height;
	int j = 0, yp = 0, i = 0;
	unsigned short *dst16 = (unsigned short *)dst;
	unsigned char *yuv420sp = src;

	for (j = 0, yp = 0; j < height; j++)  {
		int uvp = frameSize + (j >> 1) * width, u = 0, v = 0;

		for (i = 0; i < width; i++, yp++) {
			int y = (0xff & ((int) yuv420sp[yp])) - 16;

			if (y < 0) y = 0;
			if ((i & 1) == 0) {
				u = (0xff & yuv420sp[uvp++]) - 128;
				v = (0xff & yuv420sp[uvp++]) - 128;
			}

			int y1192 = 1192 * y;
			int r = (y1192 + 1634 * v);
			int g = (y1192 - 833 * v - 400 * u);
			int b = (y1192 + 2066 * u);

			if (r < 0) r = 0; else if (r > 262143) r = 262143;
			if (g < 0) g = 0; else if (g > 262143) g = 262143;
			if (b < 0) b = 0; else if (b > 262143) b = 262143;

			if(var.bits_per_pixel == 32) {
				dst[yp] = ((((r << 6) & 0xff0000)>>16)<<16)|(((((g >> 2) & 0xff00)>>8))<<8)|((((b >> 10) & 0xff))<<0);
			} else {
				dst16[yp] = RGB565((((r << 6) & 0xff0000)>>16), (((g >> 2) & 0xff00)>>8), (((b >> 10) & 0xff)));
			}
		}
	}
}

static void eng_dcamtest_switchTB(uint8_t *buffer, uint16_t width, uint16_t height, uint8_t pixel)
{
	uint32_t j;
	uint32_t linesize;
	uint8_t *dst = NULL;
	uint8_t *src = NULL;
	uint8_t *tmpBuf = NULL;

    ALOGI("Native MMI Test: %s,%d IN\n", __func__, __LINE__);

    /*  */
	linesize = width * (pixel/8);

	tmpBuf = (uint8_t *)malloc(linesize);
	if(!tmpBuf){
        ALOGE("Native MMI Test: %s,%d Fail to alloc temp buffer\n", __func__, __LINE__);
		return;
	}

    /*  */
    for(j=0; j<height/2; j++) {
        src = buffer + j * linesize;
        dst = buffer + height * linesize - (j + 1) * linesize;
        memcpy(tmpBuf,src,linesize);

        for(j=0; j<height/2; j++) {
            src = buffer + j * linesize;
            dst = buffer + height * linesize - (j + 1) * linesize;
            memcpy(tmpBuf,src,linesize);
            memcpy(src,dst,linesize);
            memcpy(dst,tmpBuf,linesize);
        }

    }
    free(tmpBuf); // modify for coverity issue 107764 & 107766
}

static int eng_test_rotation(uint32_t agree, uint32_t width, uint32_t height, uint32_t in_addr, uint32_t out_addr)
{
	struct _rot_cfg_tag rot_params;

    /* set rotation params */
	rot_params.format = ROT_YUV420;
	switch(agree){
		case 90:
			rot_params.angle = ROT_90;
			break;
		case 180:
			rot_params.angle = ROT_180;
			break;
		case 270:
			rot_params.angle = ROT_270;
			break;
		default:
			rot_params.angle = ROT_ANGLE_MAX;
			break;
	}
	rot_params.img_size.w = width;
	rot_params.img_size.h = height;
	rot_params.src_addr.y_addr = in_addr;
	rot_params.src_addr.u_addr = rot_params.src_addr.y_addr + rot_params.img_size.w * rot_params.img_size.h;
	rot_params.src_addr.v_addr = rot_params.src_addr.u_addr + rot_params.img_size.w * rot_params.img_size.h/4;
	rot_params.dst_addr.y_addr = out_addr;
	rot_params.dst_addr.u_addr = rot_params.dst_addr.y_addr + rot_params.img_size.w * rot_params.img_size.h;
	rot_params.dst_addr.v_addr = rot_params.dst_addr.u_addr + rot_params.img_size.w * rot_params.img_size.h/4;

    /* open rotation device  */
	rot_fd = open(ROT_DEV, O_RDWR, 0);
	if (-1 == rot_fd) {
        ALOGE("Native MMI Test: %s,%d Fail to open rotation device\n", __func__, __LINE__);
		return -1;
	}

    /* call ioctl */
	if (-1 == ioctl(rot_fd, ROT_IO_START, &rot_params)) {
        ALOGE("Native MMI Test: %s,%d Fail to SC8800G_ROTATION_DONE\n", __func__, __LINE__);
		return -1;
	}

	return 0;
}

static int eng_test_fb_open(void)
{
	int i;
	void *bits;
	int offset_page_align;

    ALOGI("Native MMI Test: %s,%d IN\n", __func__, __LINE__);

	var.yres = lcd_h;
	var.xres = lcd_w;
	var.bits_per_pixel = (8 * gr_draw->pixel_bytes);
	ALOGI("Native MMI Test: var.yres = %d, var.xres = %d, var.bits_per_pixel = %d\n", var.yres, var.xres, var.bits_per_pixel);
	/* set framebuffer address */
	memset(&fb_buf, 0, sizeof(fb_buf));

	fb_buf[0].virt_addr = (size_t)gr_draw->data;

	fb_buf[1].virt_addr = (size_t)var.yres * var.xres * (var.bits_per_pixel/8);

	fb_buf[2].virt_addr = (size_t)tmpbuf2;

	fb_buf[3].virt_addr = (size_t)tmpbuf3;

/*
	for(i = 0; i < 6; i++){
		ALOGD("DCAM: buf[%d] virt_addr=0x%x, phys_addr=0x%x, length=%d", \
			i, fb_buf[i].virt_addr,fb_buf[i].phys_addr,fb_buf[i].length);
	}
*/

	return 0;
}

static unsigned int getPreviewBufferIDForPhy(cmr_uint phy_addr)
{
    unsigned int i = 0;

    ALOGI("Native MMI Test: %s,%d IN\n", __func__, __LINE__);

    for (i = 0; i < PREVIEW_BUFF_NUM; i++) {
        if (!previewHeapArray[i]) continue;

        if (!(cmr_uint)previewHeapArray[i]->phys_addr) continue;

        if ((cmr_uint)previewHeapArray[i]->phys_addr == phy_addr) return i;
    }

    return 0xFFFFFFFF;
}

static void eng_test_fb_update(const camera_frame_type *frame)
{
    //int width, height;
    int crtc = 0;
    unsigned int buffer_id;

    ALOGI("Native MMI Test: %s,%d IN\n", __func__, __LINE__);

    if (!frame) return;

    buffer_id = getPreviewBufferIDForPhy(frame->y_phy_addr);

	gr_draw = gr_backend->flip(gr_backend);
	fb_buf[0].virt_addr = (size_t)gr_draw->data;

    /*  */
    if(!previewHeapArray[buffer_id]) {
        ALOGI("Native MMI Test: %s,%d preview heap array empty, do nothing\n", __func__, __LINE__);
        return;
    }

    /*  */
    if(NULL == mHalOem || NULL == mHalOem->ops) {
        ALOGI("Native MMI Test: oem is null or oem ops is null, do nothing\n");
        return;
    }
    mHalOem->ops->camera_set_preview_buffer(oem_handle, (cmr_uint)previewHeapArray[buffer_id]->phys_addr, (cmr_uint)previewHeapArray[buffer_id]->data);

}

void eng_test_camera_close(void)
{
    ALOGI("Native MMI Test: %s,%d IN\n", __func__, __LINE__);
    return;
}

int eng_test_flashlight_ctrl(uint32_t flash_status)
{
	int ret = 0;

    ALOGI("Native MMI Test: %s,%d IN\n", __func__, __LINE__);

	return ret;
}

static void data_mirror(uint8_t *des,uint8_t *src,int width,int height, int bits)
{
    int size = 0;
    int i,j;
    int lineunm;
    int m = bits >> 3;
	unsigned int *des32 = (unsigned int *)des;
	unsigned int *src32 = (unsigned int *)src;
	unsigned short *des16 = (unsigned short *)des;
	unsigned short *src16 = (unsigned short *)src;

    /*  */
    if ((!des)||(!src)) return;

    /*  */

    if(m == 4){
	   for(j = 0; j < height; j++) {
		   size += width;
		   for(i = 0; i < width; i++) {
			   *(des32 + size - i - 1)= *src32++;
		   }
	  }
    }else{
         for(j = 0; j < height; j++) {
	         size += width;
		     for(i = 0; i < width; i++) {
			     *(des16 + size - i - 1)= *src16++;
		      }
	      }
    }
    return;
}

void eng_tst_camera_cb(enum camera_cb_type cb , const void *client_data , enum camera_func_type func , void* parm4)
{
    int test_num=0,row,i;
    struct camera_frame_type *frame = (struct camera_frame_type *)parm4;

    ALOGI("Native MMI Test: %s,%d IN\n", __func__, __LINE__);

    if(!frame) {
		ALOGI("Native MMI Test: %s,%d, camera call back parm4 error: NULL, do nothing\n", __func__, __LINE__);
        return;
    }

    if(CAMERA_FUNC_START_PREVIEW != func) {
		ALOGI("Native MMI Test: %s,%d, camera func type error: %d, do nothing\n", __func__, __LINE__, func);
        return;
    }

    if(CAMERA_EVT_CB_FRAME != cb) {
		ALOGI("Native MMI Test: %s,%d, camera cb type error: %d, do nothing\n", __func__, __LINE__, cb);
        return;
    }

    /*lock*/
    previewLock.lock();

    /*empty preview arry, do nothing*/
    if(!previewHeapArray[frame->buf_id]) {
		ALOGI("Native MMI Test: %s,%d, preview heap array empty, do nothine\n", __func__, __LINE__);

        previewLock.unlock();
        return;
    }

    /*get frame buffer id*/
    frame->buf_id=getPreviewBufferIDForPhy(frame->y_phy_addr);

    /*preview enable or disable?*/
    if(!previewvalid) {
		ALOGI("Native MMI Test: %s,%d, preview disabled, do nothing\n", __func__, __LINE__);

        previewLock.unlock();
        return;
    }

    //1.yuv -> rgb
    yuv420_to_rgb(PREVIEW_WIDTH,PREVIEW_HIGHT, (unsigned char*)previewHeapArray[frame->buf_id]->data, post_preview_buf);

    /*unlock*/
    previewLock.unlock();

	/* fore && back camera: istrech,mirror,rotate*/
	if(0 == camera_id) {
		//2. stretch
		StretchColors((void *)(fb_buf[2].virt_addr), var.yres, var.xres, var.bits_per_pixel, post_preview_buf, PREVIEW_WIDTH, PREVIEW_HIGHT, var.bits_per_pixel);
		/*FIXME: here need 2 or 3 framebuffer pingpang ??*/
		if(!(frame_num % 2)) {

			//3. rotation
			RGBRotate90_anticlockwise((void *)(fb_buf[0].virt_addr),var.xres, var.yres, var.bits_per_pixel, (void*)fb_buf[2].virt_addr, PREVIEW_WIDTH, PREVIEW_HIGHT, var.bits_per_pixel);
		} else {

			//3. rotation
			RGBRotate90_anticlockwise((void *)(fb_buf[0].virt_addr),var.xres, var.yres, var.bits_per_pixel, (void*)fb_buf[2].virt_addr, PREVIEW_WIDTH, PREVIEW_HIGHT, var.bits_per_pixel);
			//RGBRotate90_anticlockwise((uint8_t *)(fb_buf[1].virt_addr), (uint8_t*)fb_buf[2].virt_addr, var.yres, var.xres, var.bits_per_pixel);
        }
	} else if (1 == camera_id) {
		//2. stretch
		StretchColors((void *)(fb_buf[2].virt_addr), var.yres, var.xres, var.bits_per_pixel, post_preview_buf, PREVIEW_WIDTH, PREVIEW_HIGHT, var.bits_per_pixel);
		if(!(frame_num % 2)) {
			RGBRotate90_clockwise((void *)(fb_buf[0].virt_addr),var.xres, var.yres, var.bits_per_pixel, (void*)fb_buf[2].virt_addr, PREVIEW_WIDTH, PREVIEW_HIGHT, var.bits_per_pixel);
		} else {
			RGBRotate90_clockwise((void *)(fb_buf[0].virt_addr),var.xres, var.yres, var.bits_per_pixel, (void*)fb_buf[2].virt_addr, PREVIEW_WIDTH, PREVIEW_HIGHT, var.bits_per_pixel);
			//RGBRotate90_anticlockwise((uint8_t *)(fb_buf[1].virt_addr), (uint8_t*)fb_buf[2].virt_addr, var.yres, var.xres, var.bits_per_pixel);
		}
	} else if (2 == camera_id) {
		//2. stretch
		StretchColors((void *)(fb_buf[2].virt_addr), var.yres, var.xres, var.bits_per_pixel, post_preview_buf, PREVIEW_WIDTH, PREVIEW_HIGHT, var.bits_per_pixel);

		/*FIXME: here need 2 or 3 framebuffer pingpang ??*/
		if(!(frame_num % 2)) {

			//3. rotation
			RGBRotate90_anticlockwise((void *)(fb_buf[0].virt_addr),var.xres, var.yres, var.bits_per_pixel, (void*)fb_buf[2].virt_addr, PREVIEW_WIDTH, PREVIEW_HIGHT, var.bits_per_pixel);
		} else {

			//3. rotation
			RGBRotate90_anticlockwise((void *)(fb_buf[0].virt_addr),var.xres, var.yres, var.bits_per_pixel, (void*)fb_buf[2].virt_addr, PREVIEW_WIDTH, PREVIEW_HIGHT, var.bits_per_pixel);
			//RGBRotate90_anticlockwise((uint8_t *)(fb_buf[1].virt_addr), (uint8_t*)fb_buf[2].virt_addr, var.yres, var.xres, var.bits_per_pixel);
        }
	}

    /*lock*/
    previewLock.lock();
    ALOGI("Native MMI Test: fb_buf[0].virt_addr=%0x,var.xres=%d,var.yres=%d,var.bits_per_pixel=%d\n",fb_buf[0].virt_addr,var.xres,var.yres,var.bits_per_pixel);

	//4. update
	eng_test_fb_update(frame);

    /*unlock*/
    previewLock.unlock();

    frame_num++;

}

static void freeCameraMem(sprd_camera_memory_t* memory)
{
    ALOGI("Native MMI Test: %s,%d IN\n", __func__, __LINE__);

    if(!memory) return;

    if(memory->ion_heap) {
        if (s_mem_method)
            memory->ion_heap->free_iova(ION_MM,memory->phys_addr, memory->phys_size);

        delete memory->ion_heap;
        memory->ion_heap = NULL;
    }

    free(memory);
}

static int Callback_OtherFree(enum camera_mem_cb_type type, cmr_uint *phy_addr, cmr_uint *vir_addr, cmr_u32 sum)
{
	unsigned int i;
	ALOGI("Native MMI Test: %s,%d IN\n", __func__, __LINE__);

	if (type == CAMERA_PREVIEW_RESERVED) {
		if (NULL != mPreviewHeapReserved) {
			freeCameraMem(mPreviewHeapReserved);
			mPreviewHeapReserved = NULL;
		}
	}

	if (type == CAMERA_ISP_LSC) {
		if (NULL != mIspLscHeapReserved) {
			freeCameraMem(mIspLscHeapReserved);
			mIspLscHeapReserved = NULL;
		}
	}

	if (type == CAMERA_ISP_BINGING4AWB) {
		for (i=0 ; i < kISPB4awbCount ; i++) {
			if (NULL != mIspB4awbHeapReserved[i]) {
				freeCameraMem(mIspB4awbHeapReserved[i]);
				mIspB4awbHeapReserved[i] = NULL;
			}
		}
	}

	if (type == CAMERA_ISP_RAWAE) {
		for (i=0 ; i < kISPB4awbCount ; i++) {
			if (NULL != mIspRawAemHeapReserved[i]) {
				mIspRawAemHeapReserved[i]->ion_heap->free_kaddr();
				freeCameraMem(mIspRawAemHeapReserved[i]);
			}
			mIspRawAemHeapReserved[i] = NULL;
		}
	}

	if (type == CAMERA_ISP_RAWAF) {
		for (i=0 ; i < kISPB4awbCount ; i++) {
			if (NULL != mIspRawAfmHeapReserved[i]) {
				mIspRawAfmHeapReserved[i]->ion_heap->free_kaddr();
				freeCameraMem(mIspRawAfmHeapReserved[i]);
			}
			mIspRawAfmHeapReserved[i] = NULL;    // for coverity issue 132040 , looks like a copy-paste error
		}
	}

	if (type == CAMERA_ISP_ANTI_FLICKER) {
		if (NULL != mIspAFLHeapReserved) {
			freeCameraMem(mIspAFLHeapReserved);
			mIspAFLHeapReserved = NULL;
		}
	}

	return 0;
}

static int Callback_PreviewFree(cmr_uint *phy_addr, cmr_uint *vir_addr, cmr_u32 sum)
{
	cmr_u32 i;

    ALOGI("Native MMI Test: %s,%d IN\n", __func__, __LINE__);

    /*lock*/
    previewLock.lock();

    /*  */
    for (i = 0 ; i < mPreviewHeapNum ; i++) {
		if (!previewHeapArray[i]) continue;

		freeCameraMem(previewHeapArray[i]);
		previewHeapArray[i] = NULL;
	}

	mPreviewHeapNum = 0;

    /*unlock*/
    previewLock.unlock();

	return 0;
}

static cmr_int Callback_Free(enum camera_mem_cb_type type, cmr_uint *phy_addr, cmr_uint *vir_addr, cmr_u32 sum, void* private_data)
{
	cmr_int ret = 0;

    ALOGI("Native MMI Test: %s,%d IN\n", __func__, __LINE__);

    /*  */
	if (!private_data || !phy_addr || !vir_addr) {
		ALOGE("Native MMI Test: %s,%d, error param 0x%lx 0x%lx 0x%lx\n", __func__, __LINE__, (cmr_uint)phy_addr, (cmr_uint)vir_addr, (cmr_uint)private_data);
		return -1;
	}

	if (CAMERA_MEM_CB_TYPE_MAX <= type) {
		ALOGE("Native MMI Test: %s,%d, mem type error %d\n", __func__, __LINE__, type);
		return -1;
	}

	if (CAMERA_PREVIEW == type) {
		ret = Callback_PreviewFree(phy_addr, vir_addr, sum);
	} else if (type == CAMERA_PREVIEW_RESERVED || type == CAMERA_ISP_LSC
			|| type == CAMERA_ISP_BINGING4AWB || type == CAMERA_ISP_RAWAE || type == CAMERA_ISP_RAWAF|| type == CAMERA_ISP_ANTI_FLICKER) {
		ret = Callback_OtherFree(type, phy_addr, vir_addr, sum);
	} else {
        ALOGI("Native MMI Test: %s,%d, type ignore: %d, do nothing\n", __func__, __LINE__, type);
    }

    /* disable preview flag */
    previewvalid = 0;

	return ret;
}

static sprd_camera_memory_t* allocCameraMem(int buf_size, int num_bufs, uint32_t is_cache)
{
	int ret = 0;
	size_t psize = 0;
	size_t mem_size = 0;
	unsigned long paddr = 0;
	MemoryHeapIon *pHeapIon = NULL;
	sprd_camera_memory_t *memory = NULL;

    ALOGI("Native MMI Test: %s,%d IN\n", __func__, __LINE__);

    /*  */
	mem_size = buf_size * num_bufs ;
	if(!mem_size) {
        ALOGE("Native MMI Test: %s,%d, failed: mem size err.\n", __func__, __LINE__);
		return NULL;
	}

    /*  */
	memory = (sprd_camera_memory_t *)malloc(sizeof(sprd_camera_memory_t));
	if (!memory) {
        ALOGE("Native MMI Test: %s,%d, failed: malloc memory err.\n", __func__, __LINE__);
		return NULL;
	}

	memset(memory, 0 , sizeof(sprd_camera_memory_t));
	//memory->busy_flag = false;

    /*  */
	if (0 == s_mem_method) {
		if (is_cache) {
			pHeapIon = new MemoryHeapIon("/dev/ion", mem_size ,0 , (1<<31) | ION_HEAP_ID_MASK_MM);
		} else {
			pHeapIon = new MemoryHeapIon("/dev/ion", mem_size , MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_MM);
		}
	} else {
		if (is_cache) {
			pHeapIon = new MemoryHeapIon("/dev/ion", mem_size ,0 , (1<<31) | ION_HEAP_ID_MASK_SYSTEM);
		} else {
			pHeapIon = new MemoryHeapIon("/dev/ion", mem_size , MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
		}
	}

    /*  */
	if (!pHeapIon) {
        ALOGE("Native MMI Test: %s,%d, failed: malloc heap err.\n", __func__, __LINE__);
		goto getpmem_end;
	}
	if (0 > pHeapIon->getHeapID()) {
        ALOGE("Native MMI Test: %s,%d, failed: ion get heapid err.\n", __func__, __LINE__);
		goto getpmem_end;
	}
	if (!pHeapIon->getBase() || MAP_FAILED == pHeapIon->getBase()) {
        ALOGE("Native MMI Test: %s,%d, failed: ion get base err.\n", __func__, __LINE__);
		goto getpmem_end;
	}

    /*  */
	if (0 == s_mem_method) {//0 == s_mem_method
		ret = pHeapIon->get_phy_addr_from_ion(&paddr, &psize);
        if (ret < 0) {
            ALOGE("Native MMI Test: %s,%d, failed: %d get phy_addr from ion err.\n", __func__, __LINE__, ret);
            goto getpmem_end;
        }
	} else {
		ret = pHeapIon->get_iova(ION_MM,&paddr, &psize);
        if (ret < 0) {
            ALOGE("Native MMI Test: %s,%d, failed: %d get phy_addr from mm iova err.\n", __func__, __LINE__, ret);
            goto getpmem_end;
        }
	}

getpmem_end:
	memory->ion_heap = pHeapIon;
	memory->phys_addr = paddr;
	memory->phys_size = psize;

    /*  */
	if (pHeapIon)
		memory->data = pHeapIon->getBase();

	return memory;
}

static int Callback_PreviewMalloc(cmr_u32 size, cmr_u32 sum, cmr_uint *phy_addr, cmr_uint *vir_addr)
{
	cmr_int i = 0;
	*phy_addr = 0;
	*vir_addr = 0;
	sprd_camera_memory_t *memory = NULL;

    ALOGI("Native MMI Test: %s,%d IN\n", __func__, __LINE__);

    for (i = 0; i < PREVIEW_BUFF_NUM; i++) {
        memory = allocCameraMem(size, 1, true);
        if (!memory) {
            ALOGE("Native MMI Test: %s,%d, failed: alloc camera mem err.\n", __func__, __LINE__);

            Callback_PreviewFree(0, 0, 0);
            return -1;
        }

        previewHeapArray[i] = memory;
        mPreviewHeapNum++;

        *phy_addr++ = (cmr_uint)memory->phys_addr;
        *vir_addr++ = (cmr_uint)memory->data;
    }

	return 0;
}

int eng_test_came_save_to_buf(uint32_t** ppbuf_addr,TEST_CMR_CONTEXT_T_PTR save_cmr_cxt_ptr,int image_type){
    LOGD("in save from buf\n");
    int ret_val = -1;
    cmr_uint capture_addr = 0;
    test_commandid = image_type;

    if ((0 == image_type) || (2 == image_type)) {
        test_image_size = save_cmr_cxt_ptr->capture_width * save_cmr_cxt_ptr->capture_height * 2;
        capture_addr = save_cmr_cxt_ptr->capture_raw_vir_addr;
    } else if(1 == image_type) {
        test_image_size = save_cmr_cxt_ptr->jpg_size;
        capture_addr = (cmr_uint)save_cmr_cxt_ptr->jpg_buffer;
    }
    if ((ppbuf_addr != NULL) && (save_cmr_cxt_ptr != NULL)) {
        *ppbuf_addr = (uint32_t *)malloc(test_image_size);
        if (!*ppbuf_addr) {
            return ret_val;
        }
        memset((void*)*ppbuf_addr, 0x00, test_image_size);
        memcpy(*ppbuf_addr,(void *)capture_addr,test_image_size);
        ret_val = 0;
    } else {
        LOGE("%s poiter of  ppbuf_addr or save_cmr_cxt_ptr is null ,line=%d \n",__func__,__LINE__);
    }
    return ret_val;
}

static int eng_dcam_save_jpg(int image_type,int cameraId){
    LOGD("in save jpg\n");
    int32_t rtn = 0;
    int res = 0;
    TEST_CMR_CONTEXT_T_PTR cmr_cxt_ptr = g_eng_test_cmr_cxt_ptr;

    SENSOR_EXP_INFO_T* sensor_ptr = (SENSOR_EXP_INFO_T*)Sensor_GetInfo();

    FILE* fp = 0;
    char file_name[250] = "/data/local/factorytest_log/capture_jpg_tool_temp.jpg";
    char back_test_cap_jpg_file[250] = "/data/local/factorytest_log/capture_jpg_tool_temp0.jpg";
    char front_test_cap_jpg_file[250] = "/data/local/factorytest_log/capture_jpg_tool_temp1.jpg";
    if(cameraId == 0)
        sprintf(file_name, "%s", back_test_cap_jpg_file);
    else
        sprintf(file_name, "%s", front_test_cap_jpg_file);
    eng_test_came_save_to_buf(&test_camer_saveaddr,cmr_cxt_ptr,image_type);
    fp = fopen(file_name, "wb");
    if (fp != NULL) {
        res = fwrite(cmr_cxt_ptr->jpg_buffer , 1, cmr_cxt_ptr->jpg_size , fp);
        if (res != 0){
            printf("save result = %d\n",res);
            char cmd[200] = " ";
            sprintf(cmd,"echo 0x%02x > /sys/class/flash_test/flash_test/flash_value",17);
            system(cmd);
        }
        fclose(fp);
    }else {
        rtn = -1;
    }
    return rtn;
}

static int Callback_OtherMalloc(enum camera_mem_cb_type type, cmr_u32 size, cmr_u32 sum, cmr_uint *phy_addr, cmr_uint *vir_addr)
{
	unsigned int i;
	*phy_addr = 0;
	*vir_addr = 0;
	sprd_camera_memory_t *memory = NULL;

	ALOGI("Native MMI Test: %s,%d IN\n", __func__, __LINE__);

	if (type == CAMERA_PREVIEW_RESERVED) {
		if(NULL == mPreviewHeapReserved) {
			memory = allocCameraMem(size, 1, true);
			if (NULL == memory) {
				ALOGE("Native MMI Test: %s,%d, failed: alloc camera mem err.\n", __func__, __LINE__);
				Callback_OtherFree(type, 0, 0, 0);
				return -1;
			}
			mPreviewHeapReserved = memory;
			*phy_addr++ = (cmr_uint)memory->phys_addr;
			*vir_addr++ = (cmr_uint)memory->data;
		} else {
			*phy_addr++ = (cmr_uint)mPreviewHeapReserved->phys_addr;
			*vir_addr++ = (cmr_uint)mPreviewHeapReserved->data;
		}
	} else if (type == CAMERA_ISP_LSC) {
		if(mIspLscHeapReserved == NULL) {
			memory = allocCameraMem(size, 1, false);
			if (NULL == memory) {
				ALOGE("Native MMI Test: %s,%d, failed: alloc camera mem err.\n", __func__, __LINE__);
				Callback_OtherFree(type, 0, 0, 0);
				return -1;
			}
			mIspLscHeapReserved = memory;
			*phy_addr++ = (cmr_uint)memory->phys_addr;
			*vir_addr++ = (cmr_uint)memory->data;
		} else {
			HAL_LOGI("malloc isp lsc memory, malloced type %d,request num %d, request size 0x%x",
				type, sum, size);
			*phy_addr++ = (cmr_uint)mIspLscHeapReserved->phys_addr;
			*vir_addr++ = (cmr_uint)mIspLscHeapReserved->data;
		}
	} else if (type == CAMERA_ISP_BINGING4AWB) {
			cmr_u64* phy_addr_64 = (cmr_u64*)phy_addr;
			cmr_u64* vir_addr_64 = (cmr_u64*)vir_addr;
		   for (i = 0; i < sum; i++) {
				memory = allocCameraMem(size, 1, false);
				if(NULL == memory) {
					ALOGE("Native MMI Test: %s,%d, failed: alloc camera mem err.\n", __func__, __LINE__);
					Callback_OtherFree(type, 0, 0, 0);
					return -1;
				}
			mIspB4awbHeapReserved[i] = memory;
			*phy_addr_64++ = (cmr_u64)memory->phys_addr;
			*vir_addr_64++ = (cmr_u64)memory->data;
			}
	} else if (type == CAMERA_ISP_RAWAE) {
		cmr_u64 kaddr = 0;
		cmr_u64* phy_addr_64 = (cmr_u64*)phy_addr;
		cmr_u64* vir_addr_64 = (cmr_u64*)vir_addr;
		size_t ksize = 0;
		for (i = 0; i < sum; i++) {
				memory = allocCameraMem(size, 1, false);
				if(NULL == memory) {
					ALOGE("Native MMI Test: error memory is null,malloced type %d",type);
					Callback_OtherFree(type, 0, 0, 0);
					return -1;
				}
			mIspRawAemHeapReserved[i] = memory;
			memory->ion_heap->get_kaddr(&kaddr, &ksize);
			*phy_addr_64++ = kaddr;
			*vir_addr_64++ = (cmr_u64)(memory->data);
		}
	} else if (type == CAMERA_ISP_RAWAF) {
		cmr_u64 kaddr = 0;
		cmr_u64* phy_addr_64 = (cmr_u64*)phy_addr;
		cmr_u64* vir_addr_64 = (cmr_u64*)vir_addr;
		size_t ksize = 0;
		for (i = 0; i < sum; i++) {
				memory = allocCameraMem(size, 1, false);
				if(NULL == memory) {
					ALOGE("Native MMI Test: error memory is null,malloced type %d",type);
					Callback_OtherFree(type, 0, 0, 0);
					return -1;
				}
			mIspRawAfmHeapReserved[i] = memory;
			memory->ion_heap->get_kaddr(&kaddr, &ksize);
			*phy_addr_64++ = kaddr;
			*vir_addr_64++ = (cmr_u64)(memory->data);
		}
	} else if (type == CAMERA_ISP_ANTI_FLICKER) {
		if(mIspAFLHeapReserved == NULL) {
			memory = allocCameraMem(size, 1, false);
			if (NULL == memory) {
				ALOGE("Native MMI Test: %s,%d, failed: alloc camera mem err.\n", __func__, __LINE__);
				Callback_OtherFree(type, 0, 0, 0);
				return -1;
			}
			mIspAFLHeapReserved = memory;
			*phy_addr++ = (cmr_uint)memory->phys_addr;
			*vir_addr++ = (cmr_uint)memory->data;
		} else {
			HAL_LOGI("malloc isp afl memory, malloced type %d,request num %d, request size 0x%x",
				type, sum, size);
			*phy_addr++ = (cmr_uint)mIspAFLHeapReserved->phys_addr;
			*vir_addr++ = (cmr_uint)mIspAFLHeapReserved->data;
		}
	} else {
		ALOGE("Native MMI Test: %s,%d, type ignore: %d, do nothing\n", __func__, __LINE__, type);
		return 0;
	}

	return 0;
}

static cmr_int Callback_Malloc(enum camera_mem_cb_type type, cmr_u32 *size_ptr, cmr_u32 *sum_ptr, cmr_uint *phy_addr, cmr_uint *vir_addr, void* private_data)
{
	cmr_int ret = 0;
	cmr_u32 size;
	cmr_u32 sum;

	ALOGI("Native MMI Test: %s,%d IN\n", __func__, __LINE__);

	/*lock*/
	previewLock.lock();

	if (!phy_addr || !vir_addr || !size_ptr || !sum_ptr || (0 == *size_ptr) || (0 == *sum_ptr)) {
		ALOGE("Native MMI Test: %s,%d, alloc param error 0x%lx 0x%lx 0x%lx\n",
				__func__, __LINE__ , (cmr_uint)phy_addr, (cmr_uint)vir_addr, (cmr_uint)size_ptr);
		/*unlock*/
		previewLock.unlock();
		return -1;
	}

	size = *size_ptr;
	sum = *sum_ptr;

	if (type == CAMERA_PREVIEW) {
		ret = Callback_PreviewMalloc(size, sum, phy_addr, vir_addr);
	} else if (type == CAMERA_PREVIEW_RESERVED || type == CAMERA_ISP_LSC
			|| type == CAMERA_ISP_BINGING4AWB || type == CAMERA_ISP_RAWAE || type == CAMERA_ISP_RAWAF || type == CAMERA_ISP_ANTI_FLICKER) {
		ret = Callback_OtherMalloc(type, size, sum, phy_addr, vir_addr);
	} else {
		ALOGE("Native MMI Test: %s,%d, type ignore: %d, do nothing.\n", __func__, __LINE__, type);
	}

	/* enable preview flag */
	previewvalid = 1;

	/*unlock*/
	previewLock.unlock();

	return ret;
}

static void eng_tst_camera_startpreview(void)
{
	cmr_int ret = 0;
    struct img_size preview_size;
    struct cmr_zoom_param zoom_param;
    struct img_size capture_size;
    struct cmr_preview_fps_param fps_param;

    if (!oem_handle || NULL == mHalOem || NULL == mHalOem->ops) return;

    ALOGI("Native MMI Test: %s,%d IN\n", __func__, __LINE__);

    /*  */
    preview_size.width = PREVIEW_WIDTH;
    preview_size.height = PREVIEW_HIGHT;

    zoom_param.mode = 1;
    zoom_param.zoom_level = 1;
    zoom_param.zoom_info.zoom_ratio = 1.00000;
    zoom_param.zoom_info.output_ratio = (float)PREVIEW_WIDTH/PREVIEW_HIGHT;

    fps_param.frame_rate = 25;
    fps_param.video_mode = 0;

    /*  */
    mHalOem->ops->camera_fast_ctrl(oem_handle, CAMERA_FAST_MODE_FD, 0);

    /*  */
    SET_PARM(mHalOem, oem_handle , CAMERA_PARAM_PREVIEW_SIZE   , (cmr_uint)&preview_size);
    SET_PARM(mHalOem, oem_handle , CAMERA_PARAM_AF_MODE   , CAMERA_FOCUS_MODE_CAF);
    //SET_PARM(oem_handle , CAMERA_PARAM_VIDEO_SIZE     , (cmr_uint)&video_size);
    //SET_PARM(oem_handle , CAMERA_PARAM_CAPTURE_SIZE   , (cmr_uint)&capture_size);
    SET_PARM(mHalOem, oem_handle , CAMERA_PARAM_PREVIEW_FORMAT , CAMERA_DATA_FORMAT_YUV420);
    //SET_PARM(oem_handle , CAMERA_PARAM_CAPTURE_FORMAT , CAMERA_DATA_FORMAT_YUV420);
    SET_PARM(mHalOem, oem_handle , CAMERA_PARAM_SENSOR_ROTATION, 0);
    SET_PARM(mHalOem, oem_handle , CAMERA_PARAM_ZOOM           , (cmr_uint)&zoom_param);
    SET_PARM(mHalOem, oem_handle , CAMERA_PARAM_PREVIEW_FPS    , (cmr_uint)&fps_param);

    /* set malloc && free callback*/
    ret = mHalOem->ops->camera_set_mem_func(oem_handle, (void*)Callback_Malloc, (void*)Callback_Free, NULL);
    if (CMR_CAMERA_SUCCESS != ret) {
        ALOGE("Native MMI Test: %s,%d, failed: camera set mem func error.\n", __func__, __LINE__);
        return;
    }

    /*start preview*/
    ret = mHalOem->ops->camera_start_preview(oem_handle, CAMERA_NORMAL_MODE);
    if (CMR_CAMERA_SUCCESS != ret) {
        ALOGE("Native MMI Test: %s,%d, failed: camera start preview error.\n", __func__, __LINE__);
        return;
    }
}

static int eng_tst_camera_stoppreview(void)
{
    int ret = 0;

    ALOGI("Native MMI Test: %s,%d IN\n", __func__, __LINE__);

    if (!oem_handle || NULL == mHalOem || NULL == mHalOem->ops) {
        ALOGI("Native MMI Test: oem is null or oem ops is null, do nothing\n");
        return -1;
    }

    ret = mHalOem->ops->camera_stop_preview(oem_handle);

    return ret;
}

int eng_test_camera_capture(int cameraId){
    LOGD("in camera capture\n");
    int ret = 0;

    if (CMR_CAMERA_SUCCESS != mHalOem->ops->camera_set_mem_func(oem_handle,(void*)Callback_Malloc,(void*)Callback_Free,NULL)){
        LOGD("set mem function failed\n");
        return -1;
    }
    printf("set mem function\n");

    if (CMR_CAMERA_SUCCESS != mHalOem->ops->camera_take_picture(oem_handle,CAMERA_NORMAL_MODE)){
        LOGD("camera take picture failed\n");
        return -1;
    }
    printf("camera take picture done\n");
    struct timespec ts;
    TEST_CMR_CONTEXT_T_PTR cmr_cxt_ptr = g_eng_test_cmr_cxt_ptr;
    if (clock_gettime(CLOCK_REALTIME,&ts)){
        LOGD("get Time failed\n");
        return -1;
    }
    printf("get time successfully\n");
    ts.tv_sec += 6;
    if (sem_timedwait(&(cmr_cxt_ptr->sem_cap_jpg_done),&ts)){
        LOGD("wait time failed\n");
        return -1;
    }
    return 0;
}

void eng_set_capture_parameter(int cameraId){
    TEST_CMR_CONTEXT_T_PTR cmr_cxt_ptr = g_eng_test_cmr_cxt_ptr;
    struct img_size req_size;
    struct cmr_zoom_param zoom_param;
    struct img_size capture_size;

    ALOGI("Native MMI Test: %s,%d IN\n", __func__, __LINE__);

    if (cameraId == 1){
        cmr_cxt_ptr->capture_width = (cmr_u32)640;
        cmr_cxt_ptr->capture_height = (cmr_u32)480;
    }else if (cameraId == 0){
        cmr_cxt_ptr->capture_width = (cmr_u32)2595;
        cmr_cxt_ptr->capture_height = (cmr_u32)1944;
    }
    zoom_param.mode = 1;
    zoom_param.zoom_level = 1;
    zoom_param.zoom_info.zoom_ratio = 1.00000;
    zoom_param.zoom_info.output_ratio = 4.0/3.0;

    req_size.width = (cmr_u32)cmr_cxt_ptr->capture_width;
    req_size.height = (cmr_u32)cmr_cxt_ptr->capture_height;

    SET_PARM(mHalOem, oem_handle,CAMERA_PARAM_CAPTURE_SIZE,(cmr_uint)&req_size);
    SET_PARM(mHalOem, oem_handle,CAMERA_PARAM_CAPTURE_FORMAT,CAMERA_DATA_FORMAT_YUV420);
    SET_PARM(mHalOem, oem_handle,CAMERA_PARAM_SENSOR_ROTATION,1);

    struct img_rect zoom1 = {0,0,0,0};
    struct cmr_ae_param ae_param;
    mHalOem->ops->camera_get_sensor_trim(oem_handle,&zoom1);
    ae_param.mode = CAMERA_AE_CENTER_WEIGHTED;
    ae_param.win_area.count = 1;
    ae_param.win_area.rect[0].start_x = zoom1.start_x;
    ae_param.win_area.rect[0].start_y = zoom1.start_y;
    ae_param.win_area.rect[0].width = zoom1.width;
    ae_param.win_area.rect[0].height = zoom1.height;

    SET_PARM(mHalOem, oem_handle,CAMERA_PARAM_AUTO_EXPOSURE_MODE,(cmr_uint)&ae_param);
    SET_PARM(mHalOem, oem_handle,CAMERA_PARAM_BRIGHTNESS,3);
    SET_PARM(mHalOem, oem_handle,CAMERA_PARAM_WB,CAMERA_WB_AUTO);
    SET_PARM(mHalOem, oem_handle,CAMERA_PARAM_EXPOSURE_COMPENSATION,4);
    SET_PARM(mHalOem, oem_handle,CAMERA_PARAM_ISO,5);
    SET_PARM(mHalOem, oem_handle,CAMERA_PARAM_FLASH,CAMERA_FLASH_MODE_AUTO);

    SET_PARM(mHalOem, oem_handle,CAMERA_PARAM_ZOOM,(cmr_uint)&zoom_param);

    SET_PARM(mHalOem, oem_handle,CAMERA_PARAM_SHOT_NUM,100);
    SET_PARM(mHalOem, oem_handle,CAMERA_PARAM_FOCAL_LENGTH,2481);
    SET_PARM(mHalOem, oem_handle,CAMERA_PARAM_JPEG_QUALITY,95);
    SET_PARM(mHalOem, oem_handle,CAMERA_PARAM_THUMB_QUALITY,0);

    struct img_size jpeg_thumb_size;
    jpeg_thumb_size.width = 0;
    jpeg_thumb_size.height = 0;
    SET_PARM(mHalOem, oem_handle,CAMERA_PARAM_THUMB_SIZE,(cmr_uint)&jpeg_thumb_size);
}

static void eng_test_dcam_cap_cb(camera_cb_type cb, const void *client_data, camera_func_type func, void* parm4)
{
    TEST_CMR_CONTEXT_T_PTR cmr_cxt_ptr = g_eng_test_cmr_cxt_ptr;
    ALOGI("Native MMI Test: %s,%d IN\n", __func__, __LINE__);

    if (CAMERA_FUNC_TAKE_PICTURE == func) {
	switch(cb) {
	case CAMERA_RSP_CB_SUCCESS:
	    break;
	case CAMERA_EVT_CB_SNAPSHOT_DONE:
	    {
        camera_frame_type *frame_snap = (camera_frame_type *)parm4;
        cmr_cxt_ptr->capture_raw_vir_addr = (uintptr_t)frame_snap->y_vir_addr;
        cmr_cxt_ptr->capture_width = frame_snap->width;
        cmr_cxt_ptr->capture_height= frame_snap->height;
	    }
	        break;
	case CAMERA_EXIT_CB_DONE:
	    {
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
}

extern "C" {
int eng_tst_camera_deinit()
{
    cmr_int ret;

    ALOGI("Native MMI Test: %s,%d IN\n", __func__, __LINE__);

    ret = eng_tst_camera_stoppreview();

    if (oem_handle != NULL && mHalOem != NULL && mHalOem->ops != NULL) {
        ret = mHalOem->ops->camera_deinit(oem_handle);
        if(NULL != mHalOem->dso)    //    modify for coverity issue 131573 , avoid condition "mHalOem == NULL"
           dlclose(mHalOem->dso);
        free((void *)mHalOem);
        mHalOem = NULL;
    }

    free(tmpbuf2);
    free(tmpbuf3);
    return ret;
}

cmr_int autotest_load_hal_lib(void){
	int ret = 0;
       if(!mHalOem) {
           void *handle;
           oem_module_t *omi;

           mHalOem = (oem_module_t *)malloc(sizeof(oem_module_t));
           mHalOem->dso = NULL;    // some modify in this function for coverity issue 131730
           handle = dlopen("libcamoem.so", RTLD_NOW);

           if (handle == NULL) {
               char const *err_str = dlerror();
               ALOGE("dlopen error%s", err_str?err_str:"unknown");
	        ret = -1;
		goto loaderror;
           }
            mHalOem->dso = handle;

           /* Get the address of the struct hal_module_info. */
           const char *sym = OEM_MODULE_INFO_SYM_AS_STR;
           omi = (oem_module_t *)dlsym(handle, sym);
           if (omi == NULL) {
               ALOGE("load: couldn't find symbol %s", sym);
		 ret = -1;
		 goto loaderror;
           }

           mHalOem->ops = omi->ops;

           ALOGV("loaded HAL libcamoem handle=%p", handle);
       }

	 return ret;

loaderror:

	 if(NULL != mHalOem->dso){
               dlclose(mHalOem->dso);
	 }
	 free((void *)mHalOem);
	 mHalOem = NULL;
	 return ret;
}

int eng_tst_camera_init(int cameraId, minui_backend* backend, GRSurface* draw)
{
    int ret = 0;

    ALOGI("Native MMI Test: %s,%d IN\n", __func__, __LINE__);

	gr_backend = backend;
	gr_draw = draw;
	if(2 == cameraId)
		camera_id = 2; // auxiliary camera
	else if (1 == cameraId || 3 == cameraId)
		camera_id = 1; // fore camera
	else
		camera_id = 0; // back camera
	if (getLcdSize(&lcd_w, &lcd_h)){
		/*update preivew size by lcd*/
		ALOGI("%s Native MMI Test: lcd_w=%d,lcd_h=%d\n", __func__,lcd_w, lcd_h);
	}

    tmpbuf2 = (uint8_t*)malloc(lcd_w*lcd_h*4);
	tmpbuf3 = (uint8_t*)malloc(lcd_w*lcd_h*4);
    eng_test_fb_open();
    if(autotest_load_hal_lib()){
        return -1;
    }

    ret = mHalOem->ops->camera_init_with_mem_func(cameraId, eng_tst_camera_cb , &client_data , 0 , &oem_handle,(void*)Callback_Malloc, (void*)Callback_Free);

    eng_tst_camera_startpreview();

    return ret;
}

int eng_camera_capture(int cameraId){
    TEST_CMR_CONTEXT_T_PTR cmr_cxt_ptr = g_eng_test_cmr_cxt_ptr;
    int ret = 0;

    ALOGI("Native MMI Test: %s,%d IN\n", __func__, __LINE__);

    sem_init(&(cmr_cxt_ptr->sem_cap_raw_done),0,0);
    sem_init(&(cmr_cxt_ptr->sem_cap_jpg_done),0,0);

    if(autotest_load_hal_lib()){
        return -1;
    }

    ret = mHalOem->ops->camera_init(cameraId, eng_test_dcam_cap_cb, &client_data, 0, &oem_handle);
    eng_set_capture_parameter(cameraId);
    eng_test_camera_capture(cameraId);
    eng_dcam_save_jpg(1,cameraId);
    mHalOem->ops->camera_deinit(oem_handle);
    printf("test end!!\n");
    return 0;
}

}
