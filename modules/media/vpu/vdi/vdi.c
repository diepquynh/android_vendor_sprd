//------------------------------------------------------------------------------
// File: vdi.c
//
// Copyright (c) 2006, Chips & Media.  All rights reserved.
//------------------------------------------------------------------------------
#if defined(linux) || defined(__linux) || defined(ANDROID)

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef	_KERNEL_
#include <linux/delay.h>
#endif
#include <signal.h>		/* SIGIO */
#include <fcntl.h>		/* fcntl */
#include <pthread.h>
#include <sys/mman.h>		/* mmap */
#include <sys/ioctl.h>		/* fopen/fread */
#include <sys/errno.h>		/* fopen/fread */
#include <sys/types.h>
#include <sys/time.h>
#include "linux/vpu.h"
#include "vdi.h"
#include "vdi_osal.h"


typedef pthread_mutex_t	MUTEX_HANDLE;
#define VPU_DEVICE_NAME "/dev/sprd_coda7l"


#	define SUPPORT_INTERRUPT
#	define VPU_BIT_REG_SIZE	(0x4000*MAX_NUM_VPU_CORE)
#		define VDI_SRAM_BASE_ADDR	0x00000000	// if we can know the sram address in SOC directly for vdi layer. it is possible to set in vdi layer without allocation from driver
#		define VDI_SRAM_SIZE		0x20000		// FHD MAX size, 0x17D00  4K MAX size 0x34600
#	define VDI_SYSTEM_ENDIAN VDI_LITTLE_ENDIAN
#define VDI_NUM_LOCK_HANDLES				4

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
#define VPU_CORE_BASE_OFFSET 0x4000
#endif

#define USE_MALLOC_FOR_INSTANCE_POOL



typedef struct vpudrv_buffer_pool_t
{
    vpudrv_buffer_t vdb;
    int inuse;
} vpudrv_buffer_pool_t;

typedef struct  {
    unsigned long coreIdx;
    int vpu_fd;
    vpu_instance_pool_t *pvip;
    int task_num;
    int clock_state;

    vpudrv_buffer_t vdb_register;
    vpu_buffer_t vpu_common_memory;
    vpu_buffer_t vpu_instance_pool_memory;
    vpudrv_buffer_pool_t vpu_buffer_pool[MAX_VPU_BUFFER_POOL];
    int vpu_buffer_pool_count;
    void* vpu_mutex;
    void* vpu_disp_mutex;
} vdi_info_t;


static vdi_info_t s_vdi_info[MAX_NUM_VPU_CORE];

pthread_mutex_t g_vdi_init_lock = PTHREAD_MUTEX_INITIALIZER;
static int vdiInit_lock();
static void vdiInit_Unlock();

static int swap_endian(unsigned char *data, int len, int endian);
static int allocate_common_memory(unsigned long coreIdx);


int vdi_probe(unsigned long coreIdx)
{
    int ret;

    ret = vdi_init(coreIdx);
    vdi_release(coreIdx);

    return ret;
}


int vdi_init(unsigned long coreIdx)
{
    vdi_info_t *vdi;
    int i=0;

    i = i;
    if (coreIdx > MAX_NUM_VPU_CORE)
        return 0;

    vdi = &s_vdi_info[coreIdx];

    if (0 != vdiInit_lock())
    {
        VLOG(ERR, "[VDI] the thread vdiInit_lock get mutex failed. \n");
        return -1;
    }

    if (vdi->vpu_fd != -1 && vdi->vpu_fd != 0x00)
    {
        vdi->task_num++;
        VLOG(INFO, "[VDI] vdi->vpu_fd=%d, vdi->task_num=%d \n", vdi->vpu_fd, vdi->task_num);
        vdiInit_Unlock();
        return 0;
    }

    vdi->vpu_fd = open(VPU_DEVICE_NAME, O_RDWR);	// if this API supports VPU parallel processing using multi VPU. the driver should be made to open multiple times.
    if (vdi->vpu_fd < 0) {
        VLOG(ERR, "[VDI] Can't open vpu driver. permission would make the problem. try to run vdi/linux/driver/load.sh script\n");
        vdiInit_Unlock();
        return -1;
    }

    VLOG(INFO, "[VDI] vdi->vpu_fd=%d \n", vdi->vpu_fd);


    memset(&vdi->vpu_buffer_pool, 0x00, sizeof(vpudrv_buffer_pool_t)*MAX_VPU_BUFFER_POOL);

    if (!vdi_get_instance_pool(coreIdx))
    {
        VLOG(INFO, "[VDI] fail to create shared info for saving context \n");
        goto ERR_VDI_INIT;
    }

    if(!vdi->pvip->instance_pool_inited)
    {
        int* pCodecInst;

        pthread_mutexattr_t mutexattr;
        pthread_mutexattr_init(&mutexattr);
        pthread_mutexattr_setpshared(&mutexattr, PTHREAD_PROCESS_SHARED);
        pthread_mutex_init((MUTEX_HANDLE *)vdi->vpu_mutex, &mutexattr);
        pthread_mutex_init((MUTEX_HANDLE *)vdi->vpu_disp_mutex, &mutexattr);

        for( i = 0; i < MAX_NUM_INSTANCE; i++) {
            pCodecInst = (int *)vdi->pvip->codecInstPool[i];
            pCodecInst[1] = i;
            pCodecInst[0] = 0;
        }

        vdi->pvip->instance_pool_inited = 1;
    }

    vdi->vdb_register.size = VPU_BIT_REG_SIZE;
    vdi->vdb_register.virt_addr = (unsigned long)mmap(NULL, vdi->vdb_register.size, PROT_READ | PROT_WRITE, MAP_SHARED, vdi->vpu_fd, 0);
    if ((void *)vdi->vdb_register.virt_addr == MAP_FAILED)
    {
        VLOG(ERR, "[VDI] fail to map vpu registers \n");
        goto ERR_VDI_INIT;
    }

    vdi_set_clock_gate(coreIdx, 1);

    if (vdi_read_register(coreIdx, BIT_CUR_PC) == 0) // if BIT processor is not running.
    {
        for (i=0; i<64; i++)
            vdi_write_register(coreIdx, (i*4) + 0x100, 0x0);
    }

    VLOG(INFO, "[VDI] map vdb_register coreIdx=%d, virtaddr=0x%lx, size=%d\n", (int)coreIdx, vdi->vdb_register.virt_addr, (int)vdi->vdb_register.size);

    if (vdi_lock(coreIdx) < 0)
    {
        VLOG(ERR, "[VDI] fail to handle lock function\n");
        goto ERR_VDI_INIT;
    }

    if (allocate_common_memory(coreIdx) < 0)
    {
        VLOG(ERR, "[VDI] fail to get vpu common buffer from driver\n");
        goto ERR_VDI_INIT;
    }

    vdi->coreIdx = coreIdx;
    vdi->task_num++;
    vdi_unlock(coreIdx);

    vdiInit_Unlock();

    VLOG(INFO, "[VDI] success to init driver \n");
    return 0;

ERR_VDI_INIT:
    vdi->task_num++;	// add number of task for releasing the resource.
    vdi_unlock(coreIdx);
    vdiInit_Unlock();
    vdi_release(coreIdx);
    return -1;
}

