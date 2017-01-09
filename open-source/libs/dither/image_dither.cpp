#include <stdio.h>
#include <stdlib.h>
#include "math.h"
#include <utils/Timers.h>
#include <time.h>
#include <sys/time.h>
#include <utils/Log.h>
#include <arm_neon.h>
#include "image_dither.h"

#define DEBUG_STR     "dither: L %d, %s: "
#define DEBUG_ARGS    __LINE__,__FUNCTION__
//#define IMG_DITHER_LOGV(format,...) ALOGE(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)
#define IMG_DITHER_LOGV(format,...)
#define IMG_DITHER_LOGE(format,...) ALOGE(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)


#define IMG_CHN_NUM (4)
#define Q_SIZE (8)
#define HILBERT_LEVEL     (4) //the size of block is 16 * 16
#define BLK_WIDTH     (1<<HILBERT_LEVEL)
#define DITHER_MAGIC_FLAG (0x77889521)
#define PNULL (0)
#define PROCESS_BOUNDERY 0

int16_t s_weight[Q_SIZE][Q_SIZE] =
{
    {8, 1, 1, 2, 2, 2, 4, 4},
    {4, 8, 1, 1, 2, 2, 2, 4},
    {4, 4, 8, 1, 1, 2, 2, 2},
    {2, 4, 4, 8, 1, 1, 2, 2},
    {2, 2, 4, 4, 8, 1, 1, 2},
    {2, 2, 2, 4, 4, 8, 1, 1},
    {1, 2, 2, 2, 4, 4, 8, 1},
    {1, 1, 2, 2, 2, 4, 4, 8}
};

int8_t s_weight_n[Q_SIZE][Q_SIZE*2] =
{
    {8, 1, 1, 2, 2, 2, 4, 4, 8, 1, 1, 2, 2, 2, 4, 4},
    {4, 8, 1, 1, 2, 2, 2, 4, 4, 8, 1, 1, 2, 2, 2, 4},
    {4, 4, 8, 1, 1, 2, 2, 2, 4, 4, 8, 1, 1, 2, 2, 2},
    {2, 4, 4, 8, 1, 1, 2, 2, 2, 4, 4, 8, 1, 1, 2, 2},
    {2, 2, 4, 4, 8, 1, 1, 2, 2, 2, 4, 4, 8, 1, 1, 2},
    {2, 2, 2, 4, 4, 8, 1, 1, 2, 2, 2, 4, 4, 8, 1, 1},
    {1, 2, 2, 2, 4, 4, 8, 1, 1, 2, 2, 2, 4, 4, 8, 1},
    {1, 1, 2, 2, 2, 4, 4, 8, 1, 1, 2, 2, 2, 4, 4, 8}
};

//int32_t s_weight[Q_SIZE] = {1, 1, 2, 2, 3, 4, 6, 8};
uint16_t s_hilbert_curve[4][BLK_WIDTH*BLK_WIDTH*2] =
{
    {
     0,  0,  1,  0,  1,  1,  0,  1,  0,  2,  0,  3,  1,  3,  1,  2,  2,  2,  2,  3,  3,  3,  3,  2,  3,  1,  2,  1,  2,  0,  3,  0,
     4,  0,  4,  1,  5,  1,  5,  0,  6,  0,  7,  0,  7,  1,  6,  1,  6,  2,  7,  2,  7,  3,  6,  3,  5,  3,  5,  2,  4,  2,  4,  3,
     4,  4,  4,  5,  5,  5,  5,  4,  6,  4,  7,  4,  7,  5,  6,  5,  6,  6,  7,  6,  7,  7,  6,  7,  5,  7,  5,  6,  4,  6,  4,  7,
     3,  7,  2,  7,  2,  6,  3,  6,  3,  5,  3,  4,  2,  4,  2,  5,  1,  5,  1,  4,  0,  4,  0,  5,  0,  6,  1,  6,  1,  7,  0,  7,
     0,  8,  0,  9,  1,  9,  1,  8,  2,  8,  3,  8,  3,  9,  2,  9,  2, 10,  3, 10,  3, 11,  2, 11,  1, 11,  1, 10,  0, 10,  0, 11,
     0, 12,  1, 12,  1, 13,  0, 13,  0, 14,  0, 15,  1, 15,  1, 14,  2, 14,  2, 15,  3, 15,  3, 14,  3, 13,  2, 13,  2, 12,  3, 12,
     4, 12,  5, 12,  5, 13,  4, 13,  4, 14,  4, 15,  5, 15,  5, 14,  6, 14,  6, 15,  7, 15,  7, 14,  7, 13,  6, 13,  6, 12,  7, 12,
     7, 11,  7, 10,  6, 10,  6, 11,  5, 11,  4, 11,  4, 10,  5, 10,  5,  9,  4,  9,  4,  8,  5,  8,  6,  8,  6,  9,  7,  9,  7,  8,
     8,  8,  8,  9,  9,  9,  9,  8, 10,  8, 11,  8, 11,  9, 10,  9, 10, 10, 11, 10, 11, 11, 10, 11,  9, 11,  9, 10,  8, 10,  8, 11,
     8, 12,  9, 12,  9, 13,  8, 13,  8, 14,  8, 15,  9, 15,  9, 14, 10, 14, 10, 15, 11, 15, 11, 14, 11, 13, 10, 13, 10, 12, 11, 12,
    12, 12, 13, 12, 13, 13, 12, 13, 12, 14, 12, 15, 13, 15, 13, 14, 14, 14, 14, 15, 15, 15, 15, 14, 15, 13, 14, 13, 14, 12, 15, 12,
    15, 11, 15, 10, 14, 10, 14, 11, 13, 11, 12, 11, 12, 10, 13, 10, 13,  9, 12,  9, 12,  8, 13,  8, 14,  8, 14,  9, 15,  9, 15,  8,
    15,  7, 14,  7, 14,  6, 15,  6, 15,  5, 15,  4, 14,  4, 14,  5, 13,  5, 13,  4, 12,  4, 12,  5, 12,  6, 13,  6, 13,  7, 12,  7,
    11,  7, 11,  6, 10,  6, 10,  7,  9,  7,  8,  7,  8,  6,  9,  6,  9,  5,  8,  5,  8,  4,  9,  4, 10,  4, 10,  5, 11,  5, 11,  4,
    11,  3, 11,  2, 10,  2, 10,  3,  9,  3,  8,  3,  8,  2,  9,  2,  9,  1,  8,  1,  8,  0,  9,  0, 10,  0, 10,  1, 11,  1, 11,  0,
    12,  0, 13,  0, 13,  1, 12,  1, 12,  2, 12,  3, 13,  3, 13,  2, 14,  2, 14,  3, 15,  3, 15,  2, 15,  1, 14,  1, 14,  0, 15,  0,
    },

    {
    15, 15, 15, 14, 14, 14, 14, 15, 13, 15, 12, 15, 12, 14, 13, 14, 13, 13, 12, 13, 12, 12, 13, 12, 14, 12, 14, 13, 15, 13, 15, 12,
    15, 11, 14, 11, 14, 10, 15, 10, 15,  9, 15,  8, 14,  8, 14,  9, 13,  9, 13,  8, 12,  8, 12,  9, 12, 10, 13, 10, 13, 11, 12, 11,
    11, 11, 10, 11, 10, 10, 11, 10, 11,  9, 11,  8, 10,  8, 10,  9,  9,  9,  9,  8,  8,  8,  8,  9,  8, 10,  9, 10,  9, 11,  8, 11,
     8, 12,  8, 13,  9, 13,  9, 12, 10, 12, 11, 12, 11, 13, 10, 13, 10, 14, 11, 14, 11, 15, 10, 15,  9, 15,  9, 14,  8, 14,  8, 15,
     7, 15,  6, 15,  6, 14,  7, 14,  7, 13,  7, 12,  6, 12,  6, 13,  5, 13,  5, 12,  4, 12,  4, 13,  4, 14,  5, 14,  5, 15,  4, 15,
     3, 15,  3, 14,  2, 14,  2, 15,  1, 15,  0, 15,  0, 14,  1, 14,  1, 13,  0, 13,  0, 12,  1, 12,  2, 12,  2, 13,  3, 13,  3, 12,
     3, 11,  3, 10,  2, 10,  2, 11,  1, 11,  0, 11,  0, 10,  1, 10,  1,  9,  0,  9,  0,  8,  1,  8,  2,  8,  2,  9,  3,  9,  3,  8,
     4,  8,  5,  8,  5,  9,  4,  9,  4, 10,  4, 11,  5, 11,  5, 10,  6, 10,  6, 11,  7, 11,  7, 10,  7,  9,  6,  9,  6,  8,  7,  8,
     7,  7,  6,  7,  6,  6,  7,  6,  7,  5,  7,  4,  6,  4,  6,  5,  5,  5,  5,  4,  4,  4,  4,  5,  4,  6,  5,  6,  5,  7,  4,  7,
     3,  7,  3,  6,  2,  6,  2,  7,  1,  7,  0,  7,  0,  6,  1,  6,  1,  5,  0,  5,  0,  4,  1,  4,  2,  4,  2,  5,  3,  5,  3,  4,
     3,  3,  3,  2,  2,  2,  2,  3,  1,  3,  0,  3,  0,  2,  1,  2,  1,  1,  0,  1,  0,  0,  1,  0,  2,  0,  2,  1,  3,  1,  3,  0,
     4,  0,  5,  0,  5,  1,  4,  1,  4,  2,  4,  3,  5,  3,  5,  2,  6,  2,  6,  3,  7,  3,  7,  2,  7,  1,  6,  1,  6,  0,  7,  0,
     8,  0,  8,  1,  9,  1,  9,  0, 10,  0, 11,  0, 11,  1, 10,  1, 10,  2, 11,  2, 11,  3, 10,  3,  9,  3,  9,  2,  8,  2,  8,  3,
     8,  4,  9,  4,  9,  5,  8,  5,  8,  6,  8,  7,  9,  7,  9,  6, 10,  6, 10,  7, 11,  7, 11,  6, 11,  5, 10,  5, 10,  4, 11,  4,
    12,  4, 13,  4, 13,  5, 12,  5, 12,  6, 12,  7, 13,  7, 13,  6, 14,  6, 14,  7, 15,  7, 15,  6, 15,  5, 14,  5, 14,  4, 15,  4,
    15,  3, 15,  2, 14,  2, 14,  3, 13,  3, 12,  3, 12,  2, 13,  2, 13,  1, 12,  1, 12,  0, 13,  0, 14,  0, 14,  1, 15,  1, 15,  0,
    },

    {
    15, 15, 14, 15, 14, 14, 15, 14, 15, 13, 15, 12, 14, 12, 14, 13, 13, 13, 13, 12, 12, 12, 12, 13, 12, 14, 13, 14, 13, 15, 12, 15,
    11, 15, 11, 14, 10, 14, 10, 15,  9, 15,  8, 15,  8, 14,  9, 14,  9, 13,  8, 13,  8, 12,  9, 12, 10, 12, 10, 13, 11, 13, 11, 12,
    11, 11, 11, 10, 10, 10, 10, 11,  9, 11,  8, 11,  8, 10,  9, 10,  9,  9,  8,  9,  8,  8,  9,  8, 10,  8, 10,  9, 11,  9, 11,  8,
    12,  8, 13,  8, 13,  9, 12,  9, 12, 10, 12, 11, 13, 11, 13, 10, 14, 10, 14, 11, 15, 11, 15, 10, 15,  9, 14,  9, 14,  8, 15,  8,
    15,  7, 15,  6, 14,  6, 14,  7, 13,  7, 12,  7, 12,  6, 13,  6, 13,  5, 12,  5, 12,  4, 13,  4, 14,  4, 14,  5, 15,  5, 15,  4,
    15,  3, 14,  3, 14,  2, 15,  2, 15,  1, 15,  0, 14,  0, 14,  1, 13,  1, 13,  0, 12,  0, 12,  1, 12,  2, 13,  2, 13,  3, 12,  3,
    11,  3, 10,  3, 10,  2, 11,  2, 11,  1, 11,  0, 10,  0, 10,  1,  9,  1,  9,  0,  8,  0,  8,  1,  8,  2,  9,  2,  9,  3,  8,  3,
     8,  4,  8,  5,  9,  5,  9,  4, 10,  4, 11,  4, 11,  5, 10,  5, 10,  6, 11,  6, 11,  7, 10,  7,  9,  7,  9,  6,  8,  6,  8,  7,
     7,  7,  7,  6,  6,  6,  6,  7,  5,  7,  4,  7,  4,  6,  5,  6,  5,  5,  4,  5,  4,  4,  5,  4,  6,  4,  6,  5,  7,  5,  7,  4,
     7,  3,  6,  3,  6,  2,  7,  2,  7,  1,  7,  0,  6,  0,  6,  1,  5,  1,  5,  0,  4,  0,  4,  1,  4,  2,  5,  2,  5,  3,  4,  3,
     3,  3,  2,  3,  2,  2,  3,  2,  3,  1,  3,  0,  2,  0,  2,  1,  1,  1,  1,  0,  0,  0,  0,  1,  0,  2,  1,  2,  1,  3,  0,  3,
     0,  4,  0,  5,  1,  5,  1,  4,  2,  4,  3,  4,  3,  5,  2,  5,  2,  6,  3,  6,  3,  7,  2,  7,  1,  7,  1,  6,  0,  6,  0,  7,
     0,  8,  1,  8,  1,  9,  0,  9,  0, 10,  0, 11,  1, 11,  1, 10,  2, 10,  2, 11,  3, 11,  3, 10,  3,  9,  2,  9,  2,  8,  3,  8,
     4,  8,  4,  9,  5,  9,  5,  8,  6,  8,  7,  8,  7,  9,  6,  9,  6, 10,  7, 10,  7, 11,  6, 11,  5, 11,  5, 10,  4, 10,  4, 11,
     4, 12,  4, 13,  5, 13,  5, 12,  6, 12,  7, 12,  7, 13,  6, 13,  6, 14,  7, 14,  7, 15,  6, 15,  5, 15,  5, 14,  4, 14,  4, 15,
     3, 15,  2, 15,  2, 14,  3, 14,  3, 13,  3, 12,  2, 12,  2, 13,  1, 13,  1, 12,  0, 12,  0, 13,  0, 14,  1, 14,  1, 15,  0, 15,
    },

    {
     0,  0,  0,  1,  1,  1,  1,  0,  2,  0,  3,  0,  3,  1,  2,  1,  2,  2,  3,  2,  3,  3,  2,  3,  1,  3,  1,  2,  0,  2,  0,  3,
     0,  4,  1,  4,  1,  5,  0,  5,  0,  6,  0,  7,  1,  7,  1,  6,  2,  6,  2,  7,  3,  7,  3,  6,  3,  5,  2,  5,  2,  4,  3,  4,
     4,  4,  5,  4,  5,  5,  4,  5,  4,  6,  4,  7,  5,  7,  5,  6,  6,  6,  6,  7,  7,  7,  7,  6,  7,  5,  6,  5,  6,  4,  7,  4,
     7,  3,  7,  2,  6,  2,  6,  3,  5,  3,  4,  3,  4,  2,  5,  2,  5,  1,  4,  1,  4,  0,  5,  0,  6,  0,  6,  1,  7,  1,  7,  0,
     8,  0,  9,  0,  9,  1,  8,  1,  8,  2,  8,  3,  9,  3,  9,  2, 10,  2, 10,  3, 11,  3, 11,  2, 11,  1, 10,  1, 10,  0, 11,  0,
    12,  0, 12,  1, 13,  1, 13,  0, 14,  0, 15,  0, 15,  1, 14,  1, 14,  2, 15,  2, 15,  3, 14,  3, 13,  3, 13,  2, 12,  2, 12,  3,
    12,  4, 12,  5, 13,  5, 13,  4, 14,  4, 15,  4, 15,  5, 14,  5, 14,  6, 15,  6, 15,  7, 14,  7, 13,  7, 13,  6, 12,  6, 12,  7,
    11,  7, 10,  7, 10,  6, 11,  6, 11,  5, 11,  4, 10,  4, 10,  5,  9,  5,  9,  4,  8,  4,  8,  5,  8,  6,  9,  6,  9,  7,  8,  7,
     8,  8,  9,  8,  9,  9,  8,  9,  8, 10,  8, 11,  9, 11,  9, 10, 10, 10, 10, 11, 11, 11, 11, 10, 11,  9, 10,  9, 10,  8, 11,  8,
    12,  8, 12,  9, 13,  9, 13,  8, 14,  8, 15,  8, 15,  9, 14,  9, 14, 10, 15, 10, 15, 11, 14, 11, 13, 11, 13, 10, 12, 10, 12, 11,
    12, 12, 12, 13, 13, 13, 13, 12, 14, 12, 15, 12, 15, 13, 14, 13, 14, 14, 15, 14, 15, 15, 14, 15, 13, 15, 13, 14, 12, 14, 12, 15,
    11, 15, 10, 15, 10, 14, 11, 14, 11, 13, 11, 12, 10, 12, 10, 13,  9, 13,  9, 12,  8, 12,  8, 13,  8, 14,  9, 14,  9, 15,  8, 15,
     7, 15,  7, 14,  6, 14,  6, 15,  5, 15,  4, 15,  4, 14,  5, 14,  5, 13,  4, 13,  4, 12,  5, 12,  6, 12,  6, 13,  7, 13,  7, 12,
     7, 11,  6, 11,  6, 10,  7, 10,  7,  9,  7,  8,  6,  8,  6,  9,  5,  9,  5,  8,  4,  8,  4,  9,  4, 10,  5, 10,  5, 11,  4, 11,
     3, 11,  2, 11,  2, 10,  3, 10,  3,  9,  3,  8,  2,  8,  2,  9,  1,  9,  1,  8,  0,  8,  0,  9,  0, 10,  1, 10,  1, 11,  0, 11,
     0, 12,  0, 13,  1, 13,  1, 12,  2, 12,  3, 12,  3, 13,  2, 13,  2, 14,  3, 14,  3, 15,  2, 15,  1, 15,  1, 14,  0, 14,  0, 15,
    }
};

