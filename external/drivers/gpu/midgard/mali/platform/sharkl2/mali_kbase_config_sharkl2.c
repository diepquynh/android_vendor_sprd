/*
 *
 * (C) COPYRIGHT ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * A copy of the licence is included with the program, and can also be obtained
 * from Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 */

#include <linux/ioport.h>
#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <mali_kbase_config.h>
#include <linux/regmap.h>
#ifdef CONFIG_PM_RUNTIME
#include <linux/pm_runtime.h>
#endif
#ifdef CONFIG_OF
#include <linux/of.h>
#endif
#ifdef CONFIG_MALI_HOTPLUG
#include <hotplug/mali_kbase_hotplug.h>
#endif
#define DTS_CLK_OFFSET 2
#define PM_RUNTIME_DELAY_MS 50
#define UP_THRESHOLD 9/1
typedef unsigned int UINT32 ;
#define HARD_RESET_AT_POWER_OFF 0

#ifndef CONFIG_OF
 struct kbase_io_resources io_resources = {
	.job_irq_number = 68,
	.mmu_irq_number = 69,
	.gpu_irq_number = 70,
	.io_memory_region = {
			     .start = 0x2f010000,
			     .end = 0x2f010000 + (4096 * 4) - 1}
};

#endif

DEFINE_SEMAPHORE(gpu_dfs_sem);



 int mali_platform_init(struct kbase_device *kbdev)
{

        UINT32* MODULE_EN=(UINT32*)ioremap_nocache(0x402E0000,4); //sharkl2 gpu enable
        UINT32* CLKGate =(UINT32*)ioremap_nocache(0x402D0040, 4); // sharkl2 clock gate
        UINT32* GPLL_APsys =(UINT32*)ioremap_nocache(0x402B00A8, 4); // sharkl2 GPLL selected by AP sys
        UINT32* CLK =(UINT32*)ioremap_nocache(0x60100004, 4); // sharkl2 clock set
        UINT32* Power =(UINT32*)ioremap_nocache(0x402B0020, 4); // sharkl2 power set

       #define REG_RAW_WRITE(addr,value)       (*(volatile UINT32*)(addr)=(value))
       #define REG_RAW_CLEAR(addr,bit)         (*(volatile UINT32*)(addr) &= ~(1UL<<(bit)))
       #define REG_RAW_SET(addr,bit)           (*(volatile UINT32*)(addr) |= (1UL<<(bit)))
       #define REG_RAW_OR(addr,value)          (*(volatile UINT32*)(addr) |= (value))
       #define REG_RAW_AND(addr,value)         (*(volatile UINT32*)(addr) &= (value))
	REG_RAW_SET(MODULE_EN,27);
        *(volatile UINT32*)(CLKGate) &=~(0x3f<<25);
        REG_RAW_CLEAR(CLKGate,16);
        REG_RAW_SET(GPLL_APsys,0);
	REG_RAW_CLEAR(CLK,0);
	REG_RAW_CLEAR(Power,25);
	KBASE_DEBUG_PRINT(4, "mali------------------- mali_platform_init\n");
       #if 0
	//gpu freq
	 mali_freq_init(kbdev->dev);
        //power on
	sci_glb_write(REG_PMU_APB_PD_GPU_TOP_PWR_CFG1,BIT_PMU_APB_PD_GPU_TOP_PWR_ON_DLY(1),0xFF0000);//0xE42B0054,bit 16=1
	sci_glb_write(REG_PMU_APB_PD_GPU_TOP_PWR_CFG1,BIT_PMU_APB_PD_GPU_TOP_PWR_ON_SEQ_DLY(1),0xFF00);//0xE42B0054,bit 8=1
	sci_glb_write(REG_PMU_APB_PD_GPU_TOP_PWR_CFG1,BIT_PMU_APB_PD_GPU_TOP_ISO_ON_DLY(1),0xFF);//0xE42B0054,bit 0=1
        mali_power_on();
	//clock on
	mali_clock_on();
	mali_power_on_late(kbdev);
        #endif

        #ifdef CONFIG_MALI_HOTPLUG
	kbase_hotplug_wake_up();
        #endif
        //gpu_dfs_ctx.gpu_dfs_workqueue = create_singlethread_workqueue()
	return 0;
}

 void mali_platform_term(struct kbase_device *kbdev)
{
	KBASE_DEBUG_PRINT(4, "mali------------------- mali_platform_term\n");
	/*down(gpu_dfs_ctx.sem);
	//destory work queue
	destroy_workqueue(gpu_dfs_ctx.gpu_dfs_workqueue);

	//clock off
	mali_clock_off();

	//power off
	mali_power_off();
	//free

	vfree(gpu_freq_list);
	vfree(gpu_dfs_ctx.freq_list);
	vfree(gpu_dfs_ctx.gpu_clk_src);
	up(gpu_dfs_ctx.sem);*/
}

