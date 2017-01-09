
#include <string.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>

#include "ion_sprd.h"
#include "MemoryHeapIon.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
//#include <unistd.h>
#include <time.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <math.h>

#include <linux/ion.h>
//#include <binder/MemoryHeapIon.h>
//using namespace android;


#include "infrastructure.h"
#include "gralloc_priv.h"
#include "legacy_case.h"
#include "color_block_gen.h"
#include "display_raw_data.h"
#include "md5.h"




Thread_Params g_thread_params[THREAD_MAX];

static void ut_general_case_config(GSP_INFO_T *gsp_info)
{
    static char layerdname[128]= {0};
    GSP_PROCESS      gsp_process=NULL;
    GSP_GETCAPABILITY gsp_getCapability=NULL;

    if(gsp_info==NULL) {
        return;
    }
    gsp_process = gsp_info->misc.gsp_process;
    gsp_getCapability = gsp_info->misc.gsp_getCapability;
    memset_gsp((void*)gsp_info,0,sizeof(*gsp_info));
    gsp_info->misc.gsp_process = gsp_process;
    gsp_info->misc.gsp_getCapability = gsp_getCapability;

    gsp_info->layer0.pitch.w = 1400;
    gsp_info->layer0.pitch.h = 800;
    gsp_info->layer0.format = GSP_SRC_FMT_YUV420_2P;
    gsp_info->layer0.filename = const_cast<char*>("1920x1080_YUV4202P.raw");
    //gsp_info->layer0.addr_fd = 0;
    gsp_info->layer0.addr_type = 0;
    gsp_info->layer0.clip_start.x = 40;
    gsp_info->layer0.clip_start.y = 30;
    gsp_info->layer0.clip_size.w = 1280;
    gsp_info->layer0.clip_size.h = 720;
    gsp_info->layer0.out_start.x = 54;//20
    gsp_info->layer0.out_start.y = 96;//30
    gsp_info->layer0.out_size.w = 540-2*gsp_info->layer0.out_start.x;
    gsp_info->layer0.out_size.h = 960-2*gsp_info->layer0.out_start.y;
    gsp_info->layer0.rotation = 1;//90 degree
    gsp_info->layer0.alpha = 0xFF;
    gsp_info->layer0.tap_row= 8;
    gsp_info->layer0.tap_col = 8;
    gsp_info->layer0.pm_mod = 0;


    gsp_info->layer1.clip_start.x = 60;
    gsp_info->layer1.clip_start.y = 30;
    gsp_info->layer1.pitch.w = 960+3*gsp_info->layer1.clip_start.x;
    gsp_info->layer1.pitch.h = 540+3*gsp_info->layer1.clip_start.y;
    gsp_info->layer1.clip_size.w = 960;
    gsp_info->layer1.clip_size.h = 540;
    gsp_info->layer1.format = GSP_SRC_FMT_ARGB888;
    gsp_info->layer1.filename = const_cast<char*>("1140x630_ARGB888.raw");
    //gsp_info->layer1.addr_fd = 0;
    gsp_info->layer1.addr_type = 0;
    gsp_info->layer1.out_start.x = 0;
    gsp_info->layer1.out_start.y = 0;
    gsp_info->layer1.rotation = 3;//270 degree
    gsp_info->layer1.alpha = 0x60;//0x30

    gsp_info->layerd.pitch.w = 540;
    gsp_info->layerd.pitch.h = 960;
    gsp_info->layerd.format = GSP_DST_FMT_YUV420_2P;
    sprintf(layerdname,"/data/gsp/out/%dx%d_YUV4202P.raw",gsp_info->layerd.pitch.w,gsp_info->layerd.pitch.h);
    gsp_info->layerd.filename = layerdname;
    //gsp_info->layerd.addr_fd = 0;
    gsp_info->layerd.addr_type = 0;
}

static void ut_frame_scaling_config(GSP_INFO_T *gsp_info)
{
    gsp_info->layer0.clip_start.x = 0;
    gsp_info->layer0.clip_start.y = 0;
    gsp_info->layer0.clip_size.w = gsp_info->layer0.pitch.w;
    gsp_info->layer0.clip_size.h = gsp_info->layer0.pitch.h;
}

