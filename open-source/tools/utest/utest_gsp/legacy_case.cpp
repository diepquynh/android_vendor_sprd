

#include "infrastructure.h"
#include "legacy_case.h"
#include "color_block_gen.h"
#include "display_raw_data.h"

static void usage()
{
    printf("usage:\n");
    printf("sprdgsptest -f0 /data/gsp/640x480_YUV420SP.raw -cf0 4 -pw0 640 -ph0 480 -ix0 0 -iy0 0 -iw0 640 -ih0 480 -rot0 1 -ox0 0 -oy0 0 -ow0 320 -oh0 480 -fd /data/gsp/out/320x480_YUV420SP.raw -cfd 4 -pwd 320 -phd 480\n");
    printf("-fx			string : Layer 0/1/d raw filename\n");
    printf("-cfx		integer: Layer 0/1 raw file format,0-ARGB888 1-XRGB888 2-ARGB565 3-RGB565 4-YUV420_2P 5-YUV420_3P 6-YUV400_1P 7-YUV422_2P\n");
    printf("-cfd		integer: Layer d raw file format,0-ARGB888 1-XRGB888 2-ARGB565 3-RGB565 4-YUV420_2P 5-YUV420_3P 6-YUV422_2P\n");
    printf("-pwx		integer: Layer 0/1/d raw file width\n");
    printf("-phx		integer: Layer 0/1/d raw file height\n");
    printf("-ixx		integer: Layer 0/1 clip region start point x\n");
    printf("-iyx		integer: Layer 0/1 clip region start point y\n");
    printf("-oxx		integer: Layer 0/1 out region start point x\n");
    printf("-oyx		integer: Layer 0/1 out region start point y\n");
    printf("-iwx		integer: Layer 0/1 clip region width\n");
    printf("-ihx		integer: Layer 0/1 clip region height\n");
    printf("-ow0		integer: Layer 0 out region width\n");
    printf("-oh0		integer: Layer 0 out region height\n");
    printf("-rotx		integer: Layer 0/1 rotation angle,0-0 degree,1-90 degree,2=180 degree,3-270 degree\n");
    printf("-btx		integer: Layer 0/1/d buffer type, 0-physical buffer, 1-virtual buffer\n");
    printf("-cbtx		integer: Layer 0/1 copy temp buffer type, 0-physical buffer, 1-virtual buffer\n");
    printf("-cpyx		integer: Layer 0/1 need copy flag\n");
    printf("-help			   : show this help message\n");
    printf("Built on %s %s, Written by Rico.yin(tianci.yin@spreadtrum.com)\n", __DATE__, __TIME__);
}


static void print_main_params(int argc, char **argv)
{
    int i;
    printf("argc:%d\n", argc);
    for (i=1; i<argc; i++) {
        printf("argv[%d]:%s\n", i, argv[i]);
    }
}