struct kbase_platform_funcs_conf platform_funcs = {
	.platform_init_func = mali_platform_init,
	.platform_term_func = mali_platform_term
};

 void mali_power_mode_change(struct kbase_device *kbdev, int power_mode)
{
	/*if (!gpu_power_alawys_on)
	{
		down(gpu_dfs_ctx.sem);
		KBASE_DEBUG_PRINT(3, "mali_power_mode_change: %d, gpu_power_on=%d gpu_clock_on=%d\n",power_mode,gpu_dfs_ctx.gpu_power_on,gpu_dfs_ctx.gpu_clock_on);
		switch (power_mode)
		{
		case 0://power on
			if (!gpu_dfs_ctx.gpu_power_on)
			{
				mali_power_on();
				mali_clock_on();
				mali_power_on_late(kbdev);
			}

			if (!gpu_dfs_ctx.gpu_clock_on)
			{
				mali_clock_on();
			}
#ifdef CONFIG_MALI_HOTPLUG
			kbase_hotplug_wake_up();
#endif
			break;

		case 1://light sleep
		case 2://deep sleep
			if(gpu_dfs_ctx.gpu_clock_on)
			{
				mali_clock_off();
			}

			if(gpu_dfs_ctx.gpu_power_on)
			{
				mali_power_off();
			}
			break;

		default:
			break;
		}
		up(gpu_dfs_ctx.sem);
	}*/
}

 void pm_callback_power_off(struct kbase_device *kbdev)
{
#ifdef CONFIG_PM_RUNTIME
	int res;

	KBASE_DEBUG_PRINT(4, "mali------------------- pm_callback_power_off\n");
	res = pm_runtime_put_sync(kbdev->dev);
	if (res < 0)
	{
		printk(KERN_ERR "mali----pm_runtime_put_sync return (%d)\n", res);
	}
#endif
}

 int pm_callback_power_on(struct kbase_device *kbdev)
{

        UINT32* MODULE_EN =(UINT32*)ioremap_nocache(0x402E0000,4); //sharkl2 gpu enable
        UINT32* CLKGate =(UINT32*)ioremap_nocache(0x402D0040, 4); // sharkl2 clock gate
        UINT32* GPLL_APsys =(UINT32*)ioremap_nocache(0x402B00A8, 4); // sharkl2 GPLL selected by AP sys
        UINT32* CLK =(UINT32*)ioremap_nocache(0x60100004, 4); // sharkl2 clock set
        UINT32* Power =(UINT32*)ioremap_nocache(0x402B0020, 4); // sharkl2 power set
       #define REG_RAW_WRITE(addr,value)       (*(volatile UINT32*)(addr)=(value))
       #define REG_RAW_CLEAR(addr,bit)         (*(volatile UINT32*)(addr) &= ~(1UL<<(bit)))
       #define REG_RAW_SET(addr,bit)           (*(volatile UINT32*)(addr) |= (1UL<<(bit)))
       #define REG_RAW_OR(addr,value)          (*(volatile UINT32*)(addr) |= (value))
       #define REG_RAW_AND(addr,value)         (*(volatile UINT32*)(addr) &= (value))
        REG_RAW_SET(MODULE_EN,27);
	(*(volatile UINT32*)(CLKGate))&=~(0x7f<<25);// soft control reg use the 0x7f
	REG_RAW_CLEAR(CLKGate,16);
	REG_RAW_SET(GPLL_APsys,0);
        REG_RAW_CLEAR(CLK,0);
        REG_RAW_CLEAR(Power,25);//power on
       // (*(volatile UINT32*)(CLK))|=0x6;  //GPU clock 600MHZ
#ifdef CONFIG_PM_RUNTIME
	int res;
	KBASE_DEBUG_PRINT(4, "mali------------------- pm_callback_power_on\n");
	res = pm_runtime_get_sync(kbdev->dev);
	if (res < 0)
	{
		printk(KERN_ERR "mali----pm_runtime_get_sync return (%d)\n", res);
	}
#endif
	return 1;
}

 void pm_callback_power_suspend(struct kbase_device *kbdev)
{
	KBASE_DEBUG_PRINT(4, "mali------------------- pm_callback_power_suspend\n");
	mali_power_mode_change(kbdev, 2);
}

 void pm_callback_power_resume(struct kbase_device *kbdev)
{
	KBASE_DEBUG_PRINT(4, "mali------------------- pm_callback_power_resume\n");
	mali_power_mode_change(kbdev, 0);
}
 struct kbase_pm_callback_conf pm__callbacks = {
        .power_on_callback = pm_callback_power_on,
        .power_off_callback = pm_callback_power_off,
        .power_suspend_callback  = NULL,
        .power_resume_callback = NULL
};

