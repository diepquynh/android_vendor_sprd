
#ifndef _UTEST_GSP_INFRASTRUCTURE_H_
#define _UTEST_GSP_INFRASTRUCTURE_H_

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/mman.h>
#include <sys/time.h>

#include "ion_sprd.h"
#include "../../libs/hwcomposer/sc8830/gsp_hal.h"
#include "MemoryHeapIon.h"
//#include "ansidecl.h"


#ifndef PAGE_SIZE
#define PAGE_SHIFT 12
#define PAGE_SIZE (1UL << PAGE_SHIFT)
#endif

#define GSP_BUFFER_FD
#define SHOW_GSP_OUTPUT
//#define GSP_HAL_CONCURRENT_TEST


/*
 * Android log priority values, in ascending priority order.
 */

enum {
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL,
} ;
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "GSP"

#define STR_LOG_INFO        "INF:"
#define STR_LOG_WARN        "WRN:"
#define STR_LOG_ERROR       "ERR:"
#define STR_LOG_FATAL       "FTL:"

#ifndef ulong
#define ulong unsigned long
#endif
#ifndef int64_t
#define int64_t long long
#endif

extern uint32_t log_mask;
extern uint32_t get_local_tid(pthread_t tid);

/*
 * Simplified macro to send an error log message using the current LOG_TAG.
 */
#define CPNDITION(cond)     (__builtin_expect(((1<<(cond))&log_mask)!=0, 1))

#ifdef ALOGE
#undef ALOGE
#endif
#define ALOGE(...) \
    if(CPNDITION(LOG_ERROR))\
    {\
        printf(LOG_TAG);\
        printf("[%u] ",get_local_tid(pthread_self()));\
        printf(STR_LOG_ERROR);\
        (void)printf(__VA_ARGS__);\
    }\
    else\
    {\
        (void)0;\
    }


#ifdef ALOGI
#undef ALOGI
#endif

#define ALOGI(cond, ...) \
    if(CPNDITION(cond))\
    {\
        printf(LOG_TAG);\
        printf("[%u] ",get_local_tid(pthread_self()));\
        printf(STR_LOG_INFO);\
        (void)printf(__VA_ARGS__);\
    }\
    else\
    {\
        (void)0;\
    }



#define ALIGN(x, a) (((x) + (a) - 1) & ~((a) + 1))


#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof((arr)[0]))
#define PRINTF_RETURN() printf("%s[%d] return.\n",__func__,__LINE__)

#ifndef MAX
#define MAX(a,b)                   (((a)>(b))?(a):(b))
#endif
#ifndef MIN
#define MIN(a,b)                   (((a)<(b))?(a):(b))
#endif

typedef enum {
    GSP_INTF_INVALID,
    GSP_INTF_GET,
    GSP_INTF_PUT,
    GSP_INTF_MAX,
}
GSP_INTF_OPS_E;// the address type of gsp can process

typedef struct {
    ulong addr_y;
    ulong addr_uv;
    ulong addr_v;   //for 3 plane
}
GSP_DATA_ADDR_T_ULONG;

typedef struct {
    char                    *filename;
    char                    pallet;// pallet enable flag
    char                    pm_en;// pre-multi enable
    char                    pm_mod;// pre-multi mod
    char                    reduce;// is reduce yuv?

    char                    format;
    char                    pixel_format;// for alloc buffer
    char                    rotation;
    char                    alpha;
    uint32_t                pixel_w;// pixel size, for add frame

    FILE                    *raw_fp;// raw image file fp

    char                    map_once;// 0:map every time; 1:once
    char                    tap_col;//
    char                    tap_row;//
    //char                    addr_fd;// 1: pass addr by fd; 0: pass by real addr

    char                    addr_type;//0 physical; 1 iova
    GSP_DATA_ADDR_T         pa;
    GSP_DATA_ADDR_T_ULONG         va;
    uint32_t                buffersize;
    //private_handle_t      *Buffer_handle;
    class MemoryHeapIon      *MemoryHeap;


    char                    need_copy;
    char                    addr_type_cpy;//0 physical; 1 iova
    GSP_DATA_ADDR_T         pa_cpy;
    GSP_DATA_ADDR_T_ULONG         va_cpy;
    uint32_t                buffersize_cpy;
    //private_handle_t      *buffer_handle_cpy;
    class MemoryHeapIon *MemoryHeap_cpy;


    uint32_t size_y;
    uint32_t size_u;
    uint32_t size_v;
    uint32_t size_all;


    GSP_RECT_SIZE_T         pitch;
    GSP_POSITION_T          clip_start;
    GSP_RECT_SIZE_T         clip_size;
    GSP_POSITION_T          out_start;
    GSP_RECT_SIZE_T         out_size;
}
GSP_LAYER_INFO_T;

