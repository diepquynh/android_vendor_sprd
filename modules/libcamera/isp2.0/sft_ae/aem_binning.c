/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "aem_binning"
#include "aem_binning.h"
#include "cmr_common.h"

/**---------------------------------------------------------------------------*
 ** 				Compiler Flag					*
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif
/**---------------------------------------------------------------------------*
**				Macro Define					*
**----------------------------------------------------------------------------*/


uint32_t aem_binning(uint16_t bayermode, uint16_t width, uint16_t height, uint16_t *raw_in, uint16_t start_x,
						uint16_t start_y, uint16_t blk_width, uint16_t blk_height, uint32_t *aem_out)
{
	uint32_t rtn = 0;
	int32_t i,j,m,n;
	uint32_t 	*ae_r	= aem_out;
	uint32_t 	*ae_g	= aem_out + 32*32;
	uint32_t 	*ae_b	= aem_out + 32*32*2;
	uint16_t blk_num_h = 32,blk_x,blk_y;
	uint16_t blk_num_v = 32;
	uint16_t blk_id=0;
	uint32_t *ae_block[4] = {ae_g, ae_r, ae_b, ae_g};
	uint16_t data_in;
    uint16_t pixel_type;

	if(NULL == raw_in || NULL == aem_out) {
		CMR_LOGE("Pointer NULL error");
		return -1;
	}

	if((start_x + blk_width * 32) > width || (start_y + blk_height * 32) > height) {
		CMR_LOGE("AEM window out of range error");
		return -1;
	}

	if((start_x % 2 != 0) || (start_y % 2 != 0) || (blk_width % 2 != 0) || (blk_width % 2 != 0)) {
		CMR_LOGE("Alignment error");
		return -1;
	}
	for (i = 0; i < (blk_num_h * blk_num_v); i++)
	{
	    ae_r[i] = 0;
	    ae_g[i] = 0;
	    ae_b[i] = 0;
	}
        for(m=0;m<blk_num_v;m++)
        {
          blk_y=m*blk_height+start_y;//blk起始地址y
          for(n=0;n<blk_num_h;n++)
          {
             blk_x=n*blk_width+start_x;//blk起始地址x

             for(i=0;i<blk_height;i+=2)
             {
               for(j=0;j<blk_width;j+=2)
               {
                  pixel_type = bayermode^((((i)&1)<<1)|((j)&1));
                  ae_block[pixel_type][blk_id] += raw_in[(i+blk_y)*width+j+blk_x];

                  pixel_type = bayermode^((((i)&1)<<1)|((j+1)&1));
                  ae_block[pixel_type][blk_id] += raw_in[(i+blk_y)*width+j+blk_x+1];

                  pixel_type = bayermode^((((i+1)&1)<<1)|((j)&1));
                  ae_block[pixel_type][blk_id] += raw_in[(i+blk_y+1)*width+j+blk_x];

                  pixel_type = bayermode^((((i+1)&1)<<1)|((j+1)&1));
                  ae_block[pixel_type][blk_id] += raw_in[(i+blk_y+1)*width+j+blk_x+1];
               }
             }

          blk_id++;
          }
        }
	for (i=0;i<32*32;i++)
	{
		ae_g[i] = (ae_g[i]+1)>>1;
	}
	return 0;
}

/**----------------------------------------------------------------------------*
**					Compiler Flag				**
**----------------------------------------------------------------------------*/
#ifdef	__cplusplus
}
#endif

