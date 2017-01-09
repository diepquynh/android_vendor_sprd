#include "dump.h"
#include <cutils/properties.h>
#include <hardware/hardware.h>
#include <hardware/hwcomposer.h>
#include <ui/Rect.h>
#include <ui/GraphicBufferMapper.h>

//static char valuePath[PROPERTY_VALUE_MAX];

static int64_t GeometryChangedNum = 0;
static bool GeometryChanged = false;
static bool GeometryChangedFirst = false;
char dumpPath[MAX_DUMP_PATH_LENGTH];

using namespace android;

static int dump_bmp(const char* filename, void* buffer_addr, unsigned int buffer_format, unsigned int buffer_width, unsigned int buffer_height)
{
    FILE* fp;
    WORD bfType;
    BITMAPINFO bmInfo;
    RGBQUAD quad;
    int ret = 0;
    fp = fopen(filename, "wb");
    if(!fp)
    {
        ret = -1;
        goto fail_open;
    }
    bfType = 0x4D42;

    memset(&bmInfo, 0, sizeof(BITMAPINFO));

    bmInfo.bmfHeader.bfOffBits = sizeof(WORD) + sizeof(BITMAPINFO);
    bmInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmInfo.bmiHeader.biWidth = buffer_width;
    bmInfo.bmiHeader.biHeight = -buffer_height;
    bmInfo.bmiHeader.biPlanes = 1;

    switch (buffer_format)
    {
    case HAL_PIXEL_FORMAT_RGB_565:
        bmInfo.bmfHeader.bfOffBits += 4*sizeof(U32);
        bmInfo.bmiHeader.biBitCount = 16;
        bmInfo.bmiHeader.biCompression = BI_BITFIELDS;
        quad.rgbRedMask      = 0x001F;
        quad.rgbGreenMask    = 0x07E0;
        quad.rgbBlueMask     = 0xF800;
        quad.rgbReservedMask = 0;
        bmInfo.bmiHeader.biSizeImage = buffer_width * buffer_height * sizeof(U16);
        break;

    case HAL_PIXEL_FORMAT_RGBA_8888:
        bmInfo.bmfHeader.bfOffBits += 4*sizeof(U32);
        bmInfo.bmiHeader.biBitCount = 32;
        bmInfo.bmiHeader.biCompression = BI_BITFIELDS;
        quad.rgbRedMask      = 0x00FF0000;
        quad.rgbGreenMask    = 0x0000FF00;
        quad.rgbBlueMask     = 0x000000FF;
        quad.rgbReservedMask = 0xFF000000;
        bmInfo.bmiHeader.biSizeImage = buffer_width * buffer_height * sizeof(U32);
        break;
    case HAL_PIXEL_FORMAT_RGBX_8888:/*not sure need investigation*/
        bmInfo.bmfHeader.bfOffBits += 4*sizeof(U32);
        bmInfo.bmiHeader.biBitCount = 32;
        bmInfo.bmiHeader.biCompression = BI_BITFIELDS;
        quad.rgbRedMask      = 0x00FF0000;
        quad.rgbGreenMask    = 0x0000FF00;
        quad.rgbBlueMask     = 0x000000FF;
        quad.rgbReservedMask = 0x00000000;
        bmInfo.bmiHeader.biSizeImage = buffer_width * buffer_height * sizeof(U32);
        break;
    case 	HAL_PIXEL_FORMAT_BGRA_8888:/*not sure need investigation*/
        bmInfo.bmfHeader.bfOffBits += 4*sizeof(U32);
        bmInfo.bmiHeader.biBitCount = 32;
        bmInfo.bmiHeader.biCompression = BI_BITFIELDS;
        quad.rgbRedMask      = 0x000000FF;
        quad.rgbGreenMask    = 0x0000FF00;
        quad.rgbBlueMask     = 0x00FF0000;
        quad.rgbReservedMask = 0xFF000000;
        bmInfo.bmiHeader.biSizeImage = buffer_width * buffer_height * sizeof(U32);
        break;
    case HAL_PIXEL_FORMAT_RGB_888:/*not sure need investigation*/
        bmInfo.bmfHeader.bfOffBits += 4*sizeof(U32);
        bmInfo.bmiHeader.biBitCount = 24;
        bmInfo.bmiHeader.biCompression = BI_BITFIELDS;
        quad.rgbRedMask      = 0x000000FF;
        quad.rgbGreenMask    = 0x0000FF00;
        quad.rgbBlueMask     = 0x00FF0000;
        quad.rgbReservedMask = 0x00000000;
        bmInfo.bmiHeader.biSizeImage = buffer_width * buffer_height * sizeof(U8) * 3;
        break;
#if 0
    /* do not support HAL_PIXEL_FORMAT_RGBA_5551 */
    case HAL_PIXEL_FORMAT_RGBA_5551: /*not sure need investigation*/
        bmInfo.bmfHeader.bfOffBits += 4*sizeof(U32);
        bmInfo.bmiHeader.biBitCount = 16;
        bmInfo.bmiHeader.biCompression = BI_BITFIELDS;
        quad.rgbRedMask      = 0x000000FF;
        quad.rgbGreenMask    = 0x0000FF00;
        quad.rgbBlueMask     = 0x00FF0000;
        quad.rgbReservedMask = 0x00000000;
        bmInfo.bmiHeader.biSizeImage = buffer_width * buffer_height * sizeof(U8) * 2;
        break;
    case HAL_PIXEL_FORMAT_RGBA_4444:/*not sure need investigation*/
        bmInfo.bmfHeader.bfOffBits += 4*sizeof(U32);
        bmInfo.bmiHeader.biBitCount = 16;
        bmInfo.bmiHeader.biCompression = BI_BITFIELDS;
        quad.rgbRedMask      = 0x000000FF;
        quad.rgbGreenMask    = 0x0000FF00;
        quad.rgbBlueMask     = 0x00FF0000;
        quad.rgbReservedMask = 0x00000000;
        bmInfo.bmiHeader.biSizeImage = buffer_width * buffer_height * sizeof(U8) * 2;
        break;
#endif
    case HAL_PIXEL_FORMAT_YCbCr_420_SP:
    case HAL_PIXEL_FORMAT_YCrCb_420_SP:
    case HAL_PIXEL_FORMAT_YCbCr_420_P:
    case HAL_PIXEL_FORMAT_YV12:
        bmInfo.bmfHeader.bfOffBits += 256*sizeof(U32);
        bmInfo.bmiHeader.biBitCount = 8;
        bmInfo.bmiHeader.biCompression = BI_RGB;
        {
            for(int i=0; i<256; i++)
            {
                quad.table[i].rgbRed      = i;
                quad.table[i].rgbGreen    = i;
                quad.table[i].rgbBlue     = i;
                quad.table[i].rgbReserved = 0;
            }
        }
        bmInfo.bmiHeader.biSizeImage = (buffer_width * buffer_height * sizeof(U8) * 3)>>1;
        break;

    default:
        assert(false);
    }

    bmInfo.bmfHeader.bfSize = bmInfo.bmfHeader.bfOffBits + bmInfo.bmiHeader.biSizeImage;

    switch (buffer_format)
    {
    case HAL_PIXEL_FORMAT_RGB_565:
    case HAL_PIXEL_FORMAT_RGBA_8888:
    case HAL_PIXEL_FORMAT_RGB_888:
    case HAL_PIXEL_FORMAT_BGRA_8888:
#if 0
    case HAL_PIXEL_FORMAT_RGBA_5551:
    case HAL_PIXEL_FORMAT_RGBA_4444:
#endif
    case HAL_PIXEL_FORMAT_RGBX_8888:
        fwrite(&bfType, sizeof(WORD), 1, fp);
        fwrite(&bmInfo, sizeof(BITMAPINFO), 1, fp);
        fwrite(&quad, 4*sizeof(U32), 1, fp);
        break;
    case HAL_PIXEL_FORMAT_YCbCr_420_SP:
    case HAL_PIXEL_FORMAT_YCrCb_420_SP:
    case HAL_PIXEL_FORMAT_YCbCr_420_P:
    case HAL_PIXEL_FORMAT_YV12:
        //fwrite(&quad, 256*sizeof(U32), 1, fp);
        break;
    }
    fwrite(buffer_addr, bmInfo.bmiHeader.biSizeImage, 1, fp);
    fclose(fp);
    return ret;
fail_open:
    ALOGE("dump layer failed to open path is:%s" , filename);
    return ret;
}
static int dump_layer(const char* path ,const char* pSrc , const char* ptype ,  int width , int height , int format , int randNum ,  int index , int LayerIndex = 0) {
    char fileName[MAX_DUMP_PATH_LENGTH + MAX_DUMP_FILENAME_LENGTH];
    static int cnt = 0;
    switch(format)
    {
        case HAL_PIXEL_FORMAT_RGBA_8888:
            sprintf(fileName , "%s%d_%d_%s_%d_rgba_%dx%d_%d.bmp" ,path, cnt,randNum , ptype , LayerIndex , width, height,index);
            break;
        case HAL_PIXEL_FORMAT_RGBX_8888:
            sprintf(fileName , "%s%d_%d_%s_%d_rgbx_%dx%d_%d.bmp" ,path, cnt,randNum , ptype , LayerIndex , width, height,index);
            break;
        case HAL_PIXEL_FORMAT_BGRA_8888:
            sprintf(fileName , "%s%d_%d_%s_%d_bgra_%dx%d_%d.bmp" ,path, cnt,randNum , ptype , LayerIndex ,width, height,index);
            break;
        case HAL_PIXEL_FORMAT_RGB_888:
            sprintf(fileName , "%s%d_%d_%s_%d_rgb888_%dx%d_%d.bmp" ,path, cnt,randNum , ptype , LayerIndex ,width, height,index);
            break;
#if 0
        case HAL_PIXEL_FORMAT_RGBA_5551:
            sprintf(fileName , "%s%d_%d_%s_%d_rgba5551_%dx%d_%d.bmp" ,path, cnt,randNum , ptype , LayerIndex , width, height,index);
            break;
        case HAL_PIXEL_FORMAT_RGBA_4444:
            sprintf(fileName , "%s%d_%d_%s_%d_rgba4444_%dx%d_%d.bmp" ,path,cnt, randNum , ptype , LayerIndex ,width, height,index);
            break;
#endif
        case HAL_PIXEL_FORMAT_RGB_565:
            sprintf(fileName , "%s%d_%d_%s_%d_rgb565_%dx%d_%d.bmp" ,path,cnt, randNum , ptype , LayerIndex , width, height,index);
            break;
        case HAL_PIXEL_FORMAT_YCbCr_420_SP:
            sprintf(fileName , "%s%d_%d_%s_%d_ybrsp_%dx%d_%d.yuv" ,path,cnt, randNum , ptype , LayerIndex , width, height,index);
            break;
        case HAL_PIXEL_FORMAT_YCrCb_420_SP:
            sprintf(fileName , "%s%d_%d_%s_%d_yrbsp_%dx%d_%d.yuv" ,path,cnt, randNum , ptype , LayerIndex , width, height,index);
            break;
        case HAL_PIXEL_FORMAT_YV12:
            sprintf(fileName , "%s%d_%d_%s_%d_yv12_%dx%d_%d.yuv" ,path, cnt,randNum , ptype , LayerIndex , width, height,index);
            break;
        case HAL_PIXEL_FORMAT_YCbCr_420_P:
            sprintf(fileName , "%s%d_%d_%s_%d_ybrp_%dx%d_%d.yuv" ,path, cnt,randNum , ptype , LayerIndex , width, height,index);
            break;
        default:
            ALOGE("dump layer failed because of error format %d" , format);
            return -2;
    }
    cnt++;
    return dump_bmp(fileName , (void*)pSrc, format,width,height);
}

