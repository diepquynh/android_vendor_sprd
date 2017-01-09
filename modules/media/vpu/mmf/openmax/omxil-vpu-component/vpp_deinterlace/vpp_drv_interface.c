/******************************************************************************
 ** File Name:    vpp_drv.c
 *
 ** Author:       Duan Xue                                                  *
 ** DATE:         11/24/2014                                                  *
 ** Copyright:    2014 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE             NAME            DESCRIPTION                                 *
 ** 11/24/2014    Duan Xue      Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "vpp_drv_interface.h"

/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

int32 vpp_open_dev(VPPObject *vo)
{
    int ret =0;

    SCI_TRACE_LOW("vpp_open_dev s_vpp_fd = %d\n", vo->s_vpp_fd);

    if (-1 == vo->s_vpp_fd)
    {
        SCI_TRACE_LOW("open SPRD_VPP_DRIVER \n ");
        if((vo->s_vpp_fd = open(SPRD_VPP_DRIVER, O_RDWR)) < 0)
        {
            SCI_TRACE_LOW("open SPRD_VSP_DRIVER ERR: s_vpp_fd = %d \n", vo->s_vpp_fd);
            return -1;
        } else
        {
            vo->s_vpp_Vaddr_base = (unsigned long)mmap(NULL,SPRD_VPP_MAP_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED, vo->s_vpp_fd, 0);
        }
    }

    SCI_TRACE_LOW("vpp_open_dev s_vpp_fd = %d, s_vpp_Vaddr_base = 0x%x\n", vo->s_vpp_fd, vo->s_vpp_Vaddr_base);

    ret = (*(volatile uint32 *)(vo->s_vpp_Vaddr_base));
    SCI_TRACE_LOW("ret = %d\n", ret);

    return 0;
}

int32 vpp_close_dev(VPPObject *vo)
{
    if(vo->s_vpp_fd > 0)
    {
        if (munmap((void *)vo->s_vpp_Vaddr_base, SPRD_VPP_MAP_SIZE))
        {
            //SCI_TRACE_LOW("%s, %d, %d", __FUNCTION__, __LINE__, errno);
            return -1;
        }

        close(vo->s_vpp_fd);
    }
    else
    {
        SCI_TRACE_LOW ("%s, error", __FUNCTION__);
        return -1;
    }

    return 0;
}

int32 vpp_allocate_dma_memory(VPPObject *vo, VPP_DRV_BUFFER_T *vb)
{
    VPP_DRV_BUFFER_T vdb;

    memset(&vdb, 0x00, sizeof(VPP_DRV_BUFFER_T));

    vdb.size = vb->size;

    if (ioctl(vo->s_vpp_fd, SPRD_VPP_ALLOCATE_PHYSICAL_MEMORY, &vdb) < 0)
    {
        SCI_TRACE_LOW("fail to vpp_allocate_dma_memory size=%d\n", vdb.size);
        return -1;
    }

    //map to virtual address
    vdb.virt_addr = (unsigned long)mmap(NULL, vdb.size, PROT_READ | PROT_WRITE,
                                        MAP_SHARED, vo->s_vpp_fd, vdb.phys_addr);
    if ((void *)vdb.virt_addr == MAP_FAILED)
    {
        return -1;
    }

    vb->virt_addr = vdb.virt_addr;
    vb->phys_addr = (unsigned long)vdb.phys_addr;
    vb->base = (unsigned long)vdb.base;

    SCI_TRACE_LOW("vpp_allocate_dma_memory, physaddr=0x%x, virtaddr=0x%x, size=%d\n", (int)vb->phys_addr, (int)vb->virt_addr, vb->size);
    return 0;
}

void vpp_free_dma_memory(VPPObject *vo, VPP_DRV_BUFFER_T *vb)
{
    if (vb->size == 0)
        return ;

    SCI_TRACE_LOW("base: 0x%x, phys_addr: 0x%x, size: 0x%x, virt_addr: 0x%x\n", vb->base, vb->phys_addr, vb->size, vb->virt_addr);

    ioctl(vo->s_vpp_fd, SPRD_VPP_FREE_PHYSICAL_MEMORY, vb);

    if (munmap((void *)vb->virt_addr, vb->size) != 0)
    {
        SCI_TRACE_LOW("fail to vpp_free_dma_memory virtial address = 0x%x\n", (int)vb->virt_addr);
    }

    memset(vb, 0, sizeof(VPP_DRV_BUFFER_T));

    return;
}

#define MAX_POLL_CNT    1
int32 vpp_poll_complete(VPPObject *vo)
{
    if(vo->s_vpp_fd > 0)
    {
        int32 ret;
        int32 cnt = 0;

        do
        {
            ioctl(vo->s_vpp_fd, SPRD_VPP_DEINT_COMPLETE, &ret);
            cnt++;
            SCI_TRACE_LOW ("%s, ret = 0x%x \n", __FUNCTION__, ret);
        } while ((ret < 0) &&  (cnt < MAX_POLL_CNT));

        if( ret < 0)
        {
            SCI_TRACE_LOW("%s, %d, can not get interrupt \n", __FUNCTION__, __LINE__);
            return -1;
        }

        return 0;
    } else
    {
        SCI_TRACE_LOW ("%s, error", __FUNCTION__);
        return -1;
    }
}

int32 vpp_acquire_dev(VPPObject *vo)
{
    int32 ret ;

    if(vo->s_vpp_fd <  0)
    {
        SCI_TRACE_LOW("%s: failed :fd <  0 \n", __FUNCTION__);
        return -1;
    }

    ret = ioctl(vo->s_vpp_fd, SPRD_VPP_DEINT_ACQUIRE, NULL);
    if(ret)
    {
        SCI_TRACE_LOW("%s:  failed,  %d\n",__FUNCTION__, ret);
        return -1;
    }

#if 0
    ret = ioctl(vo->s_vpp_fd, VPP_RESET, NULL);
    if (ret < 0)
    {
        SCI_TRACE_LOW("%s: VPP_RESET failed %d\n",__FUNCTION__, ret);
        return -1;
    }
#endif

    SCI_TRACE_LOW("%s, %d: acquire end %d\n",__FUNCTION__, __LINE__, ret);

    return 0;
}

int32 vpp_release_dev(VPPObject *vo)
{
    int ret = 0;

    if(vo->s_vpp_fd > 0)
    {
#if 0
        if (VPP_READ_REG_POLL(AXIM_STS, V_BIT_1|V_BIT_2, 0x0, TIME_OUT_CLK_FRAME, "Polling AXIM_STS: not Axim_wch/rch_busy"))
        {
            SCI_TRACE_LOW("%s, %d, Axim_wch/rch_busy", __FUNCTION__, __LINE__);
        }

        ret = ioctl(vo->s_vpp_fd, VPP_RESET, NULL);
        if (ret < 0)
        {
            SCI_TRACE_LOW("%s: VPP_RESET failed %d\n",__FUNCTION__, ret);
            ret = -1;
            //goto RELEASE_END;
        }
#endif

        ret = ioctl(vo->s_vpp_fd, SPRD_VPP_DEINT_RELEASE, NULL);
        if(ret < 0)
        {
            SCI_TRACE_LOW("%s: VSP_RELEASE failed, %d\n",__FUNCTION__, ret);
            ret = -1;
            goto RELEASE_END;
        }
    } else
    {
        SCI_TRACE_LOW("%s: failed :fd <  0", __FUNCTION__);
        ret = -1;
    }

RELEASE_END:

    if (vo->error_flag || (ret < 0))
    {
        usleep(20*1000);
    }

    SCI_TRACE_LOW("%s, %d, ret: %d \n", __FUNCTION__, __LINE__, ret);

    return ret;
}

int32 vpp_read_reg_poll(VPPObject *vo, uint32 reg_addr, uint32 msk_data,uint32 exp_value, uint32 time, char *pstring)
{
    uint32 tmp, vsp_time_out_cnt = 0;

    tmp=(*((volatile uint32*)(reg_addr+((VPPObject *)vo)->s_vpp_Vaddr_base)))&msk_data;
    while((tmp != exp_value) && (vsp_time_out_cnt < time))
    {
        tmp=(*((volatile uint32*)(reg_addr+((VPPObject *)vo)->s_vpp_Vaddr_base)))&msk_data;
        vsp_time_out_cnt++;
    }

    if (vsp_time_out_cnt >= time)
    {
        //ioctl(vo->s_vpp_fd, VSP_HW_INFO, &mm_eb_reg);
        vo->error_flag |= ER_HW_ID;
        SCI_TRACE_LOW ("vsp_time_out_cnt %d \n",vsp_time_out_cnt);
        return 1;
    }

    return 0;
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End