static uint16_t s_offset_tbl_0[] =
{
       0,        1,      241,      240,      480,      720,      721,      481,      482,      722,      723,      483,      243,      242,        2,        3,
       4,      244,      245,        5,        6,        7,      247,      246,      486,      487,      727,      726,      725,      485,      484,      724,
     964,     1204,     1205,      965,      966,      967,     1207,     1206,     1446,     1447,     1687,     1686,     1685,     1445,     1444,     1684,
    1683,     1682,     1442,     1443,     1203,      963,      962,     1202,     1201,      961,      960,     1200,     1440,     1441,     1681,     1680,
    1920,     2160,     2161,     1921,     1922,     1923,     2163,     2162,     2402,     2403,     2643,     2642,     2641,     2401,     2400,     2640,
    2880,     2881,     3121,     3120,     3360,     3600,     3601,     3361,     3362,     3602,     3603,     3363,     3123,     3122,     2882,     2883,
    2884,     2885,     3125,     3124,     3364,     3604,     3605,     3365,     3366,     3606,     3607,     3367,     3127,     3126,     2886,     2887,
    2647,     2407,     2406,     2646,     2645,     2644,     2404,     2405,     2165,     2164,     1924,     1925,     1926,     2166,     2167,     1927,
    1928,     2168,     2169,     1929,     1930,     1931,     2171,     2170,     2410,     2411,     2651,     2650,     2649,     2409,     2408,     2648,
    2888,     2889,     3129,     3128,     3368,     3608,     3609,     3369,     3370,     3610,     3611,     3371,     3131,     3130,     2890,     2891,
    2892,     2893,     3133,     3132,     3372,     3612,     3613,     3373,     3374,     3614,     3615,     3375,     3135,     3134,     2894,     2895,
    2655,     2415,     2414,     2654,     2653,     2652,     2412,     2413,     2173,     2172,     1932,     1933,     1934,     2174,     2175,     1935,
    1695,     1694,     1454,     1455,     1215,      975,      974,     1214,     1213,      973,      972,     1212,     1452,     1453,     1693,     1692,
    1691,     1451,     1450,     1690,     1689,     1688,     1448,     1449,     1209,     1208,      968,      969,      970,     1210,     1211,      971,
     731,      491,      490,      730,      729,      728,      488,      489,      249,      248,        8,        9,       10,      250,      251,       11,
      12,       13,      253,      252,      492,      732,      733,      493,      494,      734,      735,      495,      255,      254,       14,       15,
};

static uint16_t s_offset_tbl_1[] =
{
    3615,     3375,     3374,     3614,     3613,     3612,     3372,     3373,     3133,     3132,     2892,     2893,     2894,     3134,     3135,     2895,
    2655,     2654,     2414,     2415,     2175,     1935,     1934,     2174,     2173,     1933,     1932,     2172,     2412,     2413,     2653,     2652,
    2651,     2650,     2410,     2411,     2171,     1931,     1930,     2170,     2169,     1929,     1928,     2168,     2408,     2409,     2649,     2648,
    2888,     3128,     3129,     2889,     2890,     2891,     3131,     3130,     3370,     3371,     3611,     3610,     3609,     3369,     3368,     3608,
    3607,     3606,     3366,     3367,     3127,     2887,     2886,     3126,     3125,     2885,     2884,     3124,     3364,     3365,     3605,     3604,
    3603,     3363,     3362,     3602,     3601,     3600,     3360,     3361,     3121,     3120,     2880,     2881,     2882,     3122,     3123,     2883,
    2643,     2403,     2402,     2642,     2641,     2640,     2400,     2401,     2161,     2160,     1920,     1921,     1922,     2162,     2163,     1923,
    1924,     1925,     2165,     2164,     2404,     2644,     2645,     2405,     2406,     2646,     2647,     2407,     2167,     2166,     1926,     1927,
    1687,     1686,     1446,     1447,     1207,      967,      966,     1206,     1205,      965,      964,     1204,     1444,     1445,     1685,     1684,
    1683,     1443,     1442,     1682,     1681,     1680,     1440,     1441,     1201,     1200,      960,      961,      962,     1202,     1203,      963,
     723,      483,      482,      722,      721,      720,      480,      481,      241,      240,        0,        1,        2,      242,      243,        3,
       4,        5,      245,      244,      484,      724,      725,      485,      486,      726,      727,      487,      247,      246,        6,        7,
       8,      248,      249,        9,       10,       11,      251,      250,      490,      491,      731,      730,      729,      489,      488,      728,
     968,      969,     1209,     1208,     1448,     1688,     1689,     1449,     1450,     1690,     1691,     1451,     1211,     1210,      970,      971,
     972,      973,     1213,     1212,     1452,     1692,     1693,     1453,     1454,     1694,     1695,     1455,     1215,     1214,      974,      975,
     735,      495,      494,      734,      733,      732,      492,      493,      253,      252,       12,       13,       14,      254,      255,       15,
};

static uint16_t s_offset_tbl_2[] =
{
    3615,     3614,     3374,     3375,     3135,     2895,     2894,     3134,     3133,     2893,     2892,     3132,     3372,     3373,     3613,     3612,
    3611,     3371,     3370,     3610,     3609,     3608,     3368,     3369,     3129,     3128,     2888,     2889,     2890,     3130,     3131,     2891,
    2651,     2411,     2410,     2650,     2649,     2648,     2408,     2409,     2169,     2168,     1928,     1929,     1930,     2170,     2171,     1931,
    1932,     1933,     2173,     2172,     2412,     2652,     2653,     2413,     2414,     2654,     2655,     2415,     2175,     2174,     1934,     1935,
    1695,     1455,     1454,     1694,     1693,     1692,     1452,     1453,     1213,     1212,      972,      973,      974,     1214,     1215,      975,
     735,      734,      494,      495,      255,       15,       14,      254,      253,       13,       12,      252,      492,      493,      733,      732,
     731,      730,      490,      491,      251,       11,       10,      250,      249,        9,        8,      248,      488,      489,      729,      728,
     968,     1208,     1209,      969,      970,      971,     1211,     1210,     1450,     1451,     1691,     1690,     1689,     1449,     1448,     1688,
    1687,     1447,     1446,     1686,     1685,     1684,     1444,     1445,     1205,     1204,      964,      965,      966,     1206,     1207,      967,
     727,      726,      486,      487,      247,        7,        6,      246,      245,        5,        4,      244,      484,      485,      725,      724,
     723,      722,      482,      483,      243,        3,        2,      242,      241,        1,        0,      240,      480,      481,      721,      720,
     960,     1200,     1201,      961,      962,      963,     1203,     1202,     1442,     1443,     1683,     1682,     1681,     1441,     1440,     1680,
    1920,     1921,     2161,     2160,     2400,     2640,     2641,     2401,     2402,     2642,     2643,     2403,     2163,     2162,     1922,     1923,
    1924,     2164,     2165,     1925,     1926,     1927,     2167,     2166,     2406,     2407,     2647,     2646,     2645,     2405,     2404,     2644,
    2884,     3124,     3125,     2885,     2886,     2887,     3127,     3126,     3366,     3367,     3607,     3606,     3605,     3365,     3364,     3604,
    3603,     3602,     3362,     3363,     3123,     2883,     2882,     3122,     3121,     2881,     2880,     3120,     3360,     3361,     3601,     3600,
};

static uint16_t s_offset_tbl_3[] =
{
       0,      240,      241,        1,        2,        3,      243,      242,      482,      483,      723,      722,      721,      481,      480,      720,
     960,      961,     1201,     1200,     1440,     1680,     1681,     1441,     1442,     1682,     1683,     1443,     1203,     1202,      962,      963,
     964,      965,     1205,     1204,     1444,     1684,     1685,     1445,     1446,     1686,     1687,     1447,     1207,     1206,      966,      967,
     727,      487,      486,      726,      725,      724,      484,      485,      245,      244,        4,        5,        6,      246,      247,        7,
       8,        9,      249,      248,      488,      728,      729,      489,      490,      730,      731,      491,      251,      250,       10,       11,
      12,      252,      253,       13,       14,       15,      255,      254,      494,      495,      735,      734,      733,      493,      492,      732,
     972,     1212,     1213,      973,      974,      975,     1215,     1214,     1454,     1455,     1695,     1694,     1693,     1453,     1452,     1692,
    1691,     1690,     1450,     1451,     1211,      971,      970,     1210,     1209,      969,      968,     1208,     1448,     1449,     1689,     1688,
    1928,     1929,     2169,     2168,     2408,     2648,     2649,     2409,     2410,     2650,     2651,     2411,     2171,     2170,     1930,     1931,
    1932,     2172,     2173,     1933,     1934,     1935,     2175,     2174,     2414,     2415,     2655,     2654,     2653,     2413,     2412,     2652,
    2892,     3132,     3133,     2893,     2894,     2895,     3135,     3134,     3374,     3375,     3615,     3614,     3613,     3373,     3372,     3612,
    3611,     3610,     3370,     3371,     3131,     2891,     2890,     3130,     3129,     2889,     2888,     3128,     3368,     3369,     3609,     3608,
    3607,     3367,     3366,     3606,     3605,     3604,     3364,     3365,     3125,     3124,     2884,     2885,     2886,     3126,     3127,     2887,
    2647,     2646,     2406,     2407,     2167,     1927,     1926,     2166,     2165,     1925,     1924,     2164,     2404,     2405,     2645,     2644,
    2643,     2642,     2402,     2403,     2163,     1923,     1922,     2162,     2161,     1921,     1920,     2160,     2400,     2401,     2641,     2640,
    2880,     3120,     3121,     2881,     2882,     2883,     3123,     3122,     3362,     3363,     3603,     3602,     3601,     3361,     3360,     3600,
};


struct img_dither_handle{
    uint32_t magic_flag;
    uint32_t handle;
    uint32_t alg_id;
};

struct curve_item_info {
    uint32_t width;
    uint32_t height;
    uint32_t pitch_stride;
    int16_t *weight_ptr[Q_SIZE];
    uint16_t *offset_tbl[4];//0: up; 1: right; 2: down; 3: left
};

struct block_info {
    int16_t *weight_ptr[Q_SIZE];
    uint16_t *offset_tbl;
 };


#define CLIP8(val)\
    if (val & (~255)) {\
        val = ((-val)>>16) & 0xff;\
    }\


#define QCLIP8(_x) \
    do \
    {\
        (_x) &= (~(_x)) >> 31;    \
        (_x) -= 0xff;    \
        (_x) &= (_x) >> 31;    \
        (_x) += 0xff;    \
    }while(0)

struct error_group
{
    int16_t r;
    int16_t g;
    int16_t b;
};


#define CHECK_START_X         2      // start offset to left of screen
#define CHECK_START_Y         2      // start offset to top of screen
#define CHECK_END_X           2      // end offset to right of screen
#define CHECK_END_Y           2      // end offset to bottom of screen
#define ZOOM_RATIO           10      // total samples of n*n points to check within screen range
#define COLOR_DIFF_ALLOWED    2      // do dither if all color difference in picture is less than this threshhold
#define ACTUAL_CHECK_POINTS   (ZOOM_RATIO - CHECK_START_Y - CHECK_END_Y) * (ZOOM_RATIO - CHECK_START_X - CHECK_END_X)