int vdi_set_bit_firmware_to_pm(unsigned long coreIdx, const unsigned short *code)
{
    int i;
    vpu_bit_firmware_info_t bit_firmware_info;
    vdi_info_t *vdi;

    if (coreIdx >= MAX_NUM_VPU_CORE)
        return 0;

    vdi = &s_vdi_info[coreIdx];

    if (!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
        return 0;

    bit_firmware_info.size = sizeof(vpu_bit_firmware_info_t);
    bit_firmware_info.core_idx = coreIdx;
#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
    bit_firmware_info.reg_base_offset = (coreIdx*VPU_CORE_BASE_OFFSET);
#else
    bit_firmware_info.reg_base_offset = 0;
#endif
    for (i=0; i<512; i++)
        bit_firmware_info.bit_code[i] = code[i];

    if (write(vdi->vpu_fd, &bit_firmware_info, bit_firmware_info.size) < 0)
    {
        VLOG(ERR, "[VDI] fail to vdi_set_bit_firmware core=%d\n", bit_firmware_info.core_idx);
        return -1;
    }


    return 0;
}

int vdi_release(unsigned long coreIdx)
{
    int i;
    vpudrv_buffer_t vdb;
    vdi_info_t *vdi;

    if (coreIdx >= MAX_NUM_VPU_CORE)
        return 0;

    vdi = &s_vdi_info[coreIdx];

    if (!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
        return 0;


    if (vdi_lock(coreIdx) < 0)
    {
        VLOG(ERR, "[VDI] fail to handle lock function\n");
        return -1;
    }

    if (vdi->task_num > 1) // means that the opened instance remains
    {
        VLOG(INFO, "[VDI] vdi->vpu_fd=%d, vdi->task_num=%d \n", vdi->vpu_fd, vdi->task_num);
        vdi->task_num--;
        vdi_unlock(coreIdx);
        return 0;
    }

    if (vdi->vdb_register.virt_addr)
        munmap((void *)vdi->vdb_register.virt_addr, vdi->vdb_register.size);

    osal_memset(&vdi->vdb_register, 0x00, sizeof(vpudrv_buffer_t));
    vdb.size = 0;

    // get common memory information to free virtual address
    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_common_memory.phys_addr >= vdi->vpu_buffer_pool[i].vdb.phys_addr &&
                vdi->vpu_common_memory.phys_addr < (vdi->vpu_buffer_pool[i].vdb.phys_addr + vdi->vpu_buffer_pool[i].vdb.size))
        {
            vdi->vpu_buffer_pool[i].inuse = 0;
            vdi->vpu_buffer_pool_count--;
            vdb = vdi->vpu_buffer_pool[i].vdb;
            break;
        }
    }

    vdi_unlock(coreIdx);  /* note: need to complete all operations before release memory */

    if (vdb.size > 0) // release common memory
    {
#ifndef USE_MMU_MEMORY
        munmap((void *)vdb.virt_addr, vdb.size);
#else
        free_mem_from_mmu(&vdi->pvip->vpu_mem_pool, &vdb);
#endif

        memset(&vdi->vpu_common_memory, 0x00, sizeof(vpu_buffer_t));
    }


    if (vdi->vpu_instance_pool_memory.virt_addr)
    {
#ifdef USE_MALLOC_FOR_INSTANCE_POOL
        free((void *)vdi->vpu_instance_pool_memory.virt_addr);
        vdi->vpu_instance_pool_memory.virt_addr = 0;
#else
        munmap((void *)vdi->vpu_instance_pool_memory.virt_addr, vdi->vpu_instance_pool_memory.size);
#endif
    }


    osal_memset(&vdi->vpu_instance_pool_memory, 0x00, sizeof(vpudrv_buffer_t));


    vdi->task_num--;

    if (vdi->vpu_fd != -1 && vdi->vpu_fd != 0x00)
    {
        VLOG(INFO, "[VDI] vdi->vpu_fd=%d, vdi->task_num=%d, close driver.. \n", vdi->vpu_fd, vdi->task_num);
        close(vdi->vpu_fd);
        vdi->vpu_fd = -1;

    }

    memset(vdi, 0x00, sizeof(vdi_info_t));


    return 0;
}




int vdi_get_common_memory(unsigned long coreIdx, vpu_buffer_t *vb)
{
    vdi_info_t *vdi;

    if (coreIdx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd==0x00)
        return -1;

    osal_memcpy(vb, &vdi->vpu_common_memory, sizeof(vpu_buffer_t));

    return 0;
}


int allocate_common_memory(unsigned long coreIdx)
{
    vdi_info_t *vdi = &s_vdi_info[coreIdx];

    vpudrv_buffer_t vdb;
    int i;

    if (coreIdx >= MAX_NUM_VPU_CORE)
        return -1;

    if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd==0x00)
        return -1;

    osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));


    vdb.size = SIZE_COMMON*MAX_NUM_VPU_CORE;

#ifndef USE_MMU_MEMORY
    if (ioctl(vdi->vpu_fd, VDI_IOCTL_GET_COMMON_MEMORY, &vdb) < 0)
    {
        VLOG(ERR, "[VDI] fail to vdi_allocate_dma_memory size=%d\n", vdb.size);
        return -1;
    }

    vdb.virt_addr = (unsigned long)mmap(NULL, vdb.size, PROT_READ | PROT_WRITE, MAP_SHARED, vdi->vpu_fd, vdb.phys_addr);
    if ((void *)vdb.virt_addr == MAP_FAILED)
    {
        VLOG(ERR, "[VDI] fail to map common memory phyaddr=0x%x, size = %d\n", (int)vdb.phys_addr, (int)vdb.size);
        return -1;
    }
#else
    if (alloc_mem_from_mmu(&vdi->pvip->vpu_mem_pool, &vdb))
    {
        VLOG(ERR, "[VDI] fail to vdi_allocate_dma_memory size=%d\n", vdb.size);
        return -1;
    }
