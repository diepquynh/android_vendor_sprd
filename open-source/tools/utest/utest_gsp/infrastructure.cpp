
#include "infrastructure.h"
//#include "color_block_gen.h"

uint32_t log_mask = 0xf;

void memset_gsp(void *p, uint8_t value, uint32_t num)
{
    uint32_t i;
    unsigned char *s = (unsigned char *)p;
    for(i = 0; i < num; i++) {
        *s = value;
        s++;
    }
}
void memcpy_gsp(void *dst, const void *src, uint32_t num)
{
    uint32_t i;
    unsigned char *dst_gsp = (unsigned char *)dst;
    unsigned char *src_gsp = (unsigned char *)src;
    for(i = 0; i < num; i++) {
        *dst_gsp = *src_gsp;
        dst_gsp++;
        src_gsp++;
    }
}

uint32_t get_local_tid(pthread_t tid)
{
    uint32_t i = 0;
    while(i<THREAD_MAX) {
        if( g_thread_params[i].tid == tid) {
            return g_thread_params[i].thread_id+1;
        }
        i++;
    }
    return 0;
}

void print_data(uint64_t base,uint32_t c, uint32_t log_level)
{
    uint32_t* pWord = (uint32_t*) base;
    if(CPNDITION(log_level)) {
        while(c) {
            printf("%08x ", *pWord);
            pWord++;
            c--;
        }
        printf("\n");
    }
}


void print_layer_params(GSP_LAYER_INFO_T *pLayer)
{
    ALOGI(LOG_INFO,"{%04dx%04d[(%04d,%04d)%04dx%04d]} ==rot:%d alpha:%03d copy:%d==> [(%04d,%04d)%04dx%04d] bufferType:%d format:%d file:%s \n",
          pLayer->pitch.w,
          pLayer->pitch.h,
          pLayer->clip_start.x,
          pLayer->clip_start.y,
          pLayer->clip_size.w,
          pLayer->clip_size.h,
          pLayer->rotation,
          pLayer->alpha,
          pLayer->need_copy,
          pLayer->out_start.x,
          pLayer->out_start.y,
          pLayer->out_size.w,
          pLayer->out_size.h,
          pLayer->addr_type,
          pLayer->format,
          pLayer->filename);
}

void print_misc_params(GSP_MISC_INFO_T *pMisc)
{
    ALOGI(LOG_INFO,"%s[%d],performance_flag:%d, power_flag:%d, hold_flag:%d, layers params:\n", __func__, __LINE__,
          pMisc->performance_flag,
          pMisc->power_flag,
          pMisc->hold_flag);
}

int calc_input_plane_size(GSP_LAYER_INFO_T *pLayer)
{
    switch(pLayer->format) {
        case GSP_SRC_FMT_ARGB888:
        case GSP_SRC_FMT_RGB888:
            pLayer->pixel_w = 4;
            pLayer->size_y = pLayer->pitch.w * pLayer->pitch.h * 4;
            pLayer->size_u = 0;
            pLayer->size_v = 0;
            //pLayer->pixel_format = HAL_PIXEL_FORMAT_RGBA_8888;
            break;
        case GSP_SRC_FMT_ARGB565:
            pLayer->pixel_w = 2;
            pLayer->size_y = pLayer->pitch.w * pLayer->pitch.h * 2;
            pLayer->size_u = pLayer->pitch.w * pLayer->pitch.h;
            pLayer->size_v = 0;
            //pLayer->pixel_format = HAL_PIXEL_FORMAT_RGB_565;
            break;
        case GSP_SRC_FMT_RGB565:
            pLayer->pixel_w = 2;
            pLayer->size_y = pLayer->pitch.w * pLayer->pitch.h * 2;
            pLayer->size_u = 0;
            pLayer->size_v = 0;
            //pLayer->pixel_format = HAL_PIXEL_FORMAT_RGB_565;
            break;
        case GSP_SRC_FMT_YUV420_2P:
            pLayer->pixel_w = 1;
            pLayer->size_y = pLayer->pitch.w * pLayer->pitch.h;
            pLayer->size_u = pLayer->size_y/2;
            pLayer->size_v = 0;
            //pLayer->pixel_format = HAL_PIXEL_FORMAT_YCbCr_420_SP;
            break;
        case GSP_SRC_FMT_YUV420_3P:
            pLayer->pixel_w = 1;
            pLayer->size_y = pLayer->pitch.w * pLayer->pitch.h;
            pLayer->size_u = pLayer->size_y/4;
            pLayer->size_v = pLayer->size_u;
            //pLayer->pixel_format = HAL_PIXEL_FORMAT_YCbCr_420_SP;
            break;
        case GSP_SRC_FMT_YUV400_1P:
            pLayer->pixel_w = 1;
            pLayer->size_y = pLayer->pitch.w * pLayer->pitch.h;
            pLayer->size_u = 0;
            pLayer->size_v = 0;
            //pLayer->pixel_format = HAL_PIXEL_FORMAT_YCbCr_420_SP;
            break;
        case GSP_SRC_FMT_YUV422_2P:
            pLayer->pixel_w = 1;
            pLayer->size_y = pLayer->pitch.w * pLayer->pitch.h;
            pLayer->size_u = pLayer->size_y;
            pLayer->size_v = 0;
            //pLayer->pixel_format = HAL_PIXEL_FORMAT_YCbCr_422_SP;
            break;
        case GSP_SRC_FMT_8BPP:
            pLayer->pixel_w = 1;
            pLayer->size_y = pLayer->pitch.w * pLayer->pitch.h;
            pLayer->size_u = 0;
            pLayer->size_v = 0;
            //pLayer->pixel_format = HAL_PIXEL_FORMAT_YCbCr_420_SP;
            break;
        default:
            return -1;//testing not support the other color format
            break;
    }

    pLayer->size_all = pLayer->size_y + pLayer->size_u + pLayer->size_v;
    return 0;
}