#ifdef CONFIG_PM_RUNTIME
 int pm_callback_power_runtime_init(struct kbase_device *kbdev)
{
	KBASE_DEBUG_PRINT(4, "mali------------------- pm_callback_power_runtime_init\n");
	pm_runtime_set_active(kbdev->dev);
	pm_suspend_ignore_children(kbdev->dev, true);
	pm_runtime_set_autosuspend_delay(kbdev->dev, PM_RUNTIME_DELAY_MS);
	pm_runtime_use_autosuspend(kbdev->dev);
	pm_runtime_enable(kbdev->dev);

	return 0;
}

 void pm_callback_power_runtime_term(struct kbase_device *kbdev)
{
	KBASE_DEBUG_PRINT(4, "mali------------------- pm_callback_power_runtime_term\n");
	pm_runtime_disable(kbdev->dev);
}

 void pm_callback_power_runtime_off(struct kbase_device *kbdev)
{
	KBASE_DEBUG_PRINT(4, "mali------------------- pm_callback_power_runtime_off\n");
	mali_power_mode_change(kbdev, 1);
}

 int pm_callback_power_runtime_on(struct kbase_device *kbdev)
{
	KBASE_DEBUG_PRINT(4, "mali------------------- pm_callback_power_runtime_on\n");
	mali_power_mode_change(kbdev, 0);
	return 0;
}
#endif/*CONFIG_PM_RUNTIME*/

struct kbase_pm_callback_conf pm_callbacks = {
	.power_off_callback = pm_callback_power_off,
	.power_on_callback = pm_callback_power_on,
	.power_suspend_callback = pm_callback_power_suspend,
	.power_resume_callback = pm_callback_power_resume,
#ifdef CONFIG_PM_RUNTIME
	.power_runtime_init_callback = pm_callback_power_runtime_init,
	.power_runtime_term_callback = pm_callback_power_runtime_term,
	.power_runtime_off_callback = pm_callback_power_runtime_off,
	.power_runtime_on_callback = pm_callback_power_runtime_on
#endif
};


 struct kbase_platform_config versatile_platform_config = {
#ifndef CONFIG_OF
	.io_resources = &io_resources
#endif
};