#endif

    VLOG(INFO, "[VDI] allocate_common_memory, physaddr=0x%x, virtaddr=0x%x\n", (int)vdb.phys_addr, (int)vdb.virt_addr);

    // convert os driver buffer type to vpu buffer type
#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
    vdi->pvip->vpu_common_buffer.size = SIZE_COMMON;
    vdi->pvip->vpu_common_buffer.phys_addr = (unsigned long)(vdb.phys_addr + (coreIdx*SIZE_COMMON));
    vdi->pvip->vpu_common_buffer.base = (unsigned long)(vdb.base + (coreIdx*SIZE_COMMON));
    vdi->pvip->vpu_common_buffer.virt_addr = (unsigned long)(vdb.virt_addr + (coreIdx*SIZE_COMMON));
#else
    vdi->pvip->vpu_common_buffer.size = SIZE_COMMON;
    vdi->pvip->vpu_common_buffer.phys_addr = (unsigned long)(vdb.phys_addr);
    vdi->pvip->vpu_common_buffer.base = (unsigned long)(vdb.base);
    vdi->pvip->vpu_common_buffer.virt_addr = (unsigned long)(vdb.virt_addr);
#endif
    osal_memcpy(&vdi->vpu_common_memory, &vdi->pvip->vpu_common_buffer, sizeof(vpudrv_buffer_t));

    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_buffer_pool[i].inuse == 0)
        {
            vdi->vpu_buffer_pool[i].vdb = vdb;
            vdi->vpu_buffer_pool_count++;
            vdi->vpu_buffer_pool[i].inuse = 1;
            break;
        }
    }

    VLOG(INFO, "[VDI] vdi_get_common_memory physaddr=0x%x, size=%d, virtaddr=0x%x\n", (int)vdi->vpu_common_memory.phys_addr, (int)vdi->vpu_common_memory.size, (int)vdi->vpu_common_memory.virt_addr);

    return 0;
}



vpu_instance_pool_t *vdi_get_instance_pool(unsigned long coreIdx)
{
    vdi_info_t *vdi;
    vpudrv_buffer_t vdb;

    if (coreIdx >= MAX_NUM_VPU_CORE)
        return NULL;

    vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00 )
        return NULL;

    osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));
    if (!vdi->pvip)
    {
        vdb.size = sizeof(vpu_instance_pool_t) + sizeof(MUTEX_HANDLE)*VDI_NUM_LOCK_HANDLES;
        VLOG(INFO, "[VDI] sizeof(vpu_instance_pool_t)=%d, sizeof(MUTEX_HANDLE)=%d, vdb.phys_addr=%x\n",
             sizeof(vpu_instance_pool_t), sizeof(MUTEX_HANDLE), vdb.phys_addr);

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
        vdb.size  *= MAX_NUM_VPU_CORE;
#endif

#ifdef USE_MALLOC_FOR_INSTANCE_POOL
        vdb.virt_addr = (unsigned long)osal_malloc(vdb.size);
        osal_memset((void *)vdb.virt_addr, 0x00, vdb.size);
#else
        if (ioctl(vdi->vpu_fd, VDI_IOCTL_GET_INSTANCE_POOL, &vdb) < 0)
        {
            VLOG(ERR, "[VDI] fail to allocate get instance pool physical space=%d\n", (int)vdb.size);
            return NULL;
        }

        VLOG(ERR, "[VDI] map instance pool phyaddr=0x%x, size = %d\n", (int)vdb.phys_addr, (int)vdb.size);
        vdb.virt_addr = (unsigned long)mmap(NULL, vdb.size, PROT_READ | PROT_WRITE, MAP_SHARED, vdi->vpu_fd, vdb.phys_addr);
        if ((void *)vdb.virt_addr == MAP_FAILED)
        {
            VLOG(ERR, "[VDI] fail to map instance pool phyaddr=0x%x, size = %d\n", (int)vdb.phys_addr, (int)vdb.size);
            return NULL;
        }
#endif

        osal_memcpy(&vdi->vpu_instance_pool_memory, &vdb, sizeof(vpudrv_buffer_t));

        vdi->pvip = (vpu_instance_pool_t *)(vdb.virt_addr + (coreIdx*(sizeof(vpu_instance_pool_t) + sizeof(MUTEX_HANDLE)*VDI_NUM_LOCK_HANDLES)));

        vdi->vpu_mutex = (void *)((unsigned long)vdi->pvip + sizeof(vpu_instance_pool_t));	//change the pointer of vpu_mutex to at end pointer of vpu_instance_pool_t to assign at allocated position.
        vdi->vpu_disp_mutex = (void *)((unsigned long)vdi->pvip + sizeof(vpu_instance_pool_t) + sizeof(MUTEX_HANDLE)*2);

        VLOG(INFO, "[VDI] instance pool physaddr=0x%x, virtaddr=0x%x, base=0x%x, size=%d\n", (int)vdb.phys_addr, (int)vdb.virt_addr, (int)vdb.base, (int)vdb.size);
    }

    return (vpu_instance_pool_t *)vdi->pvip;
}

