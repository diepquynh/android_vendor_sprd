/*
 * Copyright (C) 2007 The Android Open Source Project
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

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <stdio.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>

#include <linux/fb.h>
#include <linux/kd.h>

#include <time.h>

//#include "font_10x18.h"
//#include "roboto_15x24.h"
//#include "fonten25_15x30.h"
#include "fontcn_37x47.h"
//#include "fontcn_18x35.h"

#include "graphics.h"
int tp_flag1 = 0;

typedef struct {
    GRSurface* texture;
    unsigned offset[97];
    void** fontdata;
    unsigned count;
    unsigned *unicodemap;
    unsigned char *cwidth;
    unsigned char *cheight;
    unsigned ascent;
} GRFont;

static void** font_data;

static GRFont* gr_font = NULL;
static minui_backend* gr_backend = NULL;

static int overscan_percent = OVERSCAN_PERCENT;
static int overscan_offset_x = 0;
static int overscan_offset_y = 0;

static int gr_vt_fd = -1;

static unsigned char gr_current_r = 255;
static unsigned char gr_current_g = 255;
static unsigned char gr_current_b = 255;
static unsigned char gr_current_a = 255;

static GRSurface* gr_draw = NULL;
struct utf8_table {
    int     cmask;
    int     cval;
    int     shift;
    long    lmask;
    long    lval;
};

static struct utf8_table utf8_table[6] =
{
    {0x80,  0x00,   0*6,    0x7F,           0,         /* 1 byte sequence */},
    {0xE0,  0xC0,   1*6,    0x7FF,          0x80,      /* 2 byte sequence */},
    {0xF0,  0xE0,   2*6,    0xFFFF,         0x800,     /* 3 byte sequence */},
    {0xF8,  0xF0,   3*6,    0x1FFFFF,       0x10000,   /* 4 byte sequence */},
    {0xFC,  0xF8,   4*6,    0x3FFFFFF,      0x200000,  /* 5 byte sequence */},
    {0xFE,  0xFC,   5*6,    0x7FFFFFFF,     0x4000000, /* 6 byte sequence */},
};

static bool outside(int x, int y)
{
    return x < 0 || x >= gr_draw->width || y < 0 || y >= gr_draw->height;
}

int gr_measure(const char *s)
{
    return 0;//gr_font->cwidth * strlen(s);
}
/*
void gr_font_size(int *x, int *y)
{
    *x = gr_font->cwidth;
    *y = gr_font->cheight;
}
*/
void gr_font_size( int *cx,int *cy,int *ex,int *ey)
{
    *cx = font.cwidth;
    *cy = font.cheight;
    *ex = font.ewidth;
    *ey = font.eheight;
}

static void text_blend(unsigned char* src_p, int src_row_bytes,
                       unsigned char* dst_p, int dst_row_bytes,
                       int width, int height)
{
	int j, i;
    for (j = 0; j < height; ++j) {
        unsigned char* sx = src_p;
        unsigned char* px = dst_p;
        for (i = 0; i < width; ++i) {
            unsigned char a = *sx++;
            if (gr_current_a < 255) a = ((int)a * gr_current_a) / 255;
            if (a == 255) {
                *px++ = gr_current_r;
                *px++ = gr_current_g;
                *px++ = gr_current_b;
                px++;
            } else if (a > 0) {
                *px = (*px * (255-a) + gr_current_r * a) / 255;
                ++px;
                *px = (*px * (255-a) + gr_current_g * a) / 255;
                ++px;
                *px = (*px * (255-a) + gr_current_b * a) / 255;
                ++px;
                ++px;
            } else {
                px += 4;
            }
        }
        src_p += src_row_bytes;
        dst_p += dst_row_bytes;
    }
}

int getCharID(unsigned unicode){
        unsigned i;
        for (i = 0; i < font.count; i++){
            if (unicode == font.unicodemap[i])
                return i;
        }
        return 0;
}