static void ut_no_scaling_config(GSP_INFO_T *gsp_info)
{
    gsp_info->layer0.out_size.w = 540-2*gsp_info->layer0.out_start.x;
    gsp_info->layer0.out_size.h = 960-2*gsp_info->layer0.out_start.y;

    if(gsp_info->layer0.rotation & 0x1) {
        gsp_info->layer0.clip_size.w = gsp_info->layer0.out_size.h;
        gsp_info->layer0.clip_size.h = gsp_info->layer0.out_size.w;
    } else {
        gsp_info->layer0.clip_size.w = gsp_info->layer0.out_size.w;
        gsp_info->layer0.clip_size.h = gsp_info->layer0.out_size.h;
    }
}
static void ut_no_rot_config(GSP_INFO_T *gsp_info)
{
    uint16_t    t = gsp_info->layer0.out_size.w;
    gsp_info->layer0.out_size.w = gsp_info->layer0.out_size.h;
    gsp_info->layer0.out_size.h = t;
    gsp_info->layer0.rotation = 0;//0 degree

    gsp_info->layer1.rotation = 0;//0 degree
    gsp_info->layerd.pitch.w = 960;
    gsp_info->layerd.pitch.h = 540;
}

static void ut_iova_config(GSP_INFO_T *gsp_info)
{
    gsp_info->layer0.addr_type=1;
    gsp_info->layer1.addr_type=1;
    gsp_info->layerd.addr_type=1;

    //gsp_info->layer0.addr_fd=1;
    //gsp_info->layer1.addr_fd=1;
    //gsp_info->layerd.addr_fd=1;
}