int vdi_open_instance(unsigned long coreIdx, unsigned long instIdx)
{
    vdi_info_t *vdi;
    vpudrv_inst_info_t inst_info;
    int inst_num;

    if (coreIdx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
        return -1;

    inst_info.core_idx = coreIdx;
    inst_info.inst_idx = instIdx;
    if (ioctl(vdi->vpu_fd, VDI_IOCTL_OPEN_INSTANCE, &inst_info) < 0)
    {
        VLOG(ERR, "[VDI] fail to deliver open instance num instIdx=%d\n", (int)instIdx);
        return -1;
    }

    vdi->pvip->vpu_instance_num = inst_info.inst_open_count;

    return 0;
}

int vdi_close_instance(unsigned long coreIdx, unsigned long instIdx)
{
    vdi_info_t *vdi;
    vpudrv_inst_info_t inst_info;

    if (coreIdx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
        return -1;

    inst_info.core_idx = coreIdx;
    inst_info.inst_idx = instIdx;
    if (ioctl(vdi->vpu_fd, VDI_IOCTL_CLOSE_INSTANCE, &inst_info) < 0)
    {
        VLOG(ERR, "[VDI] fail to deliver open instance num instIdx=%d\n", (int)instIdx);
        return -1;
    }

    vdi->pvip->vpu_instance_num = inst_info.inst_open_count;

    return 0;
}

int vdi_get_instance_num(unsigned long coreIdx)
{
    vdi_info_t *vdi;

    if (coreIdx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[coreIdx];


    if(!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
        return -1;

    return vdi->pvip->vpu_instance_num;
}

int vdi_hw_reset(unsigned long coreIdx) // DEVICE_ADDR_SW_RESET
{
    vdi_info_t *vdi;

    if (coreIdx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
        return -1;

    return ioctl(vdi->vpu_fd, VDI_IOCTL_RESET, 0);

}

static void restore_mutex_in_dead(MUTEX_HANDLE *mutex)
{
   /* int mutex_value;

    if (!mutex)
        return;
#ifdef ANDROID
    mutex_value = mutex->value;
#else
    memcpy(&mutex_value, mutex, sizeof(mutex_value));
#endif
    if (mutex_value == (int)0xdead10cc) // destroy by device driver
    {*/
        pthread_mutexattr_t mutexattr;
        pthread_mutexattr_init(&mutexattr);
        pthread_mutexattr_setpshared(&mutexattr, PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(mutex, &mutexattr);
    //}
}

int vdi_lock(unsigned long coreIdx)
{
    vdi_info_t *vdi;
    int lock_err = 0;
    if (coreIdx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
        return -1;

/*#ifdef ANDROID
    restore_mutex_in_dead((MUTEX_HANDLE *)vdi->vpu_mutex);
    pthread_mutex_lock((MUTEX_HANDLE *)vdi->vpu_mutex);
#else*/
    while(1)
    {
        const int MUTEX_TIMEOUT = 0x7fffffff;
        int _ret, i;
        for(i = 0; (_ret = pthread_mutex_trylock((MUTEX_HANDLE *)vdi->vpu_mutex)) != 0 && i < MUTEX_TIMEOUT; i++)
        {
            if(i == 0)
                VLOG(ERR, "vdi_lock: mutex is already locked - try again\n");
#ifdef	_KERNEL_
            udelay(1*1000);
#else
            usleep(1*1000);
#endif // _KERNEL_
        }

        if(_ret == 0)
            break;

        if (_ret == EINVAL) {
            Uint32* pInt = (Uint32*)vdi->vpu_mutex;
            *pInt = 0xdead10cc;
            restore_mutex_in_dead((MUTEX_HANDLE*)vdi->vpu_mutex);
        }

        VLOG(ERR, "vdi_lock: can't get lock - force to unlock. [%d:%s]\n", _ret, strerror(_ret));
        vdi_unlock(coreIdx);
    }
//#endif /* ANDROID */

    return 0;
}

int vdi_lock_check(unsigned long coreIdx)
{
    vdi_info_t *vdi;
    int ret;

    if (coreIdx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
        return -1;

    ret = pthread_mutex_trylock((MUTEX_HANDLE *)vdi->vpu_mutex);
    if(ret == 0)
    {
        vdi_unlock(coreIdx);
        return -1;
    }
    else
    {
        return 0;
    }
}

void vdi_unlock(unsigned long coreIdx)
{
    vdi_info_t *vdi;

    if (coreIdx >= MAX_NUM_VPU_CORE)
        return;

    vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
        return;
    pthread_mutex_unlock((MUTEX_HANDLE *)vdi->vpu_mutex);
}


int vdi_disp_lock(unsigned long coreIdx)
{
    vdi_info_t *vdi = NULL;
    int lock_err = 0;
    if (coreIdx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[coreIdx];

/*#ifdef ANDROID
    restore_mutex_in_dead((MUTEX_HANDLE *)vdi->vpu_disp_mutex);
    pthread_mutex_lock((MUTEX_HANDLE *)vdi->vpu_disp_mutex);
#else*/
    while(1)
    {
        const int MUTEX_TIMEOUT = 5000;  // ms
        int _ret, i;
        for(i = 0; (_ret = pthread_mutex_trylock((MUTEX_HANDLE *)vdi->vpu_disp_mutex)) != 0 && i < MUTEX_TIMEOUT; i++)
        {
            if(i == 0)
                VLOG(ERR, "vdi_disp_lock: mutex is already locked - try again\n");
#ifdef	_KERNEL_
            udelay(1*1000);
#else
            usleep(1*1000);
#endif // _KERNEL_
        }

        if(_ret == 0)
            break;

        if (_ret == EINVAL) {
            Uint32* pInt = (Uint32*)vdi->vpu_disp_mutex;
            *pInt = 0xdead10cc;
            restore_mutex_in_dead((MUTEX_HANDLE*)vdi->vpu_disp_mutex);
        }

        VLOG(ERR, "vdi_disp_lock: can't get lock - force to unlock. [%d:%s]\n", _ret, strerror(_ret));
        vdi_disp_unlock(coreIdx);
    }
//#endif /* ANDROID */
    return 0;
}

void vdi_disp_unlock(unsigned long coreIdx)
{
    vdi_info_t *vdi;

    if (coreIdx >= MAX_NUM_VPU_CORE)
        return;

    vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
        return;
    pthread_mutex_unlock((MUTEX_HANDLE *)vdi->vpu_disp_mutex);
}

int vdi_write_register(unsigned long coreIdx, unsigned int addr, unsigned int data)
{
    vdi_info_t *vdi;
    unsigned long *reg_addr;

    if (coreIdx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
        return -1;

    reg_addr = reg_addr; // To avoid compile errors.

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
    reg_addr = (unsigned long *)(addr + vdi->vdb_register.virt_addr + (coreIdx*VPU_CORE_BASE_OFFSET));
#else
    reg_addr = (unsigned long *)(addr + vdi->vdb_register.virt_addr);
#endif
    *(volatile unsigned int *)reg_addr = data;

    return 0;
}




unsigned int vdi_read_register(unsigned long coreIdx, unsigned int addr)
{
    vdi_info_t *vdi;
    unsigned long *reg_addr;

    if (coreIdx >= MAX_NUM_VPU_CORE)
        return (unsigned int)-1;

    vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
        return (unsigned int)-1;

    reg_addr = reg_addr;

    //VLOG(ERR, "vdi_read_register addr = %x \n", addr);

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
    reg_addr = (unsigned long *)(addr + vdi->vdb_register.virt_addr + (coreIdx*VPU_CORE_BASE_OFFSET));
#else
    reg_addr = (unsigned long *)(addr + vdi->vdb_register.virt_addr);
#endif

    //VLOG(ERR, "vdi_read_register reg_addr = %x \n", reg_addr);

    return *(volatile unsigned int *)reg_addr;
}

int vdi_write_memory(unsigned long coreIdx, unsigned int addr, unsigned char *data, int len, int endian)
{
    vdi_info_t *vdi;
    vpudrv_buffer_t vdb;
    unsigned long offset = 0;
    int i;
    offset = offset;

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
    coreIdx = 0;
#endif
    if (coreIdx >= MAX_NUM_VPU_CORE)
        return -1;
    vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
        return -1;

    osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));

    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_buffer_pool[i].inuse == 1)
        {
            vdb = vdi->vpu_buffer_pool[i].vdb;
            if (addr >= vdb.phys_addr && addr < (vdb.phys_addr + vdb.size))
                break;
        }
    }

    if (!vdb.size) {
        VLOG(ERR, "address 0x%08x is not mapped address!!!\n", (int)addr);
        return -1;
    }

    offset = addr - (unsigned long)vdb.phys_addr;

    //VLOG(ERR, "vdi_write_memory, phys_addr = 0x%x, virt_addr = 0x%x, addr = 0x%x\n", vdb.phys_addr, vdb.virt_addr, addr);

    swap_endian(data, len, endian);
    osal_memcpy((void *)((unsigned long)vdb.virt_addr+offset), data, len);

    return len;
}

int vdi_read_memory(unsigned long coreIdx, unsigned int addr, unsigned char *data, int len, int endian)
{
    vdi_info_t *vdi;
    vpudrv_buffer_t vdb;
    unsigned long offset = 0;
    int i = 0;
    offset = offset;
#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
    coreIdx = 0;
#endif
    if (coreIdx >= MAX_NUM_VPU_CORE)
        return -1;
    vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
        return -1;

    osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));

    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_buffer_pool[i].inuse == 1)
        {
            vdb = vdi->vpu_buffer_pool[i].vdb;
            if (addr >= vdb.phys_addr && addr < (vdb.phys_addr + vdb.size))
                break;
        }
    }

    if (!vdb.size)
        return -1;

    offset = addr - (unsigned long)vdb.phys_addr;

    osal_memcpy(data, (const void *)((unsigned long)vdb.virt_addr+offset), len);
    swap_endian(data, len,  endian);

    return len;
}


int vdi_allocate_dma_memory(unsigned long coreIdx, vpu_buffer_t *vb)
{
    vdi_info_t *vdi;
    int i;
    vpudrv_buffer_t vdb;

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
    coreIdx = 0;
#endif

    vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
        return -1;

    osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));

    vdb.size = vb->size;

#ifndef USE_MMU_MEMORY
    if (ioctl(vdi->vpu_fd, VDI_IOCTL_ALLOCATE_PHYSICAL_MEMORY, &vdb) < 0)
#else
    if (alloc_mem_from_mmu(&vdi->pvip->vpu_mem_pool, &vdb))
#endif
    {
        VLOG(ERR, "[VDI] fail to vdi_allocate_dma_memory size=%d\n", vb->size);
        return -1;
    }

    vb->phys_addr = (unsigned long)vdb.phys_addr;
    vb->base = (unsigned long)vdb.base;

#ifndef USE_MMU_MEMORY
    //map to virtual address
    vdb.virt_addr = (unsigned long)mmap(NULL, vdb.size, PROT_READ | PROT_WRITE,
                                        MAP_SHARED, vdi->vpu_fd, vdb.phys_addr);
    if ((void *)vdb.virt_addr == MAP_FAILED)
    {
        memset(vb, 0x00, sizeof(vpu_buffer_t));
        return -1;
    }
#endif

    vb->virt_addr = vdb.virt_addr;

    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_buffer_pool[i].inuse == 0)
        {
            vdi->vpu_buffer_pool[i].vdb = vdb;
            vdi->vpu_buffer_pool_count++;
            vdi->vpu_buffer_pool[i].inuse = 1;
            break;
        }
    }

    VLOG(INFO, "[VDI] vdi_allocate_dma_memory, physaddr=0x%x, virtaddr=0x%x, size=%d\n", (int)vb->phys_addr, (int)vb->virt_addr, vb->size);
    return 0;
}