int utf8towc(wchar_t *p, const char *s, int n){
    wchar_t l;
    int c0, c, nc;
    struct utf8_table *t;
    nc = 0;
    c0 = *s;
    l = c0;
    for (t = utf8_table; t->cmask; t++) {
        nc++;
        if ((c0 & t->cmask) == t->cval) {
            l &= t->lmask;
            if (l < (wchar_t)t->lval)
                return -nc;
            *p = l;
            return nc;
        }
        if (n <= nc)
            return 0;
        s++;
        c = (*s ^ 0x80) & 0xFF;
        if (c & 0xC0)
            return -nc;
        l = (l << 6) | c;
    }
    return -nc;
}

void gr_text(int x, int y, const char *s)
{
    int n;
    unsigned unicode;
//    LOGD("offset_x = %d,offset_y = %d",overscan_offset_x,overscan_offset_y);
    LOGD("font->cheight = %d,font->cwidth = %d",font.cheight,font.cwidth);
    x += overscan_offset_x;
    y += overscan_offset_y;
    y -= font.cheight;
    while(*s){
        if(*(unsigned char*)(s) < 0x20){
            s++;
            continue;
        }
        n = utf8towc((wchar_t*)(&unicode),s,strlen(s));
        if(n<=0)
            break;
        s += n;
        int charID = getCharID(unicode);
        unsigned char* src_p = font_data[charID];
        unsigned char* dst_p = gr_draw->data +y*gr_draw->row_bytes+x*gr_draw->pixel_bytes;
        if (charID <= 95){
            text_blend(src_p,font.ewidth,dst_p,gr_draw->row_bytes,font.ewidth,font.eheight);
            x += font.ewidth;
        }
        else{
            text_blend(src_p,font.cwidth,dst_p,gr_draw->row_bytes,font.cwidth,font.cheight);
            x += font.cwidth;
        }
    }
}

void gr_texticon(int x, int y, GRSurface* icon) {
    if (icon == NULL) return;

    if (icon->pixel_bytes != 1) {
        printf("gr_texticon: source has wrong format\n");
        return;
    }

    x += overscan_offset_x;
    y += overscan_offset_y;

    if (outside(x, y) || outside(x+icon->width-1, y+icon->height-1)) return;

    unsigned char* src_p = icon->data;
    unsigned char* dst_p = gr_draw->data + y*gr_draw->row_bytes + x*gr_draw->pixel_bytes;

    text_blend(src_p, icon->row_bytes,
               dst_p, gr_draw->row_bytes,
               icon->width, icon->height);
}

void gr_color(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    gr_current_r = r;
    gr_current_g = g;
    gr_current_b = b;
    gr_current_a = a;
}

void gr_clear()
{
	int y, x;
	
    if (gr_current_r == gr_current_g && gr_current_r == gr_current_b) {
        memset(gr_draw->data, gr_current_r, gr_draw->height * gr_draw->row_bytes);
    } else {
        unsigned char* px = gr_draw->data;
        for (y = 0; y < gr_draw->height; ++y) {
            for (x = 0; x < gr_draw->width; ++x) {
                *px++ = gr_current_r;
                *px++ = gr_current_g;
                *px++ = gr_current_b;
                px++;
            }
            px += gr_draw->row_bytes - (gr_draw->width * gr_draw->pixel_bytes);
        }
    }
}

void gr_fill(int x1, int y1, int x2, int y2)
{
    x1 += overscan_offset_x;
    y1 += overscan_offset_y;

    x2 += overscan_offset_x;
    y2 += overscan_offset_y;

    if (outside(x1, y1) || outside(x2-1, y2-1)) return;

    unsigned char* p = gr_draw->data + y1 * gr_draw->row_bytes + x1 * gr_draw->pixel_bytes;
    if (gr_current_a == 255) {
        int x, y;
        for (y = y1; y < y2; ++y) {
            unsigned char* px = p;
            for (x = x1; x < x2; ++x) {
                *px++ = gr_current_r;
                *px++ = gr_current_g;
                *px++ = gr_current_b;
                px++;
            }
            p += gr_draw->row_bytes;
        }
    } else if (gr_current_a > 0) {
        int x, y;
        for (y = y1; y < y2; ++y) {
            unsigned char* px = p;
            for (x = x1; x < x2; ++x) {
                *px = (*px * (255-gr_current_a) + gr_current_r * gr_current_a) / 255;
                ++px;
                *px = (*px * (255-gr_current_a) + gr_current_g * gr_current_a) / 255;
                ++px;
                *px = (*px * (255-gr_current_a) + gr_current_b * gr_current_a) / 255;
                ++px;
                ++px;
            }
            p += gr_draw->row_bytes;
        }
    }
}