static int getDumpPath(char *pPath)
{
    int mDebugFlag = 0;
    char value[PROPERTY_VALUE_MAX];

    queryDebugFlag(&mDebugFlag);

    if(0 == property_get("dump.hwc.path" , value , "0")) {
        ALOGE_IF(mDebugFlag, "fail to getDumpPath not set path");
        return -1;
    }
    if(strchr(value , '/') != NULL) {
        sprintf(pPath , "%s" , value);
		ALOGE_IF(mDebugFlag, "getDumpPath %s",pPath);
        return 0;
    } else
        pPath[0] = 0;
    ALOGE_IF(mDebugFlag, "fail to getDumpPath path format error");
    return -2;
}

void queryDebugFlag(int *debugFlag)
{
    char value[PROPERTY_VALUE_MAX];
    static int openFileFlag = 0;

    if (debugFlag == NULL)
    {
        ALOGE("queryDebugFlag, input parameter is NULL");
        return;
    }

    property_get("debug.hwc.info", value, "0");

    if (atoi(value) == 1)
    {
        *debugFlag = 1;
    }
    if (atoi(value) == 2)
    {
        *debugFlag = 0;
    }

#define HWC_LOG_PATH "/data/hwc.cfg"
    if (access(HWC_LOG_PATH, R_OK) != 0)
    {
        return;
    }

    FILE *fp = NULL;
    char * pch;
    char cfg[100];

    fp = fopen(HWC_LOG_PATH, "r");
    if (fp != NULL)
    {
        if (openFileFlag == 0)
        {
            int ret;
            memset(cfg, '\0', 100);
            ret = fread(cfg, 1, 99, fp);
            if (ret < 1) {
                ALOGE("fread return size is wrong %d", ret);
            }
            cfg[sizeof(cfg) - 1] = 0;
            pch = strstr(cfg, "enable");
            if (pch != NULL)
            {
                *debugFlag = 1;
                openFileFlag = 1;
            }
        }
        else
        {
            *debugFlag = 1;
        }
        fclose(fp);
    }
}