struct kbase_platform_config *kbase_get_platform_config(void)
{
	return &versatile_platform_config;
}

int kbase_platform_early_init(void)
{
	/* Nothing needed at this stage */
	return 0;
}

/*static int freq_search(struct gpu_freq_info freq_list[], int len, int key)
{
	int low=0, high=len-1, mid;

	if (0 > key)
	{
		return -1;
	}

	while(low <= high)
	{
		mid = (low+high)/2;
		if(key == freq_list[mid].freq)
		{
			return mid;
		}

		if(key < freq_list[mid].freq)
		{
			high = mid-1;
		}
		else
		{
			low = mid+1;
		}
	}
	return -1;
}

static int gpu_change_freq(void)
{
	int result = -1;

	down(gpu_dfs_ctx.sem);
	if(gpu_dfs_ctx.gpu_power_on && gpu_dfs_ctx.gpu_clock_on)
	{
		if(gpu_dfs_ctx.freq_next != gpu_dfs_ctx.freq_cur)
		{
			if(gpu_dfs_ctx.freq_next->clk_src != gpu_dfs_ctx.freq_cur->clk_src)
			{
				clk_set_parent(gpu_dfs_ctx.gpu_clock, gpu_dfs_ctx.freq_next->clk_src);
			}
			if(gpu_dfs_ctx.freq_next->div != gpu_dfs_ctx.freq_cur->div)
			{
				mali_set_div(gpu_dfs_ctx.freq_next);
			}

			gpu_dfs_ctx.freq_cur = gpu_dfs_ctx.freq_next;
			gpu_freq_cur = gpu_dfs_ctx.freq_cur->freq;
		}
		result = 0;
	}
	up(gpu_dfs_ctx.sem);

	return (result);
}*/

#ifdef CONFIG_MALI_MIDGARD_DVFS
/*static const struct gpu_freq_info* get_next_freq(const struct gpu_freq_info* min_freq, const struct gpu_freq_info* max_freq, int target)
{
	const struct gpu_freq_info* freq;

	for (freq = min_freq; freq <= max_freq; freq++)
	{
		if (freq->up_threshold > target)
		{
			return freq;
		}
	}
	return max_freq;
}

static void gpu_dfs_func(struct work_struct *work)
{
	gpu_change_freq();
}

static DECLARE_WORK(gpu_dfs_work, &gpu_dfs_func);*/