static bool image_dither_precheck(struct img_dither_in_param *in_param)
{
#ifndef ABS
#define ABS(x) (((x)> 0) ? (x) : -(x))
#endif
    uint32_t *src_ptr = (uint32_t *)in_param->data_addr;
    int step_x =  in_param->width / ZOOM_RATIO;
    int step_y =  in_param->height / ZOOM_RATIO;
    int end_x = in_param->width - CHECK_END_X * step_x;
    int end_y = in_param->height - CHECK_END_Y * step_y;
	int i, i0 = CHECK_START_X * step_x + (step_x / 2);
    int j, j0 = CHECK_START_Y * step_y + (step_y / 2);
    uint32_t value = *(src_ptr + i0 + in_param->width * j0);
    uint8_t c0 = value & 0xff;
    uint8_t c1 = (value >> 8) & 0xff;
    uint8_t c2 = (value >> 16) & 0xff;\

	IMG_DITHER_LOGV("image_dither_precheck %d %d %d %d %d %d", i0, j0, step_x, step_y, end_x, end_y);
    for(j = j0; j < end_y; j += step_y) {
        for(i = i0; i < end_x; i += step_x) {
            uint8_t x0, x1, x2, x3;
            value = *(src_ptr + i + j * in_param->width);
            x0 = value & 0xff;
            x1 = (value >> 8) & 0xff;
            x2 = (value >> 16) & 0xff;
            if((ABS((x0 - c0)) > COLOR_DIFF_ALLOWED) 
				|| (ABS((x1 - c1)) > COLOR_DIFF_ALLOWED) 
				|| (ABS((x2 - c2)) > COLOR_DIFF_ALLOWED)) {
				IMG_DITHER_LOGV("******************** true do dither %d %d %d %d %d %d %d %d", i, j, x0, x1, x2, c0, c1, c2);
                return true;
            }
        }
    }
	IMG_DITHER_LOGV("-------------- false no dither %d %d %d %d %d", i, j, c0, c1, c2);
    return false;
}

static void dither_block(uint32_t *src_ptr, uint32_t width, struct block_info *blk_info)
{
    uint16_t *coor_ptr = (uint16_t *)blk_info->offset_tbl;
    int16_t **weights_ptr = blk_info->weight_ptr;
    uint32_t k=0;

    int32_t r_err = 0;
    int32_t g_err = 0;
    int32_t b_err = 0;

    int16x8_t v_error_r;
    int16x8_t v_error_g;
    int16x8_t v_error_b;

    int16x8_t v_weight0 ;
    int16x8_t v_weight1 ;
    int16x8_t v_weight2 ;
    int16x8_t v_weight3 ;
    int16x8_t v_weight4 ;
    int16x8_t v_weight5 ;
    int16x8_t v_weight6 ;
    int16x8_t v_weight7 ;

    v_error_r = vdupq_n_s16(0);
    v_error_g = vdupq_n_s16(0);
    v_error_b = vdupq_n_s16(0);

    v_weight0 = vld1q_s16(weights_ptr[0]);
    v_weight1 = vld1q_s16(weights_ptr[1]);
    v_weight2 = vld1q_s16(weights_ptr[2]);
    v_weight3 = vld1q_s16(weights_ptr[3]);
    v_weight4 = vld1q_s16(weights_ptr[4]);
    v_weight5 = vld1q_s16(weights_ptr[5]);
    v_weight6 = vld1q_s16(weights_ptr[6]);
    v_weight7 = vld1q_s16(weights_ptr[7]);


    do
    {
        uint32_t offset;
        int32_t r_value, g_value, b_value;
        int32_t src_r_val, src_g_val, src_b_val;
        uint32_t argb = 0;

        int16x8_t v_r_16x8;
        int32x4_t v_r_32x4;
        int64x2_t v_r_64x2;

        int16x8_t v_g_16x8;
        int32x4_t v_g_32x4;
        int64x2_t v_g_64x2;

        int16x8_t v_b_16x8;
        int32x4_t v_b_32x4;
        int64x2_t v_b_64x2;


        // 1st pixel
        {
            offset = *coor_ptr++;
            argb = *(src_ptr + offset);
            src_b_val = argb & 0xff;
            src_g_val = (argb & 0xff00) >> 8;
            src_r_val = (argb & 0xff0000) >> 16;

            b_value = src_b_val + (b_err >> 3);
            g_value = src_g_val + (g_err >> 3);
            r_value = src_r_val + (r_err >> 3);

            CLIP8(b_value)
            CLIP8(g_value)
            CLIP8(r_value)

            b_value = b_value>>3;
            g_value = g_value>>2;
            r_value = r_value>>3;

            argb = argb & 0xff000000;
            argb = (b_value << 3) | (g_value << 10);
            argb |= (r_value << 19);

            b_err = src_b_val - (b_value<<3);
            g_err = src_g_val - (g_value<<2);
            r_err = src_r_val - (r_value<<3);

            *(src_ptr + offset) = argb;

            v_error_r = vsetq_lane_s16(r_err, v_error_r, 0);
            v_error_g = vsetq_lane_s16(g_err, v_error_g, 0);
            v_error_b = vsetq_lane_s16(b_err, v_error_b, 0);

            v_b_16x8 = vmulq_s16(v_error_b, v_weight0);
            v_g_16x8 = vmulq_s16(v_error_g, v_weight0);
            v_r_16x8 = vmulq_s16(v_error_r, v_weight0);

            v_b_32x4 = vpaddlq_s16(v_b_16x8);
            v_g_32x4 = vpaddlq_s16(v_g_16x8);
            v_r_32x4 = vpaddlq_s16(v_r_16x8);

            v_b_64x2 = vpaddlq_s32(v_b_32x4);
            v_g_64x2 = vpaddlq_s32(v_g_32x4);
            v_r_64x2 = vpaddlq_s32(v_r_32x4);

            b_err = vgetq_lane_s64(v_b_64x2, 0) + vgetq_lane_s64(v_b_64x2, 1);
            g_err = vgetq_lane_s64(v_g_64x2, 0) + vgetq_lane_s64(v_g_64x2, 1);
            r_err = vgetq_lane_s64(v_r_64x2, 0) + vgetq_lane_s64(v_r_64x2, 1);
        }

        // 2nd pixel
        {
            offset = *coor_ptr++;

            argb = *(src_ptr + offset);
            src_b_val = argb & 0xff;
            src_g_val = (argb & 0xff00) >> 8;
            src_r_val = (argb & 0xff0000) >> 16;

            b_value = src_b_val + (b_err >> 3);
            g_value = src_g_val + (g_err >> 3);
            r_value = src_r_val + (r_err >> 3);

            CLIP8(b_value)
            CLIP8(g_value)
            CLIP8(r_value)

            b_value = b_value>>3;
            g_value = g_value>>2;
            r_value = r_value>>3;

            argb = argb & 0xff000000;
            argb = (b_value << 3) | (g_value << 10);
            argb |= (r_value << 19);

            b_err = src_b_val - (b_value<<3);
            g_err = src_g_val - (g_value<<2);
            r_err = src_r_val - (r_value<<3);

            *(src_ptr + offset) = argb;

            v_error_r = vsetq_lane_s16(r_err, v_error_r, 1);
            v_error_g = vsetq_lane_s16(g_err, v_error_g, 1);
            v_error_b = vsetq_lane_s16(b_err, v_error_b, 1);

            v_b_16x8 = vmulq_s16(v_error_b, v_weight1);
            v_g_16x8 = vmulq_s16(v_error_g, v_weight1);
            v_r_16x8 = vmulq_s16(v_error_r, v_weight1);

            v_b_32x4 = vpaddlq_s16(v_b_16x8);
            v_g_32x4 = vpaddlq_s16(v_g_16x8);
            v_r_32x4 = vpaddlq_s16(v_r_16x8);

            v_b_64x2 = vpaddlq_s32(v_b_32x4);
            v_g_64x2 = vpaddlq_s32(v_g_32x4);
            v_r_64x2 = vpaddlq_s32(v_r_32x4);

            b_err = vgetq_lane_s64(v_b_64x2, 0) + vgetq_lane_s64(v_b_64x2, 1);
            g_err = vgetq_lane_s64(v_g_64x2, 0) + vgetq_lane_s64(v_g_64x2, 1);
            r_err = vgetq_lane_s64(v_r_64x2, 0) + vgetq_lane_s64(v_r_64x2, 1);
        }

        // 3rd pixel
        {
            offset = *coor_ptr++;

            argb = *(src_ptr + offset);
            src_b_val = argb & 0xff;
            src_g_val = (argb & 0xff00) >> 8;
            src_r_val = (argb & 0xff0000) >> 16;

            b_value = src_b_val + (b_err >> 3);
            g_value = src_g_val + (g_err >> 3);
            r_value = src_r_val + (r_err >> 3);

            CLIP8(b_value)
            CLIP8(g_value)
            CLIP8(r_value)

            b_value = b_value>>3;
            g_value = g_value>>2;
            r_value = r_value>>3;

            argb = argb & 0xff000000;
            argb = (b_value << 3) | (g_value << 10);
            argb |= (r_value << 19);

            b_err = src_b_val - (b_value<<3);
            g_err = src_g_val - (g_value<<2);
            r_err = src_r_val - (r_value<<3);

            *(src_ptr + offset) = argb;

            v_error_r = vsetq_lane_s16(r_err, v_error_r, 2);
            v_error_g = vsetq_lane_s16(g_err, v_error_g, 2);
            v_error_b = vsetq_lane_s16(b_err, v_error_b, 2);

            v_b_16x8 = vmulq_s16(v_error_b, v_weight2);
            v_g_16x8 = vmulq_s16(v_error_g, v_weight2);
            v_r_16x8 = vmulq_s16(v_error_r, v_weight2);

            v_b_32x4 = vpaddlq_s16(v_b_16x8);
            v_g_32x4 = vpaddlq_s16(v_g_16x8);
            v_r_32x4 = vpaddlq_s16(v_r_16x8);

            v_b_64x2 = vpaddlq_s32(v_b_32x4);
            v_g_64x2 = vpaddlq_s32(v_g_32x4);
            v_r_64x2 = vpaddlq_s32(v_r_32x4);

            b_err = vgetq_lane_s64(v_b_64x2, 0) + vgetq_lane_s64(v_b_64x2, 1);
            g_err = vgetq_lane_s64(v_g_64x2, 0) + vgetq_lane_s64(v_g_64x2, 1);
            r_err = vgetq_lane_s64(v_r_64x2, 0) + vgetq_lane_s64(v_r_64x2, 1);

        }

        // 4th pixel
        {
            offset = *coor_ptr++;
            argb = *(src_ptr + offset);
            src_b_val = argb & 0xff;
            src_g_val = (argb & 0xff00) >> 8;
            src_r_val = (argb & 0xff0000) >> 16;

            b_value = src_b_val + (b_err >> 3);
            g_value = src_g_val + (g_err >> 3);
            r_value = src_r_val + (r_err >> 3);

            CLIP8(b_value)
            CLIP8(g_value)
            CLIP8(r_value)

            b_value = b_value>>3;
            g_value = g_value>>2;
            r_value = r_value>>3;

            argb = argb & 0xff000000;
            argb = (b_value << 3) | (g_value << 10);
            argb |= (r_value << 19);

            b_err = src_b_val - (b_value<<3);
            g_err = src_g_val - (g_value<<2);
            r_err = src_r_val - (r_value<<3);

            *(src_ptr + offset) = argb;

            v_error_r = vsetq_lane_s16(r_err, v_error_r, 3);
            v_error_g = vsetq_lane_s16(g_err, v_error_g, 3);
            v_error_b = vsetq_lane_s16(b_err, v_error_b, 3);

            v_b_16x8 = vmulq_s16(v_error_b, v_weight3);
            v_g_16x8 = vmulq_s16(v_error_g, v_weight3);
            v_r_16x8 = vmulq_s16(v_error_r, v_weight3);

            v_b_32x4 = vpaddlq_s16(v_b_16x8);
            v_g_32x4 = vpaddlq_s16(v_g_16x8);
            v_r_32x4 = vpaddlq_s16(v_r_16x8);

            v_b_64x2 = vpaddlq_s32(v_b_32x4);
            v_g_64x2 = vpaddlq_s32(v_g_32x4);
            v_r_64x2 = vpaddlq_s32(v_r_32x4);

            b_err = vgetq_lane_s64(v_b_64x2, 0) + vgetq_lane_s64(v_b_64x2, 1);
            g_err = vgetq_lane_s64(v_g_64x2, 0) + vgetq_lane_s64(v_g_64x2, 1);
            r_err = vgetq_lane_s64(v_r_64x2, 0) + vgetq_lane_s64(v_r_64x2, 1);
        }

        //5th pixel
        {
            offset = *coor_ptr++;
            argb = *(src_ptr + offset);
            src_b_val = argb & 0xff;
            src_g_val = (argb & 0xff00) >> 8;
            src_r_val = (argb & 0xff0000) >> 16;

            b_value = src_b_val + (b_err >> 3);
            g_value = src_g_val + (g_err >> 3);
            r_value = src_r_val + (r_err >> 3);

            CLIP8(b_value)
            CLIP8(g_value)
            CLIP8(r_value)

            b_value = b_value>>3;
            g_value = g_value>>2;
            r_value = r_value>>3;

            argb = argb & 0xff000000;
            argb = (b_value << 3) | (g_value << 10);
            argb |= (r_value << 19);

            b_err = src_b_val - (b_value<<3);
            g_err = src_g_val - (g_value<<2);
            r_err = src_r_val - (r_value<<3);

            *(src_ptr + offset) = argb;

            v_error_r = vsetq_lane_s16(r_err, v_error_r, 4);
            v_error_g = vsetq_lane_s16(g_err, v_error_g, 4);
            v_error_b = vsetq_lane_s16(b_err, v_error_b, 4);

            v_b_16x8 = vmulq_s16(v_error_b, v_weight4);
            v_g_16x8 = vmulq_s16(v_error_g, v_weight4);
            v_r_16x8 = vmulq_s16(v_error_r, v_weight4);

            v_b_32x4 = vpaddlq_s16(v_b_16x8);
            v_g_32x4 = vpaddlq_s16(v_g_16x8);
            v_r_32x4 = vpaddlq_s16(v_r_16x8);

            v_b_64x2 = vpaddlq_s32(v_b_32x4);
            v_g_64x2 = vpaddlq_s32(v_g_32x4);
            v_r_64x2 = vpaddlq_s32(v_r_32x4);

            b_err = vgetq_lane_s64(v_b_64x2, 0) + vgetq_lane_s64(v_b_64x2, 1);
            g_err = vgetq_lane_s64(v_g_64x2, 0) + vgetq_lane_s64(v_g_64x2, 1);
            r_err = vgetq_lane_s64(v_r_64x2, 0) + vgetq_lane_s64(v_r_64x2, 1);
        }

        //6th pixel
        {
            offset = *coor_ptr++;
            argb = *(src_ptr + offset);
            src_b_val = argb & 0xff;
            src_g_val = (argb & 0xff00) >> 8;
            src_r_val = (argb & 0xff0000) >> 16;

            b_value = src_b_val + (b_err >> 3);
            g_value = src_g_val + (g_err >> 3);
            r_value = src_r_val + (r_err >> 3);

            CLIP8(b_value)
            CLIP8(g_value)
            CLIP8(r_value)

            b_value = b_value>>3;
            g_value = g_value>>2;
            r_value = r_value>>3;

            argb = argb & 0xff000000;
            argb = (b_value << 3) | (g_value << 10);
            argb |= (r_value << 19);

            b_err = src_b_val - (b_value<<3);
            g_err = src_g_val - (g_value<<2);
            r_err = src_r_val - (r_value<<3);

            *(src_ptr + offset) = argb;

            v_error_r = vsetq_lane_s16(r_err, v_error_r, 5);
            v_error_g = vsetq_lane_s16(g_err, v_error_g, 5);
            v_error_b = vsetq_lane_s16(b_err, v_error_b, 5);

            v_b_16x8 = vmulq_s16(v_error_b, v_weight5);
            v_g_16x8 = vmulq_s16(v_error_g, v_weight5);
            v_r_16x8 = vmulq_s16(v_error_r, v_weight5);

            v_b_32x4 = vpaddlq_s16(v_b_16x8);
            v_g_32x4 = vpaddlq_s16(v_g_16x8);
            v_r_32x4 = vpaddlq_s16(v_r_16x8);

            v_b_64x2 = vpaddlq_s32(v_b_32x4);
            v_g_64x2 = vpaddlq_s32(v_g_32x4);
            v_r_64x2 = vpaddlq_s32(v_r_32x4);

            b_err = vgetq_lane_s64(v_b_64x2, 0) + vgetq_lane_s64(v_b_64x2, 1);
            g_err = vgetq_lane_s64(v_g_64x2, 0) + vgetq_lane_s64(v_g_64x2, 1);
            r_err = vgetq_lane_s64(v_r_64x2, 0) + vgetq_lane_s64(v_r_64x2, 1);
        }

        //7th pixel
        {
            offset = *coor_ptr++;
            argb = *(src_ptr + offset);
            src_b_val = argb & 0xff;
            src_g_val = (argb & 0xff00) >> 8;
            src_r_val = (argb & 0xff0000) >> 16;

            b_value = src_b_val + (b_err >> 3);
            g_value = src_g_val + (g_err >> 3);
            r_value = src_r_val + (r_err >> 3);

            CLIP8(b_value)
            CLIP8(g_value)
            CLIP8(r_value)

            b_value = b_value>>3;
            g_value = g_value>>2;
            r_value = r_value>>3;

            argb = argb & 0xff000000;
            argb = (b_value << 3) | (g_value << 10);
            argb |= (r_value << 19);

            b_err = src_b_val - (b_value<<3);
            g_err = src_g_val - (g_value<<2);
            r_err = src_r_val - (r_value<<3);

            *(src_ptr + offset) = argb;

            v_error_r = vsetq_lane_s16(r_err, v_error_r, 6);
            v_error_g = vsetq_lane_s16(g_err, v_error_g, 6);
            v_error_b = vsetq_lane_s16(b_err, v_error_b, 6);

            v_b_16x8 = vmulq_s16(v_error_b, v_weight6);
            v_g_16x8 = vmulq_s16(v_error_g, v_weight6);
            v_r_16x8 = vmulq_s16(v_error_r, v_weight6);

            v_b_32x4 = vpaddlq_s16(v_b_16x8);
            v_g_32x4 = vpaddlq_s16(v_g_16x8);
            v_r_32x4 = vpaddlq_s16(v_r_16x8);

            v_b_64x2 = vpaddlq_s32(v_b_32x4);
            v_g_64x2 = vpaddlq_s32(v_g_32x4);
            v_r_64x2 = vpaddlq_s32(v_r_32x4);

            b_err = vgetq_lane_s64(v_b_64x2, 0) + vgetq_lane_s64(v_b_64x2, 1);
            g_err = vgetq_lane_s64(v_g_64x2, 0) + vgetq_lane_s64(v_g_64x2, 1);
            r_err = vgetq_lane_s64(v_r_64x2, 0) + vgetq_lane_s64(v_r_64x2, 1);
        }

        //8th pixel
        {
            offset = *coor_ptr++;
            argb = *(src_ptr + offset);
            src_b_val = argb & 0xff;
            src_g_val = (argb & 0xff00) >> 8;
            src_r_val = (argb & 0xff0000) >> 16;

            b_value = src_b_val + (b_err >> 3);
            g_value = src_g_val + (g_err >> 3);
            r_value = src_r_val + (r_err >> 3);

            CLIP8(b_value)
            CLIP8(g_value)
            CLIP8(r_value)

            b_value = b_value>>3;
            g_value = g_value>>2;
            r_value = r_value>>3;

            argb = argb & 0xff000000;
            argb = (b_value << 3) | (g_value << 10);
            argb |= (r_value << 19);

            b_err = src_b_val - (b_value<<3);
            g_err = src_g_val - (g_value<<2);
            r_err = src_r_val - (r_value<<3);

            *(src_ptr + offset) = argb;

            v_error_r = vsetq_lane_s16(r_err, v_error_r, 7);
            v_error_g = vsetq_lane_s16(g_err, v_error_g, 7);
            v_error_b = vsetq_lane_s16(b_err, v_error_b, 7);

            v_b_16x8 = vmulq_s16(v_error_b, v_weight7);
            v_g_16x8 = vmulq_s16(v_error_g, v_weight7);
            v_r_16x8 = vmulq_s16(v_error_r, v_weight7);

            v_b_32x4 = vpaddlq_s16(v_b_16x8);
            v_g_32x4 = vpaddlq_s16(v_g_16x8);
            v_r_32x4 = vpaddlq_s16(v_r_16x8);

            v_b_64x2 = vpaddlq_s32(v_b_32x4);
            v_g_64x2 = vpaddlq_s32(v_g_32x4);
            v_r_64x2 = vpaddlq_s32(v_r_32x4);

            b_err = vgetq_lane_s64(v_b_64x2, 0) + vgetq_lane_s64(v_b_64x2, 1);
            g_err = vgetq_lane_s64(v_g_64x2, 0) + vgetq_lane_s64(v_g_64x2, 1);
            r_err = vgetq_lane_s64(v_r_64x2, 0) + vgetq_lane_s64(v_r_64x2, 1);
        }

        k++;
    }while(k<32);
}