int vdi_attach_dma_memory(unsigned long coreIdx, vpu_buffer_t *vb)
{
    vdi_info_t *vdi;
    int i;

    vpudrv_buffer_t vdb;

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
    coreIdx = 0;
#endif

    vdi = &s_vdi_info[coreIdx];

    if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
        return -1;

    osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));

    vdb.size = vb->size;
    vdb.phys_addr = vb->phys_addr;
    vdb.base = vb->base;


    vdb.virt_addr = vb->virt_addr;

    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_buffer_pool[i].vdb.phys_addr == vb->phys_addr)
        {
            vdi->vpu_buffer_pool[i].vdb = vdb;
            vdi->vpu_buffer_pool[i].inuse = 1;
            break;
        }
        else
        {
            if (vdi->vpu_buffer_pool[i].inuse == 0)
            {
                vdi->vpu_buffer_pool[i].vdb = vdb;
                vdi->vpu_buffer_pool_count++;
                vdi->vpu_buffer_pool[i].inuse = 1;
                break;
            }
        }
    }


    return 0;
}

int vdi_dettach_dma_memory(unsigned long coreIdx, vpu_buffer_t *vb)
{
    vdi_info_t *vdi;
    int i;

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
    coreIdx = 0;
#endif
    if (coreIdx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[coreIdx];

    if(!vb || !vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
        return -1;

    if (vb->size == 0)
        return -1;

    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_buffer_pool[i].vdb.phys_addr == vb->phys_addr)
        {
            vdi->vpu_buffer_pool[i].inuse = 0;
            vdi->vpu_buffer_pool_count--;
            break;
        }
    }

    return 0;
}

void vdi_free_dma_memory(unsigned long coreIdx, vpu_buffer_t *vb)
{
    vdi_info_t *vdi;
    int i;
    vpudrv_buffer_t vdb;

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
    coreIdx = 0;
#endif

    if (coreIdx >= MAX_NUM_VPU_CORE)
        return;
    vdi = &s_vdi_info[coreIdx];

    if(!vb || !vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
        return ;

    if (vb->size == 0)
        return ;

    osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));

    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (vdi->vpu_buffer_pool[i].vdb.phys_addr == vb->phys_addr)
        {
            vdi->vpu_buffer_pool[i].inuse = 0;
            vdi->vpu_buffer_pool_count--;
            vdb = vdi->vpu_buffer_pool[i].vdb;
            break;
        }
    }

    if (!vdb.size)
    {
        VLOG(ERR, "[VDI] invalid buffer to free address = 0x%x\n", (int)vdb.virt_addr);
        return ;
    }

