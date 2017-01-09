/*!
@Title          System Configuration
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@License        Dual MIT/GPLv2

The contents of this file are subject to the MIT license as set out below.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Alternatively, the contents of this file may be used under the terms of
the GNU General Public License Version 2 ("GPL") in which case the provisions
of GPL are applicable instead of those above.

If you wish to allow use of your version of this file only under the terms of
GPL, and not to allow others to use your version of this file under the terms
of the MIT license, indicate your decision by deleting the provisions above
and replace them with the notice and other provisions required by GPL as set
out in the file called "GPL-COPYING" included in this distribution. If you do
not delete the provisions above, a recipient may use your version of this file
under the terms of either the MIT license or GPL.

This License is also included in this distribution in the file called
"MIT-COPYING".

EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
@Description    System Configuration functions
*/

#include <linux/io.h>
#include <linux/of.h>
#include <linux/delay.h>
#include <linux/regmap.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/platform_device.h>
#include <linux/mfd/syscon.h>
#include <linux/mfd/syscon/sprd-glb.h>
#include <linux/semaphore.h>
#include <linux/regulator/consumer.h>
#ifdef CONFIG_PM_RUNTIME
#include <linux/pm_runtime.h>
#endif
#include "rgxdevice.h"
#include "pvr_debug.h"
#include "img_types.h"
#include "sprd_init.h"
#include <linux/vmalloc.h>
#define SYS_RGX_ACTIVE_POWER_LATENCY_MS (100)
#define PM_RUNTIME_DELAY_MS (50)

#define DTS_CLK_OFFSET          4
#define FREQ_KHZ                1000

#if defined(PVR_DVFS)
#define GPU_POLL_MS             100
#define GPU_UP_THRESHOLD        90
#define GPU_DOWN_DIFFERENTIAL   10
#endif

struct gpu_freq_info {
	struct clk* clk_src;
	int freq;    //kHz
	int volt;    //uV
	int div;
};

struct gpu_dvfs_context {
	int gpu_clock_on;
	int gpu_power_on;

	int cur_voltage;	//uV

	struct regulator *gpu_regulator;

	struct clk*  clk_gpu_i;
	struct clk*  clk_gpu_core;
	struct clk*  clk_gpu_mem;
	struct clk*  clk_gpu_sys;
	struct clk** gpu_clk_src;
	int gpu_clk_num;

	struct gpu_freq_info* freq_list;
	int freq_list_len;

	const struct gpu_freq_info* freq_cur;

	const struct gpu_freq_info* freq_default;

	struct semaphore* sem;

	struct regmap* pmu_apb_reg_base;
	struct regmap* aon_apb_reg_base;
};

DEFINE_SEMAPHORE(gpu_dvfs_sem);
static struct gpu_dvfs_context gpu_dvfs_ctx=
{
	.gpu_clock_on=0,
	.gpu_power_on=0,

	.sem=&gpu_dvfs_sem,
};

static int pmu_glb_set(unsigned long reg, u32 bit)
{
	int value;
	//base:0xE42B000
	regmap_read(gpu_dvfs_ctx.pmu_apb_reg_base, reg , &value);
	value = value | bit;
	regmap_write(gpu_dvfs_ctx.pmu_apb_reg_base, reg, value);
	return 0;
}

static int pmu_glb_clr(unsigned long reg, u32 bit)
{
	int value;
	//base:0xE42B000
	regmap_read(gpu_dvfs_ctx.pmu_apb_reg_base, reg , &value);
	value = value & ~bit;
	regmap_write(gpu_dvfs_ctx.pmu_apb_reg_base, reg, value);
	return 0;
}

static PVRSRV_ERROR RgxDeviceInit(PVRSRV_DEVICE_CONFIG* psDevConfig, struct platform_device *pDevice)
{
	PVRSRV_ERROR result = PVRSRV_OK;
	struct resource *reg_res = NULL;
	struct resource *irq_res = NULL;

	//the first memory resource is the physical address of the GPU registers
	reg_res = platform_get_resource(pDevice, IORESOURCE_MEM, 0);
	if (!reg_res) {
		PVR_DPF((PVR_DBG_ERROR, "RgxDeviceInit No MEM resource"));
		result = PVRSRV_ERROR_INIT_FAILURE;
		return (result);
	}

	/* Device setup information */
	psDevConfig->sRegsCpuPBase.uiAddr = reg_res->start;
	psDevConfig->ui32RegsSize = resource_size(reg_res);

	//init irq
	irq_res = platform_get_resource(pDevice, IORESOURCE_IRQ, 0);
	if (!irq_res) {
		PVR_DPF((PVR_DBG_ERROR, "RgxDeviceInit No IRQ resource"));
		result = PVRSRV_ERROR_INIT_FAILURE;
		return (result);
	}
	psDevConfig->ui32IRQ            = irq_res->start;
	psDevConfig->eCacheSnoopingMode = PVRSRV_DEVICE_SNOOP_NONE;

	//get regulator
	//gpu_dvfs_ctx.gpu_regulator = regulator_get_optional(&pDevice->dev, "rogue");
	gpu_dvfs_ctx.gpu_regulator = regulator_get(NULL, "fairchild_fan53555");

	return (result);
}