uint32_t _dither(void* src, void* dst, struct curve_item_info *itm_info_ptr)
{
    struct curve_item_info *curve_infp_ptr = (struct curve_item_info*)itm_info_ptr;
    struct block_info blk_info;

    uint32_t width = 0, height = 0, i = 0;
    uint32_t *src_ptr = NULL;
    uint32_t v_cnts = 0, h_cnts = 0;
    uint32_t src_stride = 0, dst_stride = 0;

    nsecs_t begin_time;
    nsecs_t end_time;

    width = curve_infp_ptr->width;
    height = curve_infp_ptr->height;
    h_cnts = (width>>HILBERT_LEVEL);
    v_cnts = (height>>HILBERT_LEVEL);
    src_stride = BLK_WIDTH * width;
    dst_stride = BLK_WIDTH * width;

    for (i=0; i<Q_SIZE; i++)
    {
        blk_info.weight_ptr[i] = curve_infp_ptr->weight_ptr[i];
    }

    begin_time = systemTime(SYSTEM_TIME_MONOTONIC);

    src_ptr = (uint32_t*)src;
    for (i=0; i<v_cnts; i++)
    {
        uint32_t *src_blk_ptr = (uint32_t*)src_ptr;
        uint32_t j;

        for (j=0; j<h_cnts; j++)
        {
            uint32_t k;
            uint32_t coor_mode = ((i & 0x01)<<1) + (j & 0x01);
            blk_info.offset_tbl = curve_infp_ptr->offset_tbl[coor_mode];

            dither_block(src_blk_ptr, width, &blk_info);

            src_blk_ptr += BLK_WIDTH;
        }

        src_ptr += src_stride;
    }

    end_time = systemTime(SYSTEM_TIME_MONOTONIC);

    IMG_DITHER_LOGV("dither end: size (%d, %d), time = %lldum", width, height, (end_time - begin_time)/1000);

    return 0;
}


void _calc_offset_table(uint16_t *offset_tbl, uint16_t *coor_tbl, uint32_t width)
{
    uint32_t i=0;
    uint32_t counts = 0;
    counts = BLK_WIDTH * BLK_WIDTH;

    for (i=0; i<counts; i++)
    {
        uint32_t x = *coor_tbl++;
        uint32_t y = *coor_tbl++;

        *offset_tbl++ = y * width + x;
    }
}