int calc_output_plane_size(GSP_LAYER_INFO_T *pLayer)
{
    switch(pLayer->format) {
        case GSP_DST_FMT_ARGB888:
        case GSP_DST_FMT_RGB888:
            pLayer->pixel_w = 4;
            pLayer->size_y = pLayer->pitch.w * pLayer->pitch.h * 4;
            pLayer->size_u = 0;
            pLayer->size_v = 0;
            //pLayer->pixel_format = HAL_PIXEL_FORMAT_RGBA_8888;
            break;
        case GSP_DST_FMT_ARGB565:
            pLayer->pixel_w = 2;
            pLayer->size_y = pLayer->pitch.w * pLayer->pitch.h * 2;
            pLayer->size_u = pLayer->pitch.w * pLayer->pitch.h;
            pLayer->size_v = 0;
            //pLayer->pixel_format = HAL_PIXEL_FORMAT_RGB_565;
            break;
        case GSP_DST_FMT_RGB565:
            pLayer->pixel_w = 2;
            pLayer->size_y = pLayer->pitch.w * pLayer->pitch.h * 2;
            pLayer->size_u = 0;
            pLayer->size_v = 0;
            //pLayer->pixel_format = HAL_PIXEL_FORMAT_RGB_565;
            break;
        case GSP_DST_FMT_YUV420_2P:
            pLayer->pixel_w = 1;
            pLayer->size_y = pLayer->pitch.w * pLayer->pitch.h;
            pLayer->size_u = pLayer->size_y/2;
            pLayer->size_v = 0;
            //pLayer->pixel_format = HAL_PIXEL_FORMAT_YCbCr_420_SP;
            break;
        case GSP_DST_FMT_YUV420_3P:
            pLayer->pixel_w = 1;
            pLayer->size_y = pLayer->pitch.w * pLayer->pitch.h;
            pLayer->size_u = pLayer->size_y/4;
            pLayer->size_v = pLayer->size_u;
            //pLayer->pixel_format = HAL_PIXEL_FORMAT_YCbCr_420_SP;
            break;
        case GSP_DST_FMT_YUV422_2P:
            pLayer->pixel_w = 1;
            pLayer->size_y = pLayer->pitch.w * pLayer->pitch.h;
            pLayer->size_u = pLayer->size_y;
            pLayer->size_v = 0;
            //pLayer->pixel_format = HAL_PIXEL_FORMAT_YCbCr_422_SP;
            break;
        default:
            ALOGE("not supported format!");
            return -1;//testing not support the other color format
    }

    pLayer->size_all = pLayer->size_y + pLayer->size_u + pLayer->size_v;
    return 0;
}


