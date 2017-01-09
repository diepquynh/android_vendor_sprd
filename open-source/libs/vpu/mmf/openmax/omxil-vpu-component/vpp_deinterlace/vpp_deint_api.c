#include "vpp_deint.h"
#include "vpp_drv_interface.h"

#include <utils/Log.h>

int init_deint_reg(VPPObject *vo, DEINT_PARAMS_T *p_deint_params, uint32 frame_no)
{
    uint32 val;

    if (0 == frame_no)
    {
        val = VPP_READ_REG(VPP_CFG, "VPP cfg read");
        val |= 0x02;
        VPP_WRITE_REG(VPP_CFG, val, "enable deinterlace");

        val = VPP_READ_REG(VPP_INT_MSK, "interrupt mask");
        val |= 0x02;
        VPP_WRITE_REG(VPP_INT_MSK, val, "interrupt mask");

        VPP_WRITE_REG(AXIM_BURST_GAP, 0x0, "");
        VPP_WRITE_REG(AXIM_BURST_PAUSE, 0x0, "");

        val = VPP_READ_REG(AXIM_ENDIAN_SET, "endian set");
        val |= 0x5500;
        VPP_WRITE_REG(AXIM_ENDIAN_SET, val, "endian set");

        val = VPP_READ_REG(DEINT_PATH_CFG, "VPP cfg read");
        val = p_deint_params->threshold << 8;
        if(p_deint_params->interleave)
        {
            val &= (~0x02);
        }
        else
        {
            val |= 0x02;
        }
        VPP_WRITE_REG(DEINT_PATH_CFG, val, "deinterlace path cfg");

        if((p_deint_params->width >>12) + (p_deint_params->height >> 12) > 0)
        {
            return -1;
        }
        val = p_deint_params->width;
        val |= (p_deint_params->height << 16);
        VPP_WRITE_REG(DEINT_IMG_SIZE, val, "deinterlace img size");
    }

    val = VPP_READ_REG(DEINT_PATH_CFG, "VPP cfg read");
    if(0 == frame_no)
    {
        val |= 0x01;
    }
    else
    {
        val &= (~0x01);
    }
    VPP_WRITE_REG(DEINT_PATH_CFG, val, "deinterlace path cfg");
    //SCI_TRACE_LOW("after DEINT_PATH_CFG : 0x%x\n", val);

    return 0;
}

int write_deint_frame_addr(VPPObject *vo, unsigned long src_frame, unsigned long ref_frame, unsigned long dst_frame,  DEINT_PARAMS_T* p_deint_params)
{
    uint32 y_len = p_deint_params->y_len;
    uint32 c_len = p_deint_params->c_len;
    unsigned long src_frame_addr = src_frame;
    unsigned long ref_frame_addr = ref_frame;
    unsigned long dst_frame_addr = dst_frame;

    //SCI_TRACE_LOW("%s,  0x%x, 0x%x, 0x%x\n: y_len: %d, c_len:%d", __FUNCTION__, src_frame_addr, ref_frame_addr, dst_frame_addr, y_len, c_len);

    if (p_deint_params->interleave)
    {
        VPP_WRITE_REG(DEINT_SRC_Y_ADDR,   (src_frame_addr           ) >> 3, "");
        VPP_WRITE_REG(DEINT_SRC_UV_ADDR, (src_frame_addr + y_len) >> 3, "");
        VPP_WRITE_REG(DEINT_SRC_V_ADDR,   0,                                          "");

        VPP_WRITE_REG(DEINT_DST_Y_ADDR,   (dst_frame_addr           ) >> 3, "");
        VPP_WRITE_REG(DEINT_DST_UV_ADDR, (dst_frame_addr + y_len) >> 3, "");
        VPP_WRITE_REG(DEINT_DST_V_ADDR,   0,                                           "");

        VPP_WRITE_REG(DEINT_REF_Y_ADDR,   (ref_frame_addr           ) >> 3, "");
        VPP_WRITE_REG(DEINT_REF_UV_ADDR, (ref_frame_addr + y_len) >> 3, "");
        VPP_WRITE_REG(DEINT_REF_V_ADDR,   0,                                          "");
    }
    else
    {
        VPP_WRITE_REG(DEINT_SRC_Y_ADDR,   (src_frame_addr           ) >> 3, "");
        VPP_WRITE_REG(DEINT_SRC_UV_ADDR, (src_frame_addr + y_len) >> 3, "");
        VPP_WRITE_REG(DEINT_SRC_V_ADDR,   (src_frame_addr + y_len + c_len) >> 3, "");

        VPP_WRITE_REG(DEINT_DST_Y_ADDR,   (dst_frame_addr           ) >> 3, "");
        VPP_WRITE_REG(DEINT_DST_UV_ADDR, (dst_frame_addr + y_len) >> 3, "");
        VPP_WRITE_REG(DEINT_DST_V_ADDR,   (dst_frame_addr + y_len + c_len) >> 3, "");

        VPP_WRITE_REG(DEINT_REF_Y_ADDR,   (ref_frame_addr           ) >> 3, "");
        VPP_WRITE_REG(DEINT_REF_UV_ADDR, (ref_frame_addr + y_len) >> 3, "");
        VPP_WRITE_REG(DEINT_REF_V_ADDR,   (ref_frame_addr + y_len + c_len) >> 3, "");
    }

    return 0;
}

int32 vpp_deint_init (VPPObject *vo)
{
    int ret;

    if (NULL == vo)
    {
        return -1;
    }

    memset(vo, 0, sizeof(VPPObject));
    vo->s_vpp_fd = -1;

    ret = vpp_open_dev(vo);
    if (ret != 0) {
        SCI_TRACE_LOW("vpp_open_dev failed ret: 0x%x\n", ret);
        return -1;
    }

    return 0;
}

int32 vpp_deint_release(VPPObject *vo)
{
    vpp_close_dev(vo);

    return 0;
}

int32 vpp_deint_process(VPPObject *vo, unsigned long src_frame, unsigned long ref_frame,  unsigned long dst_frame,
                        uint32 frame_no, DEINT_PARAMS_T* p_deint_params)
{
    vpp_acquire_dev(vo);

    SCI_TRACE_LOW("vpp_deint_process: src_frame = 0x%x, ref_frame = 0x%x, dst_frame = 0x%x, frame_no = %d\n", src_frame, ref_frame, dst_frame, frame_no);

    if(init_deint_reg(vo, p_deint_params, frame_no))
    {
        SCI_TRACE_LOW("Init params error\n");
        vpp_release_dev(vo);
        return -1;
    }

    write_deint_frame_addr(vo, src_frame, ref_frame, dst_frame, p_deint_params);

    VPP_WRITE_REG(DEINT_PATH_START, 0x01, "deinterlace path start");

#if 1
    if(vpp_poll_complete(vo))
    {
        SCI_TRACE_LOW("Error, Get interrupt timeout!!!! \n");
        vpp_release_dev(vo);
        return -1;
    }
#else
    {
        uint32 cmd = VPP_READ_REG(VPP_INT_RAW, "check interrupt type");
        while ((cmd&V_BIT_1)==0)
        {
            cmd = VPP_READ_REG(VPP_INT_RAW, "check interrupt type");
        }
        SCI_TRACE_LOW("%s, %d, int_status: %0x", __FUNCTION__, __LINE__, cmd);

        VPP_WRITE_REG(VPP_INT_CLR, V_BIT_1,"VSP_INT_CLR, clear all prossible interrupt");
    }
#endif

    vpp_release_dev(vo);
    return 0;
}



