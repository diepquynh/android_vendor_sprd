/*
 * Copyright (C) 2012 Spreadtrum Communications Inc.
 *
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <jpeglib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/fb.h>
#include <linux/kd.h>

#include "slog.h"

struct _FBInfo;
typedef struct _FBInfo FBInfo;
typedef int (*UnpackPixel)(FBInfo* fb, unsigned char* pixel, 
	unsigned char* r, unsigned char* g, unsigned char* b);

struct _FBInfo
{
	int fd;
	UnpackPixel unpack;
	unsigned char *bits;
	struct fb_fix_screeninfo fi;
	struct fb_var_screeninfo vi;
};

#define fb_width(fb)  ((fb)->vi.xres)
#define fb_height(fb) ((fb)->vi.yres)
#define fb_bpp(fb)    ((fb)->vi.bits_per_pixel>>3)
#define fb_size(fb)   ((fb)->vi.xres * (fb)->vi.yres * fb_bpp(fb))

static int fb_unpack_rgb565(FBInfo* fb, unsigned char* pixel, 
	unsigned char* r, unsigned char* g, unsigned char* b)
{
	unsigned short color = *(unsigned short*)pixel;

	*r = ((color >> 11) & 0xff) << 3;
	*g = ((color >> 5) & 0xff)  << 2;
	*b = (color & 0xff )<< 3;

	return 0;
}

static int fb_unpack_rgb24(FBInfo* fb, unsigned char* pixel, 
	unsigned char* r, unsigned char* g, unsigned char* b)
{
	*r = pixel[fb->vi.red.offset>>3];
	*g = pixel[fb->vi.green.offset>>3];
	*b = pixel[fb->vi.blue.offset>>3];

	return 0;
}

static int fb_unpack_argb32(FBInfo* fb, unsigned char* pixel, 
	unsigned char* r, unsigned char* g, unsigned char* b)
{
	*r = pixel[fb->vi.red.offset>>3];
	*g = pixel[fb->vi.green.offset>>3];
	*b = pixel[fb->vi.blue.offset>>3];

	return 0;
}

static int fb_unpack_none(FBInfo* fb, unsigned char* pixel, 
	unsigned char* r, unsigned char* g, unsigned char* b)
{
	*r = *g = *b = 0;

	return 0;
}

static void set_pixel_unpacker(FBInfo* fb)
{
	if(fb_bpp(fb) == 2) {
		fb->unpack = fb_unpack_rgb565;
	} else if(fb_bpp(fb) == 3) {
		fb->unpack = fb_unpack_rgb24;
	} else if(fb_bpp(fb) == 4) {
		fb->unpack = fb_unpack_argb32;
	} else {
		fb->unpack = fb_unpack_none;
		err_log("not supported format.");
	}
	return;
}

static int fb_open(FBInfo* fb, const char* fbfilename)
{
	fb->fd = open(fbfilename, O_RDWR);
	if (fb->fd < 0) {
		err_log("can't open %s.", fbfilename);
		return -1;
	}

	if (ioctl(fb->fd, FBIOGET_FSCREENINFO, &fb->fi) < 0)
		goto fail;

	if (ioctl(fb->fd, FBIOGET_VSCREENINFO, &fb->vi) < 0)
		goto fail;

	fb->bits = mmap(0, fb_size(fb), PROT_READ | PROT_WRITE, MAP_SHARED, fb->fd, 0);
	if (fb->bits == MAP_FAILED)
		goto fail;

	debug_log("---------------framebuffer---------------\n");
	debug_log("%s: \n  width : %8d\n  height: %8d\n  bpp   : %8d\n  r(%2d, %2d)\n  g(%2d, %2d)\n  b(%2d, %2d)\n",
			fbfilename, fb_width(fb), fb_height(fb), fb_bpp(fb), 
			fb->vi.red.offset, fb->vi.red.length,
			fb->vi.green.offset, fb->vi.green.length,
			fb->vi.blue.offset, fb->vi.blue.length);
	debug_log("-----------------------------------------\n");

	set_pixel_unpacker(fb);
	return 0;

fail:
	err_log("%s is not a framebuffer.", fbfilename);
	close(fb->fd);

	return -1;
}

static void fb_close(FBInfo* fb)
{
	munmap(fb->bits, fb_size(fb));
	close(fb->fd);

	return;
}

static int snap2jpg(const char * filename, int quality, FBInfo* fb)
{
	int row_stride = 0; 
	FILE * outfile = NULL;
	JSAMPROW row_pointer[1] = {0};
	struct jpeg_error_mgr jerr;
	struct jpeg_compress_struct cinfo;

	memset(&jerr, 0x00, sizeof(jerr));
	memset(&cinfo, 0x00, sizeof(cinfo));

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	if ((outfile = fopen(filename, "wb+")) == NULL) {
		fprintf(stderr, "can't open %s\n", filename);
		return -1;
	}

	jpeg_stdio_dest(&cinfo, outfile);
	cinfo.image_width = fb_width(fb);
	cinfo.image_height = fb_height(fb);
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;
	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, quality, TRUE);
	jpeg_start_compress(&cinfo, TRUE);

	row_stride = fb_width(fb) * 2;
	JSAMPLE* image_buffer = malloc(3 * fb_width(fb));

	while (cinfo.next_scanline < cinfo.image_height) 
	{
		unsigned int i = 0;
		int offset = 0;
		unsigned char* line = fb->bits + cinfo.next_scanline * fb_width(fb) * fb_bpp(fb);

		for(i = 0; i < fb_width(fb); i++, offset += 3, line += fb_bpp(fb))
		{
			fb->unpack(fb, line, image_buffer+offset, image_buffer + offset + 1, image_buffer + offset + 2);
		}

		row_pointer[0] = image_buffer;
		(void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}

	jpeg_finish_compress(&cinfo);
	fclose(outfile);

	jpeg_destroy_compress(&cinfo);
	free(image_buffer);
	return 0;
}

int screen_shot(const char *name)
{
	FBInfo fb;

	if (!screenshot_enable)
		return 0;

	memset(&fb, 0, sizeof(fb));
	if (fb_open(&fb, FB_DEV_NODE) != 0) {
		err_log("open fb devices failed!");
		return -1;
	}
	snap2jpg(name, 100, &fb);
	fb_close(&fb);
	return 0;
}

