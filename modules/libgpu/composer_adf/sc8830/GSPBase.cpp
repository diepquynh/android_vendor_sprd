/*
 * Copyright (C) 2010 The Android Open Source Project
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

/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          Module              DESCRIPTION                             *
 ** 28/02/2015    GSP HAL/GSP class   Used to process HWC accelerate request. *
 **                                   bind with  GSP hw.                      *
 ******************************************************************************
 ** File: GSPBase.cpp                 DESCRIPTION                             *
 **                                   GSP class define                        *
 ******************************************************************************
 ** Author:         tianci.yin@spreadtrum.com                                 *
 *****************************************************************************/

#include "GSPBase.h"

using namespace android;

GSPBase::GSPBase()
    : mDevFd(-1),
      mDebugFlag(0),
      mOutputFormat(0),
      mFBWidth(540),
      mFBHeight(960),
      mSclTmpBuffer(NULL)
{

}

GSPBase::~GSPBase()
{

}

int64_t GSPBase::getSystemTime()
{
    struct timespec t;
    t.tv_sec = t.tv_nsec = 0;
    clock_gettime(CLOCK_MONOTONIC, &t);
    //printf("time: %u:%u.\n",t.tv_sec,t.tv_nsec);
    return t.tv_sec*1000000000LL + t.tv_nsec;
}

void GSPBase::printLayerInfo(hwc_layer_1_t const* l)
{
    GSP_ALOGI_IF(mDebugFlag, "\ttype=%d, flags=%08x, handle=%p, tr=%02x, blend=%04x, {%f,%f,%f,%f}, {%d,%d,%d,%d}",
          l->compositionType, l->flags, l->handle, l->transform, l->blending,
          l->sourceCropf.left,
          l->sourceCropf.top,
          l->sourceCropf.right,
          l->sourceCropf.bottom,
          l->displayFrame.left,
          l->displayFrame.top,
          l->displayFrame.right,
          l->displayFrame.bottom);
}

/** set frame buffer w & h */
void GSPBase::setFBInfo(uint32_t w,uint32_t h)
{
    mFBWidth = w;
    mFBHeight = h;
	ALOGD("GSPBase setFBInfo w:%d,h: %d", w, h);
}

void GSPBase::updateOutputFormat(uint32_t format)
{
    mOutputFormat = format;
}

/*
func:test_gen_white_boundary
desc:draw a white framework boundary in source video layer to check holonomy of video frame
*/
void GSPBase::test_gen_white_boundary(char* base,uint32_t w,uint32_t h,uint32_t format)
{
    uint32_t i=0,r=0,c0=0,c1=0,c2=0,c3=0,c4=0;
    char* base_walk = base;
    uint32_t first_r=0,second_r=16;
    uint32_t first_c=0,second_c=16;

    if(format == HAL_PIXEL_FORMAT_YCbCr_420_SP
       ||format == HAL_PIXEL_FORMAT_YCrCb_420_SP) {
        memset(base_walk+w*first_r, 0xff,w); // 0
        memset(base_walk+w*second_r, 0xff,w); // 10
        memset(base_walk+(w*h>>1), 0xff,w);
        memset(base_walk+w*(h-1-second_r), 0xff,w);
        memset(base_walk+w*(h-1-first_r), 0xff,w);

        base_walk = base;
        r=0;
        c0=first_c;
        c1=second_c;
        c2=(w>>1);
        c3=w-1-second_c;
        c4=w-1-first_c;
        while(r < h) {
            *(base_walk+c0) = 0xff;
            *(base_walk+c1) = 0xff;
            *(base_walk+c2) = 0xff;
            *(base_walk+c3) = 0xff;
            *(base_walk+c4) = 0xff;
            base_walk += w;
            r++;
        }
    }
}