static int parse_main_params(int argc, char **argv,
                             GSP_LAYER_INFO_T *pLayer0,
                             GSP_LAYER_INFO_T *pLayer1,
                             GSP_LAYER_INFO_T *pLayerd,
                             GSP_MISC_INFO_T *pMisc)
{
    int i;
    memset((void*)pLayer0,0,sizeof(GSP_LAYER_INFO_T));
    memset((void*)pLayer1,0,sizeof(GSP_LAYER_INFO_T));
    memset((void*)pLayerd,0,sizeof(GSP_LAYER_INFO_T));
    memset((void*)pMisc,0,sizeof(GSP_MISC_INFO_T));
    for (i=1; i<argc; i+=2) {
        //ALOGE("%s:%s\n", argv[i], argv[1+i]);
        if (strcmp(argv[i], "-f0") == 0 && (i < argc-1)) {
            pLayer0->filename = argv[1+i];
        } else if (strcmp(argv[i], "-f1") == 0 && (i < argc-1)) {
            pLayer1->filename = argv[1+i];
        } else if (strcmp(argv[i], "-pm_mod0") == 0 && (i < argc-1)) {
            pLayer0->pm_mod = atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-pm_mod1") == 0 && (i < argc-1)) {
            pLayer1->pm_mod = atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-pm_en0") == 0 && (i < argc-1)) {
            pLayer0->pm_en = atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-pm_en1") == 0 && (i < argc-1)) {
            pLayer1->pm_en = atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-reduce0") == 0 && (i < argc-1)) {
            pLayer0->reduce = atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-reduce1") == 0 && (i < argc-1)) {
            pLayer1->reduce = atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-tapr") == 0 && (i < argc-1)) {
            pLayer0->tap_row = atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-tapc") == 0 && (i < argc-1)) {
            pLayer1->tap_col = atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-plt0") == 0 && (i < argc-1)) {
            pLayer0->pallet= atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-plt1") == 0 && (i < argc-1)) {
            pLayer1->pallet= atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-fd") == 0 && (i < argc-1)) {
            pLayerd->filename = argv[1+i];
        } else if (strcmp(argv[i], "-cf0") == 0 && (i < argc-1)) {
            pLayer0->format = atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-cf1") == 0 && (i < argc-1)) {
            pLayer1->format = atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-cfd") == 0 && (i < argc-1)) {
            pLayerd->format = atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-pw0") == 0 && (i < argc-1)) {
            pLayer0->pitch.w = atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-ph0") == 0 && (i < argc-1)) {
            pLayer0->pitch.h = atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-pw1") == 0 && (i < argc-1)) {
            pLayer1->pitch.w = atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-ph1") == 0 && (i < argc-1)) {
            pLayer1->pitch.h = atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-pwd") == 0 && (i < argc-1)) {
            pLayerd->pitch.w = atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-phd") == 0 && (i < argc-1)) {
            pLayerd->pitch.h = atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-ix0") == 0 && (i < argc-1)) {
            pLayer0->clip_start.x = atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-iy0") == 0 && (i < argc-1)) {
            pLayer0->clip_start.y = atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-ix1") == 0 && (i < argc-1)) {
            pLayer1->clip_start.x = atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-iy1") == 0 && (i < argc-1)) {
            pLayer1->clip_start.y = atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-ox0") == 0 && (i < argc-1)) {
            pLayer0->out_start.x = atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-oy0") == 0 && (i < argc-1)) {
            pLayer0->out_start.y = atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-ox1") == 0 && (i < argc-1)) {
            pLayer1->out_start.x = atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-oy1") == 0 && (i < argc-1)) {
            pLayer1->out_start.y = atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-iw0") == 0 && (i < argc-1)) {
            pLayer0->clip_size.w = atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-ih0") == 0 && (i < argc-1)) {
            pLayer0->clip_size.h = atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-iw1") == 0 && (i < argc-1)) {
            pLayer1->clip_size.w = atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-ih1") == 0 && (i < argc-1)) {
            pLayer1->clip_size.h = atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-ow0") == 0 && (i < argc-1)) {
            pLayer0->out_size.w = atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-oh0") == 0 && (i < argc-1)) {
            pLayer0->out_size.h = atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-rot0") == 0 && (i < argc-1)) {
            pLayer0->rotation = atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-rot1") == 0 && (i < argc-1)) {
            pLayer1->rotation = atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-bt0") == 0 && (i < argc-1)) { // buffer type, 0 physical buffer, 1 iova
            pLayer0->addr_type = atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-bt1") == 0 && (i < argc-1)) { // buffer type, 0 physical buffer, 1 iova
            pLayer1->addr_type = atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-btd") == 0 && (i < argc-1)) { // buffer type, 0 physical buffer, 1 iova
            pLayerd->addr_type = atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-cbt0") == 0 && (i < argc-1)) { //cpy buffer type, 0 physical buffer, 1 iova
            pLayer0->addr_type_cpy = atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-cbt1") == 0 && (i < argc-1)) { //cpy buffer type, 0 physical buffer, 1 iova
            pLayer1->addr_type_cpy = atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-cpy0") == 0 && (i < argc-1)) {
            pLayer0->need_copy = atoi(argv[1+i]);//copy first
        } else if (strcmp(argv[i], "-cpy1") == 0 && (i < argc-1)) {
            pLayer1->need_copy = atoi(argv[1+i]);//copy first
        } else if (strcmp(argv[i], "-hold") == 0 && (i < argc-1)) {
            pMisc->hold_flag = atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-perf") == 0 && (i < argc-1)) {
            pMisc->performance_flag = atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-pwr") == 0 && (i < argc-1)) {
            pMisc->power_flag = atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-map") == 0 && (i < argc-1)) {
            pMisc->map_once = atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-sclrand") == 0 && (i < argc-1)) {
            pMisc->scl_rand = atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-alpha0") == 0 && (i < argc-1)) {
            pLayer0->alpha = atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-alpha1") == 0 && (i < argc-1)) {
            pLayer1->alpha = atoi(argv[1+i]);
        } else if (strcmp(argv[i], "-help") == 0) {
            printf("%s[%d]:%s\n", __func__, __LINE__,argv[i]);
            usage();
            return -1;
        } else {
            printf("%s[%d]:%s\n", __func__, __LINE__,argv[i]);
            usage();
            return -1;
        }
    }
    return 0;
}