typedef int32_t (*GSP_PROCESS)(GSP_CONFIG_INFO_T *pgsp_cfg_info);
typedef int32_t (*GSP_GETCAPABILITY)(GSP_CAPABILITY_T *pGsp_cap);

typedef struct {
    char                    hold_flag;// don't exit programe when gsp return, to take a special checking
    char                    performance_flag;// test 1000 times to get an average time cost
    char                    power_flag;// take a test at fixed frequence to test power

    GSP_PROCESS      gsp_process;
    GSP_GETCAPABILITY gsp_getCapability;
    char                    map_once;// 0:map every time; 1:once
    char                    scl_rand;
}
GSP_MISC_INFO_T;


typedef struct {
    GSP_LAYER_INFO_T        layer0 ;
    GSP_LAYER_INFO_T        layer1;
    GSP_LAYER_INFO_T        layerd;
    GSP_MISC_INFO_T         misc;
} GSP_INFO_T;

/*
 *  Available layer rectangle.
 * */
struct sprdRect {
    uint32_t x;
    uint32_t y;
    uint32_t w;
    uint32_t h;
};

#include "display_raw_data.h"// display the gsp output

typedef void (*ut_case_config)(GSP_INFO_T *);

typedef struct {
    uint32_t times_limit;// repeat times
    uint32_t sleep_time;// after case finish , have a sleep
    GSP_CAPABILITY_T* gsp_capability;
    GSP_INFO_T* gsp_info;//general case
    FrameBufferInfo *fbInfo;
    uint32_t *md5;// the reference md5 digest
    uint32_t md5_result;// md5 check result
    uint32_t execute_flag;// this case executes or not?
    ut_case_config cfg;
    const char *case_title;// case name
} GSP_Case_Info;


typedef struct {
    uint32_t thread_id;// my 1 2 3 4 ...
    pthread_t tid;// real thread id
    GSP_Case_Info *case_list;
    int64_t end_time;
    uint32_t idx_min;
    uint32_t idx_max;
} Thread_Params;

#define THREAD_MAX 4
extern Thread_Params g_thread_params[];
#if 0
#define foot_print() \
{\
    ALOGE("%s[%d] \n",__func__,__LINE__);\
}
#else
#define foot_print() \
{\
    ALOGE("%s[%d] \n",__func__,__LINE__);\
    sleep(1);\
}
#endif
extern void memset_gsp(void *p, uint8_t value, uint32_t num);
extern void memcpy_gsp(void *dst, const void *src, uint32_t num);
extern void print_data(uint64_t base,uint32_t c, uint32_t log_level);
extern void print_layer_params(GSP_LAYER_INFO_T *pLayer);
extern void print_misc_params(GSP_MISC_INFO_T *pMisc);
extern int calc_input_plane_size(GSP_LAYER_INFO_T *pLayer);
extern int calc_output_plane_size(GSP_LAYER_INFO_T *pLayer);

extern int alloc_buffer(GSP_LAYER_INFO_T *pLayer);
extern int free_buffer(GSP_LAYER_INFO_T *pLayer);
extern int open_raw_file(GSP_LAYER_INFO_T *pLayer,const char *pFlag);
extern int read_raw_file(GSP_LAYER_INFO_T *pLayer);
extern int write_raw_file(GSP_LAYER_INFO_T *pLayer);
extern int close_raw_file(GSP_LAYER_INFO_T *pLayer);

extern int set_gsp_cfg_info(GSP_CONFIG_INFO_T *pgsp_cfg_info,
                     GSP_LAYER_INFO_T *pLayer0,
                     GSP_LAYER_INFO_T *pLayer1,
                     GSP_LAYER_INFO_T *pLayerd,
                     GSP_MISC_INFO_T *pMisc);

extern int64_t systemTime();

extern int get_gsp_interface(GSP_MISC_INFO_T *misc, GSP_INTF_OPS_E ops);

extern int dump_raw_file(GSP_LAYER_INFO_T *pLayer,GSP_CONFIG_INFO_T *gsp_cfg_info);

#endif