#ifndef USE_MMU_MEMORY
    ioctl(vdi->vpu_fd, VDI_IOCTL_FREE_PHYSICALMEMORY, &vdb);
    if (munmap((void *)vdb.virt_addr, vdb.size) != 0)
    {
        VLOG(ERR, "[VDI] fail to vdi_free_dma_memory virtial address = 0x%x\n", (int)vdb.virt_addr);
    }
#else
    free_mem_from_mmu(&vdi->pvip->vpu_mem_pool, &vdb);
#endif
    osal_memset(vb, 0, sizeof(vpu_buffer_t));
}

int vdi_get_sram_memory(unsigned long coreIdx, vpu_buffer_t *vb)
{
    vdi_info_t *vdi = &s_vdi_info[coreIdx];
    vpudrv_buffer_t vdb;

    if(!vb || !vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
        return -1;



    osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));

    if (VDI_SRAM_SIZE > 0)	// if we can know the sram address directly in vdi layer, we use it first for sdram address
    {
        vb->phys_addr = VDI_SRAM_BASE_ADDR+(coreIdx*VDI_SRAM_SIZE);
        vb->size = VDI_SRAM_SIZE;

        return 0;
    }

    return 0;
}


int vdi_set_clock_gate(unsigned long coreIdx, int enable)
{

    vdi_info_t *vdi = NULL;
    int ret;
    if (coreIdx >= MAX_NUM_VPU_CORE)
        return -1;
    vdi = &s_vdi_info[coreIdx];
    if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
        return -1;

    vdi->clock_state = enable;
    ret = ioctl(vdi->vpu_fd, VDI_IOCTL_SET_CLOCK_GATE, &enable);

    return ret;
}

int vdi_get_clock_gate(unsigned long coreIdx)
{
    vdi_info_t *vdi;
    int ret;

    if (coreIdx >= MAX_NUM_VPU_CORE)
        return -1;

    vdi = &s_vdi_info[coreIdx];
    if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
        return -1;

    ret = vdi->clock_state;
    return ret;
}



int vdi_wait_bus_busy(unsigned long coreIdx, int timeout, unsigned int gdi_busy_flag)
{
    Int64 elapse, cur;
    struct timeval tv;

    tv.tv_sec = 0;
    tv.tv_usec = 0;
    gettimeofday(&tv, NULL);
    elapse = tv.tv_sec*1000 + tv.tv_usec/1000;

    while(1)
    {
        if (vdi_read_register(coreIdx, gdi_busy_flag) == 0x77)
            break;

        gettimeofday(&tv, NULL);
        cur = tv.tv_sec * 1000 + tv.tv_usec / 1000;

        if ((cur - elapse) > timeout)
        {
            VLOG(ERR, "[VDI] vdi_wait_vpu_busy timeout, PC=0x%x\n", vdi_read_register(coreIdx, 0x018));
            return -1;
        }
#ifdef	_KERNEL_	//do not use in real system. use SUPPORT_INTERRUPT;
        udelay(1*1000);
#else
        usleep(1*1000);
#endif // _KERNEL_
    }
    return 0;

}


int vdi_wait_vpu_busy(unsigned long coreIdx, int timeout, unsigned int addr_bit_busy_flag)
{

    Int64 elapse, cur;
    struct timeval tv;

    tv.tv_sec = 0;
    tv.tv_usec = 0;
    gettimeofday(&tv, NULL);
    elapse = tv.tv_sec*1000 + tv.tv_usec/1000;

    while(1)
    {
        if (vdi_read_register(coreIdx, addr_bit_busy_flag) == 0)
            break;

        gettimeofday(&tv, NULL);
        cur = tv.tv_sec * 1000 + tv.tv_usec / 1000;

        if ((cur - elapse) > timeout)
        {
            VLOG(ERR, "[VDI] vdi_wait_vpu_busy timeout, PC=0x%x\n", vdi_read_register(coreIdx, 0x018));
            return -1;
        }
#ifdef	_KERNEL_	//do not use in real system. use SUPPORT_INTERRUPT;
        udelay(1*1000);
#else
        usleep(1*1000);
#endif // _KERNEL_
    }
    return 0;

}


int vdi_wait_interrupt(unsigned long coreIdx, int timeout, unsigned int addr_bit_int_reason)
{
#ifdef SUPPORT_INTERRUPT
    vdi_info_t *vdi = &s_vdi_info[coreIdx];
    int ret;
    ret = ioctl(vdi->vpu_fd, VDI_IOCTL_WAIT_INTERRUPT, timeout);
    if (ret != 0)
        ret = -1;
    return ret;
#else
    Int64 elapse, cur;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    gettimeofday(&tv, NULL);
    elapse = tv.tv_sec*1000 + tv.tv_usec/1000;

    while(1)
    {
        if (vdi_read_register(coreIdx, BIT_INT_STS))
        {
            if (vdi_read_register(coreIdx, addr_bit_int_reason))
                break;
        }

        gettimeofday(&tv, NULL);
        cur = tv.tv_sec * 1000 + tv.tv_usec / 1000;

        if ((cur - elapse) > timeout)
        {
            return -1;
        }
#ifdef	_KERNEL_	//do not use in real system. use SUPPORT_INTERRUPT;
        udelay(1*1000);
#else
        usleep(1*1000);
#endif // _KERNEL_
    }
    return 0;
#endif
}


int vdiInit_lock()
{
    pthread_mutex_lock(&g_vdi_init_lock);

    return 0;
}

void vdiInit_Unlock()
{
    pthread_mutex_unlock(&g_vdi_init_lock);
}


static int read_pinfo_buffer(int coreIdx, int addr)
{
    int ack;
    int rdata;
#define VDI_LOG_GDI_PINFO_ADDR  (0x1068)
#define VDI_LOG_GDI_PINFO_REQ   (0x1060)
#define VDI_LOG_GDI_PINFO_ACK   (0x1064)
#define VDI_LOG_GDI_PINFO_DATA  (0x106c)
    //------------------------------------------
    // read pinfo - indirect read
    // 1. set read addr     (GDI_PINFO_ADDR)
    // 2. send req          (GDI_PINFO_REQ)
    // 3. wait until ack==1 (GDI_PINFO_ACK)
    // 4. read data         (GDI_PINFO_DATA)
    //------------------------------------------
    vdi_write_register(coreIdx, VDI_LOG_GDI_PINFO_ADDR, addr);
    vdi_write_register(coreIdx, VDI_LOG_GDI_PINFO_REQ, 1);

    ack = 0;
    while (ack == 0)
    {
        ack = vdi_read_register(coreIdx, VDI_LOG_GDI_PINFO_ACK);
    }

    rdata = vdi_read_register(coreIdx, VDI_LOG_GDI_PINFO_DATA);

    //printf("[READ PINFO] ADDR[%x], DATA[%x]", addr, rdata);
    return rdata;
}