int gsp_cpy_process(GSP_CONFIG_INFO_T *pgsp_cfg_info,
                    GSP_LAYER_INFO_T *pLayer0,
                    GSP_LAYER_INFO_T *pLayer1,
                    GSP_MISC_INFO_T *pMisc)
{
    GSP_CONFIG_INFO_T gsp_cfg_info_cpy = *pgsp_cfg_info;
    int ret = 0;

    if(pLayer0->need_copy) {
        if(gsp_cfg_info_cpy.layer0_info.layer_en == 1) {
            gsp_cfg_info_cpy.layer1_info.layer_en = 0;
            gsp_cfg_info_cpy.layer0_info.clip_rect.rect_w = gsp_cfg_info_cpy.layer0_info.pitch;
            gsp_cfg_info_cpy.layer0_info.clip_rect.st_x = 0;
            gsp_cfg_info_cpy.layer0_info.clip_rect.rect_h += gsp_cfg_info_cpy.layer0_info.clip_rect.st_y;
            gsp_cfg_info_cpy.layer0_info.clip_rect.st_y = 0;

            gsp_cfg_info_cpy.layer_des_info.src_addr.addr_y = pLayer0->pa_cpy.addr_y;
            gsp_cfg_info_cpy.layer_des_info.src_addr.addr_uv =
                gsp_cfg_info_cpy.layer_des_info.src_addr.addr_v =
                    gsp_cfg_info_cpy.layer_des_info.src_addr.addr_y +
                    gsp_cfg_info_cpy.layer0_info.clip_rect.rect_w*gsp_cfg_info_cpy.layer0_info.clip_rect.rect_h;
            gsp_cfg_info_cpy.layer0_info.des_rect = gsp_cfg_info_cpy.layer0_info.clip_rect;
            gsp_cfg_info_cpy.layer0_info.rot_angle = GSP_ROT_ANGLE_0;
            gsp_cfg_info_cpy.layer_des_info.img_format = (GSP_LAYER_DST_DATA_FMT_E)gsp_cfg_info_cpy.layer0_info.img_format;
            memset_gsp(&gsp_cfg_info_cpy.layer0_info.endian_mode,0,sizeof(gsp_cfg_info_cpy.layer0_info.endian_mode));
            gsp_cfg_info_cpy.layer_des_info.endian_mode = gsp_cfg_info_cpy.layer0_info.endian_mode;
            gsp_cfg_info_cpy.layer_des_info.pitch = gsp_cfg_info_cpy.layer0_info.clip_rect.rect_w;
            gsp_cfg_info_cpy.misc_info.split_pages = 1;//
            //ALOGE("copy0 L:%d\n", __LINE__);
            ret = (*pMisc->gsp_process)(&gsp_cfg_info_cpy);
            if(ret == 0) {
                if(gsp_cfg_info_cpy.layer0_info.layer_en == 1) {
                    pgsp_cfg_info->layer0_info.src_addr.addr_y = gsp_cfg_info_cpy.layer_des_info.src_addr.addr_y;
                    pgsp_cfg_info->layer0_info.src_addr.addr_uv =
                        pgsp_cfg_info->layer0_info.src_addr.addr_v = gsp_cfg_info_cpy.layer_des_info.src_addr.addr_v;
                }
            } else {
                return -1;
            }
        }
    }


    gsp_cfg_info_cpy = *pgsp_cfg_info;
    if(pLayer1->need_copy) {

        if(gsp_cfg_info_cpy.layer1_info.layer_en == 1) {
            gsp_cfg_info_cpy.layer0_info.layer_en = 0;
            gsp_cfg_info_cpy.layer1_info.clip_rect.rect_w = gsp_cfg_info_cpy.layer1_info.pitch;
            gsp_cfg_info_cpy.layer1_info.clip_rect.st_x = 0;
            gsp_cfg_info_cpy.layer1_info.clip_rect.rect_h += gsp_cfg_info_cpy.layer1_info.clip_rect.st_y;
            gsp_cfg_info_cpy.layer1_info.clip_rect.st_y = 0;

            gsp_cfg_info_cpy.layer_des_info.src_addr.addr_y = pLayer1->pa_cpy.addr_y;
            gsp_cfg_info_cpy.layer_des_info.src_addr.addr_uv =
                gsp_cfg_info_cpy.layer_des_info.src_addr.addr_v =
                    gsp_cfg_info_cpy.layer_des_info.src_addr.addr_y +
                    gsp_cfg_info_cpy.layer1_info.clip_rect.rect_w*gsp_cfg_info_cpy.layer1_info.clip_rect.rect_h;
            //gsp_cfg_info_cpy.layer1_info.des_rect = gsp_cfg_info_cpy.layer1_info.clip_rect;
            gsp_cfg_info_cpy.layer1_info.des_pos.pos_pt_x =
                gsp_cfg_info_cpy.layer1_info.des_pos.pos_pt_y = 0;
            gsp_cfg_info_cpy.layer1_info.rot_angle = GSP_ROT_ANGLE_0;
            gsp_cfg_info_cpy.layer_des_info.img_format = (GSP_LAYER_DST_DATA_FMT_E)gsp_cfg_info_cpy.layer1_info.img_format;
            memset_gsp(&gsp_cfg_info_cpy.layer1_info.endian_mode,0,sizeof(gsp_cfg_info_cpy.layer1_info.endian_mode));
            gsp_cfg_info_cpy.layer_des_info.endian_mode = gsp_cfg_info_cpy.layer1_info.endian_mode;
            gsp_cfg_info_cpy.layer_des_info.pitch = gsp_cfg_info_cpy.layer1_info.clip_rect.rect_w;
            gsp_cfg_info_cpy.misc_info.split_pages = 1;//
            //ALOGE("copy1 L:%d\n", __LINE__);
            ret = (*pMisc->gsp_process)(&gsp_cfg_info_cpy);
            if(ret == 0) {
                if(gsp_cfg_info_cpy.layer0_info.layer_en == 1) {
                    pgsp_cfg_info->layer1_info.src_addr.addr_y = gsp_cfg_info_cpy.layer_des_info.src_addr.addr_y;
                    pgsp_cfg_info->layer1_info.src_addr.addr_uv =
                        pgsp_cfg_info->layer1_info.src_addr.addr_v = gsp_cfg_info_cpy.layer_des_info.src_addr.addr_v;
                }
            } else {
                return -1;
            }
        }
    }

    return 0;
}