static void RgxFreqInit(struct device *dev)
{
	int i = 0, clk_cnt = 0;

	gpu_dvfs_ctx.pmu_apb_reg_base = syscon_regmap_lookup_by_phandle(dev->of_node,"sprd,syscon-aon-pwu-apb");
	PVR_ASSERT(NULL != gpu_dvfs_ctx.pmu_apb_reg_base);

	gpu_dvfs_ctx.clk_gpu_i = of_clk_get(dev->of_node, 0);
	PVR_ASSERT(NULL != gpu_dvfs_ctx.clk_gpu_i);

	gpu_dvfs_ctx.clk_gpu_core = of_clk_get(dev->of_node, 1);
	PVR_ASSERT(NULL != gpu_dvfs_ctx.clk_gpu_core);

	gpu_dvfs_ctx.clk_gpu_mem = of_clk_get(dev->of_node, 2);
	PVR_ASSERT(NULL != gpu_dvfs_ctx.clk_gpu_mem);

	gpu_dvfs_ctx.clk_gpu_sys = of_clk_get(dev->of_node, 3);
	PVR_ASSERT(NULL != gpu_dvfs_ctx.clk_gpu_sys);

	clk_cnt = of_clk_get_parent_count(dev->of_node);
	gpu_dvfs_ctx.gpu_clk_num = clk_cnt - DTS_CLK_OFFSET;

	gpu_dvfs_ctx.gpu_clk_src = vmalloc(sizeof(struct clk*) * gpu_dvfs_ctx.gpu_clk_num);
	PVR_ASSERT(NULL != gpu_dvfs_ctx.gpu_clk_src);

	for (i = 0; i < gpu_dvfs_ctx.gpu_clk_num; i++)
	{
		gpu_dvfs_ctx.gpu_clk_src[i] = of_clk_get(dev->of_node, i+DTS_CLK_OFFSET);
		PVR_ASSERT(NULL != gpu_dvfs_ctx.gpu_clk_src[i]);
	}

	gpu_dvfs_ctx.freq_list_len = of_property_count_elems_of_size(dev->of_node,"sprd,dvfs-lists",4*sizeof(u32));
	gpu_dvfs_ctx.freq_list = vmalloc(sizeof(struct gpu_freq_info) * gpu_dvfs_ctx.freq_list_len);
	PVR_ASSERT(NULL != gpu_dvfs_ctx.freq_list);

	for(i=0; i<gpu_dvfs_ctx.freq_list_len; i++)
	{
		int clk = 0;

		of_property_read_u32_index(dev->of_node, "sprd,dvfs-lists", 4*i+2, &clk);
		gpu_dvfs_ctx.freq_list[i].clk_src = gpu_dvfs_ctx.gpu_clk_src[clk-DTS_CLK_OFFSET];
		PVR_ASSERT(NULL != gpu_dvfs_ctx.freq_list[i].clk_src);
		of_property_read_u32_index(dev->of_node, "sprd,dvfs-lists", 4*i,   &gpu_dvfs_ctx.freq_list[i].freq);
		of_property_read_u32_index(dev->of_node, "sprd,dvfs-lists", 4*i+1, &gpu_dvfs_ctx.freq_list[i].volt);
		of_property_read_u32_index(dev->of_node, "sprd,dvfs-lists", 4*i+3, &gpu_dvfs_ctx.freq_list[i].div);
	}

	of_property_read_u32(dev->of_node, "sprd,dvfs-default", &i);
	gpu_dvfs_ctx.freq_default = &gpu_dvfs_ctx.freq_list[i];
	PVR_ASSERT(NULL !=gpu_dvfs_ctx.freq_default);

	gpu_dvfs_ctx.freq_cur = &gpu_dvfs_ctx.freq_list[0];//gpu_dvfs_ctx.freq_default;
	gpu_dvfs_ctx.cur_voltage = gpu_dvfs_ctx.freq_cur->volt;
}