int kbase_platform_dvfs_event(struct kbase_device *kbdev, u32 utilisation,
	u32 util_gl_share, u32 util_cl_share[2])
{
	int result = 0;/*, min_index = -1, max_index = -1;

	gpu_dfs_ctx.cur_load = utilisation;

	KBASE_DEBUG_PRINT(3, "MALI_DVFS utilisation=%d util_gl_share=%d, util_cl_share[0]=%d, util_cl_share[1]=%d \n",
		utilisation, util_gl_share, util_cl_share[0], util_cl_share[1]);
	KBASE_DEBUG_PRINT(3, "MALI_DVFS gpu_boost_level:%d \n", gpu_boost_level);

	switch(gpu_boost_level)
	{
	case 10:
		gpu_dfs_ctx.freq_max =
		gpu_dfs_ctx.freq_min = &gpu_dfs_ctx.freq_list[gpu_dfs_ctx.freq_list_len-1];
		break;

	case 9:
		gpu_dfs_ctx.freq_max =
		gpu_dfs_ctx.freq_min = gpu_dfs_ctx.freq_9;
		break;

	case 7:
		gpu_dfs_ctx.freq_max =
		gpu_dfs_ctx.freq_min = gpu_dfs_ctx.freq_7;
		break;

	case 5:
		gpu_dfs_ctx.freq_max =
		gpu_dfs_ctx.freq_min = gpu_dfs_ctx.freq_5;
		break;

	case 0:
	default:
		gpu_dfs_ctx.freq_max = gpu_dfs_ctx.freq_range_max;
		gpu_dfs_ctx.freq_min = gpu_dfs_ctx.freq_range_min;
		break;
	}

	gpu_boost_level = 0;

	//limit min freq
	min_index = freq_search(gpu_dfs_ctx.freq_list, gpu_dfs_ctx.freq_list_len, gpu_freq_min_limit);
	if ((0 <= min_index) &&
		(gpu_dfs_ctx.freq_min->freq < gpu_dfs_ctx.freq_list[min_index].freq))
	{
		gpu_dfs_ctx.freq_min = &gpu_dfs_ctx.freq_list[min_index];
		if (gpu_dfs_ctx.freq_min->freq > gpu_dfs_ctx.freq_max->freq)
		{
			gpu_dfs_ctx.freq_max = gpu_dfs_ctx.freq_min;
		}
	}

    //limit max freq
	max_index = freq_search(gpu_dfs_ctx.freq_list, gpu_dfs_ctx.freq_list_len, gpu_freq_max_limit);
	if ((0 <= max_index) &&
		(gpu_dfs_ctx.freq_max->freq > gpu_dfs_ctx.freq_list[max_index].freq))
	{
		gpu_dfs_ctx.freq_max = &gpu_dfs_ctx.freq_list[max_index];
		if (gpu_dfs_ctx.freq_max->freq < gpu_dfs_ctx.freq_min->freq)
		{
			gpu_dfs_ctx.freq_min = gpu_dfs_ctx.freq_max;
		}
	}

	// if the loading ratio is greater then 90%, switch the clock to the maximum
	if(gpu_dfs_ctx.cur_load >= (100*UP_THRESHOLD))
	{
		gpu_dfs_ctx.freq_next = gpu_dfs_ctx.freq_max;
	}
	else
	{
		int target_freq = gpu_dfs_ctx.freq_cur->freq * gpu_dfs_ctx.cur_load / 100;
		gpu_dfs_ctx.freq_next = get_next_freq(gpu_dfs_ctx.freq_min, gpu_dfs_ctx.freq_max, target_freq);
	}

	KBASE_DEBUG_PRINT(3, "MALI_DVFS util %3d; cur_freq %6d -> next_freq %6d\n",
		gpu_dfs_ctx.cur_load, gpu_dfs_ctx.freq_cur->freq, gpu_dfs_ctx.freq_next->freq);

	if(gpu_dfs_ctx.freq_next->freq != gpu_dfs_ctx.freq_cur->freq)
	{
		queue_work(gpu_dfs_ctx.gpu_dfs_workqueue, &gpu_dfs_work);
	}*/

	return (result);
}
#endif/*CONFIG_MALI_MIDGARD_DVFS*/

#ifdef CONFIG_MALI_DEVFREQ
#define FREQ_KHZ    1000

int kbase_platform_get_init_freq(void)
{
	return 750000000;//(gpu_dfs_ctx.freq_default->freq * FREQ_KHZ);
}

int kbase_platform_set_freq(int freq)
{
	int result = -1;/*, index = -1;

	freq = freq/FREQ_KHZ;
	index = freq_search(gpu_dfs_ctx.freq_list, gpu_dfs_ctx.freq_list_len, freq);
	if (0 <= index)
	{
		gpu_dfs_ctx.freq_next = &gpu_dfs_ctx.freq_list[index];
		result = gpu_change_freq();
	}*/

	return (result);
}
#endif

bool kbase_mali_is_powered(void)
{
	bool result = true;

	/*down(gpu_dfs_ctx.sem);
	if (gpu_dfs_ctx.gpu_power_on && gpu_dfs_ctx.gpu_clock_on)
	{
		result = true;
	}
	up(gpu_dfs_ctx.sem);*/

	return (result);



}