void gr_blit(GRSurface* source, int sx, int sy, int w, int h, int dx, int dy) {
    if (source == NULL) return;

    if (gr_draw->pixel_bytes != source->pixel_bytes) {
        printf("gr_blit: source has wrong format\n");
        return;
    }

    dx += overscan_offset_x;
    dy += overscan_offset_y;

    if (outside(dx, dy) || outside(dx+w-1, dy+h-1)) return;

    unsigned char* src_p = source->data + sy*source->row_bytes + sx*source->pixel_bytes;
    unsigned char* dst_p = gr_draw->data + dy*gr_draw->row_bytes + dx*gr_draw->pixel_bytes;

    int i;
    for (i = 0; i < h; ++i) {
        memcpy(dst_p, src_p, w * source->pixel_bytes);
        src_p += source->row_bytes;
        dst_p += gr_draw->row_bytes;
    }
}

unsigned int gr_get_width(GRSurface* surface) {
    if (surface == NULL) {
        return 0;
    }
    return surface->width;
}

unsigned int gr_get_height(GRSurface* surface) {
    if (surface == NULL) {
        return 0;
    }
    return surface->height;
}

static void gr_init_font(void)
{
    unsigned char *in, data;
    int bmp, pos;
    unsigned i, d, n;

    font_data = (void**)malloc(font.count * sizeof(void*));
    gr_font = calloc(sizeof(*gr_font),1);
    for(n = 0; n < font.count; n++) {
        if (n<95) {
            font_data[n] = malloc(font.ewidth*font.eheight);
            memset(font_data[n], 0, font.ewidth*font.eheight);
        } else {
            font_data[n] = malloc(font.cwidth*font.cheight);
            memset(font_data[n], 0, font.cwidth * font.cheight);
        }
    }
    d = 0;
    in = font.rundata;
    while(data = *in++) {
        n = data & 0x7f;
        for(i = 0; i < n; i++, d++) {
            if (d<95*font.ewidth*font.eheight) {
                bmp = d/(font.ewidth*font.eheight);
                pos = d%(font.ewidth*font.eheight);
            } else {
                bmp = (d-95*font.ewidth*font.eheight)/(font.cwidth*font.cheight)+95;
                pos = (d-95*font.ewidth*font.eheight)%(font.cwidth*font.cheight);
            }
            ((unsigned char*)(font_data[bmp]))[pos] = (data & 0x80) ? 0xff : 0;
        }
    }
}

#if 0
// Exercises many of the gr_*() functions; useful for testing.
static void gr_test() {
    GRSurface** images;
    int frames;
    int result = res_create_multi_surface("icon_installing", &frames, &images);
    if (result < 0) {
        printf("create surface %d\n", result);
        gr_exit();
        return;
    }

    time_t start = time(NULL);
    int x;
    for (x = 0; x <= 1200; ++x) {
        if (x < 400) {
            gr_color(0, 0, 0, 255);
        } else {
            gr_color(0, (x-400)%128, 0, 255);
        }
        gr_clear();

        gr_color(255, 0, 0, 255);
        gr_surface frame = images[x%frames];
        gr_blit(frame, 0, 0, frame->width, frame->height, x, 0);

        gr_color(255, 0, 0, 128);
        gr_fill(400, 150, 600, 350);

        gr_color(255, 255, 255, 255);
        gr_text(500, 225, "hello, world!", 0);
        gr_color(255, 255, 0, 128);
        gr_text(300+x, 275, "pack my box with five dozen liquor jugs", 1);

        gr_color(0, 0, 255, 128);
        gr_fill(gr_draw->width - 200 - x, 300, gr_draw->width - x, 500);

        gr_draw = gr_backend->flip(gr_backend);
    }
    printf("getting end time\n");
    time_t end = time(NULL);
    printf("got end time\n");
    printf("start %ld end %ld\n", (long)start, (long)end);
    if (end > start) {
        printf("%.2f fps\n", ((double)x) / (end-start));
    }
}
#endif