/*
func:gsp_special_case
desc:the old command parameters process
*/
int gsp_special_case(FrameBufferInfo *fbInfo,GSP_PROCESS gsp_process,GSP_CAPABILITY_T *gsp_capability,int argc, char **argv)
{
    GSP_LAYER_INFO_T        layer0;
    GSP_LAYER_INFO_T        layer1;
    GSP_LAYER_INFO_T        layerd;
    GSP_MISC_INFO_T         misc ;

    int ret = 0;
    uint32_t test_cnt = 0;
    uint32_t test_cnt_max = 1;

    int64_t start_time = 0;
    int64_t end_time = 0;
    int64_t single_max = 0;//
    int64_t single_min = 1000000;//
    GSP_CONFIG_INFO_T gsp_cfg_info;
    //print_main_params();

    if (argc < 4) {
        usage();
        return -1;
    }

    if(gsp_process==NULL) {
        ALOGE("%s[%d], get gsp so interface failed !!\n",__func__,__LINE__);
        return -1;
    }


    /* parse argument */
    ret = parse_main_params( argc, argv,
                             &layer0, &layer1, &layerd, &misc);
    print_misc_params(&misc);
    print_layer_params(&layerd);
    print_layer_params(&layer0);
    print_layer_params(&layer1);
    misc.gsp_process = gsp_process;

    if((layer0.addr_type == 1 || layer1.addr_type == 1 || layerd.addr_type == 1)
       && (gsp_capability->magic != CAPABILITY_MAGIC_NUMBER || gsp_capability->buf_type_support != GSP_ADDR_TYPE_IOVIRTUAL)  ) {
        ret = -1;
    }
    /*params check*/
    if(ret || layerd.filename == NULL) {
        PRINTF_RETURN();
        return ret;
    }
#ifdef GSP_BUFFER_FD
    misc.map_once = 0;
#endif

    //calc size of each plane
    ret = calc_input_plane_size(&layer0);
    ret |= calc_input_plane_size(&layer1);
    ret |= calc_output_plane_size(&layerd);
    if(ret) {
        PRINTF_RETURN();
        return ret;
    }
    ALOGI(LOG_INFO,"cf0:%d, total_size:%d\n",layer0.format,layer0.size_all);
    ALOGI(LOG_INFO,"cf1:%d, total_size:%d\n",layer1.format,layer1.size_all);
    ALOGI(LOG_INFO,"cfd:%d, total_size:%d\n",layerd.format,layerd.size_all);
    /*size check*/
    if((layer0.size_all == 0 && layer1.size_all == 0) || (layerd.size_all == 0)) {
        PRINTF_RETURN();
        return ret;
    }


    if(misc.map_once) {
        layer0.map_once = misc.map_once;
        layer1.map_once = misc.map_once;
        layerd.map_once = misc.map_once;
    }
    if(layer0.filename != NULL && layer0.size_all > 0) {
        ALOGI(LOG_INFO,"L0 alloc_buffer \n");
        ret = alloc_buffer(&layer0);
        if(ret) {
            ALOGE("L0 alloc_buffer failed\n");
            goto free_layer_buff;
        }
    }

    if(layer1.filename != NULL && layer1.size_all > 0) {
        ALOGI(LOG_INFO,"L1 alloc_buffer \n");
        ret = alloc_buffer(&layer1);
        if(ret) {
            ALOGE("L1 alloc_buffer failed\n");
            goto free_layer_buff;
        }
    }

    ALOGI(LOG_INFO,"Ld alloc_buffer \n");
    ret = alloc_buffer(&layerd);
    if(ret) {
        ALOGE("Ld alloc_buffer failed\n");
        goto free_layer_buff;
    }

    ALOGI(LOG_INFO,"%s[%d],out alloc pa:0x%08x va:0x%08lx ,size:%d\n", __func__, __LINE__,layerd.pa.addr_y,layerd.va.addr_y,layerd.size_all);
    misc.power_flag |= misc.scl_rand;

    ret = open_raw_file(&layer0,"r");
    ret |= open_raw_file(&layer1,"r");
    ret |= open_raw_file(&layerd,"wb");
    if(ret) {
        if(!(misc.power_flag || misc.performance_flag)) {
            goto close_file;
        }
    }


    ret = read_raw_file(&layer0);
    ret |= read_raw_file(&layer1);
    if(ret) {
        if(!(misc.power_flag || misc.performance_flag)) {
            goto close_file;
        }
    }

    //test_gen_color_blocks((char*)layer0.va.addr_y,layer0.pitch.w,layer0.pitch.h,layer0.format,2);
    //add_frame_boundary(&layer0);


    if(misc.performance_flag == 1) {
        test_cnt_max = 10000;
    }
    if(misc.performance_flag) {
        test_cnt_max = 1000;
    } else if(misc.power_flag || misc.scl_rand) {
        test_cnt_max = 10000000;
    }
    start_time = systemTime()/1000;
    while(test_cnt < test_cnt_max) {
        //ALOGE("test time test_cnt %d\n", test_cnt);

        int64_t single_end = 0;//used for power test
        int64_t single_st = 0;//used for power test
        GSP_CONFIG_INFO_T gsp_cfg_info;
        if(misc.power_flag || misc.performance_flag) {
            single_st = systemTime()/1000;
        }
        memset_gsp((void*)&gsp_cfg_info,0,sizeof(gsp_cfg_info));

        //ALOGE("set_data begin!\n");
        //set_data(layerd.va.addr_y,layerd.buffersize);
        //set_data(layerd.va.addr_y,0x20);
        //ALOGE("set_data over!\n");

        set_gsp_cfg_info(&gsp_cfg_info,&layer0,&layer1,&layerd,&misc);
        gsp_cpy_process(&gsp_cfg_info,&layer0,&layer1,&misc);

        if((layer0.addr_type == 1 || layer1.addr_type == 1 || layerd.addr_type == 1)
           && gsp_capability->magic == CAPABILITY_MAGIC_NUMBER
           && gsp_capability->buf_type_support == GSP_ADDR_TYPE_IOVIRTUAL
           && gsp_capability->video_need_copy == 0) {
            gsp_cfg_info.misc_info.split_pages = 1;
        } else {
            gsp_cfg_info.misc_info.split_pages = 0;
        }
        if(misc.performance_flag==0) {
            ALOGI (LOG_INFO,"GSP bf process, split_pages:%d \n",gsp_cfg_info.misc_info.split_pages);
        }

        ret = (*gsp_process)(&gsp_cfg_info);

        if(0 == ret) {
            ALOGI (LOG_INFO,"GSP_Proccess ok\n");
            while(misc.hold_flag);
        } else {
            ALOGE("GSP_Proccess err:%d!!\n",ret);
            goto close_file;
        }

        if(misc.power_flag || misc.performance_flag) {
            int64_t calc = 0;
            single_end = systemTime()/1000;
            calc = single_end - single_st;
            single_max = (single_max<calc)?calc:single_max;
            single_min =  (single_min<calc)?single_min:calc;
            if(calc < 30000 && misc.power_flag) {
                usleep(30000-calc);
                //sleep(2);
            }
            if(misc.scl_rand) {
                ret = dump_raw_file(&layerd,&gsp_cfg_info);
            }
        }
        test_cnt++;
    }
    /*
    sleep(2);
    usleep(520);
    */
    if(misc.performance_flag) {
        int64_t calc = 0;
        end_time = systemTime()/1000;
        calc = end_time - start_time;
        calc /= test_cnt_max;
        ALOGE("GSP start:%lld, end:%lld,max:%lld,min:%lld,avg:%lld us !!\n",start_time,end_time,single_max,single_min,calc);
    }

    ALOGI(LOG_INFO,"%s %s[%d]%s %s,write %s params: addr:0x%08lx size:%d\n", __FILE__, __func__, __LINE__, __DATE__, __TIME__,
          layerd.filename,layerd.va.addr_y,(layerd.size_y+layerd.size_u+layerd.size_v));
    ret = write_raw_file(&layerd);
    if(ret) {
        goto close_file;
    }

#ifdef SHOW_GSP_OUTPUT
    display_raw_file(&layerd,&misc,&gsp_cfg_info,fbInfo);
#else
    fbInfo;//rm warning
#endif


close_file:
    close_raw_file(&layer0);
    close_raw_file(&layer1);
    close_raw_file(&layerd);

free_layer_buff:
    free_buffer(&layer0);
    free_buffer(&layer1);
    free_buffer(&layerd);
    get_gsp_interface(&misc, GSP_INTF_PUT);
    return 0;
}