static void dither_four_block(uint32_t *src_ptr[4], uint16_t *offset_tbl[4])
{
    uint32_t k=0;
    uint16_t *offset_ptr[4] = {0};
    uint32_t mask[4] = {0xfff8fcf8, 0xfff8fcf8, 0xfff8fcf8, 0xfff8fcf8};

    uint8x16_t  v_src_u8x16;
    uint32x4_t  v_src_u32x4;

    uint16x8_t  v_src_low_u16x8;
    uint16x8_t  v_src_high_u16x8;

    uint8x16_t  v_dst_u8x16;
    uint32x4_t  v_dst_u32x4;
    uint16x8_t  v_dst_high_u16x8;
    uint16x8_t  v_dst_low_u16x8;
    uint8x8_t   v_dst_low_u8x8;
    uint8x8_t   v_dst_high_u8x8;
    uint8x16_t  v_tmp_u8x16;

    uint16x8_t  v_tmp_high_u16x8;
    uint16x8_t  v_tmp_low_u16x8;

    int16x8_t  v_tmp_high_s16x8;
    int16x8_t  v_tmp_low_s16x8;

    uint8x16_t  v_mask_u8x16;

    int8x16_t    v_err_0;
    int8x16_t    v_err_1;
    int8x16_t    v_err_2;
    int8x16_t    v_err_3;
    int8x16_t    v_err_4;
    int8x16_t    v_err_5;
    int8x16_t    v_err_6;
    int8x16_t    v_err_7;

    int8x16_t    v_sum0_s8x16;
    int8x16_t    v_sum1_s8x16;
    int8x16_t    v_sum2_s8x16;
    int8x16_t    v_sum3_s8x16;
    int8x16_t    v_sum4_s8x16;
    int8x16_t    v_8x_s8x16;
    int8x16_t    v_1x_s8x16;
    int8x16_t    v_2x_s8x16;
    int8x16_t    v_4x_s8x16;

    int8x16_t    v_tmp5_s8x16;
    int8x16_t    v_tmp6_s8x16;

    int8x16_t    v_err_s8x16;
    uint8x16_t   v_err_u8x16;

    offset_ptr[0] = offset_tbl[0];
    offset_ptr[1] = offset_tbl[1];
    offset_ptr[2] = offset_tbl[2];
    offset_ptr[3] = offset_tbl[3];

    v_mask_u8x16 = vld1q_u8((uint8_t *)mask);

    v_err_0 = vdupq_n_s8(0);
    v_err_1 = vdupq_n_s8(0);
    v_err_2 = vdupq_n_s8(0);
    v_err_3 = vdupq_n_s8(0);
    v_err_4 = vdupq_n_s8(0);
    v_err_5 = vdupq_n_s8(0);
    v_err_6 = vdupq_n_s8(0);
    v_err_7 = vdupq_n_s8(0);
    v_err_s8x16 = vdupq_n_s8(0);

    do
    {
        uint32_t offset[4];

        // 0st pixel
        {
            //read two pixels
            offset[0] = *offset_ptr[0]++;
            offset[1] = *offset_ptr[1]++;
            offset[2] = *offset_ptr[2]++;
            offset[3] = *offset_ptr[3]++;
            v_src_u32x4 = vld1q_lane_u32(src_ptr[0] + offset[0], v_src_u32x4, 0);
            v_src_u32x4 = vld1q_lane_u32(src_ptr[1] + offset[1], v_src_u32x4, 1);
            v_src_u32x4 = vld1q_lane_u32(src_ptr[2] + offset[2], v_src_u32x4, 2);
            v_src_u32x4 = vld1q_lane_u32(src_ptr[3] + offset[3], v_src_u32x4, 3);

            v_src_u8x16 = vreinterpretq_u8_u32(v_src_u32x4);

            v_src_low_u16x8 = vmovl_u8(vget_low_u8(v_src_u8x16));
            v_src_high_u16x8 = vmovl_u8(vget_high_u8(v_src_u8x16));

            v_tmp_low_s16x8 = vaddw_s8(vreinterpretq_s16_u16(v_src_low_u16x8), vget_low_s8(v_err_s8x16));
            v_tmp_high_s16x8 = vaddw_s8(vreinterpretq_s16_u16(v_src_high_u16x8), vget_high_s8(v_err_s8x16));

            v_dst_low_u8x8 = vqmovun_s16(v_tmp_low_s16x8);
            v_dst_high_u8x8 = vqmovun_s16(v_tmp_high_s16x8);

            v_tmp_u8x16 = vcombine_u8(v_dst_low_u8x8, v_dst_high_u8x8);

            v_dst_u8x16 = vandq_u8(v_tmp_u8x16, v_mask_u8x16);
            v_err_u8x16 = vsubq_u8(v_src_u8x16, v_dst_u8x16);
            v_err_s8x16 = vreinterpretq_s8_u8(v_err_u8x16);
            v_err_0 = v_err_s8x16;
            v_dst_u32x4 = vreinterpretq_u32_u8(v_dst_u8x16);

            vst1q_lane_u32(src_ptr[0] + offset[0], v_dst_u32x4, 0);
            vst1q_lane_u32(src_ptr[1] + offset[1], v_dst_u32x4, 1);
            vst1q_lane_u32(src_ptr[2] + offset[2], v_dst_u32x4, 2);
            vst1q_lane_u32(src_ptr[3] + offset[3], v_dst_u32x4, 3);

            //weight: {8, 1, 1, 2, 2, 2, 4, 4}
            v_8x_s8x16 = vshlq_n_s8(v_err_0, 3);          // x8
            v_1x_s8x16 = vaddq_s8(v_err_1, v_err_2);      // x1
            v_2x_s8x16 = vaddq_s8(v_err_3, v_err_4);
            v_4x_s8x16 = vaddq_s8(v_err_6, v_err_7);
            v_2x_s8x16 = vaddq_s8(v_2x_s8x16, v_err_5);

            v_tmp5_s8x16 = vshlq_n_s8(v_2x_s8x16, 1);    // x2
            v_tmp6_s8x16 = vshlq_n_s8(v_4x_s8x16, 2);    // x4

            // error = (1x+2x+4x+8x) / 8
            v_sum0_s8x16 = vaddq_s8(v_8x_s8x16, v_1x_s8x16);  // 8x + 1x
            v_sum1_s8x16 = vaddq_s8(v_tmp6_s8x16, v_tmp5_s8x16);  //1// 4x + 2x
            v_sum2_s8x16 = vqaddq_s8(v_sum0_s8x16, v_sum1_s8x16);
            v_err_s8x16 = vshrq_n_s8(v_sum2_s8x16, 3);
        }

        // 1st pixel
        {
            //read two pixels
            offset[0] = *offset_ptr[0]++;
            offset[1] = *offset_ptr[1]++;
            offset[2] = *offset_ptr[2]++;
            offset[3] = *offset_ptr[3]++;
            v_src_u32x4 = vld1q_lane_u32(src_ptr[0] + offset[0], v_src_u32x4, 0);
            v_src_u32x4 = vld1q_lane_u32(src_ptr[1] + offset[1], v_src_u32x4, 1);
            v_src_u32x4 = vld1q_lane_u32(src_ptr[2] + offset[2], v_src_u32x4, 2);
            v_src_u32x4 = vld1q_lane_u32(src_ptr[3] + offset[3], v_src_u32x4, 3);

            v_src_u8x16 = vreinterpretq_u8_u32(v_src_u32x4);

            v_src_low_u16x8 = vmovl_u8(vget_low_u8(v_src_u8x16));
            v_src_high_u16x8 = vmovl_u8(vget_high_u8(v_src_u8x16));

            v_tmp_low_s16x8 = vaddw_s8(vreinterpretq_s16_u16(v_src_low_u16x8), vget_low_s8(v_err_s8x16));
            v_tmp_high_s16x8 = vaddw_s8(vreinterpretq_s16_u16(v_src_high_u16x8), vget_high_s8(v_err_s8x16));

            v_dst_low_u8x8 = vqmovun_s16(v_tmp_low_s16x8);
            v_dst_high_u8x8 = vqmovun_s16(v_tmp_high_s16x8);

            v_tmp_u8x16 = vcombine_u8(v_dst_low_u8x8, v_dst_high_u8x8);

            v_dst_u8x16 = vandq_u8(v_tmp_u8x16, v_mask_u8x16);
            v_err_u8x16 = vsubq_u8(v_src_u8x16, v_dst_u8x16);
            v_err_s8x16 = vreinterpretq_s8_u8(v_err_u8x16);
            v_err_1 = v_err_s8x16;
            v_dst_u32x4 = vreinterpretq_u32_u8(v_dst_u8x16);

            vst1q_lane_u32(src_ptr[0] + offset[0], v_dst_u32x4, 0);
            vst1q_lane_u32(src_ptr[1] + offset[1], v_dst_u32x4, 1);
            vst1q_lane_u32(src_ptr[2] + offset[2], v_dst_u32x4, 2);
            vst1q_lane_u32(src_ptr[3] + offset[3], v_dst_u32x4, 3);

            //weight: {4, 8, 1, 1, 2, 2, 2, 4}
            v_8x_s8x16 = vshlq_n_s8(v_err_1, 3);          // x8
            v_1x_s8x16 = vaddq_s8(v_err_2, v_err_3);      // x1
            v_2x_s8x16 = vaddq_s8(v_err_4, v_err_5);
            v_4x_s8x16 = vaddq_s8(v_err_7, v_err_0);
            v_2x_s8x16 = vaddq_s8(v_2x_s8x16, v_err_6);

            v_tmp5_s8x16 = vshlq_n_s8(v_2x_s8x16, 1);    // x2
            v_tmp6_s8x16 = vshlq_n_s8(v_4x_s8x16, 2);    // x4

            // error = (1x+2x+4x+8x) / 8
            v_sum0_s8x16 = vaddq_s8(v_8x_s8x16, v_1x_s8x16);  // 8x + 1x
            v_sum1_s8x16 = vaddq_s8(v_tmp6_s8x16, v_tmp5_s8x16);  //1// 4x + 2x
            v_sum2_s8x16 = vqaddq_s8(v_sum0_s8x16, v_sum1_s8x16);
            v_err_s8x16 = vshrq_n_s8(v_sum2_s8x16, 3);
        }

        // 2st pixel
        {
            //read two pixels
            offset[0] = *offset_ptr[0]++;
            offset[1] = *offset_ptr[1]++;
            offset[2] = *offset_ptr[2]++;
            offset[3] = *offset_ptr[3]++;
            v_src_u32x4 = vld1q_lane_u32(src_ptr[0] + offset[0], v_src_u32x4, 0);
            v_src_u32x4 = vld1q_lane_u32(src_ptr[1] + offset[1], v_src_u32x4, 1);
            v_src_u32x4 = vld1q_lane_u32(src_ptr[2] + offset[2], v_src_u32x4, 2);
            v_src_u32x4 = vld1q_lane_u32(src_ptr[3] + offset[3], v_src_u32x4, 3);

            v_src_u8x16 = vreinterpretq_u8_u32(v_src_u32x4);

            v_src_low_u16x8 = vmovl_u8(vget_low_u8(v_src_u8x16));
            v_src_high_u16x8 = vmovl_u8(vget_high_u8(v_src_u8x16));

            v_tmp_low_s16x8 = vaddw_s8(vreinterpretq_s16_u16(v_src_low_u16x8), vget_low_s8(v_err_s8x16));
            v_tmp_high_s16x8 = vaddw_s8(vreinterpretq_s16_u16(v_src_high_u16x8), vget_high_s8(v_err_s8x16));

            v_dst_low_u8x8 = vqmovun_s16(v_tmp_low_s16x8);
            v_dst_high_u8x8 = vqmovun_s16(v_tmp_high_s16x8);

            v_tmp_u8x16 = vcombine_u8(v_dst_low_u8x8, v_dst_high_u8x8);

            v_dst_u8x16 = vandq_u8(v_tmp_u8x16, v_mask_u8x16);
            v_err_u8x16 = vsubq_u8(v_src_u8x16, v_dst_u8x16);
            v_err_s8x16 = vreinterpretq_s8_u8(v_err_u8x16);
            v_err_2 = v_err_s8x16;
            v_dst_u32x4 = vreinterpretq_u32_u8(v_dst_u8x16);

            vst1q_lane_u32(src_ptr[0] + offset[0], v_dst_u32x4, 0);
            vst1q_lane_u32(src_ptr[1] + offset[1], v_dst_u32x4, 1);
            vst1q_lane_u32(src_ptr[2] + offset[2], v_dst_u32x4, 2);
            vst1q_lane_u32(src_ptr[3] + offset[3], v_dst_u32x4, 3);

            //weight: {4, 4, 8, 1, 1, 2, 2, 2}
            v_8x_s8x16 = vshlq_n_s8(v_err_2, 3);          // x8
            v_1x_s8x16 = vaddq_s8(v_err_3, v_err_4);      // x1
            v_2x_s8x16 = vaddq_s8(v_err_5, v_err_6);
            v_4x_s8x16 = vaddq_s8(v_err_0, v_err_1);
            v_2x_s8x16 = vaddq_s8(v_2x_s8x16, v_err_7);

            v_tmp5_s8x16 = vshlq_n_s8(v_2x_s8x16, 1);    // x2
            v_tmp6_s8x16 = vshlq_n_s8(v_4x_s8x16, 2);    // x4

            // error = (1x+2x+4x+8x) / 8
            v_sum0_s8x16 = vaddq_s8(v_8x_s8x16, v_1x_s8x16);  // 8x + 1x
            v_sum1_s8x16 = vaddq_s8(v_tmp6_s8x16, v_tmp5_s8x16);  //1// 4x + 2x
            v_sum2_s8x16 = vqaddq_s8(v_sum0_s8x16, v_sum1_s8x16);
            v_err_s8x16 = vshrq_n_s8(v_sum2_s8x16, 3);
        }
        // 3st pixel
        {
            //read two pixels
            offset[0] = *offset_ptr[0]++;
            offset[1] = *offset_ptr[1]++;
            offset[2] = *offset_ptr[2]++;
            offset[3] = *offset_ptr[3]++;
            v_src_u32x4 = vld1q_lane_u32(src_ptr[0] + offset[0], v_src_u32x4, 0);
            v_src_u32x4 = vld1q_lane_u32(src_ptr[1] + offset[1], v_src_u32x4, 1);
            v_src_u32x4 = vld1q_lane_u32(src_ptr[2] + offset[2], v_src_u32x4, 2);
            v_src_u32x4 = vld1q_lane_u32(src_ptr[3] + offset[3], v_src_u32x4, 3);

            v_src_u8x16 = vreinterpretq_u8_u32(v_src_u32x4);

            v_src_low_u16x8 = vmovl_u8(vget_low_u8(v_src_u8x16));
            v_src_high_u16x8 = vmovl_u8(vget_high_u8(v_src_u8x16));

            v_tmp_low_s16x8 = vaddw_s8(vreinterpretq_s16_u16(v_src_low_u16x8), vget_low_s8(v_err_s8x16));
            v_tmp_high_s16x8 = vaddw_s8(vreinterpretq_s16_u16(v_src_high_u16x8), vget_high_s8(v_err_s8x16));

            v_dst_low_u8x8 = vqmovun_s16(v_tmp_low_s16x8);
            v_dst_high_u8x8 = vqmovun_s16(v_tmp_high_s16x8);

            v_tmp_u8x16 = vcombine_u8(v_dst_low_u8x8, v_dst_high_u8x8);

            v_dst_u8x16 = vandq_u8(v_tmp_u8x16, v_mask_u8x16);
            v_err_u8x16 = vsubq_u8(v_src_u8x16, v_dst_u8x16);
            v_err_s8x16 = vreinterpretq_s8_u8(v_err_u8x16);
            v_err_3 = v_err_s8x16;
            v_dst_u32x4 = vreinterpretq_u32_u8(v_dst_u8x16);

            vst1q_lane_u32(src_ptr[0] + offset[0], v_dst_u32x4, 0);
            vst1q_lane_u32(src_ptr[1] + offset[1], v_dst_u32x4, 1);
            vst1q_lane_u32(src_ptr[2] + offset[2], v_dst_u32x4, 2);
            vst1q_lane_u32(src_ptr[3] + offset[3], v_dst_u32x4, 3);

            //weight: {2, 4, 4, 8, 1, 1, 2, 2}
            v_8x_s8x16 = vshlq_n_s8(v_err_3, 3);          // x8
            v_1x_s8x16 = vaddq_s8(v_err_4, v_err_5);      // x1
            v_2x_s8x16 = vaddq_s8(v_err_6, v_err_7);
            v_4x_s8x16 = vaddq_s8(v_err_1, v_err_2);
            v_2x_s8x16 = vaddq_s8(v_2x_s8x16, v_err_0);

            v_tmp5_s8x16 = vshlq_n_s8(v_2x_s8x16, 1);    // x2
            v_tmp6_s8x16 = vshlq_n_s8(v_4x_s8x16, 2);    // x4

            // error = (1x+2x+4x+8x) / 8
            v_sum0_s8x16 = vaddq_s8(v_8x_s8x16, v_1x_s8x16);  // 8x + 1x
            v_sum1_s8x16 = vaddq_s8(v_tmp6_s8x16, v_tmp5_s8x16);  //1// 4x + 2x
            v_sum2_s8x16 = vqaddq_s8(v_sum0_s8x16, v_sum1_s8x16);
            v_err_s8x16 = vshrq_n_s8(v_sum2_s8x16, 3);
        }

        // 4st pixel
        {
            //read two pixels
            offset[0] = *offset_ptr[0]++;
            offset[1] = *offset_ptr[1]++;
            offset[2] = *offset_ptr[2]++;
            offset[3] = *offset_ptr[3]++;
            v_src_u32x4 = vld1q_lane_u32(src_ptr[0] + offset[0], v_src_u32x4, 0);
            v_src_u32x4 = vld1q_lane_u32(src_ptr[1] + offset[1], v_src_u32x4, 1);
            v_src_u32x4 = vld1q_lane_u32(src_ptr[2] + offset[2], v_src_u32x4, 2);
            v_src_u32x4 = vld1q_lane_u32(src_ptr[3] + offset[3], v_src_u32x4, 3);

            v_src_u8x16 = vreinterpretq_u8_u32(v_src_u32x4);

            v_src_low_u16x8 = vmovl_u8(vget_low_u8(v_src_u8x16));
            v_src_high_u16x8 = vmovl_u8(vget_high_u8(v_src_u8x16));

            v_tmp_low_s16x8 = vaddw_s8(vreinterpretq_s16_u16(v_src_low_u16x8), vget_low_s8(v_err_s8x16));
            v_tmp_high_s16x8 = vaddw_s8(vreinterpretq_s16_u16(v_src_high_u16x8), vget_high_s8(v_err_s8x16));

            v_dst_low_u8x8 = vqmovun_s16(v_tmp_low_s16x8);
            v_dst_high_u8x8 = vqmovun_s16(v_tmp_high_s16x8);

            v_tmp_u8x16 = vcombine_u8(v_dst_low_u8x8, v_dst_high_u8x8);

            v_dst_u8x16 = vandq_u8(v_tmp_u8x16, v_mask_u8x16);
            v_err_u8x16 = vsubq_u8(v_src_u8x16, v_dst_u8x16);
            v_err_s8x16 = vreinterpretq_s8_u8(v_err_u8x16);
            v_err_4 = v_err_s8x16;
            v_dst_u32x4 = vreinterpretq_u32_u8(v_dst_u8x16);

            vst1q_lane_u32(src_ptr[0] + offset[0], v_dst_u32x4, 0);
            vst1q_lane_u32(src_ptr[1] + offset[1], v_dst_u32x4, 1);
            vst1q_lane_u32(src_ptr[2] + offset[2], v_dst_u32x4, 2);
            vst1q_lane_u32(src_ptr[3] + offset[3], v_dst_u32x4, 3);

            //weight: {2, 2, 4, 4, 8, 1, 1, 2}
            v_8x_s8x16 = vshlq_n_s8(v_err_4, 3);          // x8
            v_1x_s8x16 = vaddq_s8(v_err_5, v_err_6);      // x1
            v_2x_s8x16 = vaddq_s8(v_err_7, v_err_0);
            v_4x_s8x16 = vaddq_s8(v_err_2, v_err_3);
            v_2x_s8x16 = vaddq_s8(v_2x_s8x16, v_err_1);

            v_tmp5_s8x16 = vshlq_n_s8(v_2x_s8x16, 1);    // x2
            v_tmp6_s8x16 = vshlq_n_s8(v_4x_s8x16, 2);    // x4

            // error = (1x+2x+4x+8x) / 8
            v_sum0_s8x16 = vaddq_s8(v_8x_s8x16, v_1x_s8x16);  // 8x + 1x
            v_sum1_s8x16 = vaddq_s8(v_tmp6_s8x16, v_tmp5_s8x16);  //1// 4x + 2x
            v_sum2_s8x16 = vqaddq_s8(v_sum0_s8x16, v_sum1_s8x16);
            v_err_s8x16 = vshrq_n_s8(v_sum2_s8x16, 3);
        }

        // 5st pixel
        {
            //read two pixels
            offset[0] = *offset_ptr[0]++;
            offset[1] = *offset_ptr[1]++;
            offset[2] = *offset_ptr[2]++;
            offset[3] = *offset_ptr[3]++;
            v_src_u32x4 = vld1q_lane_u32(src_ptr[0] + offset[0], v_src_u32x4, 0);
            v_src_u32x4 = vld1q_lane_u32(src_ptr[1] + offset[1], v_src_u32x4, 1);
            v_src_u32x4 = vld1q_lane_u32(src_ptr[2] + offset[2], v_src_u32x4, 2);
            v_src_u32x4 = vld1q_lane_u32(src_ptr[3] + offset[3], v_src_u32x4, 3);

            v_src_u8x16 = vreinterpretq_u8_u32(v_src_u32x4);

            v_src_low_u16x8 = vmovl_u8(vget_low_u8(v_src_u8x16));
            v_src_high_u16x8 = vmovl_u8(vget_high_u8(v_src_u8x16));

            v_tmp_low_s16x8 = vaddw_s8(vreinterpretq_s16_u16(v_src_low_u16x8), vget_low_s8(v_err_s8x16));
            v_tmp_high_s16x8 = vaddw_s8(vreinterpretq_s16_u16(v_src_high_u16x8), vget_high_s8(v_err_s8x16));

            v_dst_low_u8x8 = vqmovun_s16(v_tmp_low_s16x8);
            v_dst_high_u8x8 = vqmovun_s16(v_tmp_high_s16x8);

            v_tmp_u8x16 = vcombine_u8(v_dst_low_u8x8, v_dst_high_u8x8);

            v_dst_u8x16 = vandq_u8(v_tmp_u8x16, v_mask_u8x16);
            v_err_u8x16 = vsubq_u8(v_src_u8x16, v_dst_u8x16);
            v_err_s8x16 = vreinterpretq_s8_u8(v_err_u8x16);
            v_err_5 = v_err_s8x16;
            v_dst_u32x4 = vreinterpretq_u32_u8(v_dst_u8x16);

            vst1q_lane_u32(src_ptr[0] + offset[0], v_dst_u32x4, 0);
            vst1q_lane_u32(src_ptr[1] + offset[1], v_dst_u32x4, 1);
            vst1q_lane_u32(src_ptr[2] + offset[2], v_dst_u32x4, 2);
            vst1q_lane_u32(src_ptr[3] + offset[3], v_dst_u32x4, 3);

            //weight: {2, 2, 2, 4, 4, 8, 1, 1}
            v_8x_s8x16 = vshlq_n_s8(v_err_5, 3);          // x8
            v_1x_s8x16 = vaddq_s8(v_err_6, v_err_7);      // x1
            v_2x_s8x16 = vaddq_s8(v_err_0, v_err_1);
            v_4x_s8x16 = vaddq_s8(v_err_3, v_err_4);
            v_2x_s8x16 = vaddq_s8(v_2x_s8x16, v_err_2);

            v_tmp5_s8x16 = vshlq_n_s8(v_2x_s8x16, 1);    // x2
            v_tmp6_s8x16 = vshlq_n_s8(v_4x_s8x16, 2);    // x4

            // error = (1x+2x+4x+8x) / 8
            v_sum0_s8x16 = vaddq_s8(v_8x_s8x16, v_1x_s8x16);  // 8x + 1x
            v_sum1_s8x16 = vaddq_s8(v_tmp6_s8x16, v_tmp5_s8x16);  //1// 4x + 2x
            v_sum2_s8x16 = vqaddq_s8(v_sum0_s8x16, v_sum1_s8x16);
            v_err_s8x16 = vshrq_n_s8(v_sum2_s8x16, 3);
        }

        // 6st pixel
        {
            //read two pixels
            offset[0] = *offset_ptr[0]++;
            offset[1] = *offset_ptr[1]++;
            offset[2] = *offset_ptr[2]++;
            offset[3] = *offset_ptr[3]++;
            v_src_u32x4 = vld1q_lane_u32(src_ptr[0] + offset[0], v_src_u32x4, 0);
            v_src_u32x4 = vld1q_lane_u32(src_ptr[1] + offset[1], v_src_u32x4, 1);
            v_src_u32x4 = vld1q_lane_u32(src_ptr[2] + offset[2], v_src_u32x4, 2);
            v_src_u32x4 = vld1q_lane_u32(src_ptr[3] + offset[3], v_src_u32x4, 3);

            v_src_u8x16 = vreinterpretq_u8_u32(v_src_u32x4);

            v_src_low_u16x8 = vmovl_u8(vget_low_u8(v_src_u8x16));
            v_src_high_u16x8 = vmovl_u8(vget_high_u8(v_src_u8x16));

            v_tmp_low_s16x8 = vaddw_s8(vreinterpretq_s16_u16(v_src_low_u16x8), vget_low_s8(v_err_s8x16));
            v_tmp_high_s16x8 = vaddw_s8(vreinterpretq_s16_u16(v_src_high_u16x8), vget_high_s8(v_err_s8x16));

            v_dst_low_u8x8 = vqmovun_s16(v_tmp_low_s16x8);
            v_dst_high_u8x8 = vqmovun_s16(v_tmp_high_s16x8);

            v_tmp_u8x16 = vcombine_u8(v_dst_low_u8x8, v_dst_high_u8x8);

            v_dst_u8x16 = vandq_u8(v_tmp_u8x16, v_mask_u8x16);
            v_err_u8x16 = vsubq_u8(v_src_u8x16, v_dst_u8x16);
            v_err_s8x16 = vreinterpretq_s8_u8(v_err_u8x16);
            v_err_6 = v_err_s8x16;
            v_dst_u32x4 = vreinterpretq_u32_u8(v_dst_u8x16);

            vst1q_lane_u32(src_ptr[0] + offset[0], v_dst_u32x4, 0);
            vst1q_lane_u32(src_ptr[1] + offset[1], v_dst_u32x4, 1);
            vst1q_lane_u32(src_ptr[2] + offset[2], v_dst_u32x4, 2);
            vst1q_lane_u32(src_ptr[3] + offset[3], v_dst_u32x4, 3);

            //weight: {1, 2, 2, 2, 4, 4, 8, 1}
            v_8x_s8x16 = vshlq_n_s8(v_err_6, 3);          // x8
            v_1x_s8x16 = vaddq_s8(v_err_7, v_err_0);      // x1
            v_2x_s8x16 = vaddq_s8(v_err_1, v_err_2);
            v_4x_s8x16 = vaddq_s8(v_err_4, v_err_5);
            v_2x_s8x16 = vaddq_s8(v_2x_s8x16, v_err_3);

            v_tmp5_s8x16 = vshlq_n_s8(v_2x_s8x16, 1);    // x2
            v_tmp6_s8x16 = vshlq_n_s8(v_4x_s8x16, 2);    // x4

            // error = (1x+2x+4x+8x) / 8
            v_sum0_s8x16 = vaddq_s8(v_8x_s8x16, v_1x_s8x16);  // 8x + 1x
            v_sum1_s8x16 = vaddq_s8(v_tmp6_s8x16, v_tmp5_s8x16);  //1// 4x + 2x
            v_sum2_s8x16 = vqaddq_s8(v_sum0_s8x16, v_sum1_s8x16);
            v_err_s8x16 = vshrq_n_s8(v_sum2_s8x16, 3);
        }

        // 7st pixel
        {
            //read two pixels
            offset[0] = *offset_ptr[0]++;
            offset[1] = *offset_ptr[1]++;
            offset[2] = *offset_ptr[2]++;
            offset[3] = *offset_ptr[3]++;
            v_src_u32x4 = vld1q_lane_u32(src_ptr[0] + offset[0], v_src_u32x4, 0);
            v_src_u32x4 = vld1q_lane_u32(src_ptr[1] + offset[1], v_src_u32x4, 1);
            v_src_u32x4 = vld1q_lane_u32(src_ptr[2] + offset[2], v_src_u32x4, 2);
            v_src_u32x4 = vld1q_lane_u32(src_ptr[3] + offset[3], v_src_u32x4, 3);

            v_src_u8x16 = vreinterpretq_u8_u32(v_src_u32x4);

            v_src_low_u16x8 = vmovl_u8(vget_low_u8(v_src_u8x16));
            v_src_high_u16x8 = vmovl_u8(vget_high_u8(v_src_u8x16));

            v_tmp_low_s16x8 = vaddw_s8(vreinterpretq_s16_u16(v_src_low_u16x8), vget_low_s8(v_err_s8x16));
            v_tmp_high_s16x8 = vaddw_s8(vreinterpretq_s16_u16(v_src_high_u16x8), vget_high_s8(v_err_s8x16));

            v_dst_low_u8x8 = vqmovun_s16(v_tmp_low_s16x8);
            v_dst_high_u8x8 = vqmovun_s16(v_tmp_high_s16x8);

            v_tmp_u8x16 = vcombine_u8(v_dst_low_u8x8, v_dst_high_u8x8);

            v_dst_u8x16 = vandq_u8(v_tmp_u8x16, v_mask_u8x16);
            v_err_u8x16 = vsubq_u8(v_src_u8x16, v_dst_u8x16);
            v_err_s8x16 = vreinterpretq_s8_u8(v_err_u8x16);
            v_err_7 = v_err_s8x16;
            v_dst_u32x4 = vreinterpretq_u32_u8(v_dst_u8x16);

            vst1q_lane_u32(src_ptr[0] + offset[0], v_dst_u32x4, 0);
            vst1q_lane_u32(src_ptr[1] + offset[1], v_dst_u32x4, 1);
            vst1q_lane_u32(src_ptr[2] + offset[2], v_dst_u32x4, 2);
            vst1q_lane_u32(src_ptr[3] + offset[3], v_dst_u32x4, 3);

            //weight: {1, 1, 2, 2, 2, 4, 4, 8}
            v_8x_s8x16 = vshlq_n_s8(v_err_7, 3);          // x8
            v_1x_s8x16 = vaddq_s8(v_err_0, v_err_1);      // x1
            v_2x_s8x16 = vaddq_s8(v_err_2, v_err_3);
            v_4x_s8x16 = vaddq_s8(v_err_5, v_err_6);
            v_2x_s8x16 = vaddq_s8(v_2x_s8x16, v_err_4);

            v_tmp5_s8x16 = vshlq_n_s8(v_2x_s8x16, 1);    // x2
            v_tmp6_s8x16 = vshlq_n_s8(v_4x_s8x16, 2);    // x4

            // error = (1x+2x+4x+8x) / 8
            v_sum0_s8x16 = vaddq_s8(v_8x_s8x16, v_1x_s8x16);  // 8x + 1x
            v_sum1_s8x16 = vaddq_s8(v_tmp6_s8x16, v_tmp5_s8x16);  //1// 4x + 2x
            v_sum2_s8x16 = vqaddq_s8(v_sum0_s8x16, v_sum1_s8x16);
            v_err_s8x16 = vshrq_n_s8(v_sum2_s8x16, 3);
        }

        k++;
    }while(k<32);
}