void gr_tp_flag(int flag)
{
	tp_flag1 = flag;
	return;
}

void gr_flip() {
	unsigned char *tmpbuf = gr_draw->data;
	gr_draw = gr_backend->flip(gr_backend);
	if(!tp_flag1){
	LOGD("gr_flip gr_draw->data len: %d, %d IN\n",strlen(tmpbuf),__LINE__);
	memcpy(gr_draw->data, tmpbuf, gr_draw->width*gr_draw->height*4);
	}
}
/*
void gr_camera_flip(char flag)
{
	int i;
    int width = gr_fb_width();
    int height = gr_fb_height();

  if (gr_draw->pixel_bytes == 2) {
	for(i=0; i<(height>>3); i++){
	  memset(gr_draw.data+(width*(height-(height>>3)+i))*2,0, (height>>3) * 2);
	  memcpy(gr_draw.data+(width*(height-(height>>3)+i))*2, gr_mem_surface.data+(width*(height-(height>>3)+i))*2, (height>>3) * 2);
	  memcpy(gr_draw.data+(width*(height-(height>>3)+i)+(width-(height>>3)))*2, gr_mem_surface.data+(width*(height-(height>>3)+i)+(width-(height>>3)))*2, (height>>3) * 2);
	  if(1 == flag){
			 memcpy(gr_draw.data+(width*(height-(height>>3)+i)+(width/2-(height>>4)))*2, gr_mem_surface.data+(width*(height-(height>>3)+i)+(width/2-(height>>4)))*2,	 (height>>3) * 2);
	  }
	}
  } else {
	for(i=0; i<(height>>3); i++){
	  memset(gr_draw.data+(width*(height-(height>>3)+i))*4,0, (height>>3) * 4);
	  memcpy(gr_draw.data+(width*(height-(height>>3)+i))*4, gr_mem_surface.data+(width*(height-(height>>3)+i))*4, (height>>3) * 4);
	  memcpy(gr_draw.data+(width*(height-(height>>3)+i)+(width-(height>>3)))*4, gr_mem_surface.data+(width*(height-(height>>3)+i)+(width-(height>>3)))*4, (height>>3) * 4);
	  if(1 == flag){
			 memcpy(gr_draw.data+(width*(height-(height>>3)+i)+(width/2-(height>>4)))*4, gr_mem_surface.data+(width*(height-(height>>3)+i)+(width/2-(height>>4)))*4, (height>>3) * 4);
	  }
	}
  }
}
*/

int gr_init(void)
{
    gr_init_font();

    gr_backend = open_adf();
    if (gr_backend) {
        gr_draw = gr_backend->init(gr_backend);
        if (!gr_draw) {
            gr_backend->exit(gr_backend);
        }
    }

    if (!gr_draw) {
        gr_backend = open_fbdev();
        gr_draw = gr_backend->init(gr_backend);
        if (gr_draw == NULL) {
            return -1;
        }
    }
    overscan_offset_x = gr_draw->width * overscan_percent / 100;
    overscan_offset_y = gr_draw->height * overscan_percent / 100;

    gr_flip();
    gr_flip();

    return 0;
}

void gr_exit(void)
{
    gr_backend->exit(gr_backend);
}

minui_backend* gr_backend_get(void)
{
	return gr_backend;
}

GRSurface* gr_draw_get(void)
{
	return gr_draw;
}

int gr_pixel_bytes(void)
{
	return gr_draw->pixel_bytes;
}

int gr_fb_width(void)
{
    return gr_draw->width - 2*overscan_offset_x;
}

int gr_fb_height(void)
{
    return gr_draw->height - 2*overscan_offset_y;
}

void gr_fb_blank(bool blank)
{
    gr_backend->blank(gr_backend, blank);
}