/*
func:test_gen_color_block
desc:generate a pure color block in a pitch_w*pitch_h image, the block position and width height descripted in "rect"
params:
    base: image buffer address
    format:image format
    rect: region of block
    color: block color setting
    gray:the block color have the gray feature
*/
void GSPBase::test_gen_color_block(char* base,uint32_t pitch_w,uint32_t pitch_h,uint32_t format, struct sprdRect *rect,GEN_COLOR color,uint32_t gray)
{
    char red = 0;
    char green = 0;
    char blue = 0;
    uint32_t r = 0,c = 0;

    //params check
    if(base == NULL|| (unsigned long)base & 0x3 ||pitch_w < 90||rect == NULL) {
        return;
    }

    switch(color) {
        default:
        case GEN_COLOR_BLACK:
            red = 0;
            green = 0;
            blue = 0;
            break;
        case GEN_COLOR_BLUE:
            red = 0;
            green = 0;
            blue = 0xff;
            break;
        case GEN_COLOR_RED:
            red = 0xff;
            green = 0;
            blue = 0;
            break;
        case GEN_COLOR_MAGENTA:
            red = 0xff;
            green = 0;
            blue = 0xff;
            break;
        case GEN_COLOR_GREEN:
            red = 0;
            green = 0xff;
            blue = 0;
            break;
        case GEN_COLOR_CYAN:
            red = 0;
            green = 0xff;
            blue = 0xff;
            break;
        case GEN_COLOR_YELLOW:
            red = 0xff;
            green = 0xff;
            blue = 0;
            break;
        case GEN_COLOR_WHITE:
            red = 0xff;
            green = 0xff;
            blue = 0xff;
            break;
    }



    if(format == HAL_PIXEL_FORMAT_RGBA_8888 || format == HAL_PIXEL_FORMAT_RGBX_8888) {
        uint32_t *addr_walk = (uint32_t*)base;
        uint32_t *row_head = (uint32_t*)base;
        uint32_t pixel_value = 0;

        row_head += pitch_w*rect->y+rect->x;
        //0xABGR is GSP default RGB endian
        if(gray==0) {
            pixel_value=(0xff<<24)|(blue<<16)|(green<<8)|red;

            addr_walk = row_head;
            c = 0;
            while(c<rect->w) {
                *addr_walk = pixel_value;
                addr_walk++;
                c++;
            }

            r = 1;
            while(r<rect->h) {
                memcpy((void*)(row_head+pitch_w),row_head,rect->w<<2);
                row_head += pitch_w;
                r++;
            }
        } else {
            char rt = 0;
            char gt = 0;
            char bt = 0;

            float r_step=256.0/rect->h;
            float c_step=256.0/rect->w;

            r=0;
            while(r<rect->h) {
                addr_walk = row_head;
                c=0;
                while(c<rect->w) {
                    rt=gt=bt=r*r_step+c*c_step;
                    rt&=red;
                    gt&=green;
                    bt&=blue;

                    *addr_walk=(0xff<<24)|(bt<<16)|(gt<<8)|rt;
                    addr_walk++;
                    c++;
                }
                row_head += pitch_w;
                r++;
            }
        }
    } else if(format == HAL_PIXEL_FORMAT_RGB_565) {

    } else if(format == HAL_PIXEL_FORMAT_YCbCr_420_SP || format == HAL_PIXEL_FORMAT_YCrCb_420_SP) {
        char *row_head_y = (char*)base;
        char *row_head_uv = (char*)base;
        char *addr_walk_y =  (char*)base;
        char *addr_walk_uv = (char*)base;
        char y = 0;
        char cb = 0;
        char cr = 0;
        //0xY3Y2Y1Y0 0xV1U1V0U0 is GSP default YUV endian

        if(pitch_w & 0x1 || pitch_h & 0x1
           ||rect->x & 0x1 ||rect->y & 0x1 ||rect->w & 0x1 ||rect->h & 0x1 ) {
            return;
        }

        y=0.299*red+0.587*green+0.114*blue;
        cb=-0.16874*red-0.33126*green+0.5*blue+128;
        cr=0.5*red-0.41869*green+128;

        row_head_y += pitch_w*rect->y+rect->x;

        if(gray==0) {
            memset(row_head_y,y,rect->w);
            r = 1;
            while(r<rect->h) {
                memcpy((void*)(row_head_y+pitch_w),row_head_y,rect->w);
                row_head_y += pitch_w;
                r++;
            }
        } else {
            float r_step=(235.0-16.0)/rect->h;
            float c_step=(235.0-16.0)/rect->w;

            r=0;
            while(r<rect->h) {
                addr_walk_y = row_head_y;
                c=0;
                while(c<rect->w) {
                    *addr_walk_y = 16+r*r_step+c*c_step;
                    addr_walk_y++;
                    c++;
                }
                row_head_y += pitch_w;
                r++;
            }
        }

        row_head_uv += pitch_w*pitch_h;
        row_head_uv += (pitch_w*rect->y)/2+rect->x;

        addr_walk_uv = row_head_uv;
        c=0;
        while(c<rect->w) {
            *addr_walk_uv = cb;
            addr_walk_uv++;
            *addr_walk_uv = cr;
            addr_walk_uv++;
            c+=2;
        }

        r = 2;
        while(r<rect->h) {
            memcpy((void*)(row_head_uv+pitch_w),row_head_uv,rect->w);
            row_head_uv += pitch_w;
            r+=2;
        }
    } else {
        //not supported
    }
}