void queryDumpFlag(int *dumpFlag)
{
    if (dumpFlag == NULL)
    {
        ALOGE("queryDumpFlag, input parameter is NULL");
        return;
    }

    char value[PROPERTY_VALUE_MAX];

    if (0 != property_get("dump.hwc.flag", value, "0"))
    {
        int flag =atoi(value);

        if (flag != 0)
        {
            *dumpFlag = flag;
        }
        else
        {
            *dumpFlag = 0;
        }
    }
    else
    {
        *dumpFlag = 0;
    }
}


void queryIntFlag(const char* strProperty,int *IntFlag)
{
    if (IntFlag == NULL || strProperty == NULL)
    {
        ALOGE("queryIntFlag, input parameter is NULL");
        return;
    }

    char value[PROPERTY_VALUE_MAX];

    if (0 != property_get(strProperty, value, "0"))
    {
        int flag =atoi(value);

        if (flag != 0)
        {
            *IntFlag = flag;
        }
        else
        {
            *IntFlag = -1;
        }
    }
    else
    {
        *IntFlag = -1;
    }
}


int dumpImage(hwc_display_contents_1_t *list)
{
    static int index = 0;

    if (list->flags & HWC_GEOMETRY_CHANGED)
    {
        if (GeometryChangedFirst == false)
        {
            GeometryChangedFirst = true;
            GeometryChangedNum = 0;
        }
        else
        {
            GeometryChangedNum++;
        }
        GeometryChanged = true;
    }
    else
    {
        GeometryChanged = false;
    }

    getDumpPath(dumpPath);
    if (GeometryChanged)
    {
        index = 0;
    }

    for (size_t i =0; i < list->numHwLayers; i++)
    {
        hwc_layer_1_t *l = &(list->hwLayers[i]);
        struct private_handle_t *pH = (struct private_handle_t *)l->handle;
        if (pH == NULL)
        {
            continue;
        }

        Rect bounds(pH->stride, pH->height);
        void* vaddr;

        GraphicBufferMapper::get().lock((buffer_handle_t)pH, GRALLOC_USAGE_SW_READ_OFTEN, bounds, &vaddr);

        dump_layer(dumpPath, (char *)vaddr, "Layer", pH->width, pH->height, pH->format, GeometryChangedNum, index, i);

        GraphicBufferMapper::get().unlock((buffer_handle_t)pH);
    }

    index++;

    return 0;
}

int dumpOverlayImage(private_handle_t* buffer, const char *name)
{
    static int index = 0;

    getDumpPath(dumpPath);

    Rect bounds(buffer->width, buffer->height);
    void* vaddr;

    GraphicBufferMapper::get().lock((buffer_handle_t)buffer, GRALLOC_USAGE_SW_READ_OFTEN, bounds, &vaddr);

    dump_layer(dumpPath, (char const*)vaddr, name,
                   buffer->width, buffer->height,
                   buffer->format, 0, index);

    GraphicBufferMapper::get().unlock((buffer_handle_t)buffer);

    index++;

    return 0;
}


void dumpFrameBuffer(char *virAddr, const char* ptype, int width, int height, int format)
{
    static int index = 0;

    getDumpPath(dumpPath);

    dump_layer(dumpPath, virAddr, ptype, width, height, format, 2, index);
    index++;
}