static void dither_single_block(uint32_t *src_ptr, uint16_t *offset_tbl, int16_t *weight_tbl[8])
{
    uint32_t k=0;

    int32_t r_err = 0;
    int32_t g_err = 0;
    int32_t b_err = 0;

    int16x8_t v_error_r;
    int16x8_t v_error_g;
    int16x8_t v_error_b;

    int16x8_t v_weight0 ;
    int16x8_t v_weight1 ;
    int16x8_t v_weight2 ;
    int16x8_t v_weight3 ;
    int16x8_t v_weight4 ;
    int16x8_t v_weight5 ;
    int16x8_t v_weight6 ;
    int16x8_t v_weight7 ;

    v_error_r = vdupq_n_s16(0);
    v_error_g = vdupq_n_s16(0);
    v_error_b = vdupq_n_s16(0);

    v_weight0 = vld1q_s16(weight_tbl[0]);
    v_weight1 = vld1q_s16(weight_tbl[1]);
    v_weight2 = vld1q_s16(weight_tbl[2]);
    v_weight3 = vld1q_s16(weight_tbl[3]);
    v_weight4 = vld1q_s16(weight_tbl[4]);
    v_weight5 = vld1q_s16(weight_tbl[5]);
    v_weight6 = vld1q_s16(weight_tbl[6]);
    v_weight7 = vld1q_s16(weight_tbl[7]);

    do
    {
        uint32_t offset;
        int32_t r_value, g_value, b_value;
        int32_t src_r_val, src_g_val, src_b_val;
        uint32_t argb = 0;

        int16x8_t v_r_16x8;
        int32x4_t v_r_32x4;
        int64x2_t v_r_64x2;

        int16x8_t v_g_16x8;
        int32x4_t v_g_32x4;
        int64x2_t v_g_64x2;

        int16x8_t v_b_16x8;
        int32x4_t v_b_32x4;
        int64x2_t v_b_64x2;


        // 1st pixel
        {
            offset = *offset_tbl++;
            argb = *(src_ptr + offset);
            src_b_val = argb & 0xff;
            src_g_val = (argb & 0xff00) >> 8;
            src_r_val = (argb & 0xff0000) >> 16;

            b_value = src_b_val + (b_err >> 3);
            g_value = src_g_val + (g_err >> 3);
            r_value = src_r_val + (r_err >> 3);

            CLIP8(b_value)
            CLIP8(g_value)
            CLIP8(r_value)

            b_value = b_value>>3;
            g_value = g_value>>2;
            r_value = r_value>>3;

            argb &= 0xff000000;
            argb |= (b_value << 3);
            argb |= (g_value << 10);
            argb |= (r_value << 19);

            b_err = src_b_val - (b_value<<3);
            g_err = src_g_val - (g_value<<2);
            r_err = src_r_val - (r_value<<3);

            *(src_ptr + offset) = argb;

            v_error_r = vsetq_lane_s16(r_err, v_error_r, 0);
            v_error_g = vsetq_lane_s16(g_err, v_error_g, 0);
            v_error_b = vsetq_lane_s16(b_err, v_error_b, 0);

            v_b_16x8 = vmulq_s16(v_error_b, v_weight0);
            v_g_16x8 = vmulq_s16(v_error_g, v_weight0);
            v_r_16x8 = vmulq_s16(v_error_r, v_weight0);

            v_b_32x4 = vpaddlq_s16(v_b_16x8);
            v_g_32x4 = vpaddlq_s16(v_g_16x8);
            v_r_32x4 = vpaddlq_s16(v_r_16x8);

            v_b_64x2 = vpaddlq_s32(v_b_32x4);
            v_g_64x2 = vpaddlq_s32(v_g_32x4);
            v_r_64x2 = vpaddlq_s32(v_r_32x4);

            b_err = vgetq_lane_s64(v_b_64x2, 0) + vgetq_lane_s64(v_b_64x2, 1);
            g_err = vgetq_lane_s64(v_g_64x2, 0) + vgetq_lane_s64(v_g_64x2, 1);
            r_err = vgetq_lane_s64(v_r_64x2, 0) + vgetq_lane_s64(v_r_64x2, 1);
        }

        // 2nd pixel
        {
            offset = *offset_tbl++;

            argb = *(src_ptr + offset);
            src_b_val = argb & 0xff;
            src_g_val = (argb & 0xff00) >> 8;
            src_r_val = (argb & 0xff0000) >> 16;

            b_value = src_b_val + (b_err >> 3);
            g_value = src_g_val + (g_err >> 3);
            r_value = src_r_val + (r_err >> 3);

            CLIP8(b_value)
            CLIP8(g_value)
            CLIP8(r_value)

            b_value = b_value>>3;
            g_value = g_value>>2;
            r_value = r_value>>3;

            argb &= 0xff000000;
            argb |= (b_value << 3);
            argb |= (g_value << 10);
            argb |= (r_value << 19);

            b_err = src_b_val - (b_value<<3);
            g_err = src_g_val - (g_value<<2);
            r_err = src_r_val - (r_value<<3);

            *(src_ptr + offset) = argb;

            v_error_r = vsetq_lane_s16(r_err, v_error_r, 1);
            v_error_g = vsetq_lane_s16(g_err, v_error_g, 1);
            v_error_b = vsetq_lane_s16(b_err, v_error_b, 1);

            v_b_16x8 = vmulq_s16(v_error_b, v_weight1);
            v_g_16x8 = vmulq_s16(v_error_g, v_weight1);
            v_r_16x8 = vmulq_s16(v_error_r, v_weight1);

            v_b_32x4 = vpaddlq_s16(v_b_16x8);
            v_g_32x4 = vpaddlq_s16(v_g_16x8);
            v_r_32x4 = vpaddlq_s16(v_r_16x8);

            v_b_64x2 = vpaddlq_s32(v_b_32x4);
            v_g_64x2 = vpaddlq_s32(v_g_32x4);
            v_r_64x2 = vpaddlq_s32(v_r_32x4);

            b_err = vgetq_lane_s64(v_b_64x2, 0) + vgetq_lane_s64(v_b_64x2, 1);
            g_err = vgetq_lane_s64(v_g_64x2, 0) + vgetq_lane_s64(v_g_64x2, 1);
            r_err = vgetq_lane_s64(v_r_64x2, 0) + vgetq_lane_s64(v_r_64x2, 1);
        }

        // 3rd pixel
        {
            offset = *offset_tbl++;

            argb = *(src_ptr + offset);
            src_b_val = argb & 0xff;
            src_g_val = (argb & 0xff00) >> 8;
            src_r_val = (argb & 0xff0000) >> 16;

            b_value = src_b_val + (b_err >> 3);
            g_value = src_g_val + (g_err >> 3);
            r_value = src_r_val + (r_err >> 3);

            CLIP8(b_value)
            CLIP8(g_value)
            CLIP8(r_value)

            b_value = b_value>>3;
            g_value = g_value>>2;
            r_value = r_value>>3;

            argb &= 0xff000000;
            argb |= (b_value << 3);
            argb |= (g_value << 10);
            argb |= (r_value << 19);

            b_err = src_b_val - (b_value<<3);
            g_err = src_g_val - (g_value<<2);
            r_err = src_r_val - (r_value<<3);

            *(src_ptr + offset) = argb;

            v_error_r = vsetq_lane_s16(r_err, v_error_r, 2);
            v_error_g = vsetq_lane_s16(g_err, v_error_g, 2);
            v_error_b = vsetq_lane_s16(b_err, v_error_b, 2);

            v_b_16x8 = vmulq_s16(v_error_b, v_weight2);
            v_g_16x8 = vmulq_s16(v_error_g, v_weight2);
            v_r_16x8 = vmulq_s16(v_error_r, v_weight2);

            v_b_32x4 = vpaddlq_s16(v_b_16x8);
            v_g_32x4 = vpaddlq_s16(v_g_16x8);
            v_r_32x4 = vpaddlq_s16(v_r_16x8);

            v_b_64x2 = vpaddlq_s32(v_b_32x4);
            v_g_64x2 = vpaddlq_s32(v_g_32x4);
            v_r_64x2 = vpaddlq_s32(v_r_32x4);

            b_err = vgetq_lane_s64(v_b_64x2, 0) + vgetq_lane_s64(v_b_64x2, 1);
            g_err = vgetq_lane_s64(v_g_64x2, 0) + vgetq_lane_s64(v_g_64x2, 1);
            r_err = vgetq_lane_s64(v_r_64x2, 0) + vgetq_lane_s64(v_r_64x2, 1);

        }

        // 4th pixel
        {
            offset = *offset_tbl++;
            argb = *(src_ptr + offset);
            src_b_val = argb & 0xff;
            src_g_val = (argb & 0xff00) >> 8;
            src_r_val = (argb & 0xff0000) >> 16;

            b_value = src_b_val + (b_err >> 3);
            g_value = src_g_val + (g_err >> 3);
            r_value = src_r_val + (r_err >> 3);

            CLIP8(b_value)
            CLIP8(g_value)
            CLIP8(r_value)

            b_value = b_value>>3;
            g_value = g_value>>2;
            r_value = r_value>>3;

            argb &= 0xff000000;
            argb |= (b_value << 3);
            argb |= (g_value << 10);
            argb |= (r_value << 19);

            b_err = src_b_val - (b_value<<3);
            g_err = src_g_val - (g_value<<2);
            r_err = src_r_val - (r_value<<3);

            *(src_ptr + offset) = argb;

            v_error_r = vsetq_lane_s16(r_err, v_error_r, 3);
            v_error_g = vsetq_lane_s16(g_err, v_error_g, 3);
            v_error_b = vsetq_lane_s16(b_err, v_error_b, 3);

            v_b_16x8 = vmulq_s16(v_error_b, v_weight3);
            v_g_16x8 = vmulq_s16(v_error_g, v_weight3);
            v_r_16x8 = vmulq_s16(v_error_r, v_weight3);

            v_b_32x4 = vpaddlq_s16(v_b_16x8);
            v_g_32x4 = vpaddlq_s16(v_g_16x8);
            v_r_32x4 = vpaddlq_s16(v_r_16x8);

            v_b_64x2 = vpaddlq_s32(v_b_32x4);
            v_g_64x2 = vpaddlq_s32(v_g_32x4);
            v_r_64x2 = vpaddlq_s32(v_r_32x4);

            b_err = vgetq_lane_s64(v_b_64x2, 0) + vgetq_lane_s64(v_b_64x2, 1);
            g_err = vgetq_lane_s64(v_g_64x2, 0) + vgetq_lane_s64(v_g_64x2, 1);
            r_err = vgetq_lane_s64(v_r_64x2, 0) + vgetq_lane_s64(v_r_64x2, 1);
        }

        //5th pixel
        {
            offset = *offset_tbl++;
            argb = *(src_ptr + offset);
            src_b_val = argb & 0xff;
            src_g_val = (argb & 0xff00) >> 8;
            src_r_val = (argb & 0xff0000) >> 16;

            b_value = src_b_val + (b_err >> 3);
            g_value = src_g_val + (g_err >> 3);
            r_value = src_r_val + (r_err >> 3);

            CLIP8(b_value)
            CLIP8(g_value)
            CLIP8(r_value)

            b_value = b_value>>3;
            g_value = g_value>>2;
            r_value = r_value>>3;

            argb &= 0xff000000;
            argb |= (b_value << 3);
            argb |= (g_value << 10);
            argb |= (r_value << 19);

            b_err = src_b_val - (b_value<<3);
            g_err = src_g_val - (g_value<<2);
            r_err = src_r_val - (r_value<<3);

            *(src_ptr + offset) = argb;

            v_error_r = vsetq_lane_s16(r_err, v_error_r, 4);
            v_error_g = vsetq_lane_s16(g_err, v_error_g, 4);
            v_error_b = vsetq_lane_s16(b_err, v_error_b, 4);

            v_b_16x8 = vmulq_s16(v_error_b, v_weight4);
            v_g_16x8 = vmulq_s16(v_error_g, v_weight4);
            v_r_16x8 = vmulq_s16(v_error_r, v_weight4);

            v_b_32x4 = vpaddlq_s16(v_b_16x8);
            v_g_32x4 = vpaddlq_s16(v_g_16x8);
            v_r_32x4 = vpaddlq_s16(v_r_16x8);

            v_b_64x2 = vpaddlq_s32(v_b_32x4);
            v_g_64x2 = vpaddlq_s32(v_g_32x4);
            v_r_64x2 = vpaddlq_s32(v_r_32x4);

            b_err = vgetq_lane_s64(v_b_64x2, 0) + vgetq_lane_s64(v_b_64x2, 1);
            g_err = vgetq_lane_s64(v_g_64x2, 0) + vgetq_lane_s64(v_g_64x2, 1);
            r_err = vgetq_lane_s64(v_r_64x2, 0) + vgetq_lane_s64(v_r_64x2, 1);
        }

        //6th pixel
        {
            offset = *offset_tbl++;
            argb = *(src_ptr + offset);
            src_b_val = argb & 0xff;
            src_g_val = (argb & 0xff00) >> 8;
            src_r_val = (argb & 0xff0000) >> 16;

            b_value = src_b_val + (b_err >> 3);
            g_value = src_g_val + (g_err >> 3);
            r_value = src_r_val + (r_err >> 3);

            CLIP8(b_value)
            CLIP8(g_value)
            CLIP8(r_value)

            b_value = b_value>>3;
            g_value = g_value>>2;
            r_value = r_value>>3;

            argb &= 0xff000000;
            argb |= (b_value << 3);
            argb |= (g_value << 10);
            argb |= (r_value << 19);

            b_err = src_b_val - (b_value<<3);
            g_err = src_g_val - (g_value<<2);
            r_err = src_r_val - (r_value<<3);

            *(src_ptr + offset) = argb;

            v_error_r = vsetq_lane_s16(r_err, v_error_r, 5);
            v_error_g = vsetq_lane_s16(g_err, v_error_g, 5);
            v_error_b = vsetq_lane_s16(b_err, v_error_b, 5);

            v_b_16x8 = vmulq_s16(v_error_b, v_weight5);
            v_g_16x8 = vmulq_s16(v_error_g, v_weight5);
            v_r_16x8 = vmulq_s16(v_error_r, v_weight5);

            v_b_32x4 = vpaddlq_s16(v_b_16x8);
            v_g_32x4 = vpaddlq_s16(v_g_16x8);
            v_r_32x4 = vpaddlq_s16(v_r_16x8);

            v_b_64x2 = vpaddlq_s32(v_b_32x4);
            v_g_64x2 = vpaddlq_s32(v_g_32x4);
            v_r_64x2 = vpaddlq_s32(v_r_32x4);

            b_err = vgetq_lane_s64(v_b_64x2, 0) + vgetq_lane_s64(v_b_64x2, 1);
            g_err = vgetq_lane_s64(v_g_64x2, 0) + vgetq_lane_s64(v_g_64x2, 1);
            r_err = vgetq_lane_s64(v_r_64x2, 0) + vgetq_lane_s64(v_r_64x2, 1);
        }

        //7th pixel
        {
            offset = *offset_tbl++;
            argb = *(src_ptr + offset);
            src_b_val = argb & 0xff;
            src_g_val = (argb & 0xff00) >> 8;
            src_r_val = (argb & 0xff0000) >> 16;

            b_value = src_b_val + (b_err >> 3);
            g_value = src_g_val + (g_err >> 3);
            r_value = src_r_val + (r_err >> 3);

            CLIP8(b_value)
            CLIP8(g_value)
            CLIP8(r_value)

            b_value = b_value>>3;
            g_value = g_value>>2;
            r_value = r_value>>3;

            argb &= 0xff000000;
            argb |= (b_value << 3);
            argb |= (g_value << 10);
            argb |= (r_value << 19);

            b_err = src_b_val - (b_value<<3);
            g_err = src_g_val - (g_value<<2);
            r_err = src_r_val - (r_value<<3);

            *(src_ptr + offset) = argb;

            v_error_r = vsetq_lane_s16(r_err, v_error_r, 6);
            v_error_g = vsetq_lane_s16(g_err, v_error_g, 6);
            v_error_b = vsetq_lane_s16(b_err, v_error_b, 6);

            v_b_16x8 = vmulq_s16(v_error_b, v_weight6);
            v_g_16x8 = vmulq_s16(v_error_g, v_weight6);
            v_r_16x8 = vmulq_s16(v_error_r, v_weight6);

            v_b_32x4 = vpaddlq_s16(v_b_16x8);
            v_g_32x4 = vpaddlq_s16(v_g_16x8);
            v_r_32x4 = vpaddlq_s16(v_r_16x8);

            v_b_64x2 = vpaddlq_s32(v_b_32x4);
            v_g_64x2 = vpaddlq_s32(v_g_32x4);
            v_r_64x2 = vpaddlq_s32(v_r_32x4);

            b_err = vgetq_lane_s64(v_b_64x2, 0) + vgetq_lane_s64(v_b_64x2, 1);
            g_err = vgetq_lane_s64(v_g_64x2, 0) + vgetq_lane_s64(v_g_64x2, 1);
            r_err = vgetq_lane_s64(v_r_64x2, 0) + vgetq_lane_s64(v_r_64x2, 1);
        }

        //8th pixel
        {
            offset = *offset_tbl++;
            argb = *(src_ptr + offset);
            src_b_val = argb & 0xff;
            src_g_val = (argb & 0xff00) >> 8;
            src_r_val = (argb & 0xff0000) >> 16;

            b_value = src_b_val + (b_err >> 3);
            g_value = src_g_val + (g_err >> 3);
            r_value = src_r_val + (r_err >> 3);

            CLIP8(b_value)
            CLIP8(g_value)
            CLIP8(r_value)

            b_value = b_value>>3;
            g_value = g_value>>2;
            r_value = r_value>>3;

            argb &= 0xff000000;
            argb |= (b_value << 3);
            argb |= (g_value << 10);
            argb |= (r_value << 19);

            b_err = src_b_val - (b_value<<3);
            g_err = src_g_val - (g_value<<2);
            r_err = src_r_val - (r_value<<3);

            *(src_ptr + offset) = argb;

            v_error_r = vsetq_lane_s16(r_err, v_error_r, 7);
            v_error_g = vsetq_lane_s16(g_err, v_error_g, 7);
            v_error_b = vsetq_lane_s16(b_err, v_error_b, 7);

            v_b_16x8 = vmulq_s16(v_error_b, v_weight7);
            v_g_16x8 = vmulq_s16(v_error_g, v_weight7);
            v_r_16x8 = vmulq_s16(v_error_r, v_weight7);

            v_b_32x4 = vpaddlq_s16(v_b_16x8);
            v_g_32x4 = vpaddlq_s16(v_g_16x8);
            v_r_32x4 = vpaddlq_s16(v_r_16x8);

            v_b_64x2 = vpaddlq_s32(v_b_32x4);
            v_g_64x2 = vpaddlq_s32(v_g_32x4);
            v_r_64x2 = vpaddlq_s32(v_r_32x4);

            b_err = vgetq_lane_s64(v_b_64x2, 0) + vgetq_lane_s64(v_b_64x2, 1);
            g_err = vgetq_lane_s64(v_g_64x2, 0) + vgetq_lane_s64(v_g_64x2, 1);
            r_err = vgetq_lane_s64(v_r_64x2, 0) + vgetq_lane_s64(v_r_64x2, 1);
        }

        k++;
    }while(k<32);
}