static void printf_gdi_info(int coreIdx, int num)
{
    int i;
    int bus_info_addr;
    int tmp;

    VLOG(INFO, "\n**GDI information for GDI_10\n");
#define VDI_LOG_GDI_BUS_STATUS (0x10F4)
    VLOG(INFO, "GDI_BUS_STATUS = %x\n", vdi_read_register(coreIdx, VDI_LOG_GDI_BUS_STATUS));

    for (i=0; i < num; i++)
    {
        VLOG(INFO, "index = %02d", i);
#define VDI_LOG_GDI_INFO_CONTROL 0x1400
        bus_info_addr = VDI_LOG_GDI_INFO_CONTROL + i*0x14;

        tmp = read_pinfo_buffer(coreIdx, bus_info_addr);	//TiledEn<<20 ,GdiFormat<<17,IntlvCbCr,<<16 GdiYuvBufStride
        VLOG(INFO, " control = 0x%08x", tmp);

        bus_info_addr += 4;
        tmp = read_pinfo_buffer(coreIdx, bus_info_addr);
        VLOG(INFO, " pic_size = 0x%08x", tmp);

        bus_info_addr += 4;
        tmp = read_pinfo_buffer(coreIdx, bus_info_addr);
        VLOG(INFO, " y-top = 0x%08x", tmp);

        bus_info_addr += 4;
        tmp = read_pinfo_buffer(coreIdx, bus_info_addr);
        VLOG(INFO, " cb-top = 0x%08x", tmp);


        bus_info_addr += 4;
        tmp = read_pinfo_buffer(coreIdx, bus_info_addr);
        VLOG(INFO, " cr-top = 0x%08x", tmp);
        VLOG(INFO, "\n");
    }
}