static void RgxTimingInfoInit(RGX_TIMING_INFORMATION* psRGXTimingInfo)
{
	PVR_ASSERT(NULL != psRGXTimingInfo);

	/*
	 * Setup RGX specific timing data
	 */
	psRGXTimingInfo->ui32CoreClockSpeed    = gpu_dvfs_ctx.freq_default->freq * FREQ_KHZ;
	psRGXTimingInfo->bEnableActivePM       = IMG_TRUE;
	psRGXTimingInfo->bEnableRDPowIsland    = IMG_TRUE;
	psRGXTimingInfo->ui32ActivePMLatencyms = SYS_RGX_ACTIVE_POWER_LATENCY_MS;
}

#if defined(PVR_DVFS)
static int RgxFreqSearch(struct gpu_freq_info freq_list[], int len, int key)
{
	int low = 0, high = len-1, mid = 0;

	if (0 > key)
	{
		return -1;
	}

	while (low <= high)
	{
		mid = (low+high)/2;
		if (key == freq_list[mid].freq)
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

static int RgxSetFreqVolt(IMG_UINT32 ui32Freq, IMG_UINT32 ui32Volt)
{
	int index = -1, err = -1;

	PVR_DPF((PVR_DBG_SPRD, "GPU DVFS %s cur_freq=%d cur_voltage=%d --> ui32Freq=%d ui32Volt=%d gpu_power_on=%d gpu_clock_on=%d",
		__func__, gpu_dvfs_ctx.freq_cur->freq, gpu_dvfs_ctx.cur_voltage, ui32Freq, ui32Volt,
		gpu_dvfs_ctx.gpu_power_on, gpu_dvfs_ctx.gpu_clock_on));
	ui32Freq = ui32Freq/FREQ_KHZ;
	index = RgxFreqSearch(gpu_dvfs_ctx.freq_list, gpu_dvfs_ctx.freq_list_len, ui32Freq);
	if (0 <= index)
	{
		down(gpu_dvfs_ctx.sem);
		if(gpu_dvfs_ctx.gpu_power_on && gpu_dvfs_ctx.gpu_clock_on)
		{
			//upgrade voltage
			if (ui32Volt > gpu_dvfs_ctx.cur_voltage)
			{
				err = regulator_set_voltage(gpu_dvfs_ctx.gpu_regulator, ui32Volt, ui32Volt);
				if (err)
				{
					PVR_DPF((PVR_DBG_ERROR, "RgxSetFreqVolt gpu regulator set high voltage err = %d", err));
					return (err);
				}
				gpu_dvfs_ctx.cur_voltage = ui32Volt;
			}

			if(ui32Freq != gpu_dvfs_ctx.freq_cur->freq)
			{
				//PVR_DPF((PVR_DBG_WARNING, "GPU DVFS %s cur_freq=%d cur_voltage=%d --> ui32Freq=%d ui32Volt=%d",
				//	__func__, gpu_dvfs_ctx.freq_cur->freq, gpu_dvfs_ctx.cur_voltage, ui32Freq, ui32Volt));
				//set gpu core clk
				clk_set_parent(gpu_dvfs_ctx.clk_gpu_core, gpu_dvfs_ctx.freq_list[index].clk_src);

				//set gpu mem clk
				clk_set_parent(gpu_dvfs_ctx.clk_gpu_mem, gpu_dvfs_ctx.freq_list[index].clk_src);

				gpu_dvfs_ctx.freq_cur = &gpu_dvfs_ctx.freq_list[index];
				err = 0;
			}

			//downgrade voltage
			if (ui32Volt < gpu_dvfs_ctx.cur_voltage)
			{
				err = regulator_set_voltage(gpu_dvfs_ctx.gpu_regulator, ui32Volt, ui32Volt);
				if (err)
				{
					PVR_DPF((PVR_DBG_ERROR, "RgxSetFreqVolt gpu regulator set low voltage err = %d", err));
				}
				gpu_dvfs_ctx.cur_voltage = ui32Volt;
			}
		}
		up(gpu_dvfs_ctx.sem);
	}

	return (err);
}

static void RgxDVFSInit(PVRSRV_DEVICE_CONFIG* psDevConfig)
{
	psDevConfig->sDVFS.sDVFSDeviceCfg.ui32PollMs = GPU_POLL_MS;
	psDevConfig->sDVFS.sDVFSDeviceCfg.bIdleReq = IMG_FALSE;
	psDevConfig->sDVFS.sDVFSDeviceCfg.pfnSetFreqVolt = RgxSetFreqVolt;

	psDevConfig->sDVFS.sDVFSGovernorCfg.ui32UpThreshold = GPU_UP_THRESHOLD;
	psDevConfig->sDVFS.sDVFSGovernorCfg.ui32DownDifferential = GPU_DOWN_DIFFERENTIAL;
}
#endif

static void RgxPowerOn(void)
{
	int result = 0;

	result = regulator_enable(gpu_dvfs_ctx.gpu_regulator);
	if (result)
	{
		PVR_DPF((PVR_DBG_ERROR, "RgxPowerOn gpu regulator error = %d", result));
	}

	//GPU power
	pmu_glb_clr(REG_PMU_APB_PD_GPU_TOP_PWR_CFG1, BIT_PMU_APB_PD_GPU_TOP_FORCE_SHUTDOWN);
	//pmu_glb_clr(REG_PMU_APB_PD_GPU_PHANTOM_PWR_CFG1, BIT_PMU_APB_PD_GPU_PHANTOM_FORCE_SHUTDOWN);
	pmu_glb_set(REG_PMU_APB_PD_GPU_PHANTOM_PWR_CFG1, BIT_PMU_APB_PD_GPU_PHANTOM_AUTO_SHUTDOWN_EN);

	gpu_dvfs_ctx.gpu_power_on = 1;
}

static void RgxPowerOff(void)
{
	int result = 0;

	gpu_dvfs_ctx.gpu_power_on = 0;

	//GPU power
	pmu_glb_set(REG_PMU_APB_PD_GPU_TOP_PWR_CFG1, BIT_PMU_APB_PD_GPU_TOP_FORCE_SHUTDOWN);
	//pmu_glb_set(REG_PMU_APB_PD_GPU_PHANTOM_PWR_CFG1, BIT_PMU_APB_PD_GPU_PHANTOM_FORCE_SHUTDOWN);
	pmu_glb_clr(REG_PMU_APB_PD_GPU_PHANTOM_PWR_CFG1, BIT_PMU_APB_PD_GPU_PHANTOM_AUTO_SHUTDOWN_EN);

	result = regulator_disable(gpu_dvfs_ctx.gpu_regulator);
	if (result)
	{
		PVR_DPF((PVR_DBG_ERROR, "RgxPowerOff gpu regulator error = %d", result));
	}
}

static void RgxClockOn(void)
{
	int i;

	//enable all clocks
	for(i=0;i<gpu_dvfs_ctx.gpu_clk_num;i++)
	{
		clk_prepare_enable(gpu_dvfs_ctx.gpu_clk_src[i]);
	}

	clk_prepare_enable(gpu_dvfs_ctx.clk_gpu_i);

	//enable gpu clock
	clk_prepare_enable(gpu_dvfs_ctx.clk_gpu_core);
	clk_prepare_enable(gpu_dvfs_ctx.clk_gpu_mem);
	clk_prepare_enable(gpu_dvfs_ctx.clk_gpu_sys);
	udelay(300);

	//set gpu clock parent
	clk_set_parent(gpu_dvfs_ctx.clk_gpu_core, gpu_dvfs_ctx.freq_default->clk_src);
	clk_set_parent(gpu_dvfs_ctx.clk_gpu_mem, gpu_dvfs_ctx.freq_default->clk_src);

	PVR_ASSERT(NULL != gpu_dvfs_ctx.freq_cur);
	clk_set_parent(gpu_dvfs_ctx.clk_gpu_core, gpu_dvfs_ctx.freq_cur->clk_src);
	clk_set_parent(gpu_dvfs_ctx.clk_gpu_mem, gpu_dvfs_ctx.freq_cur->clk_src);

	gpu_dvfs_ctx.gpu_clock_on = 1;
}

static void RgxClockOff(void)
{
	int i;

	gpu_dvfs_ctx.gpu_clock_on = 0;

	//disable gpu clock
	clk_disable_unprepare(gpu_dvfs_ctx.clk_gpu_core);
	clk_disable_unprepare(gpu_dvfs_ctx.clk_gpu_mem);
	clk_disable_unprepare(gpu_dvfs_ctx.clk_gpu_sys);
	clk_disable_unprepare(gpu_dvfs_ctx.clk_gpu_i);

	//disable all clocks
	for(i=0;i<gpu_dvfs_ctx.gpu_clk_num;i++)
	{
		clk_disable_unprepare(gpu_dvfs_ctx.gpu_clk_src[i]);
	}
}

static PVRSRV_ERROR SprdPrePowerState(IMG_HANDLE hSysData, PVRSRV_DEV_POWER_STATE eNewPowerState, PVRSRV_DEV_POWER_STATE eCurrentPowerState, IMG_BOOL bForced)
{
	PVRSRV_ERROR result = PVRSRV_OK;

	PVR_DPF((PVR_DBG_SPRD, "GPU power %s eNewPowerState=%d eCurrentPowerState=%d bForced=%d  gpu_power_on=%d gpu_clock_on=%d",
		__func__, eNewPowerState, eCurrentPowerState, bForced,
		gpu_dvfs_ctx.gpu_power_on, gpu_dvfs_ctx.gpu_clock_on));
	if ((PVRSRV_DEV_POWER_STATE_ON == eNewPowerState) &&
		(eNewPowerState != eCurrentPowerState))
	{
		down(gpu_dvfs_ctx.sem);
		if (!gpu_dvfs_ctx.gpu_power_on)
		{
			RgxPowerOn();
			RgxClockOn();
		}

		if (!gpu_dvfs_ctx.gpu_clock_on)
		{
			RgxClockOn();
		}
		up(gpu_dvfs_ctx.sem);
	}
	return (result);
}

static PVRSRV_ERROR SprdPostPowerState(IMG_HANDLE hSysData, PVRSRV_DEV_POWER_STATE eNewPowerState, PVRSRV_DEV_POWER_STATE eCurrentPowerState, IMG_BOOL bForced)
{
	PVRSRV_ERROR result = PVRSRV_OK;

	PVR_DPF((PVR_DBG_SPRD, "GPU power %s eNewPowerState=%d eCurrentPowerState=%d bForced=%d gpu_power_on=%d gpu_clock_on=%d",
		__func__, eNewPowerState, eCurrentPowerState, bForced,
		gpu_dvfs_ctx.gpu_power_on, gpu_dvfs_ctx.gpu_clock_on));
	if ((PVRSRV_DEV_POWER_STATE_OFF == eNewPowerState) &&
		(eNewPowerState != eCurrentPowerState))
	{
		down(gpu_dvfs_ctx.sem);
		if(gpu_dvfs_ctx.gpu_clock_on)
		{
			RgxClockOff();
		}

		if(gpu_dvfs_ctx.gpu_power_on)
		{
			RgxPowerOff();
		}
		up(gpu_dvfs_ctx.sem);
	}
	return (result);
}

static void RgxPowerManager(PVRSRV_DEVICE_CONFIG* psDevConfig)
{
	/* No power management on no HW system */
	psDevConfig->pfnPrePowerState  = SprdPrePowerState;
	psDevConfig->pfnPostPowerState = SprdPostPowerState;
}

void RgxSprdInit(PVRSRV_DEVICE_CONFIG* psDevConfig, RGX_TIMING_INFORMATION* psRGXTimingInfo, void *pvOSDevice)
{
	struct platform_device *pDevice = to_platform_device((struct device *)pvOSDevice);;

	//device init
	RgxDeviceInit(psDevConfig, pDevice);

	//gpu freq
	RgxFreqInit(&pDevice->dev);

	//rgx timing info
	RgxTimingInfoInit(psRGXTimingInfo);

#if defined(PVR_DVFS)
	//DVFS init
	RgxDVFSInit(psDevConfig);
#endif

	//rgx power manager
	RgxPowerManager(psDevConfig);

#ifdef CONFIG_PM_RUNTIME
	pm_runtime_set_active(&pDevice->dev);
	pm_suspend_ignore_children(&pDevice->dev, true);
	pm_runtime_set_autosuspend_delay(&pDevice->dev, PM_RUNTIME_DELAY_MS);
	pm_runtime_use_autosuspend(&pDevice->dev);
	pm_runtime_enable(&pDevice->dev);
#endif

}

void RgxSprdDeInit(void)
{
	down(gpu_dvfs_ctx.sem);

	//clock off
	RgxClockOff();

	//power off
	RgxPowerOff();

	//free
	vfree(gpu_dvfs_ctx.freq_list);
	vfree(gpu_dvfs_ctx.gpu_clk_src);
	up(gpu_dvfs_ctx.sem);
}