static void dither_irregular_block(uint32_t *src_ptr, uint32_t blk_w, uint32_t blk_h,
                                    uint32_t img_w)
{
    uint32_t i, j;
    uint32_t *src_line_ptr = src_ptr;
    const uint32_t dm[4] = {0x5140, 0x3726, 0x4051, 0x2637};

    for (i=0; i<blk_h; i++)
    {
        src_ptr = src_line_ptr;
        for (j=0; j<blk_w; j++)
        {
            uint32_t argb = *src_ptr;
            int32_t r, g, b;

#if PROCESS_BOUNDERY
            uint32_t d = (dm[i&3] >> ((j&3) << 2)) & 0xf;

            r = (argb >> 16) & 0xff;
            g = (argb >> 8) & 0xff;
            b = argb & 0xff;

            r = (r + d - (r >> 5)) & 0xf8;
            g = (g + (d >> 1) - (g >> 6)) & 0xfc;
            b = (b + d - (b >> 5)) & 0xf8;

            argb &= 0xff000000;
            argb |= (r << 16);
            argb |= (g << 8);
            argb |= b;
#else
            argb &= 0xfff8fcf8;
#endif

            *src_ptr++ = argb;
        }

        src_line_ptr += img_w;
    }
}

uint32_t _img_dither_00(uint32_t handle, uint8_t *data)
{
    struct curve_item_info *curve_infp_ptr = (struct curve_item_info*)handle;
    uint16_t **offset_tbl = NULL;
    int16_t **weight_tbl = NULL;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t v_cnts = 0;
    uint32_t h_cnts = 0;
    uint32_t total_cnts = 0;
    uint32_t loops = 0;
    uint32_t *line_start_ptr = NULL;
    uint32_t *ref_ptr = NULL;
    uint32_t i, j;
    uint32_t blk_w;
    uint32_t blk_h;
    nsecs_t begin_time;
    nsecs_t end_time;

    if (NULL == curve_infp_ptr)
        return 0;

    begin_time = systemTime(SYSTEM_TIME_MONOTONIC);

    width = curve_infp_ptr->width;
    height = curve_infp_ptr->height;
    offset_tbl = curve_infp_ptr->offset_tbl;
    h_cnts = (width / BLK_WIDTH);
    v_cnts = (height / BLK_WIDTH);
    total_cnts = h_cnts * v_cnts;
    loops = total_cnts / 4;//  4 16x16 block for one loop

    line_start_ptr = (uint32_t*)data;
    ref_ptr = line_start_ptr;

    for (i=0; i<loops; i++)
    {
        uint32_t *src_blk_ptr[4];

        for (j=0; j<4; j++)
        {
            src_blk_ptr[j] = ref_ptr;
            ref_ptr += BLK_WIDTH;
            if (ref_ptr - line_start_ptr + 16 > width)
            {
                line_start_ptr += width * BLK_WIDTH;
                ref_ptr = line_start_ptr;
            }
        }

        dither_four_block(src_blk_ptr, offset_tbl);
    }

    loops = total_cnts & 3;
    weight_tbl = curve_infp_ptr->weight_ptr;
    for (i=0; i<loops; i++)
    {
        dither_single_block(ref_ptr, offset_tbl[i&3], weight_tbl);
        ref_ptr += BLK_WIDTH;
        if (ref_ptr - line_start_ptr + 16 > width)
        {
            line_start_ptr += width * BLK_WIDTH;
            ref_ptr = line_start_ptr;
        }
    }

    //process the leftover of right side
    blk_w = width - h_cnts * BLK_WIDTH;
    if (blk_w > 0)
    {
        ref_ptr = (uint32_t*)data + h_cnts * BLK_WIDTH;
        blk_h = height;

        dither_irregular_block(ref_ptr, blk_w, blk_h, width);
    }

    blk_h = height - v_cnts * BLK_WIDTH;
    if (blk_h > 0)
    {
        ref_ptr = (uint32_t*)data + v_cnts * BLK_WIDTH * width;
        blk_w = width - blk_w;

        dither_irregular_block(ref_ptr, blk_w, blk_h, width);
    }


    end_time = systemTime(SYSTEM_TIME_MONOTONIC);

    IMG_DITHER_LOGV("dither spend time = %lldum", (end_time - begin_time)/1000);

    return 0;

}


