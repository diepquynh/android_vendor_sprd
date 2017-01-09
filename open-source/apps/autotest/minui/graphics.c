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

#include <pixelflinger/pixelflinger.h>

//#include "font_10x18.h"
//#include "fontcn_12x23.h"
#include "fontcn_18x35.h"

#include "minui.h"
static GGLSurface font_ftex;

typedef struct {
    GGLSurface texture;
    unsigned offset[97];
    void** fontdata;
    unsigned count;
    unsigned *unicodemap;
    unsigned char *cwidth;
    unsigned char *cheight;
    unsigned ascent;
} GRFont;

static GRFont *gr_font = 0;
static GGLContext *gr_context = 0;
static GGLSurface gr_font_texture;
static GGLSurface gr_framebuffer[2];
static GGLSurface gr_mem_surface;
static unsigned gr_active_fb = 0;
static int overscan_offset_x = 0;
static int overscan_offset_y = 0;

static int gr_fb_fd = -1;
static int gr_vt_fd = -1;

static struct fb_var_screeninfo vi;

static int get_framebuffer(GGLSurface *fb)
{
    int fd;
    struct fb_fix_screeninfo fi;
    void *bits;

    fd = open("/dev/graphics/fb0", O_RDWR);
    if (fd < 0) {
        perror("cannot open fb0");
        return -1;
    }
	perror("can open fb0");

    if (ioctl(fd, FBIOGET_FSCREENINFO, &fi) < 0) {
        perror("failed to get fb0 info");
        close(fd);
        return -1;
    }

    if (ioctl(fd, FBIOGET_VSCREENINFO, &vi) < 0) {
        perror("failed to get fb0 info");
        close(fd);
        return -1;
    }

    bits = mmap(0, fi.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (bits == MAP_FAILED) {
        perror("failed to mmap framebuffer");
        close(fd);
        return -1;
    }

	if (vi.bits_per_pixel == 16) {
		fb->version = sizeof(*fb);
		fb->width = vi.xres;
		fb->height = vi.yres;
		fb->stride = vi.xres;
		fb->data = bits;
		fb->format = GGL_PIXEL_FORMAT_RGB_565;
		fb++;
		fb->version = sizeof(*fb);
		fb->width = vi.xres;
		fb->height = vi.yres;
		fb->stride = vi.xres;
		fb->data = (void*) (((unsigned long) bits) + vi.yres * vi.xres * 2);
		fb->format = GGL_PIXEL_FORMAT_RGB_565;
	} else {
		fb->version = sizeof(*fb);
		fb->width = vi.xres;
		fb->height = vi.yres;
		fb->stride = vi.xres;
		fb->data = bits;
		fb->format = GGL_PIXEL_FORMAT_RGBA_8888;
		fb++;
		fb->version = sizeof(*fb);
		fb->width = vi.xres;
		fb->height = vi.yres;
		fb->stride = vi.xres;
		fb->data = (void*) (((unsigned long) bits) + vi.yres * vi.xres * 4);
		fb->format = GGL_PIXEL_FORMAT_RGBA_8888;
	}

    return fd;
}

static void get_memory_surface(GGLSurface* ms) {
  ms->version = sizeof(*ms);
  ms->width = vi.xres;
  ms->height = vi.yres;
  ms->stride = vi.xres;
  if (vi.bits_per_pixel == 16) {
    ms->data = malloc(vi.xres * vi.yres * 2);
    ms->format = GGL_PIXEL_FORMAT_RGB_565;
  } else {
    ms->data = malloc(vi.xres * vi.yres * 4);
    ms->format = GGL_PIXEL_FORMAT_RGBA_8888;
  }

}

static void set_active_framebuffer(unsigned n)
{
    if (n > 1) return;
    //vi.yres_virtual = vi.yres * 2;
    vi.yoffset = n * vi.yres;
    //vi.bits_per_pixel = 16;
    if (ioctl(gr_fb_fd, FBIOPUT_VSCREENINFO, &vi) < 0) {
        perror("active fb swap failed");
    }
}

void gr_flip(void)
{
    GGLContext *gl = gr_context;

    /* swap front and back buffers */
    gr_active_fb = (gr_active_fb + 1) & 1;

    /* copy data from the in-memory surface to the buffer we're about
     * to make active. */

  if (vi.bits_per_pixel == 16) {
	  memcpy(gr_framebuffer[gr_active_fb].data, gr_mem_surface.data,
			 vi.xres * vi.yres * 2);
  } else {
	  memcpy(gr_framebuffer[gr_active_fb].data, gr_mem_surface.data,
			 vi.xres * vi.yres * 4);
  }
    /* inform the display driver */
    set_active_framebuffer(gr_active_fb);
}

void gr_color(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    GGLContext *gl = gr_context;
    GGLint color[4];
    color[0] = ((r << 8) | r) + 1;
    color[1] = ((g << 8) | g) + 1;
    color[2] = ((b << 8) | b) + 1;
    color[3] = ((a << 8) | a) + 1;
    gl->color4xv(gl, color);
}

struct utf8_table {
	int     cmask;
	int     cval;
	int     shift;
	long    lmask;
	long    lval;
};

static struct utf8_table utf8_table[] =
{
    {0x80,  0x00,   0*6,    0x7F,           0,         /* 1 byte sequence */},
    {0xE0,  0xC0,   1*6,    0x7FF,          0x80,      /* 2 byte sequence */},
    {0xF0,  0xE0,   2*6,    0xFFFF,         0x800,     /* 3 byte sequence */},
    {0xF8,  0xF0,   3*6,    0x1FFFFF,       0x10000,   /* 4 byte sequence */},
    {0xFC,  0xF8,   4*6,    0x3FFFFFF,      0x200000,  /* 5 byte sequence */},
    {0xFE,  0xFC,   5*6,    0x7FFFFFFF,     0x4000000, /* 6 byte sequence */},
    {0,						       /* end of table    */}
};

int
utf8_mbtowc(wchar_t *p, const char *s, int n)
{
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
			if (l < t->lval)
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

int getCharID(const char* s, void* pFont)
{
	unsigned i, unicode;
	GRFont *gfont = (GRFont*) pFont;
	if (!gfont)  gfont = gr_font;
	utf8_mbtowc(&unicode, s, strlen(s));
	for (i = 0; i < gfont->count; i++)
	{
		if (unicode == gfont->unicodemap[i])
		return i;
	}
	return 0;
}

int gr_measure(const char *s)
{
    GRFont* fnt = NULL;
    int n, l, off;
    wchar_t ch;
     if (!fnt)   fnt = gr_font;



    n = 0;
    off = 0;
    while(*(s + off)) {
        l = utf8_mbtowc(&ch, s+off, strlen(s + off));
		n += fnt->cwidth[getCharID(s+off,NULL)];
        off += l;
    }
    return n;
}

void gr_font_size(int *x, int *y)
{
    *x = gr_font->cwidth;
    *y = gr_font->cheight;
}

int gr_text(int x, int y, const char *s)
{
    GGLContext *gl = gr_context;
    GRFont *gfont = NULL;
    unsigned off, width, height, n;
    wchar_t ch;

    /* Handle default font */
    if (!gfont)  gfont = gr_font;
    x += overscan_offset_x;
    y += overscan_offset_y;
    y -= gfont->ascent;
    // fprintf(stderr, "gr_text: x=%d,y=%d,w=%s\n", x, y, s);
    gl->texEnvi(gl, GGL_TEXTURE_ENV, GGL_TEXTURE_ENV_MODE, GGL_REPLACE);
    gl->texGeni(gl, GGL_S, GGL_TEXTURE_GEN_MODE, GGL_ONE_TO_ONE);
    gl->texGeni(gl, GGL_T, GGL_TEXTURE_GEN_MODE, GGL_ONE_TO_ONE);
    gl->enable(gl, GGL_TEXTURE_2D);

    while(*s) {
        if(*((unsigned char*)(s)) < 0x20) {
            s++;
            continue;
        }
		off = getCharID(s,NULL);
        n = utf8_mbtowc(&ch, s, strlen(s));
        if(n <= 0)
            break;
        s += n;
		width = gfont->cwidth[off];
		height = gfont->cheight[off];
        memcpy(&font_ftex, &gfont->texture, sizeof(font_ftex));
        font_ftex.width = width;
        font_ftex.height = height;
        font_ftex.stride = width;
        font_ftex.data = gfont->fontdata[off];
        gl->bindTexture(gl, &font_ftex);
	    gl->texCoord2i(gl, 0 - x, 0 - y);
        gl->recti(gl, x, y, x + width, y + height);
        x += width;
    }

    return x;
}

void gr_texticon(int x, int y, gr_surface icon) {
    if (gr_context == NULL || icon == NULL) {
        return;
    }
    GGLContext* gl = gr_context;

    x += overscan_offset_x;
    y += overscan_offset_y;

    gl->bindTexture(gl, (GGLSurface*) icon);
    gl->texEnvi(gl, GGL_TEXTURE_ENV, GGL_TEXTURE_ENV_MODE, GGL_REPLACE);
    gl->texGeni(gl, GGL_S, GGL_TEXTURE_GEN_MODE, GGL_ONE_TO_ONE);
    gl->texGeni(gl, GGL_T, GGL_TEXTURE_GEN_MODE, GGL_ONE_TO_ONE);
    gl->enable(gl, GGL_TEXTURE_2D);

    int w = gr_get_width(icon);
    int h = gr_get_height(icon);

    gl->texCoord2i(gl, -x, -y);
    gl->recti(gl, x, y, x+gr_get_width(icon), y+gr_get_height(icon));
}

void gr_fill(int x1, int y1, int x2, int y2)
{
    x1 += overscan_offset_x;
    y1 += overscan_offset_y;

    x2 += overscan_offset_x;
    y2 += overscan_offset_y;

    GGLContext *gl = gr_context;
    gl->disable(gl, GGL_TEXTURE_2D);
    gl->recti(gl, x1, y1, x2, y2);
}

void gr_blit(gr_surface source, int sx, int sy, int w, int h, int dx, int dy) {
    if (gr_context == NULL || source == NULL) {
        return;
    }
    GGLContext *gl = gr_context;

    dx += overscan_offset_x;
    dy += overscan_offset_y;

    gl->bindTexture(gl, (GGLSurface*) source);
    gl->texEnvi(gl, GGL_TEXTURE_ENV, GGL_TEXTURE_ENV_MODE, GGL_REPLACE);
    gl->texGeni(gl, GGL_S, GGL_TEXTURE_GEN_MODE, GGL_ONE_TO_ONE);
    gl->texGeni(gl, GGL_T, GGL_TEXTURE_GEN_MODE, GGL_ONE_TO_ONE);
    gl->enable(gl, GGL_TEXTURE_2D);
    gl->texCoord2i(gl, sx - dx, sy - dy);
    gl->recti(gl, dx, dy, dx + w, dy + h);
}

unsigned int gr_get_width(gr_surface surface) {
    if (surface == NULL) {
        return 0;
    }
    return ((GGLSurface*) surface)->width;
}

unsigned int gr_get_height(gr_surface surface) {
    if (surface == NULL) {
        return 0;
    }
    return ((GGLSurface*) surface)->height;
}

static void gr_init_font(void)
{
    GGLSurface *ftex;
    unsigned char *bits;
    unsigned char *in, data;
    int bmp, pos;
    unsigned i, d, n;
    void** font_data;
    unsigned char *width, *height;
    gr_font = calloc(sizeof(*gr_font), 1);
    ftex = &gr_font->texture;

    font_data = (void**)malloc(font.count * sizeof(void*));
    width = malloc(font.count);
    height = malloc(font.count);
    for(n = 0; n < font.count; n++) {
		if (n<95) {
			font_data[n] = malloc(font.ewidth*font.eheight);
			memset(font_data[n], 0, font.ewidth*font.eheight);
			width[n] = font.ewidth;
			height[n] = font.eheight;
		}
		else {
			font_data[n] = malloc(font.cwidth*font.cheight);
			memset(font_data[n], 0, font.cwidth * font.cheight);
			width[n] = font.cwidth;
			height[n] = font.cheight;
		}
	}
    d = 0;
    in = font.rundata;
    while((data = *in++)) {
        n = data & 0x7f;
        for(i = 0; i < n; i++, d++) {
			if (d<95*font.ewidth*font.eheight) {
				bmp = d/(font.ewidth*font.eheight);
				pos = d%(font.ewidth*font.eheight);
			}
			else {
				bmp = (d-95*font.ewidth*font.eheight)/(font.cwidth*font.cheight)+95;
				pos = (d-95*font.ewidth*font.eheight)%(font.cwidth*font.cheight);
			}
            ((unsigned char*)(font_data[bmp]))[pos] = (data & 0x80) ? 0xff : 0;
        }

    }

    ftex->version = sizeof(*ftex);
    ftex->format = GGL_PIXEL_FORMAT_A_8;
    gr_font->count = font.count;
    gr_font->unicodemap = font.unicodemap;
    gr_font->cwidth = width;
    gr_font->cheight = height;
    gr_font->fontdata = font_data;
    gr_font->ascent = font.cheight - 2;
}

int gr_init(void)
{
    gglInit(&gr_context);
    GGLContext *gl = gr_context;

    gr_init_font();
    /*gr_vt_fd = open("/dev/tty0", O_RDWR | O_SYNC);
    if (gr_vt_fd < 0) {
        // This is non-fatal; post-Cupcake kernels don't have tty0.
        perror("can't open /dev/tty0");
    } else if (ioctl(gr_vt_fd, KDSETMODE, (void*) KD_GRAPHICS)) {
        // However, if we do open tty0, we expect the ioctl to work.
        perror("failed KDSETMODE to KD_GRAPHICS on tty0");
        gr_exit();
        return -1;
    }*/

    gr_fb_fd = get_framebuffer(gr_framebuffer);
    if (gr_fb_fd < 0) {
        gr_exit();
        return -1;
    }

    get_memory_surface(&gr_mem_surface);

    fprintf(stderr, "framebuffer: fd %d (%d x %d)\n",
            gr_fb_fd, gr_framebuffer[0].width, gr_framebuffer[0].height);

        /* start with 0 as front (displayed) and 1 as back (drawing) */
    gr_active_fb = 0;
    set_active_framebuffer(0);
    gl->colorBuffer(gl, &gr_mem_surface);


    gl->activeTexture(gl, 0);
    gl->enable(gl, GGL_BLEND);
    gl->blendFunc(gl, GGL_SRC_ALPHA, GGL_ONE_MINUS_SRC_ALPHA);

    return 0;
}

void gr_exit(void)
{
    close(gr_fb_fd);
    gr_fb_fd = -1;

    free(gr_mem_surface.data);

    ioctl(gr_vt_fd, KDSETMODE, (void*) KD_TEXT);
    close(gr_vt_fd);
    gr_vt_fd = -1;
}

int gr_fb_width(void)
{
    return gr_framebuffer[0].width;
}

int gr_fb_height(void)
{
    return gr_framebuffer[0].height;
}

gr_pixel *gr_fb_data(void)
{
    return (unsigned short *) gr_mem_surface.data;
}