int alloc_buffer(GSP_LAYER_INFO_T *pLayer)
{
    unsigned long       mmu_addr = 0;
    size_t      temp_size = 0;
    int ret = 0;

    printf("%s, %d\n", __func__, __LINE__);
    //alloc none cached and buffered memory
    if(pLayer->addr_type == 0) { //physical buffer
        ALOGI(LOG_INFO,"[%d] alloc phy buffer \n",__LINE__);
        pLayer->MemoryHeap = new MemoryHeapIon("/dev/ion", pLayer->size_all, NO_CACHING, ION_HEAP_ID_MASK_OVERLAY);//GRALLOC_USAGE_OVERLAY_BUFFER ION_HEAP_CARVEOUT_MASK
        ret = pLayer->MemoryHeap->get_phy_addr_from_ion((unsigned long *)&pLayer->pa.addr_y, &temp_size);
        if(ret) {
            ALOGE("[%d] get_phy_addr_from_ion failed!\n",__LINE__);
            return -1;
        }
    } else { // iova
        printf("%s, %d\n", __func__, __LINE__);
        ALOGI(LOG_INFO,"[%d] alloc virt buffer \n",__LINE__);
        pLayer->MemoryHeap = new MemoryHeapIon("/dev/ion", pLayer->size_all, NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
#if 0
        if((pLayer->MemoryHeap->get_gsp_iova(&mmu_addr, &pLayer->buffersize) == 0) && (mmu_addr != 0) && (pLayer->buffersize > 0)) {
            pLayer->pa.addr_y = mmu_addr;
            ALOGI(LOG_INFO,"[%d] map iommu addr success! %p\n",__LINE__,(void*)(long)(pLayer->pa.addr_y));
        } else {
            ALOGE("[%d] map iommu addr failed!\n",__LINE__);
            return -1;
        }
#endif
    }
    pLayer->pa.addr_uv = pLayer->pa.addr_y + pLayer->size_y;
    pLayer->pa.addr_v = pLayer->pa.addr_uv + pLayer->size_u;


    pLayer->va.addr_y = (uint64_t)pLayer->MemoryHeap->get_virt_addr_from_ion();
    if(pLayer->va.addr_y == 0) return -1;
    pLayer->va.addr_uv = pLayer->va.addr_y + pLayer->size_y;
    pLayer->va.addr_v = pLayer->va.addr_uv + pLayer->size_u;
    memset_gsp((void*)pLayer->va.addr_y,0,pLayer->size_all);

    if(pLayer->need_copy == 1) {
        ALOGI(LOG_INFO,"[%d] alloc cpy buffer \n",__LINE__);
        if(pLayer->addr_type_cpy == 0) { //physical buffer
            ALOGI(LOG_INFO,"[%d] alloc cpy phy buffer \n",__LINE__);
            pLayer->MemoryHeap_cpy = new MemoryHeapIon("/dev/ion", pLayer->size_all, NO_CACHING, ION_HEAP_ID_MASK_OVERLAY);//GRALLOC_USAGE_OVERLAY_BUFFER ION_HEAP_CARVEOUT_MASK
            pLayer->MemoryHeap_cpy->get_phy_addr_from_ion((unsigned long *)&pLayer->pa_cpy.addr_y, &temp_size);
        } else { // iova
            ALOGI(LOG_INFO,"[%d] alloc cpy virt buffer \n",__LINE__);
            pLayer->MemoryHeap_cpy = new MemoryHeapIon("/dev/ion", pLayer->size_all, NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
#if 0
            if((pLayer->MemoryHeap_cpy->get_gsp_iova(&mmu_addr, &pLayer->buffersize_cpy) == 0) && (mmu_addr != 0) && (pLayer->buffersize_cpy > 0)) {
                pLayer->pa_cpy.addr_y = mmu_addr;
                ALOGI(LOG_INFO,"[%d] map cpy iommu addr success! %p\n",__LINE__,(void*)(long)(pLayer->pa.addr_y));
            } else {
                ALOGE("[%d] map cpy iommu addr failed!\n",__LINE__);
                return -1;
            }
#endif
        }
        pLayer->pa_cpy.addr_uv = pLayer->pa_cpy.addr_y + pLayer->size_y;
        pLayer->pa_cpy.addr_v = pLayer->pa_cpy.addr_uv + pLayer->size_u;

        pLayer->va_cpy.addr_y = (uint64_t)pLayer->MemoryHeap_cpy->get_virt_addr_from_ion();
        if(pLayer->va_cpy.addr_y == 0) return -1;
        pLayer->va_cpy.addr_uv = pLayer->va_cpy.addr_y + pLayer->size_y;
        pLayer->va_cpy.addr_v = pLayer->va_cpy.addr_uv + pLayer->size_u;
        memset_gsp((void*)pLayer->va_cpy.addr_y,0,pLayer->size_all);
    }
    return 0;
}


int free_buffer(GSP_LAYER_INFO_T *pLayer)
{
    //alloc none cached and buffered memory
    if(pLayer->MemoryHeap) {
        if(pLayer->addr_type == 1) { //iova buffer
            if(pLayer->pa.addr_y) {
                pLayer->MemoryHeap->free_gsp_iova(pLayer->pa.addr_y, pLayer->buffersize);
            }
        }
        delete pLayer->MemoryHeap;
        pLayer->MemoryHeap = NULL;
        pLayer->pa.addr_y = 0;
        pLayer->va.addr_y = 0;
        pLayer->buffersize = 0;
    }

    if(pLayer->need_copy == 1) {
        if(pLayer->MemoryHeap_cpy) {
            if(pLayer->addr_type_cpy == 1) { //iova buffer
                if(pLayer->pa_cpy.addr_y) {
                    pLayer->MemoryHeap_cpy->free_gsp_iova(pLayer->pa_cpy.addr_y, pLayer->buffersize_cpy);
                }
            }
            delete pLayer->MemoryHeap_cpy;
            pLayer->MemoryHeap_cpy = NULL;
            pLayer->pa_cpy.addr_y = 0;
            pLayer->va_cpy.addr_y = 0;
            pLayer->buffersize_cpy = 0;
        }
    }
    return 0;
}

int open_raw_file(GSP_LAYER_INFO_T *pLayer,const char *pFlag)
{
    if(pLayer->filename) {
        pLayer->raw_fp = fopen(pLayer->filename, pFlag);
        if (pLayer->raw_fp == NULL) {
            ALOGI(LOG_INFO,"Failed to open raw_file %s, should generate color block to replace!\n", pLayer->filename);
            return -1;
        } else {
            ALOGI(LOG_INFO,"open raw_file %s success\n", pLayer->filename);
        }
    } else {
        ALOGI(LOG_WARN,"raw file name is null\n");
    }
    return 0;
}

int read_raw_file(GSP_LAYER_INFO_T *pLayer)
{
    if(pLayer->va.addr_y != 0&& pLayer->raw_fp != NULL) {
        if (fread((void*)pLayer->va.addr_y, sizeof(unsigned char), pLayer->size_all, pLayer->raw_fp) != pLayer->size_all) {
            ALOGE("Failed to read raw_file: %s\n", pLayer->filename);
            return -1;
        } else {
            ALOGI(LOG_INFO,"read raw_file %s success\n", pLayer->filename);
            print_data(pLayer->va.addr_y,16,LOG_INFO);
        }
    }
    return 0;
}

int write_raw_file(GSP_LAYER_INFO_T *pLayer)
{
    if(pLayer->va.addr_y != 0 && pLayer->raw_fp != NULL) {
        if (fwrite((void*)pLayer->va.addr_y, sizeof(unsigned char), pLayer->size_all, pLayer->raw_fp) != pLayer->size_all) {
            ALOGE("Failed to read raw_file: %s\n", pLayer->filename);
            return -1;
        } else {
            ALOGI(LOG_INFO,"write raw_file %s success\n", pLayer->filename);
            print_data(pLayer->va.addr_y,16,LOG_INFO);
        }
    }
    return 0;
}

int close_raw_file(GSP_LAYER_INFO_T *pLayer)
{
    if (pLayer->raw_fp != NULL) {
        fclose(pLayer->raw_fp);
        pLayer->raw_fp = NULL;
    }
    return 0;
}


int dump_raw_file(GSP_LAYER_INFO_T *pLayer,GSP_CONFIG_INFO_T *gsp_cfg_info)
{
    char strname[128]= {0};
    static int cnt=0;
    FILE *raw_fp=NULL;// raw image file fp

    sprintf(strname,"/data/gsp/out/%03d_%04dx%04d_%s_%02d_%02d_%04d_%04d.raw",
            cnt, pLayer->pitch.w,pLayer->pitch.h, (pLayer->format==GSP_SRC_FMT_ARGB888)?"ARGB888":"YUV4202P",
            gsp_cfg_info->layer0_info.des_rect.st_x,gsp_cfg_info->layer0_info.des_rect.st_y,
            gsp_cfg_info->layer0_info.des_rect.rect_w,gsp_cfg_info->layer0_info.des_rect.rect_h);

    raw_fp = fopen(strname, "wb");
    if(raw_fp) {
        if(fwrite((void*)pLayer->va.addr_y, sizeof(unsigned char), pLayer->size_all, raw_fp) != pLayer->size_all) {
            ALOGE("Failed to write raw_file: %s\n", strname);
        } else {
            ALOGI(LOG_INFO,"write raw_file %s success\n", strname);
        }
        fclose(raw_fp);
        raw_fp = NULL;
    } else {
        ALOGE("Failed to open raw_file %s\n", strname);
    }
    cnt++;
    return 0;
}


int set_gsp_cfg_info(GSP_CONFIG_INFO_T *pgsp_cfg_info,
                     GSP_LAYER_INFO_T *pLayer0,
                     GSP_LAYER_INFO_T *pLayer1,
                     GSP_LAYER_INFO_T *pLayerd,
                     GSP_MISC_INFO_T *pMisc)
{
    if(pLayer0->size_all > 0) {
#ifndef GSP_BUFFER_FD
        pgsp_cfg_info->layer0_info.src_addr.addr_y = pLayer0->pa.addr_y;
        pgsp_cfg_info->layer0_info.src_addr.addr_v =
            pgsp_cfg_info->layer0_info.src_addr.addr_uv = pLayer0->pa.addr_y+pLayer0->pitch.w*pLayer0->pitch.h;
#else
        pgsp_cfg_info->layer0_info.mem_info.is_pa = !pLayer0->addr_type;
        pgsp_cfg_info->layer0_info.mem_info.share_fd = (int)pLayer0->MemoryHeap->mIonBufferFd;
        pgsp_cfg_info->layer0_info.mem_info.uv_offset = pLayer0->pitch.w*pLayer0->pitch.h;
        pgsp_cfg_info->layer0_info.mem_info.v_offset = pgsp_cfg_info->layer0_info.mem_info.uv_offset;
#endif
        pgsp_cfg_info->layer0_info.img_format = (GSP_LAYER_SRC_DATA_FMT_E)pLayer0->format;
        pgsp_cfg_info->layer0_info.pmargb_mod = pLayer0->pm_mod;
        pgsp_cfg_info->layer1_info.pmargb_mod = pLayer0->pm_mod;
        pgsp_cfg_info->layer0_info.pmargb_en = pLayer0->pm_en;
        pgsp_cfg_info->layer0_info.clip_rect.st_x = pLayer0->clip_start.x;
        pgsp_cfg_info->layer0_info.clip_rect.st_y = pLayer0->clip_start.y;
        pgsp_cfg_info->layer0_info.clip_rect.rect_w = pLayer0->clip_size.w;
        pgsp_cfg_info->layer0_info.clip_rect.rect_h = pLayer0->clip_size.h;
        pgsp_cfg_info->layer0_info.rot_angle = (GSP_ROT_ANGLE_E)pLayer0->rotation;
        pgsp_cfg_info->layer0_info.des_rect.st_x = pLayer0->out_start.x;
        pgsp_cfg_info->layer0_info.des_rect.st_y = pLayer0->out_start.y;
        pgsp_cfg_info->layer0_info.des_rect.rect_w = pLayer0->out_size.w;
        pgsp_cfg_info->layer0_info.des_rect.rect_h = pLayer0->out_size.h;
        pgsp_cfg_info->layer0_info.pitch = pLayer0->pitch.w;
        pgsp_cfg_info->layer0_info.alpha = (pLayer0->alpha==0)?0xff:pLayer0->alpha;
        pgsp_cfg_info->layer0_info.layer_en = 1;
        pgsp_cfg_info->layer0_info.row_tap_mode = pLayer0->tap_row;
        pgsp_cfg_info->layer0_info.col_tap_mode = pLayer0->tap_col;
        if(pgsp_cfg_info->layer0_info.img_format >= GSP_SRC_FMT_YUV420_2P
           && pgsp_cfg_info->layer0_info.img_format <= GSP_SRC_FMT_YUV422_2P) {
            pgsp_cfg_info->misc_info.y2r_opt = pLayer0->reduce;
        }

        if(pLayer0->filename == NULL || pLayer0->pallet) {
            pgsp_cfg_info->layer0_info.pallet_en = 1;
            pgsp_cfg_info->layer0_info.grey.a_val = 255;
            pgsp_cfg_info->layer0_info.grey.r_val = 255;
            pgsp_cfg_info->layer0_info.grey.g_val = 0;
            pgsp_cfg_info->layer0_info.grey.b_val = 0;
            //pgsp_cfg_info->layer0_info.alpha = 255;
        }
    }

    if(pLayer1->size_all > 0) {
#ifndef GSP_BUFFER_FD
        pgsp_cfg_info->layer1_info.src_addr.addr_y = pLayer1->pa.addr_y;
        pgsp_cfg_info->layer1_info.src_addr.addr_v =
            pgsp_cfg_info->layer1_info.src_addr.addr_uv = pLayer1->pa.addr_y+pLayer1->pitch.w*pLayer1->pitch.h;
#else
        pgsp_cfg_info->layer1_info.mem_info.is_pa = !pLayer1->addr_type;
        pgsp_cfg_info->layer1_info.mem_info.share_fd = (int)pLayer1->MemoryHeap->mIonBufferFd;
        pgsp_cfg_info->layer1_info.mem_info.uv_offset = pLayer1->pitch.w*pLayer1->pitch.h;
        pgsp_cfg_info->layer1_info.mem_info.v_offset = pgsp_cfg_info->layer1_info.mem_info.uv_offset;
#endif
        pgsp_cfg_info->layer1_info.img_format = (GSP_LAYER_SRC_DATA_FMT_E)pLayer1->format;
        pgsp_cfg_info->layer1_info.pmargb_mod = pLayer1->pm_mod;
        pgsp_cfg_info->layer1_info.pmargb_en = pLayer1->pm_en;
        pgsp_cfg_info->layer1_info.clip_rect.st_x = pLayer1->clip_start.x;
        pgsp_cfg_info->layer1_info.clip_rect.st_y = pLayer1->clip_start.y;
        pgsp_cfg_info->layer1_info.clip_rect.rect_w = pLayer1->clip_size.w;
        pgsp_cfg_info->layer1_info.clip_rect.rect_h = pLayer1->clip_size.h;
        pgsp_cfg_info->layer1_info.rot_angle = (GSP_ROT_ANGLE_E)pLayer1->rotation;
        pgsp_cfg_info->layer1_info.des_pos.pos_pt_x = pLayer1->out_start.x;
        pgsp_cfg_info->layer1_info.des_pos.pos_pt_y = pLayer1->out_start.y;
        pgsp_cfg_info->layer1_info.pitch = pLayer1->pitch.w;
        pgsp_cfg_info->layer1_info.alpha = (pLayer1->alpha==0)?0xff:pLayer1->alpha;
        pgsp_cfg_info->layer1_info.layer_en = pLayer1->size_all?1:0;
        if(pgsp_cfg_info->layer1_info.img_format >= GSP_SRC_FMT_YUV420_2P
           && pgsp_cfg_info->layer1_info.img_format <= GSP_SRC_FMT_YUV422_2P) {
            pgsp_cfg_info->misc_info.y2r_opt = pLayer1->reduce;
        }

        if(pLayer1->filename == NULL || pLayer1->pallet) {
            pgsp_cfg_info->layer1_info.pallet_en = 1;
            pgsp_cfg_info->layer1_info.grey.a_val = 0;
            pgsp_cfg_info->layer1_info.grey.r_val = 0;
            pgsp_cfg_info->layer1_info.grey.g_val = 0;
            pgsp_cfg_info->layer1_info.grey.b_val = 0;
            pgsp_cfg_info->layer1_info.src_addr.addr_y = pgsp_cfg_info->layer0_info.src_addr.addr_y;
            pgsp_cfg_info->layer1_info.src_addr.addr_v = pgsp_cfg_info->layer0_info.src_addr.addr_uv;
            pgsp_cfg_info->layer1_info.src_addr.addr_uv = pgsp_cfg_info->layer0_info.src_addr.addr_v;
        }
    }

    if(pMisc->scl_rand) {
        uint16_t scl_min = 4;
        uint16_t scl_max = 4;

        pgsp_cfg_info->layer0_info.clip_rect.st_x = (((pLayer0->pitch.w/3)>>1)<<1);
        pgsp_cfg_info->layer0_info.clip_rect.st_y = (((pLayer0->pitch.h*2/5)>>1)<<1);
        pgsp_cfg_info->layer0_info.clip_rect.rect_w = 320;
        pgsp_cfg_info->layer0_info.clip_rect.rect_h = 320;
        pgsp_cfg_info->layer0_info.rot_angle = (GSP_ROT_ANGLE_E)(random()%GSP_ROT_ANGLE_MAX_NUM);
        pgsp_cfg_info->layer0_info.des_rect.st_x = rand()%32;
        pgsp_cfg_info->layer0_info.des_rect.st_y = rand()%32;
        scl_min = (pgsp_cfg_info->layer0_info.clip_rect.rect_w>>4);
        scl_max = (pgsp_cfg_info->layer0_info.clip_rect.rect_w<<2);
        pgsp_cfg_info->layer0_info.des_rect.rect_w = scl_min + (rand()%(scl_max-scl_min));
        if(pgsp_cfg_info->layer0_info.clip_rect.rect_w < pgsp_cfg_info->layer0_info.des_rect.rect_w) { //scale up
            scl_min = pgsp_cfg_info->layer0_info.clip_rect.rect_h;
            scl_max = (pgsp_cfg_info->layer0_info.clip_rect.rect_h<<2);
        } else {
            scl_min = (pgsp_cfg_info->layer0_info.clip_rect.rect_h>>4);
            scl_max = pgsp_cfg_info->layer0_info.clip_rect.rect_h;
        }
        pgsp_cfg_info->layer0_info.des_rect.rect_h = scl_min + (rand()%(scl_max-scl_min));


        pgsp_cfg_info->layer1_info.clip_rect.st_x = 0;
        pgsp_cfg_info->layer1_info.clip_rect.st_y = 0;
        pgsp_cfg_info->layer1_info.clip_rect.rect_w = pLayerd->pitch.w;
        pgsp_cfg_info->layer1_info.clip_rect.rect_h = pLayerd->pitch.h;
        pgsp_cfg_info->layer1_info.pitch = pLayerd->pitch.w;
        pgsp_cfg_info->layer1_info.rot_angle = (GSP_ROT_ANGLE_E)0;
        pgsp_cfg_info->layer1_info.des_pos.pos_pt_x = 0;
        pgsp_cfg_info->layer1_info.des_pos.pos_pt_y = 0;
        pgsp_cfg_info->layer1_info.alpha = 0;
        pgsp_cfg_info->layer1_info.layer_en = 1;
        pgsp_cfg_info->layer1_info.grey.a_val = 0;
        pgsp_cfg_info->layer1_info.grey.r_val = 0;
        pgsp_cfg_info->layer1_info.grey.g_val = 0;
        pgsp_cfg_info->layer1_info.grey.b_val = 0;
        pgsp_cfg_info->layer1_info.src_addr.addr_y = pgsp_cfg_info->layer0_info.src_addr.addr_y;
        pgsp_cfg_info->layer1_info.src_addr.addr_v = pgsp_cfg_info->layer0_info.src_addr.addr_uv;
        pgsp_cfg_info->layer1_info.src_addr.addr_uv = pgsp_cfg_info->layer0_info.src_addr.addr_v;
    }

    if(pLayer0->size_all > 0 || pLayer1->size_all > 0) {
        pgsp_cfg_info->layer_des_info.img_format = (GSP_LAYER_DST_DATA_FMT_E)pLayerd->format;//GSP_DST_FMT_YUV420_2P;
        pgsp_cfg_info->layer_des_info.pitch = pLayerd->pitch.w;
#ifndef GSP_BUFFER_FD
        pgsp_cfg_info->layer_des_info.src_addr.addr_y = pLayerd->pa.addr_y;
        pgsp_cfg_info->layer_des_info.src_addr.addr_v =
            pgsp_cfg_info->layer_des_info.src_addr.addr_uv = pLayerd->pa.addr_y+pLayerd->pitch.h*pLayerd->pitch.w;
#else
        //pgsp_cfg_info->layer_des_info.mem_info.is_pa = !pLayerd->addr_type;
        pgsp_cfg_info->layer_des_info.mem_info.share_fd = (int)pLayerd->MemoryHeap->mIonBufferFd;
        pgsp_cfg_info->layer_des_info.mem_info.uv_offset = pLayerd->pitch.w*pLayerd->pitch.h;
        pgsp_cfg_info->layer_des_info.mem_info.v_offset = pgsp_cfg_info->layer_des_info.mem_info.uv_offset;
#endif
    }
    if(pMisc->performance_flag == 0) {
        ALOGI(LOG_INFO,"L1 {%04dx%04d[(%04d,%04d)%04dx%04d]} ==rot:%d alpha:%03d copy:%d==> [(%04d,%04d)%04dx%04d] bufferType:%d format:%d \n",
              pgsp_cfg_info->layer1_info.pitch,
              0,
              pgsp_cfg_info->layer1_info.clip_rect.st_x,
              pgsp_cfg_info->layer1_info.clip_rect.st_y,
              pgsp_cfg_info->layer1_info.clip_rect.rect_w,
              pgsp_cfg_info->layer1_info.clip_rect.rect_h,
              pgsp_cfg_info->layer1_info.rot_angle,
              pgsp_cfg_info->layer1_info.alpha,
              0,
              pgsp_cfg_info->layer1_info.des_pos.pos_pt_x,
              pgsp_cfg_info->layer1_info.des_pos.pos_pt_y,
              0,
              0,
              0,
              pgsp_cfg_info->layer1_info.img_format);

        ALOGI(LOG_INFO,"L0 {%04dx%04d[(%04d,%04d)%04dx%04d]} ==rot:%d alpha:%03d copy:%d==> [(%04d,%04d)%04dx%04d] bufferType:%d format:%d, L0_handle:%d \n",
              pgsp_cfg_info->layer0_info.pitch,
              0,
              pgsp_cfg_info->layer0_info.clip_rect.st_x,
              pgsp_cfg_info->layer0_info.clip_rect.st_y,
              pgsp_cfg_info->layer0_info.clip_rect.rect_w,
              pgsp_cfg_info->layer0_info.clip_rect.rect_h,
              pgsp_cfg_info->layer0_info.rot_angle,
              pgsp_cfg_info->layer0_info.alpha,
              0,
              pgsp_cfg_info->layer0_info.des_rect.st_x,
              pgsp_cfg_info->layer0_info.des_rect.st_y,
              pgsp_cfg_info->layer0_info.des_rect.rect_w,
              pgsp_cfg_info->layer0_info.des_rect.rect_h,
              0,
              pgsp_cfg_info->layer0_info.img_format,
              pgsp_cfg_info->layer0_info.mem_info.share_fd);

        ALOGI(LOG_INFO,"Ld {%04dx%04d} bufferType:%d format:%d, Ld_handle:%d\n",
              pgsp_cfg_info->layer_des_info.pitch,
              0,
              0,
              pgsp_cfg_info->layer_des_info.img_format,
              pgsp_cfg_info->layer_des_info.mem_info.share_fd);
    }
    return 0;
}


int64_t systemTime()
{
    struct timespec t;
    t.tv_sec = t.tv_nsec = 0;
    clock_gettime(CLOCK_MONOTONIC, &t);
    //printf("time: %ld:%ld.\n", t.tv_sec, t.tv_nsec);
    return t.tv_sec*1000000000LL + t.tv_nsec;

    //struct timeval curtime;
    //gettimeofday(&curtime, NULL);
}


int get_gsp_interface(GSP_MISC_INFO_T *misc, GSP_INTF_OPS_E ops)
{
    static hw_module_t const* pModule = NULL;
    static gsp_device_t *gsp_dev = NULL;
    if((GSP_INTF_PUT < ops || ops < GSP_INTF_GET || misc == NULL)
       || ((ops == GSP_INTF_GET && (pModule != NULL || gsp_dev != NULL))
           || (ops == GSP_INTF_PUT && (pModule == NULL || gsp_dev == NULL)))) {
        ALOGE("%s[%d] param err, return\n",__func__,__LINE__);
        return -1;
    }

    if(ops == GSP_INTF_GET) {
        if(hw_get_module(GSP_HARDWARE_MODULE_ID, &pModule)==0) {
            ALOGI(LOG_INFO,"%s[%d] get gsp module ok\n",__func__,__LINE__);
        } else {
            ALOGE("%s[%d] get gsp module failed\n",__func__,__LINE__);
            goto exit;
        }

        pModule->methods->open(pModule,"gsp",(hw_device_t**)&gsp_dev);
        if(gsp_dev) {
            misc->gsp_process = gsp_dev->GSP_Proccess;
            misc->gsp_getCapability = gsp_dev->GSP_GetCapability;
            ALOGI(LOG_INFO,"%s[%d] gsp_process:%p, gsp_getCapability:%p\n",__func__,__LINE__, misc->gsp_process,misc->gsp_getCapability);
        } else {
            ALOGE("%s[%d] open gsp module failed\n",__func__,__LINE__);
            goto exit;
        }
    } else {
        gsp_dev->common.close(&gsp_dev->common);
        //hw_put_module(pModule);
        gsp_dev = NULL;
        pModule = NULL;
        misc->gsp_process=NULL;
    }
    return 0;

exit:
    if(gsp_dev) {
        gsp_dev->common.close(&gsp_dev->common);
        gsp_dev = NULL;
    }
    if(pModule) {
        //hw_put_module(pModule);
        pModule = NULL;
    }
    return -1;
}