int32_t _img_dither_0_init(uint32_t width, uint32_t height)
{
    struct curve_item_info * itm_info_ptr = PNULL;
    uint32_t size = 0x00;
    uint32_t level = 0;
    uint32_t index = 0;
    uint32_t buf_size = 0;

    itm_info_ptr = (struct curve_item_info*)malloc(sizeof(struct curve_item_info));
    memset((void*)itm_info_ptr, 0x00, sizeof(struct curve_item_info));

    for (index = 0; index < Q_SIZE; index++) {
        itm_info_ptr->weight_ptr[index] = (int16_t*)s_weight[index];
    }

    buf_size = BLK_WIDTH * BLK_WIDTH * sizeof(uint16_t);
    for (index = 0; index < 4; ++index) {
        uint16_t *offset_tbl, *coor_tbl;
        if (PNULL != itm_info_ptr->offset_tbl[index]) {
            free((void*)itm_info_ptr->offset_tbl[index]);
            itm_info_ptr->offset_tbl[index] = 0;
        }
        itm_info_ptr->offset_tbl[index] = (uint16_t*)malloc(buf_size);
        memset((void*)itm_info_ptr->offset_tbl[index], 0x00, buf_size);

        offset_tbl = (uint16_t*)itm_info_ptr->offset_tbl[index];
        coor_tbl = (uint16_t*)s_hilbert_curve[index];
        _calc_offset_table(offset_tbl, coor_tbl, width);
    }

    itm_info_ptr->height = height;
    itm_info_ptr->width = width;
    itm_info_ptr->pitch_stride = width * IMG_CHN_NUM;

    return (uint32_t)itm_info_ptr;
}

int32_t _img_dither_0_deinit(uint32_t handle)
{
    uint32_t i = 0;
    struct curve_item_info *info_ptr = PNULL;
    info_ptr =(struct curve_item_info*)handle;

    if (PNULL == info_ptr) {
        IMG_DITHER_LOGE("-input param is invalidated: handle: 0x%x\n", (uint32_t)handle);

        return -img_dither_rtn_pointer_null;
    }

    for (i = 0; i < 4; ++i) {
        if (PNULL != info_ptr->offset_tbl[i]) {
            free((void*)info_ptr->offset_tbl[i]);
            info_ptr->offset_tbl[i] = PNULL;
        }
    }

    free(info_ptr);
    info_ptr = PNULL;

    return img_dither_rtn_sucess;
}

uint32_t _img_dither_0(uint32_t handle, uint8_t *data_addr)
{
    struct curve_item_info *curve_infp_ptr = (struct curve_item_info*)handle;
    struct block_info blk_info ;

    uint32_t width = 0, height = 0, i = 0;
    uint32_t *src_ptr = NULL;
    uint32_t stride = 0;
    uint32_t v_cnts = 0, h_cnts = 0;

#ifndef TEST_ENV
    nsecs_t begin_time;
    nsecs_t end_time;
#endif

    width = curve_infp_ptr->width;
    height = curve_infp_ptr->height;
    h_cnts = (width>>HILBERT_LEVEL);
    v_cnts = (height>>HILBERT_LEVEL);
    stride = BLK_WIDTH * width;

    memset((void*)&blk_info, 0x00, sizeof(struct block_info));

    for (i=0; i<Q_SIZE; i++)
    {
        blk_info.weight_ptr[i] = curve_infp_ptr->weight_ptr[i];
    }

    #ifndef TEST_ENV
    //ALOGD("dither begin (ID=3)!");
    begin_time = systemTime(SYSTEM_TIME_MONOTONIC);
    #endif

    src_ptr = (uint32_t*)data_addr;

    for (i=0; i<v_cnts; i++)
    {
        uint32_t *src_blk_ptr = src_ptr;

        uint32_t j;

        for (j=0; j<h_cnts; j++)
        {
            uint32_t k;
            uint32_t coor_mode = ((i & 0x01)<<1) + (j & 0x01);

            blk_info.offset_tbl = curve_infp_ptr->offset_tbl[coor_mode];

            dither_block(src_blk_ptr, width, &blk_info);

            src_blk_ptr += BLK_WIDTH;
        }

        src_ptr += stride;
    }

#ifndef TEST_ENV
    end_time = systemTime(SYSTEM_TIME_MONOTONIC);

    IMG_DITHER_LOGV("dither end: size (%d, %d), time = %lldum", width, height, (end_time - begin_time)/1000);

    return (uint32_t)((end_time - begin_time)/1000);
#else
    return 0;
#endif
}

int32_t img_dither_init(struct img_dither_init_in_param *in_param, struct img_dither_init_out_param *out_param)
{
    uint32_t alg_id = 0;
    int32_t rtn = img_dither_rtn_sucess;
    uint32_t width = 0, height = 0;
    struct img_dither_handle *handle = PNULL;

    IMG_DITHER_LOGV(" start!\n");

    if ((PNULL == in_param) \
        ||(PNULL == out_param)) {
        IMG_DITHER_LOGE(" param pointer is invalidated: in:0x%x, out: 0x%x \n",\
            (uint32_t)in_param, (uint32_t)out_param);
        return -img_dither_rtn_pointer_null;
    }

    alg_id = in_param->alg_id;
    out_param->param = 0;

    IMG_DITHER_LOGV(" alg: %d, size (%d, %d) \n", alg_id, in_param->width, in_param->height);

    handle = (struct img_dither_handle*)malloc(sizeof (struct img_dither_handle));
    if (PNULL == handle) {
        IMG_DITHER_LOGE(" malloc failed \n");

        return -img_dither_rtn_no_memory;
    }

    handle->magic_flag = DITHER_MAGIC_FLAG;
    switch (alg_id) {

        case 0:
            width = in_param->width;
            height = in_param->height;
            handle->handle = (uint32_t)_img_dither_0_init(width, height);
        break;

        default:
        break;
    }

    handle->alg_id = alg_id;

    IMG_DITHER_LOGV(" finished !\n");

    out_param->param = (uint32_t)handle;

    return (int32_t)rtn;
}


int32_t img_dither_deinit( uint32_t handle)
{
    uint32_t alg_id = 0;
    int32_t ret = 0;
    struct img_dither_handle *handle_ptr = PNULL;

    IMG_DITHER_LOGV(" start! \n");

    if (0 == handle) {
        IMG_DITHER_LOGE(" input is invalidated\n");
        return -img_dither_rtn_param_invalidate;
    }
    handle_ptr = (struct img_dither_handle*)handle;

    if (DITHER_MAGIC_FLAG!= handle_ptr->magic_flag) {
        IMG_DITHER_LOGE(" handle is invalidated, flag: 0x%x\n", handle_ptr->magic_flag);

        return -img_dither_rtn_param_invalidate;
    }

    alg_id = handle_ptr->alg_id;

    IMG_DITHER_LOGV("alg: %d\n", alg_id);

    switch(alg_id)
    {
        case 0:
            ret = _img_dither_0_deinit(handle_ptr->handle);
        break;

        default:
        break;
    }

    if(ret) {
        IMG_DITHER_LOGV("alg: %d de-init failed: ret = 0x%x", alg_id, (uint32_t)ret);
    }

    if (handle_ptr) {
        free(handle_ptr);
        handle_ptr = PNULL;
    }

    IMG_DITHER_LOGV(" finished \n");

    return ret;
}


int32_t img_dither_process(uint32_t handle, struct img_dither_in_param *in_param, struct img_dither_out_param *out_param)
{
    int32_t ret = img_dither_rtn_sucess;
    uint8_t *data_ptr = PNULL;
    uint32_t width = 0, height = 0;
    struct img_dither_handle *handle_ptr = PNULL;

    IMG_DITHER_LOGV("start! \n");

    if ((PNULL == in_param)\
        ||(PNULL == out_param)\
        ||(PNULL == handle)) {
        IMG_DITHER_LOGE(" param pointer is PNULL, handle: 0x%x, in: 0x%x, out: 0x%x\n",\
            (uint32_t)handle, (uint32_t)in_param, (uint32_t)out_param);

        ret = -img_dither_rtn_pointer_null;
        goto IMG_DITHER_EIXT;
    }

    handle_ptr = (struct img_dither_handle*)handle;

    if (DITHER_MAGIC_FLAG != handle_ptr->magic_flag) {
        IMG_DITHER_LOGE(" handle is invalidated\n");
        ret = -img_dither_rtn_param_invalidate;

        goto IMG_DITHER_EIXT;
    }

    if ((PNULL == in_param->data_addr)) {
        IMG_DITHER_LOGE(" data pointer is PNULL, data: 0x%x\n", \
            (uint32_t)in_param->data_addr);
        ret = -img_dither_rtn_pointer_null;
        goto IMG_DITHER_EIXT;
    }

    if(image_dither_precheck(in_param) == false) {
        IMG_DITHER_LOGV(" data is pure single color, no dither is performed\n");
        ret = -img_dither_rtn_param_invalidate;
        goto IMG_DITHER_EIXT;
    }

    IMG_DITHER_LOGV(" addr: 0x%x, size (%d, %d), alg_id: %d, format: %d\n",\
    (uint32_t)in_param->data_addr, in_param->width, in_param->height,\
    in_param->alg_id, in_param->format);

    width = in_param->width;
    height = in_param->height;

    /*image dither*/
    data_ptr = (uint8_t*)in_param->data_addr;

    switch (in_param->alg_id)
    {
        case 0:
        {
            _img_dither_00(handle_ptr->handle, (uint8_t*)data_ptr);
        }
        break;

        default:
        break;
    }

    if (ret) {
        IMG_DITHER_LOGE(" dither process failed\n, ret: 0x%x\n", (uint32_t)ret);
    }

IMG_DITHER_EIXT:

    IMG_DITHER_LOGV("finished! \n");

    return ret;

}
