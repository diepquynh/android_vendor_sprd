/*
 * Copyright (C) 2014 ARM Limited. All rights reserved.
 *
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <hardware/hardware.h>
#include <hardware/gralloc.h>

/* The extension bits are defined for indexed formats when setting
 * up the definitions. This define exports them before including
 * the block definitions.
 */
#include "format_chooser.h"

#include 	"formatdef_files/gpu_default.defs"

#if MALI_AFBC_GRALLOC == 1
#if MALI_SUPPORT_AFBC_WIDEBLK == 1
#include    "formatdef_files/gpu_afbc_wideblk.defs"
#else
#include 	"formatdef_files/gpu_afbc.defs"
#endif
#ifdef MALI_DISPLAY_VERSION
#include 	"formatdef_files/display_afbc.defs"
#endif
#endif


/* Defines a translation list of requested formats that are compatible with the internal indexed format */
const internal_fmt_info translate_internal_indexed[GRALLOC_ARM_FORMAT_INTERNAL_INDEXED_LAST] =
{
	{
		HAL_PIXEL_FORMAT_RGBA_8888,
		{GRALLOC_ARM_HAL_FORMAT_INDEXED_RGBA_8888, GRALLOC_ARM_HAL_FORMAT_INDEXED_BGRA_8888}
	},

	{
		HAL_PIXEL_FORMAT_RGBX_8888,
		{GRALLOC_ARM_HAL_FORMAT_INDEXED_RGBX_8888, GRALLOC_ARM_HAL_FORMAT_INDEXED_RGB_888}
	},

	{
		HAL_PIXEL_FORMAT_RGB_888,
		{GRALLOC_ARM_HAL_FORMAT_INDEXED_RGB_888, GRALLOC_ARM_HAL_FORMAT_INDEXED_RGBX_8888}
	},

	{
		HAL_PIXEL_FORMAT_RGB_565,
		{GRALLOC_ARM_HAL_FORMAT_INDEXED_RGB_565}
	},

	{
		HAL_PIXEL_FORMAT_BGRA_8888,
		{GRALLOC_ARM_HAL_FORMAT_INDEXED_BGRA_8888, GRALLOC_ARM_HAL_FORMAT_INDEXED_RGBA_8888}
	},

#if (PLATFORM_SDK_VERSION >= 19) && (PLATFORM_SDK_VERSION <= 22)
	{HAL_PIXEL_FORMAT_sRGB_A_8888, {GRALLOC_ARM_HAL_FORMAT_INDEXED_sRGB_A_8888} },
	{HAL_PIXEL_FORMAT_sRGB_X_8888, {GRALLOC_ARM_HAL_FORMAT_INDEXED_sRGB_X_8888} },
#else
	{0, {GRALLOC_ARM_HAL_FORMAT_INDEXED_sRGB_A_8888} },
	{0, {GRALLOC_ARM_HAL_FORMAT_INDEXED_sRGB_X_8888} },
#endif

	{
		HAL_PIXEL_FORMAT_YV12,
		{GRALLOC_ARM_HAL_FORMAT_INDEXED_YV12}
	},

#if PLATFORM_SDK_VERSION >= 18
	{HAL_PIXEL_FORMAT_Y8, {GRALLOC_ARM_HAL_FORMAT_INDEXED_Y8} },
	{HAL_PIXEL_FORMAT_Y16, {GRALLOC_ARM_HAL_FORMAT_INDEXED_Y16} },
#else
	{0, {GRALLOC_ARM_HAL_FORMAT_INDEXED_Y8} },
	{0, {GRALLOC_ARM_HAL_FORMAT_INDEXED_Y16} },
#endif

	/*{
		GRALLOC_ARM_HAL_FORMAT_INDEXED_NV12,
		{GRALLOC_ARM_HAL_FORMAT_INDEXED_YUV_420_888}
	},*/

	{
		HAL_PIXEL_FORMAT_RGBA_8888 | GRALLOC_ARM_INTFMT_AFBC,
		{GRALLOC_ARM_HAL_FORMAT_INDEXED_RGBA_8888, GRALLOC_ARM_HAL_FORMAT_INDEXED_BGRA_8888}
	},

	{
		HAL_PIXEL_FORMAT_RGBX_8888 | GRALLOC_ARM_INTFMT_AFBC,
		{GRALLOC_ARM_HAL_FORMAT_INDEXED_RGBX_8888, GRALLOC_ARM_HAL_FORMAT_INDEXED_RGB_888}
	},

	{
		HAL_PIXEL_FORMAT_RGB_888 | GRALLOC_ARM_INTFMT_AFBC,
		{GRALLOC_ARM_HAL_FORMAT_INDEXED_RGB_888, GRALLOC_ARM_HAL_FORMAT_INDEXED_RGBX_8888}
	},

	{
		HAL_PIXEL_FORMAT_RGB_565 | GRALLOC_ARM_INTFMT_AFBC,
		{GRALLOC_ARM_HAL_FORMAT_INDEXED_RGB_565}
	},

	{
		HAL_PIXEL_FORMAT_BGRA_8888 | GRALLOC_ARM_INTFMT_AFBC,
		{GRALLOC_ARM_HAL_FORMAT_INDEXED_BGRA_8888, GRALLOC_ARM_HAL_FORMAT_INDEXED_RGBA_8888}
	},

	{
		HAL_PIXEL_FORMAT_YV12 | GRALLOC_ARM_INTFMT_AFBC,
		{GRALLOC_ARM_HAL_FORMAT_INDEXED_YV12, GRALLOC_ARM_HAL_FORMAT_INDEXED_YUV420_8BIT_AFBC}
	},

	{0, {GRALLOC_ARM_HAL_FORMAT_INDEXED_YUV422_8BIT_AFBC}},

	{
		HAL_PIXEL_FORMAT_RGBA_8888 | GRALLOC_ARM_INTFMT_AFBC_SPLITBLK,
		{GRALLOC_ARM_HAL_FORMAT_INDEXED_RGBA_8888, GRALLOC_ARM_HAL_FORMAT_INDEXED_BGRA_8888}
	},

	{
		HAL_PIXEL_FORMAT_RGBX_8888 | GRALLOC_ARM_INTFMT_AFBC_SPLITBLK,
		{GRALLOC_ARM_HAL_FORMAT_INDEXED_RGBX_8888, GRALLOC_ARM_HAL_FORMAT_INDEXED_RGB_888}
	},

	{
		HAL_PIXEL_FORMAT_RGB_888 | GRALLOC_ARM_INTFMT_AFBC_SPLITBLK,
		{GRALLOC_ARM_HAL_FORMAT_INDEXED_RGB_888, GRALLOC_ARM_HAL_FORMAT_INDEXED_RGBX_8888}
	},

	{
		HAL_PIXEL_FORMAT_RGB_565 | GRALLOC_ARM_INTFMT_AFBC_SPLITBLK,
		{GRALLOC_ARM_HAL_FORMAT_INDEXED_RGB_565}
	},

	{
		HAL_PIXEL_FORMAT_BGRA_8888 | GRALLOC_ARM_INTFMT_AFBC_SPLITBLK,
		{GRALLOC_ARM_HAL_FORMAT_INDEXED_BGRA_8888, GRALLOC_ARM_HAL_FORMAT_INDEXED_RGBA_8888}
	},

	{
		HAL_PIXEL_FORMAT_YV12 | GRALLOC_ARM_INTFMT_AFBC_SPLITBLK,
		{GRALLOC_ARM_HAL_FORMAT_INDEXED_YV12, GRALLOC_ARM_HAL_FORMAT_INDEXED_YUV420_8BIT_AFBC}
	},

	{
		HAL_PIXEL_FORMAT_RGBA_8888 | GRALLOC_ARM_INTFMT_AFBC_SPLITBLK | GRALLOC_ARM_INTFMT_AFBC_WIDEBLK,
		{GRALLOC_ARM_HAL_FORMAT_INDEXED_RGBA_8888, GRALLOC_ARM_HAL_FORMAT_INDEXED_BGRA_8888}
	},

	{
		HAL_PIXEL_FORMAT_RGBX_8888 | GRALLOC_ARM_INTFMT_AFBC_SPLITBLK | GRALLOC_ARM_INTFMT_AFBC_WIDEBLK,
		{GRALLOC_ARM_HAL_FORMAT_INDEXED_RGBX_8888, GRALLOC_ARM_HAL_FORMAT_INDEXED_RGB_888}
	},

	{
		HAL_PIXEL_FORMAT_RGB_888 | GRALLOC_ARM_INTFMT_AFBC_SPLITBLK | GRALLOC_ARM_INTFMT_AFBC_WIDEBLK,
		{GRALLOC_ARM_HAL_FORMAT_INDEXED_RGB_888, GRALLOC_ARM_HAL_FORMAT_INDEXED_RGBX_8888}
	},

	{
		HAL_PIXEL_FORMAT_RGB_565 | GRALLOC_ARM_INTFMT_AFBC_WIDEBLK,
		{GRALLOC_ARM_HAL_FORMAT_INDEXED_RGB_565}
	},

	{
		HAL_PIXEL_FORMAT_BGRA_8888 | GRALLOC_ARM_INTFMT_AFBC_SPLITBLK | GRALLOC_ARM_INTFMT_AFBC_WIDEBLK,
		{GRALLOC_ARM_HAL_FORMAT_INDEXED_BGRA_8888, GRALLOC_ARM_HAL_FORMAT_INDEXED_RGBA_8888}
	},

	{
		HAL_PIXEL_FORMAT_YV12 | GRALLOC_ARM_INTFMT_AFBC_WIDEBLK,
		{GRALLOC_ARM_HAL_FORMAT_INDEXED_YV12, GRALLOC_ARM_HAL_FORMAT_INDEXED_YUV420_8BIT_AFBC}
	},

	{0, {GRALLOC_ARM_HAL_FORMAT_INDEXED_YUV422_8BIT_AFBC}},

	/* No mapping as there is no corresponding HAL formats */
	{0, {GRALLOC_ARM_HAL_FORMAT_INDEXED_Y0L2}},
	{0, {GRALLOC_ARM_HAL_FORMAT_INDEXED_P010}},
	{0, {GRALLOC_ARM_HAL_FORMAT_INDEXED_P210}},
	{0, {GRALLOC_ARM_HAL_FORMAT_INDEXED_Y210}},
	{0, {GRALLOC_ARM_HAL_FORMAT_INDEXED_Y410}},
	/* Entry at GRALLOC_ARM_FORMAT_INTERNAL_INDEXED_YUV420_10BIT_AFBC */
	{0, {GRALLOC_ARM_HAL_FORMAT_INDEXED_YUV420_10BIT_AFBC}},
	/* Entry at GRALLOC_ARM_FORMAT_INTERNAL_INDEXED_YUV422_10BIT_AFBC */
	{0, {GRALLOC_ARM_HAL_FORMAT_INDEXED_YUV422_10BIT_AFBC}},
	/* Entry at GRALLOC_ARM_FORMAT_INTERNAL_INDEXED_YUV420_10BIT_AFBC_WIDEBLK */
	{0, {GRALLOC_ARM_HAL_FORMAT_INDEXED_YUV420_10BIT_AFBC}},
	/* Entry at GRALLOC_ARM_FORMAT_INTERNAL_INDEXED_YUV422_10BIT_AFBC_WIDEBLK */
	{0, {GRALLOC_ARM_HAL_FORMAT_INDEXED_YUV422_10BIT_AFBC}},
};

