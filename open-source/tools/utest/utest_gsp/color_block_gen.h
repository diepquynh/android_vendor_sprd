#ifndef _UTEST_GSP_GEN_COLOR_BLOCK_H_
#define _UTEST_GSP_GEN_COLOR_BLOCK_H_

#include "infrastructure.h"


typedef enum
{
    GEN_COLOR_RED,
    GEN_COLOR_GREEN,
    GEN_COLOR_BLUE,
    GEN_COLOR_YELLOW,
    GEN_COLOR_MAGENTA,
    GEN_COLOR_CYAN,
    GEN_COLOR_BLACK,
    GEN_COLOR_WHITE,
    GEN_COLOR_GRAY
} GEN_COLOR;

extern int add_frame_boundary(GSP_LAYER_INFO_T *pLayer);


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
extern void test_gen_color_block(char* base,uint16_t pitch_w,uint16_t pitch_h,uint16_t format, struct sprdRect *rect,GEN_COLOR color,uint16_t gray);


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
extern void test_gen_color_blocks(char* base,uint32_t pitch_w,uint32_t pitch_h,uint16_t format,uint16_t gray);
#endif