void vdi_log(unsigned long coreIdx, int cmd, int step)
{
    // BIT_RUN command
    enum {
        SEQ_INIT = 1,
        SEQ_END = 2,
        PIC_RUN = 3,
        SET_FRAME_BUF = 4,
        ENCODE_HEADER = 5,
        ENC_PARA_SET = 6,
        DEC_PARA_SET = 7,
        DEC_BUF_FLUSH = 8,
        RC_CHANGE_PARAMETER	= 9,
        VPU_SLEEP = 10,
        VPU_WAKE = 11,
        ENC_ROI_INIT = 12,
        FIRMWARE_GET = 0xf,
        VPU_RESET = 0x10,
    };

    int i;

    switch(cmd)
    {
    case SEQ_INIT:
        if (step == 1) {
            VLOG(INFO, "\n**SEQ_INIT start\n");
        }
        else if (step == 2)	{
            VLOG(INFO, "\n**SEQ_INIT timeout\n");
        }
        else {
            VLOG(INFO, "\n**SEQ_INIT end \n");
        }
        break;
    case SEQ_END:
        if (step == 1) {
            VLOG(INFO, "\n**SEQ_END start\n");
        }
        else if (step == 2) {
            VLOG(INFO, "\n**SEQ_END timeout\n");
        }
        else {
            VLOG(INFO, "\n**SEQ_END end\n");
        }
        break;
    case PIC_RUN:
        if (step == 1) {
            VLOG(INFO, "\n**PIC_RUN start\n");
        }
        else if (step == 2) {
            VLOG(INFO, "\n**PIC_RUN timeout\n");
        }
        else  {
            VLOG(INFO, "\n**PIC_RUN end\n");
        }
        break;
    case SET_FRAME_BUF:
        if (step == 1) {
            VLOG(INFO, "\n**SET_FRAME_BUF start\n");
        }
        else if (step == 2) {
            VLOG(INFO, "\n**SET_FRAME_BUF timeout\n");
        }
        else  {
            VLOG(INFO, "\n**SET_FRAME_BUF end\n");
        }
        break;
    case ENCODE_HEADER:
        if (step == 1) {
            VLOG(INFO, "\n**ENCODE_HEADER start\n");
        }
        else if (step == 2) {
            VLOG(INFO, "\n**ENCODE_HEADER timeout\n");
        }
        else  {
            VLOG(INFO, "\n**ENCODE_HEADER end\n");
        }
        break;
    case RC_CHANGE_PARAMETER:
        if (step == 1) {
            VLOG(INFO, "\n**RC_CHANGE_PARAMETER start\n");
        }
        else if (step == 2) {
            VLOG(INFO, "\n**RC_CHANGE_PARAMETER timeout\n");
        }
        else {
            VLOG(INFO, "\n**RC_CHANGE_PARAMETER end\n");
        }
        break;

    case DEC_BUF_FLUSH:
        if (step == 1) {
            VLOG(INFO, "\n**DEC_BUF_FLUSH start\n");
        }
        else if (step == 2) {
            VLOG(INFO, "\n**DEC_BUF_FLUSH timeout\n");
        }
        else {
            VLOG(INFO, "\n**DEC_BUF_FLUSH end ");
        }
        break;
    case FIRMWARE_GET:
        if (step == 1) {
            VLOG(INFO, "\n**FIRMWARE_GET start\n");
        }
        else if (step == 2)  {
            VLOG(INFO, "\n**FIRMWARE_GET timeout\n");
        }
        else {
            VLOG(INFO, "\n**FIRMWARE_GET end\n");
        }
        break;
    case VPU_RESET:
        if (step == 1) {
            VLOG(INFO, "\n**VPU_RESET start\n");
        }
        else if (step == 2) {
            VLOG(INFO, "\n**VPU_RESET timeout\n");
        }
        else  {
            VLOG(INFO, "\n**VPU_RESET end\n");
        }
        break;
    case ENC_PARA_SET:
        if (step == 1)	//
            VLOG(INFO, "\n**ENC_PARA_SET start\n");
        else if (step == 2)
            VLOG(INFO, "\n**ENC_PARA_SET timeout\n");
        else
            VLOG(INFO, "\n**ENC_PARA_SET end\n");
        break;
    case DEC_PARA_SET:
        if (step == 1)	//
            VLOG(INFO, "\n**DEC_PARA_SET start\n");
        else if (step == 2)
            VLOG(INFO, "\n**DEC_PARA_SET timeout\n");
        else
            VLOG(INFO, "\n**DEC_PARA_SET end\n");
        break;
    default:
        if (step == 1) {
            VLOG(INFO, "\n**ANY CMD start\n");
        }
        else if (step == 2) {
            VLOG(INFO, "\n**ANY CMD timeout\n");
        }
        else {
            VLOG(INFO, "\n**ANY CMD end\n");
        }
        break;
    }

    for (i=0; i<0x200; i=i+16)
    {
        VLOG(INFO, "0x%04xh: 0x%08x 0x%08x 0x%08x 0x%08x\n", i,
             vdi_read_register(coreIdx, i), vdi_read_register(coreIdx, i+4),
             vdi_read_register(coreIdx, i+8), vdi_read_register(coreIdx, i+0xc));
    }
    if ((cmd == PIC_RUN && step== 0) || cmd == VPU_RESET)
    {
#if 0


#define VDI_LOG_MBC_BUSY 0x0444
#define VDI_LOG_MC_BASE	 0x0C00
#define VDI_LOG_MC_BUSY	 0x0C04


#define VDI_LOG_ROT_SRC_IDX	 (0x400 + 0x10C)
#define VDI_LOG_ROT_DST_IDX	 (0x400 + 0x110)
        VLOG(INFO, "MBC_BUSY = %x\n", vdi_read_register(coreIdx, VDI_LOG_MBC_BUSY));
        VLOG(INFO, "MC_BUSY = %x\n", vdi_read_register(coreIdx, VDI_LOG_MC_BUSY));
        VLOG(INFO, "MC_MB_XY_DONE=(y:%d, x:%d)\n", (vdi_read_register(coreIdx, VDI_LOG_MC_BASE) >> 20) & 0x3F, (vdi_read_register(coreIdx, VDI_LOG_MC_BASE) >> 26) & 0x3F);


        VLOG(INFO, "ROT_SRC_IDX = %x\n", vdi_read_register(coreIdx, VDI_LOG_ROT_SRC_IDX));
        VLOG(INFO, "ROT_DST_IDX = %x\n", vdi_read_register(coreIdx, VDI_LOG_ROT_DST_IDX));

        VLOG(INFO, "P_MC_PIC_INDEX_0 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x200));
        VLOG(INFO, "P_MC_PIC_INDEX_1 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x20c));
        VLOG(INFO, "P_MC_PIC_INDEX_2 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x218));
        VLOG(INFO, "P_MC_PIC_INDEX_3 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x230));
        VLOG(INFO, "P_MC_PIC_INDEX_3 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x23C));
        VLOG(INFO, "P_MC_PIC_INDEX_4 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x248));
        VLOG(INFO, "P_MC_PIC_INDEX_5 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x254));
        VLOG(INFO, "P_MC_PIC_INDEX_6 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x260));
        VLOG(INFO, "P_MC_PIC_INDEX_7 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x26C));
        VLOG(INFO, "P_MC_PIC_INDEX_8 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x278));
        VLOG(INFO, "P_MC_PIC_INDEX_9 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x284));
        VLOG(INFO, "P_MC_PIC_INDEX_a = %x\n", vdi_read_register(coreIdx, MC_BASE+0x290));
        VLOG(INFO, "P_MC_PIC_INDEX_b = %x\n", vdi_read_register(coreIdx, MC_BASE+0x29C));
        VLOG(INFO, "P_MC_PIC_INDEX_c = %x\n", vdi_read_register(coreIdx, MC_BASE+0x2A8));
        VLOG(INFO, "P_MC_PIC_INDEX_d = %x\n", vdi_read_register(coreIdx, MC_BASE+0x2B4));

        VLOG(INFO, "P_MC_PICIDX_0 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x028));
        VLOG(INFO, "P_MC_PICIDX_1 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x02C));
#endif

    }
}


int swap_endian(unsigned char *data, int len, int endian)
{
    unsigned int *p;
    unsigned int v1, v2, v3;
    int i;
    const int sys_endian = VDI_SYSTEM_ENDIAN;
    int swap = 0;
    p = (unsigned int *)data;

    if(endian == sys_endian)
        swap = 0;
    else
        swap = 1;

    if (swap)
    {
        if (endian == VDI_LITTLE_ENDIAN ||
                endian == VDI_BIG_ENDIAN) {
            for (i=0; i<len/4; i+=2)
            {
                v1 = p[i];
                v2  = ( v1 >> 24) & 0xFF;
                v2 |= ((v1 >> 16) & 0xFF) <<  8;
                v2 |= ((v1 >>  8) & 0xFF) << 16;
                v2 |= ((v1 >>  0) & 0xFF) << 24;
                v3 =  v2;
                v1  = p[i+1];
                v2  = ( v1 >> 24) & 0xFF;
                v2 |= ((v1 >> 16) & 0xFF) <<  8;
                v2 |= ((v1 >>  8) & 0xFF) << 16;
                v2 |= ((v1 >>  0) & 0xFF) << 24;
                p[i]   =  v2;
                p[i+1] = v3;
            }
        }
        else
        {
            int swap4byte = 0;
            if (endian == VDI_32BIT_LITTLE_ENDIAN)
            {
                if (sys_endian == VDI_BIG_ENDIAN)
                {
                    swap = 1;
                    swap4byte = 0;
                }
                else if (sys_endian == VDI_32BIT_BIG_ENDIAN)
                {
                    swap = 1;
                    swap4byte = 1;
                }
                else if (sys_endian == VDI_LITTLE_ENDIAN)
                {
                    swap = 0;
                    swap4byte = 1;
                }
            }
            else	// VDI_32BIT_BIG_ENDIAN
            {
                if (sys_endian == VDI_LITTLE_ENDIAN)
                {
                    swap = 1;
                    swap4byte = 0;
                }
                else if (sys_endian == VDI_32BIT_LITTLE_ENDIAN)
                {
                    swap = 1;
                    swap4byte = 1;
                }
                else if (sys_endian == VDI_BIG_ENDIAN)
                {
                    swap = 0;
                    swap4byte = 1;
                }
            }
            if (swap) {
                for (i=0; i<len/4; i++) {
                    v1 = p[i];
                    v2  = ( v1 >> 24) & 0xFF;
                    v2 |= ((v1 >> 16) & 0xFF) <<  8;
                    v2 |= ((v1 >>  8) & 0xFF) << 16;
                    v2 |= ((v1 >>  0) & 0xFF) << 24;
                    p[i] = v2;
                }
            }
            if (swap4byte) {
                for (i=0; i<len/4; i+=2) {
                    v1 = p[i];
                    v2 = p[i+1];
                    p[i]   = v2;
                    p[i+1] = v1;
                }
            }
        }
    }

    return swap;
}








#endif	//#if defined(linux) || defined(__linux) || defined(ANDROID)