blkinit blklist[] =
{
#if MALI_AFBC_GRALLOC == 1
#ifdef MALI_DISPLAY_VERSION
	{
		display_afbc_hwc_blkinit,{0,{}}
	},
#endif
	{
		gpu_afbc_write_blkinit,{0,{}}
	},
	{
		gpu_afbc_read_blkinit,{0,{}}
	},
#else
	{
		gpu_write_blkinit,{0,{}}
	},
	{
		gpu_read_blkinit,{0,{}}
	},
#endif

	/* Empty entry */
	{
		NULL,{0,{}}
	}
};

uint32_t blklist_array_size = sizeof(blklist);

void initialize_blk_conf()
{
	int i,j,k;

	i=0;
	while( blklist[i].blk_init != 0 )
	{
		int16_t *array=0;

		for(j=0;j<GRALLOC_ARM_HAL_FORMAT_INDEXED_LAST;j++)
		{
			for(k=0; k<(int) GRALLOC_ARM_FORMAT_INTERNAL_INDEXED_LAST; k++)
			{
				blklist[i].hwblkconf.weights[j][k] = DEFAULT_WEIGHT_UNSUPPORTED;
			}
		}
		blklist[i].blk_init( &blklist[i].hwblkconf , &array);
		if( *array != 0 )
		{
			for(k=GRALLOC_ARM_FORMAT_INTERNAL_INDEXED_FIRST; k<GRALLOC_ARM_FORMAT_INTERNAL_INDEXED_LAST; k++)
			{
				/* Zero weights are not suppose to be used in the preferred array because that usually means
				 * uninitialized values.
				 */
				if( array[k] != DEFAULT_WEIGHT_UNSUPPORTED && array[k] != 0 )
				{
					int n;

					/* For this internal format we will for its base format setup matching weights
					 * for itself as well as swizzled versions of the format. When initializing
					 * swizzled/compatible formats with same weight, we insert a slight preference
					 * on the base format(which is listed first) to choose that when the base is selected.
					 * Other blocks' preference might adjust this.
					 */
					for(n=0; n<MAX_COMPATIBLE; n++)
					{
						if( translate_internal_indexed[k].comp_format_list[n] != GRALLOC_ARM_HAL_FORMAT_INDEXED_INVALID )
						{
							if(n==0)
							{
								blklist[i].hwblkconf.weights[translate_internal_indexed[k].comp_format_list[n]][k] = array[k]+1;
							}
							else
							{
								blklist[i].hwblkconf.weights[translate_internal_indexed[k].comp_format_list[n]][k] = array[k];
							}
						}
					}
				}
			}
		}
		i++;
	}
}