static int gsp_clock_check(GSP_CAPABILITY_T *gsp_capability)
{
    int fd,record=0;
    uint32_t i=0;
    char *vbase;
    uint32_t  pbase,val;

    typedef struct {
        uint32_t reg;
        uint32_t mask;
        uint32_t desire_val;
    }
    GSP_REG_CHECK_T;


    /*
    default tshark
    0x20D00010 [9] //clock force enable
    0x71200028 [1:0] //gsp clock select
    */
    GSP_REG_CHECK_T array[]= {
        {0x20D00010,0x1<<9,0x0<<9},
        {0x71200028,0x3<<0,0x3<<0},
    };

    if(gsp_capability->magic == CAPABILITY_MAGIC_NUMBER) {
        if(gsp_capability->version==6) { //sharkL or sharkL64
            /*
            0x20E00010 [9] //clock force enable
            0x21500028 [1:0] //gsp clock select
            */
            array[0].reg = 0x20E00010;
            array[1].reg = 0x21500028;
        }
    }


    /* ok, we've done with the options/arguments, start to work */
    if ((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) {
        ALOGE("%s[%d] could not open /dev/mem/\n", __func__, __LINE__);
        return -1;
    }

    i=0;
    while(i<ARRAY_SIZE(array)) {
        pbase = array[i].reg &(~(PAGE_SIZE - 1));
        vbase = (char*)mmap(0, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, pbase);
        if (vbase == (char *)-1) {
            ALOGE("%s[%d]mmap failed!\n", __func__, __LINE__);
            return -1;
        }

        val = *(uint32_t*)(vbase+(array[i].reg & (PAGE_SIZE - 1)));
        ALOGI(LOG_INFO,"%s[%d] reg:0x%08x val:0x%08x, msk:0x%08x, desire val:0x%08x\n", __func__, __LINE__,
              array[i].reg,
              val,
              array[i].mask,
              array[i].desire_val);

        if( (val& array[i].mask) != array[i].desire_val) {
            ALOGE("%s[%d] register is not set properly.\n", __func__, __LINE__);
            record++;
        } else {
            ALOGI(LOG_INFO,"%s[%d]  register is set ok.\n", __func__, __LINE__);
        }
        if (munmap(vbase, PAGE_SIZE) == -1) {
            ALOGE("%s[%d] munmap failed!\n", __func__, __LINE__);
            return -1;
        }
        i++;
    }

    return record;
}
static int ut_gsp_free_all_buffer(GSP_INFO_T *gsp_info)
{
    free_buffer(&(gsp_info->layerd));
    free_buffer(&(gsp_info->layer1));
    free_buffer(&(gsp_info->layer0));
    return 0;
}

static int ut_gsp_alloc_all_buffer(GSP_INFO_T *gsp_info)
{
    int ret = 0;

    //calc size of each plane
    ret = calc_input_plane_size(&(gsp_info->layer0));
    ret |= calc_input_plane_size(&(gsp_info->layer1));
    ret |= calc_output_plane_size(&(gsp_info->layerd));

    if(ret) {
        PRINTF_RETURN();
        return ret;
    }

    ALOGI(LOG_INFO,"cf0:%d, total_size:%d\n",gsp_info->layer0.format,gsp_info->layer0.size_all);
    ALOGI(LOG_INFO,"cf1:%d, total_size:%d\n",gsp_info->layer1.format,gsp_info->layer1.size_all);
    ALOGI(LOG_INFO,"cfd:%d, total_size:%d\n",gsp_info->layerd.format,gsp_info->layerd.size_all);
    /*size check*/
    if((gsp_info->layer0.size_all == 0 && gsp_info->layer1.size_all == 0) || (gsp_info->layerd.size_all == 0)) {
        PRINTF_RETURN();
        return ret;
    }


    if(gsp_info->layer0.filename != NULL && gsp_info->layer0.size_all > 0) {
        ALOGI(LOG_INFO,"L0 alloc_buffer \n");
        ret = alloc_buffer(&(gsp_info->layer0));
        if(ret) {
            ALOGI(LOG_FATAL,"L0 alloc_buffer failed\n");
            goto exit;
        }
    }

    if(gsp_info->layer1.filename != NULL && gsp_info->layer1.size_all > 0) {
        ALOGI(LOG_INFO,"L1 alloc_buffer \n");
        ret = alloc_buffer(&(gsp_info->layer1));
        if(ret) {
            ALOGI(LOG_FATAL,"L1 alloc_buffer failed\n");
            goto exit;
        }
    }

    ALOGI(LOG_INFO,"Ld alloc_buffer \n");
    ret = alloc_buffer(&(gsp_info->layerd));
    if(ret) {
        ALOGI(LOG_FATAL,"Ld alloc_buffer failed\n");
        goto exit;
    }

    return 0;
exit:
    ut_gsp_free_all_buffer(gsp_info);
    return -1;
}


static int ut_gsp_common(GSP_CAPABILITY_T *gsp_capability,GSP_INFO_T *gsp_info, FrameBufferInfo *fbInfo,uint32_t test_cnt_max)
{
    int ret=0;
    long int start_time = 0;
    long int end_time = 0;
    unsigned long int test_cnt = 0;
    long int cost_average = 0;
    long int cost_max = 0;
    long int cost_min = 10000000;
    GSP_CONFIG_INFO_T gsp_cfg_info;

    ALOGI(LOG_INFO,"%s[%d],out alloc pa:0x%08x va:0x%08x ,size:%d\n", __func__, __LINE__,(unsigned int)gsp_info->layerd.pa.addr_y,(unsigned int)gsp_info->layerd.va.addr_y,gsp_info->layerd.size_all);
    if(gsp_info->layer0.filename != NULL) {
        ret = open_raw_file(&gsp_info->layer0,"r");
        if(ret) {
            test_gen_color_blocks((char*)gsp_info->layer0.va.addr_y,gsp_info->layer0.pitch.w,gsp_info->layer0.pitch.h,gsp_info->layer0.format,2);
            add_frame_boundary(&gsp_info->layer0);
            open_raw_file(&gsp_info->layer0,"wb");
            write_raw_file(&gsp_info->layer0);
        } else {
            ret = read_raw_file(&gsp_info->layer0);
            if(ret) {
                goto close_file;
            }
        }
    }

    if(gsp_info->layer1.filename != NULL) {
        ret = open_raw_file(&gsp_info->layer1,"r");
        if(ret) {
            //test_gen_color_blocks((char*)gsp_info.layer1.va.addr_y,gsp_info.layer1.pitch.w,gsp_info.layer1.pitch.h,gsp_info.layer1.format,2);
            struct sprdRect rect ;
            memset(&rect, 0, sizeof(sprdRect));
            rect.w = gsp_info->layer1.pitch.w;
            rect.h = gsp_info->layer1.pitch.h;
            test_gen_color_block((char*)gsp_info->layer1.va.addr_y,gsp_info->layer1.pitch.w,gsp_info->layer1.pitch.h,gsp_info->layer1.format, &rect,GEN_COLOR_WHITE,2);
            //open_raw_file(&gsp_info->layer1,"wb");
            //write_raw_file(&gsp_info->layer1);
            ALOGI(LOG_INFO,"gen color block success\n");
        } else {
            ret = read_raw_file(&gsp_info->layer1);
            if(ret) {
                goto close_file;
            }
        }
    }

    if(gsp_info->layerd.filename != NULL) {
        ret = open_raw_file(&gsp_info->layerd,"wb");
        if(ret) {
            ALOGE("open dst file failed, skip calling GSP_Proccess()!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
            goto close_file;
        }
    }

    start_time = systemTime()/1000;

    test_cnt_max = 1;
    while(test_cnt < test_cnt_max) {
        int64_t start = systemTime()/1000;
        int64_t cost = 0;

        memset_gsp((void*)&gsp_cfg_info,0,sizeof(gsp_cfg_info));
        set_gsp_cfg_info(&gsp_cfg_info,&gsp_info->layer0,&gsp_info->layer1,&gsp_info->layerd,&gsp_info->misc);

        if((gsp_info->layer0.addr_type == 1 || gsp_info->layer1.addr_type == 1 || gsp_info->layerd.addr_type == 1)
           && gsp_capability->magic == CAPABILITY_MAGIC_NUMBER
           && gsp_capability->buf_type_support == GSP_ADDR_TYPE_IOVIRTUAL
           && gsp_capability->video_need_copy == 0) {
            gsp_cfg_info.misc_info.split_pages = 1;
        } else {
            gsp_cfg_info.misc_info.split_pages = 0;
        }

        ret = (*gsp_info->misc.gsp_process)(&gsp_cfg_info);
        if(0 == ret) {
            if(test_cnt_max == 1) {
                ALOGI(LOG_INFO,"GSP_Proccess ok\n");
            }
        } else {
            ALOGE("GSP_Proccess err:%d!!\n",ret);
            goto close_file;
        }

        cost = systemTime()/1000 -start;
        cost_max = MAX(cost,cost_max);
        cost_min = MIN(cost,cost_min);
        test_cnt++;
    }
    /*
    sleep(2);
    usleep(520);
    */
    end_time = systemTime()/1000;
    cost_average = (end_time-start_time)/test_cnt_max;

    ALOGI(LOG_INFO,"%s[%d], (%ld-%ld)/%d=%ld us , max:%ld us, min:%ld us!!\n",__func__,__LINE__,
          end_time ,start_time,test_cnt_max,cost_average,cost_max,cost_min);

    dump_raw_file(&gsp_info->layerd,&gsp_cfg_info);


    ALOGI(LOG_INFO,"%s[%d], write %s params: addr:%p size:%d \n",  __func__, __LINE__,gsp_info->layerd.filename,(void*)gsp_info->layerd.va.addr_y,(gsp_info->layerd.size_y+gsp_info->layerd.size_u+gsp_info->layerd.size_v));
    ret = write_raw_file(&gsp_info->layerd);
    if(ret) {
        goto close_file;
    }



#ifdef SHOW_GSP_OUTPUT
    display_raw_file(&gsp_info->layerd,&gsp_info->misc,&gsp_cfg_info,fbInfo);
#endif

    if(0) {
        md5_context md5_cxt;
        uint8_t digest[16] = {0};
        memset(&md5_cxt, 0, sizeof(md5_context));

        md5_starts(&md5_cxt);
        md5_update(&md5_cxt, (uint8_t*)gsp_info->layerd.va.addr_y, gsp_info->layerd.size_all);
        md5_finish(&md5_cxt, (uint8_t*)&digest[0]);
        print_data((uint64_t)digest,4,LOG_INFO);
    }

close_file:
    close_raw_file(&gsp_info->layerd);
    close_raw_file(&gsp_info->layer1);
    close_raw_file(&gsp_info->layer0);
    return ret;

}


static uint32_t ut_gsp_md5(GSP_INFO_T *gsp_info,uint8_t *src_digest)
{
    md5_context md5_cxt;
    uint8_t digest[16] = {0};
    uint8_t i = 0;

    memset(&md5_cxt, 0, sizeof(md5_context));
    md5_starts(&md5_cxt);
    md5_update(&md5_cxt, (uint8_t*)gsp_info->layerd.va.addr_y, gsp_info->layerd.size_all);
    md5_finish(&md5_cxt, (uint8_t*)&digest[0]);

    while(i<16) {
        if(digest[i] != src_digest[i]) {
            ALOGE("MD5 not match!\n");
            goto exit;
        }
        i++;
    }
    ALOGI(LOG_INFO,"MD5 check success!\n");
    return 0;
exit:
    print_data((uint64_t)src_digest,4,LOG_ERROR);
    print_data((uint64_t)digest,4,LOG_ERROR);
    return 1;
}


static int process_cases(GSP_Case_Info *case_list, uint32_t i)
{
    int ret = 0;
    GSP_INFO_T gsp_info = *case_list[i].gsp_info;

    if(gsp_info.misc.gsp_process == NULL) {
        ALOGE("%s[%d], gsp_process is null\n",__func__,__LINE__);
        return -1;
    }

    ALOGI(LOG_INFO,"%s[%d], CASE: %s\n",__func__,__LINE__,case_list[i].case_title);
    if(case_list[i].cfg) {
        case_list[i].cfg(&gsp_info);
    }

    ret = ut_gsp_common(case_list[i].gsp_capability,&gsp_info,case_list[i].fbInfo,case_list[i].times_limit);
    case_list[i].execute_flag = 1;
    case_list[i].md5_result = ut_gsp_md5(&gsp_info,(uint8_t*)case_list[i].md5);
    sleep(case_list[i].sleep_time);
    return ret;
}


void gsp_robust_check(GSP_Case_Info *case_list, uint32_t i)
{
    GSP_INFO_T gsp_info = *case_list[i].gsp_info;
    int ret = 0;

    ALOGI(LOG_INFO,"%s[%d], enter.\n",__func__,__LINE__);
    gsp_info.layer0.clip_start.x += 1;
    gsp_info.layer0.clip_start.y += 2;
    ret=ut_gsp_common(case_list[i].gsp_capability,&gsp_info,case_list[i].fbInfo,case_list[i].times_limit);
    gsp_info.layer0.out_start.x += 1;
    gsp_info.layer0.out_start.y += 2;
    ret|=ut_gsp_common(case_list[i].gsp_capability,&gsp_info,case_list[i].fbInfo,case_list[i].times_limit);
    if((ret & GSP_HAL_PARAM_CHECK_ERR) == GSP_HAL_PARAM_CHECK_ERR) {
        ALOGI(LOG_INFO,"%s[%d], CASE: x y odd/even test success\n",__func__,__LINE__);
    } else {
        ALOGE("%s[%d], CASE: x y odd/even test failed\n",__func__,__LINE__);
    }

    gsp_info = *case_list[i].gsp_info;
    gsp_info.layer0.clip_size.w += 1;
    gsp_info.layer0.clip_size.h += 2;
    ret=ut_gsp_common(case_list[i].gsp_capability,&gsp_info,case_list[i].fbInfo,case_list[i].times_limit);
    gsp_info.layer0.out_size.w += 1;
    gsp_info.layer0.out_size.h += 2;
    ret|=ut_gsp_common(case_list[i].gsp_capability,&gsp_info,case_list[i].fbInfo,case_list[i].times_limit);
    if((ret & GSP_HAL_PARAM_CHECK_ERR) == GSP_HAL_PARAM_CHECK_ERR) {
        ALOGI(LOG_INFO,"%s[%d], CASE: w h odd/even test success\n",__func__,__LINE__);
    } else {
        ALOGE("%s[%d], CASE: w h odd/even test failed\n",__func__,__LINE__);
    }


    gsp_info = *case_list[i].gsp_info;
    gsp_info.layer0.clip_size.w = gsp_info.layer0.pitch.w-gsp_info.layer0.clip_start.x+2;
    gsp_info.layer0.clip_size.h = gsp_info.layer0.pitch.h-gsp_info.layer0.clip_start.y+2;
    ret=ut_gsp_common(case_list[i].gsp_capability,&gsp_info,case_list[i].fbInfo,case_list[i].times_limit);
    gsp_info.layer0.out_size.w = gsp_info.layerd.pitch.w-gsp_info.layer0.out_start.x+2;
    gsp_info.layer0.out_size.h = gsp_info.layerd.pitch.h-gsp_info.layer0.out_start.y+2;
    ret|=ut_gsp_common(case_list[i].gsp_capability,&gsp_info,case_list[i].fbInfo,case_list[i].times_limit);
    if((ret & GSP_HAL_PARAM_CHECK_ERR) == GSP_HAL_PARAM_CHECK_ERR) {
        ALOGI(LOG_INFO,"%s[%d], CASE: clip/out region beyond boundary test success\n",__func__,__LINE__);
    } else {
        ALOGE("%s[%d], CASE: clip/out region beyond boundary test failed\n",__func__,__LINE__);
    }

    gsp_info = *case_list[i].gsp_info;
    gsp_info.layer0.clip_start.x = gsp_info.layer0.clip_start.y = 0;
    gsp_info.layer0.clip_size.w = gsp_info.layer0.pitch.w;
    gsp_info.layer0.clip_size.h = gsp_info.layer0.pitch.h;
    gsp_info.layer0.out_start.x = gsp_info.layer0.out_start.y = 0;
    gsp_info.layer0.out_size.w = gsp_info.layer0.clip_size.w/16-2;
    gsp_info.layer0.out_size.h = gsp_info.layer0.clip_size.h/16-2;
    gsp_info.layer0.rotation = 0;
    ret=ut_gsp_common(case_list[i].gsp_capability,&gsp_info,case_list[i].fbInfo,case_list[i].times_limit);
    if((ret & GSP_HAL_PARAM_CHECK_ERR) == GSP_HAL_PARAM_CHECK_ERR) {
        ALOGI(LOG_INFO,"%s[%d], CASE: scaling down < 1/16 test success\n",__func__,__LINE__);
    } else {
        ALOGE("%s[%d], CASE: scaling down < 1/16 test failed\n",__func__,__LINE__);
    }

    gsp_info = *case_list[i].gsp_info;
    gsp_info.layer0.clip_start.x = gsp_info.layer0.clip_start.y = 0;
    gsp_info.layer0.clip_size.w = gsp_info.layer0.out_size.w/2;
    gsp_info.layer0.clip_size.h = gsp_info.layer0.out_size.h/2;
    gsp_info.layer0.out_start.x = gsp_info.layer0.out_start.y = 0;
    gsp_info.layer0.out_size.w = gsp_info.layer0.clip_size.w-2;
    gsp_info.layer0.out_size.h = gsp_info.layer0.clip_size.h+2;
    gsp_info.layer0.rotation = 0;
    ret=ut_gsp_common(case_list[i].gsp_capability,&gsp_info,case_list[i].fbInfo,case_list[i].times_limit);
    if((ret & GSP_HAL_PARAM_CHECK_ERR) == GSP_HAL_PARAM_CHECK_ERR) {
        ALOGI(LOG_INFO,"%s[%d], CASE: scaling down/up same time test success\n",__func__,__LINE__);
    } else {
        ALOGE("%s[%d], CASE: scaling down/up same time test failed\n",__func__,__LINE__);
    }

}


#ifdef GSP_HAL_CONCURRENT_TEST
#include <signal.h>
#include <sys/wait.h>

static volatile int force_exit_flag = 0;
static volatile int s_err_cnt = 0;
static volatile int s_total_cnt = 0;

static void* gsp_test_thread_proc(void* data)
{
    int32_t ret = 0;
    uint32_t slp_time = 0;
    uint32_t i = 0;
    Thread_Params *tp = (Thread_Params*)data;
    pid_t pid;
    pthread_t tid;

    pid = getpid();
    tid = pthread_self();

    ALOGI(LOG_INFO,"test_thread[%d]: %u enter. pid:%d tid:%lud\n", __LINE__, tp->thread_id,pid,tid);
    while(!force_exit_flag && (tp->end_time > systemTime()/1000)) {
        srand(systemTime());
        uint32_t slp_time = 50 + (rand()%256);
        ALOGI(LOG_INFO,"thread[%d],sleep:%d ms zzzzzzz...\n",tp->thread_id , slp_time);
        usleep(slp_time);

        i = tp->idx_min+(rand()%(tp->idx_max-tp->idx_min));
        if(tp->case_list[i].times_limit != 1) {
            continue;
        }
        ALOGI(LOG_INFO,"thread[%d]: <<< case:%s\n", tp->thread_id, tp->case_list[i].case_title);
        tp->case_list[i].sleep_time = 0;
        ret = process_cases(tp->case_list, i);
        if(ret) {
            s_err_cnt ++;
            ALOGI(LOG_INFO,"concurrent test err:%d, err_cnt:%d\n", ret,s_err_cnt);
        }
        s_total_cnt++;
        ALOGI(LOG_INFO,"thread[%d]: case:%s >>>\n", tp->thread_id, tp->case_list[i].case_title);
    }
    ALOGI(LOG_INFO,"thread[%d]: exit. Line:%d\n", tp->thread_id ,__LINE__);
    tp->tid = 0;
    return NULL;
}

void sigterm_handler(int signo)
{
    force_exit_flag = 1;
    ALOGI(LOG_INFO,"sigterm_handler. Line:%d, signo = %d\n", __LINE__, signo);
    //exit(0);
}

int32_t create_gsp_test_thread(GSP_Case_Info *case_list, uint32_t min,uint32_t max)
{
    int32_t ret = 0,i=0,status;
    pthread_attr_t attr;
    //static Thread_Params g_thread_params[THREAD_MAX];

    ALOGI(LOG_INFO,"create threads enter \n");
    s_err_cnt = 0;
    s_total_cnt = 0;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    i = 0;
    while(i<THREAD_MAX) {
        g_thread_params[i].thread_id = i;
        g_thread_params[i].case_list = case_list;
        g_thread_params[i].idx_min = min;
        g_thread_params[i].idx_max = max;
        g_thread_params[i].end_time = systemTime()/1000 + 1*60000000;//test 1 minutes

        ret = pthread_create(&g_thread_params[i].tid, &attr, gsp_test_thread_proc, (void*)&g_thread_params[i]);
        if(ret == 0) {
            ALOGI(LOG_INFO,"create thread:%d success\n",i);
        } else {
            ALOGE("create thread:%d failed,err:%s\n", i,strerror(ret));
        }
        i++;
    }
    pthread_attr_destroy(&attr);
    signal(SIGTERM, sigterm_handler);
    signal(SIGKILL, sigterm_handler);

    while(force_exit_flag == 0) {
        uint32_t q=0;
        sleep(1);
        i = 0;
        while(i<THREAD_MAX) {
            if(g_thread_params[i].tid == 0) {
                q++;
            }
            i++;
        }
        if(q==THREAD_MAX) {
            break;
        }
    }

    ret = waitpid(getpid(), &status, __WALL);
    status = WEXITSTATUS(status);
    if (ret == -1 || status != 0) {
        ALOGI(LOG_INFO,"Error: waitpid() returns %d, status %d\n", ret, status);
    }

    return ret;
}

#endif

int main(int argc, char **argv)
{
    printf("hello,gsp!\n");
    int ret = 0;
    char summary[256] = {0};
    GSP_CAPABILITY_T gsp_capability;
    FrameBufferInfo fbInfo;
    GSP_INFO_T gsp_info;
    memset(&gsp_capability, 0, sizeof(GSP_CAPABILITY_T));
    memset(&fbInfo, 0, sizeof(FrameBufferInfo));

    ret = get_gsp_interface(&gsp_info.misc, GSP_INTF_GET);
    if(ret||gsp_info.misc.gsp_process==NULL||gsp_info.misc.gsp_getCapability==NULL) {
        ALOGE("%s[%d], get gsp so interface failed !!\n",__func__,__LINE__);
        return -1;
    } else {
        ALOGI(LOG_INFO,"get interface success, process:%p, getCapability:%p\n",gsp_info.misc.gsp_process,gsp_info.misc.gsp_getCapability);
    }

    ret = (*gsp_info.misc.gsp_getCapability)(&gsp_capability);
    if(ret) {
        ALOGE("%s[%d], get capability failed !!\n",__func__,__LINE__);
    } else {
        ALOGI(LOG_INFO,"GSP Capability info: magic:%08x, version:%d, support %s buffer, %s page boundary issue, %s process reduce yuv\n",
              gsp_capability.magic,
              gsp_capability.version,
              (gsp_capability.buf_type_support == GSP_ADDR_TYPE_IOVIRTUAL)?"virtual":"physical",
              (gsp_capability.video_need_copy == 1)?"with":"without",
              (gsp_capability.blend_video_with_OSD == 1)?"can":"can't");
    }

#ifdef SHOW_GSP_OUTPUT
    if(fbInfo.magic != FBINFO_MAGIC) {
        if(loadFrameBufferHAL(&fbInfo) != 0 ||fbInfo.magic != FBINFO_MAGIC ) {
            return -1;
        }
    }
#endif

    if(argc > 3) {
        ret = gsp_special_case(&fbInfo,gsp_info.misc.gsp_process,&gsp_capability,argc, argv);
#ifdef SHOW_GSP_OUTPUT
        unloadFrameBufferHAL(&fbInfo);
#endif
        return ret;
    } else {
        if (argc == 3 && strcmp(argv[1], "-log") == 0) {
            log_mask =  atoi(argv[2]);
        }
    }

    //unit test case
    memset_gsp((void*)g_thread_params,0,sizeof(g_thread_params));

    //clock check
    gsp_clock_check(&gsp_capability);

    //FrameBufferInfo fbInfo;
    // GSP_INFO_T gsp_info;
    const uint32_t src_digest[][4] = {
        {0xcf38c30d, 0x59679ada, 0x5fc5146b, 0x1c7a0be9}, //IOMMU general
        {0xcf38c30d, 0x59679ada, 0x5fc5146b, 0x1c7a0be9}, //IOMMU performance
        {0xcf38c30d, 0x59679ada, 0x5fc5146b, 0x1c7a0be9}, //general
        {0x78de336c, 0x161743b0, 0xc8bf2968, 0x29cfcbbf}, //no scaling
        {0x4ec7e4bf, 0xbc86425a, 0xc9dea5c6, 0xc41c9c63}, //no rotation
        {0xcf38c30d, 0x59679ada, 0x5fc5146b, 0x1c7a0be9}, //performance
        {0x3bb520c2, 0xdf816d58, 0x4b74fe1c, 0xd7ba775b}, //boundary scaling
    };
    GSP_Case_Info case_list[]= {
        {1,     1,      &gsp_capability,        &gsp_info,      &fbInfo,    (uint32_t*)&src_digest[0],0,0,     NULL,                       "IOMMU general"},
        {1,     0,      &gsp_capability,        &gsp_info,      &fbInfo,      (uint32_t*)&src_digest[1],0,0,      NULL,                       "IOMMU performance"},
        {1,     1,      &gsp_capability,        &gsp_info,      &fbInfo,    (uint32_t*)&src_digest[2],0,0,      NULL,                       "general"},
        {1,     1,      &gsp_capability,        &gsp_info,      &fbInfo,    (uint32_t*)&src_digest[3],0,0,      ut_no_scaling_config,   "no scaling"},
        {1,     1,      &gsp_capability,        &gsp_info,      &fbInfo,    (uint32_t*)&src_digest[4],0,0,      ut_no_rot_config,       "no rotation"},
        {1,     1,      &gsp_capability,        &gsp_info,      &fbInfo,    (uint32_t*)&src_digest[6],0,0,      ut_frame_scaling_config,"boundary scaling"},
        {1,     0,      &gsp_capability,        &gsp_info,      &fbInfo,      (uint32_t*)&src_digest[5],0,0,      NULL,                       "performance"},
    };
    uint32_t list_idx=0;

    uint32_t phy_buffer_alloc_flag=0;


    //memory alloc free test
    if(0) {
        uint32_t test_cnt_max = 1;
        while(1) {
            ut_general_case_config(&gsp_info);
            //ut_iova_config(gsp_info);
            ret = ut_gsp_alloc_all_buffer(&gsp_info);
            if(ret) {
                PRINTF_RETURN();
                goto free_layer_buff;
            }

            sleep(1);
            ut_gsp_free_all_buffer(&gsp_info);
            ALOGE("%s[%d], buffer test =============%d !!\n",__func__,__LINE__,test_cnt_max);
            test_cnt_max++;
            sleep(5);
        }
        return 0;
    }

    // all iommu case
    if(gsp_capability.magic == CAPABILITY_MAGIC_NUMBER
       && gsp_capability.buf_type_support == GSP_ADDR_TYPE_IOVIRTUAL
       && gsp_capability.video_need_copy == 0) {
        //  printf("%s, %d\n", __func__, __LINE__);
        ut_general_case_config(&gsp_info);
        ut_iova_config(&gsp_info);
        ret = ut_gsp_alloc_all_buffer(&gsp_info);
        if(ret) {
            PRINTF_RETURN();
            goto free_layer_buff;
        }
        process_cases(case_list, list_idx);
        list_idx++;
        process_cases(case_list, list_idx);
        list_idx++;
        ut_gsp_free_all_buffer(&gsp_info);
    }

    // all phy case
    ut_general_case_config(&gsp_info);
    ret = ut_gsp_alloc_all_buffer(&gsp_info);
    if(ret) {
        ALOGE("%s[%d] alloc phy buffer failed, skip phy case!!\n",__func__,__LINE__);
        goto skip_phy_case;
    } else {
        phy_buffer_alloc_flag = 1;
    }
    gsp_robust_check(case_list, list_idx);
    while(list_idx<ARRAY_SIZE(case_list)) {
        process_cases(case_list, list_idx);
        list_idx++;
    }
skip_phy_case:


    // output summary log
    {
        uint32_t md5_check_r = 0;
        uint32_t md5_check_exe = 0;
        list_idx=0;
        while(list_idx<ARRAY_SIZE(case_list)) {
            if(case_list[list_idx].md5_result) {
                md5_check_r++;
            }
            if(case_list[list_idx].execute_flag) {
                md5_check_exe++;
            }

            list_idx++;
        }

        sprintf(summary,"GSP test summary: total %d case, execute %d case, and %d failed\n",
                ARRAY_SIZE(case_list),
                md5_check_exe,
                md5_check_r);
    }
#ifdef GSP_HAL_CONCURRENT_TEST
    if(phy_buffer_alloc_flag==1) {
        create_gsp_test_thread(case_list,  2, 5);
        printf("GSP test summary: concurrent test %d times, err %d times!!\n",s_total_cnt,s_err_cnt);
    } else {
        create_gsp_test_thread(case_list,  0, 1);
        printf("GSP test summary: concurrent test %d times, err %d times!!\n",s_total_cnt,s_err_cnt);
    }
#endif
    printf("%s",summary);

#ifdef SHOW_GSP_OUTPUT
    unloadFrameBufferHAL(&fbInfo);
#endif
free_layer_buff:
    ut_gsp_free_all_buffer(&gsp_info);
    return 0;
}