/*
func:test_gen_color_blocks
desc: generate color blocks to verify GSP and DISPC endian setting, block's color follows the below chart
-------------------------
|Red    |Green  |Blue   |
-------------------------
|Yellow |Magenta|Cyan   |
-------------------------
|Black  |White  |Gray   |
-------------------------
*/
void GSPBase::test_gen_color_blocks(char* base,uint32_t pitch_w,uint32_t pitch_h,uint32_t format,uint32_t gray)
{
    uint16_t c = 0;
    uint16_t r = 0;
    uint16_t i = 0;

    uint16_t block_w = 0;
    uint16_t block_h = 0;

    uint16_t block_w_end = 0;
    uint16_t block_h_end = 0;

    struct sprdRect rects[9];

    //params check
    if(base == NULL || pitch_w <90 || pitch_h < 90) {
        return;
    }

    block_w = (pitch_w/3 & 0xfffffffe);
    block_w_end = pitch_w - block_w*2;

    block_h = (pitch_h/3 & 0xfffffffe);
    block_h_end = pitch_h - block_h*2;

    memset((void*)&rects,0,sizeof(rects));
    r=0;
    while(r<3) {
        c=0;
        while(c<3) {
            rects[i].x = block_w*c;
            rects[i].y = block_h*r;

            if(c == 2) {
                rects[i].w = block_w_end;
            } else {
                rects[i].w = block_w;
            }

            if(r == 2) {
                rects[i].h = block_h_end;
            } else {
                rects[i].h = block_h;
            }
            test_gen_color_block(base, pitch_w,pitch_h,format, &rects[i],(GEN_COLOR)i,gray);
            i++;
            c++;
        }
        r++;
    }
}


/*
func:test_color
desc: generate a pure color block and write this data back to layer's data buffer.
        designed to check gsp hal endian setting, and compare the gsp accuracy with gpu.
*/
void GSPBase::test_color(struct private_handle_t *private_h, uint32_t img_format)
{
    int32_t IntFlag = 0;
    //queryIntFlag("gsp.gen.flag", &IntFlag);
    if(IntFlag>0) {
        if (private_h) {
            Rect bounds(private_h->width, private_h->height);
            void* vaddr = NULL;

            GraphicBufferMapper::get().lock((buffer_handle_t)private_h, GRALLOC_USAGE_SW_WRITE_OFTEN, bounds, &vaddr);

            if(vaddr != NULL) {
                GSP_ALOGI_IF(mDebugFlag, "test, set white framework in y plane.");
                if(IntFlag&0x1) {
                    //test yuv endian
                    test_gen_color_blocks((char*)vaddr,private_h->width,private_h->height,img_format,IntFlag&0x4);
                }
                if(IntFlag&0x2) {
                    //test holonomy of video frame
                    test_gen_white_boundary((char*)vaddr,private_h->width,private_h->height,img_format);
                }
            }
            GraphicBufferMapper::get().unlock((buffer_handle_t)private_h);
        }
    }
}

/*
func:test_color_for_prepare
desc: generate a pure color block and write this data back to layer's data buffer.
        designed to check gsp hal endian setting, and compare the gsp accuracy with gpu.
        should be called in hwc prepare phase.
*/
void GSPBase::test_color_for_prepare(hwc_display_contents_1_t *list)
{
    hwc_layer_1_t *l = &(list->hwLayers[0]);
    struct private_handle_t *private_h = (struct private_handle_t *)l->handle;
    test_color(private_h, private_h->format);
}


