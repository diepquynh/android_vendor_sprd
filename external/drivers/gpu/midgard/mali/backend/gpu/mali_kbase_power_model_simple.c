/*
 *
 * (C) COPYRIGHT 2011-2016 ARM Limited. All rights reserved.
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



#include <linux/devfreq_cooling.h>
#include <linux/thermal.h>
#include <linux/of.h>
#include <linux/sprd_otp.h>
#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <backend/gpu/mali_kbase_power_model_simple.h>
#ifdef CONFIG_MALI_HOTPLUG
#include <mali_kbase_hotplug.h>
#endif

/*
 * This model is primarily designed for the Juno platform. It may not be
 * suitable for other platforms.
 */

#define FALLBACK_STATIC_TEMPERATURE 55000

#define GPU_VOLTAGE_BASE    800    //mV
#define GPU_STATIC_CLUSTER  4530   //uW
#define GPU_STATIC_CORE     2670   //uW
#define GPU_DYN_CLUSTER     76471  //uW
#define GPU_DYN_CORE        147059 //uW
#define GPU_TSCALE_A        8      // /10^7 mW
#define GPU_TSCALE_B        -1170  // /10^7 mW
#define GPU_TSCALE_C        60800  // /10^7 mW
#define GPU_TSCALE_D        -818500// /10^7 mW
#define GPU_VSCALE_E        3644   // /10^2 mW
#define GPU_VSCALE_F        -8017  // /10^2 mW
#define GPU_VSCALE_G        5952   // /10^2 mW
#define GPU_VSCALE_H        -1402  // /10^2 mW

#define GPU_BLOCK_INDEX	15

static u32 dynamic_coefficient;
static u32 static_coefficient;
static s32 ts[4];
static struct thermal_zone_device *gpu_tz;
static struct kbase_device *gpu_kbdev;

//Tscale = 0.0000825T^3 - 0.0117T^2 + 0.608T - 8.185
//Tscale need to div 10^2
static unsigned long get_temperature_scale(unsigned long temp)
{
	int i=0;
	long coeff[] = {GPU_TSCALE_D, GPU_TSCALE_C, GPU_TSCALE_B, GPU_TSCALE_A};
	unsigned long t_scale = 0, t_exp = 1;

	//temp is oC
	for (i = 0; i < 4; i++)
	{
		t_scale += coeff[i] * t_exp;
		t_exp *= temp;
	}
	return (t_scale/1000);
}

//Vscale = 36.44V^3 - 80.17V^2 + 59.59V - 14.02
//Tscale need to div 10^3
static unsigned long get_voltage_scale(unsigned long voltage)
{
	unsigned long v_scale = 0;

	//voltage is mV, need to convert V
	v_scale = GPU_VSCALE_E * voltage * voltage * voltage / 1000;
	v_scale += GPU_VSCALE_F * voltage *  voltage ;
	v_scale += GPU_VSCALE_G * voltage * 1000;
	v_scale += GPU_VSCALE_H * 1000 * 1000;
	return v_scale / 100000;
}

//Pleakbase need to div 10^3, convert to mW
static unsigned long get_leakbase(int gpu_cores)
{
	int blk_index = GPU_BLOCK_INDEX;
	u32	val = 0;
	unsigned long gpu_cluster = 0, gpu_core = 0, Pleakbase = 0;

	//read efuse [25:21] is for GPU
	val = sprd_ap_efuse_read(blk_index);
	//KBASE_DEBUG_PRINT(2, "Jassmine get_leakbase val=%d gpu_cores=%d\n", val, gpu_cores);
	if (0 == val)
	{
		gpu_cluster =  GPU_STATIC_CLUSTER;
		gpu_core = GPU_STATIC_CORE;
	}
	else
	{
		val = (val>>21) & 0x1F;
		//Pleakbase(GPU_cluster) = (GPU_LEAK[4:0]+1) x 2mA x 0.80V x 11.84%
		gpu_cluster = (val+1) * 2 * 800 * 1184/10000;
		//Pleakbase(GPU_core) = (GPU_LEAK[4:0]+1) x 2mA x 0.80V x 22.04%
		gpu_core = (val+1) * 2 * 800 * 2204/10000;
	}

	Pleakbase = gpu_cluster + gpu_core * gpu_cores;/*uW*/
	//KBASE_DEBUG_PRINT(2, "Jassmine get_leakbase gpu_cluster=%lu gpu_core=%lu Pleakbase=%lu \n", gpu_cluster,gpu_core,Pleakbase);

	return (Pleakbase);
}

//Pleak = Pleakbase * Tscale * Vscale
//Pleakbase = gpu_cluster + gpu_core * core_num
//Tscale = aT^3 + bT^2 + cT + d
//Vscale = eV^3 + fV^2 + gV + h
static unsigned long model_static_power(unsigned long voltage)
{
	int ret = 0, gpu_cores = 0;
	unsigned long temperature = 0, Pleakbase = 0, Tscale = 0, Vscale = 0, static_power = 0;

	if (gpu_tz)
	{
		ret = gpu_tz->ops->get_temp(gpu_tz, &temperature);
		if (ret) {
			pr_warn_ratelimited("Error reading temperature for gpu thermal zone: %d\n",ret);
			temperature = FALLBACK_STATIC_TEMPERATURE;
		}
	}
	else
	{
		temperature = FALLBACK_STATIC_TEMPERATURE;
	}

	if (gpu_kbdev)
	{
		gpu_cores = hweight64(gpu_kbdev->pm.debug_core_mask_all);
	}
	//Pleakbase need to div 10^3, convert to mW
	Pleakbase = get_leakbase(gpu_cores);

	//get temprature scale,need to div 10^2
	Tscale = get_temperature_scale(temperature / 1000);

	//get voltage scale,need to div 10^3
	Vscale = get_voltage_scale(voltage);

	//static_power = (Pleakbase/1000) * (Tscale/100)* (Vscale/1000)
	static_power = (Pleakbase/100) * Tscale * Vscale / 1000000;

	return (static_power);
}

//Pdyn = Pdynperghz(mW) * Freq(MHz) * (Voltage/Vbase)^2(mV)
//Pdynperghz = gpu_cluster + gpu_core * core_num
//Whale2: gpu_cluster = 76 gpu_core = 147 Vbase = 0.8
static unsigned long model_dynamic_power(unsigned long freq,
		unsigned long voltage)
{
	int gpu_cores = 0;
	unsigned long Pdynperghz = 0, f_mhz = 0, v2 = 0, Vbase2 = 0, dyn_power = 0;
	/* The inputs: freq (f) is in Hz, and voltage (v) in mV.
	 * The coefficient (c) is in mW/(MHz mV mV).
	 */
	if (gpu_kbdev)
	{
		gpu_cores = hweight64(gpu_kbdev->pm.debug_core_mask_all);
	}

	Pdynperghz = GPU_DYN_CLUSTER + GPU_DYN_CORE * gpu_cores;/*uW*/

	//f_mhz need to div 10^3, convert to GHz
	f_mhz = freq / 1000000;   /* MHz */

	//v2 need to div 10^6, convert to V
	v2 = (voltage * voltage); /* mV */

	//Vbase2 need to div 10^6, convert to V
	Vbase2 = GPU_VOLTAGE_BASE * GPU_VOLTAGE_BASE; /* mV*/

	//dyn_power = (Pdynperghz/1000)[mW] * (f_mhz/1000)[GHz] * v2[mV] * /Vbase2[mV]
	dyn_power = Pdynperghz * f_mhz / 1000 * v2 / (Vbase2 *1000);

	return (dyn_power);
}

#ifdef CONFIG_MALI_HOTPLUG
//Pdyn = Pdynperghz(mW) * Freq(MHz) * (Voltage/Vbase)^2(mV)
//Pdynperghz = gpu_cluster
//Whale2: gpu_cluster = 76 Vbase = 0.8
static unsigned long model_cluster_dynamic_power(unsigned long freq,
		unsigned long voltage)
{
	unsigned long Pdynperghz = 0, f_mhz = 0, v2 = 0, Vbase2 = 0, dyn_power = 0;
	/* The inputs: freq (f) is in Hz, and voltage (v) in mV.
	 * The coefficient (c) is in mW/(MHz mV mV).
	 */
	Pdynperghz = GPU_DYN_CLUSTER;/*uW*/

	//f_mhz need to div 10^3, convert to GHz
	f_mhz = freq / 1000000;   /* MHz */

	//v2 need to div 10^6, convert to V
	v2 = (voltage * voltage); /* mV */

	//Vbase2 need to div 10^6, convert to V
	Vbase2 = GPU_VOLTAGE_BASE * GPU_VOLTAGE_BASE; /* mV*/

	//dyn_power = (Pdynperghz/1000)[mW] * (f_mhz/1000)[GHz] * v2[mV] * /Vbase2[mV]
	dyn_power = Pdynperghz * f_mhz / 1000 * v2 / (Vbase2 *1000);

	return (dyn_power);
}

//Pdyn = Pdynperghz(mW) * Freq(MHz) * (Voltage/Vbase)^2(mV)
//Pdynperghz = gpu_core
//Whale2: gpu_core = 147 Vbase = 0.8
static unsigned long model_core_dynamic_power(unsigned long freq,
		unsigned long voltage) //uW
{
	unsigned long Pdynperghz = 0, f_mhz = 0, v2 = 0, Vbase2 = 0, dyn_power = 0;
	/* The inputs: freq (f) is in Hz, and voltage (v) in mV.
	 * The coefficient (c) is in mW/(MHz mV mV).
	 */
	Pdynperghz = GPU_DYN_CORE;/*uW*/

	//f_mhz need to div 10^3, convert to GHz
	f_mhz = freq / 1000000;   /* MHz */

	//v2 need to div 10^6, convert to V
	v2 = (voltage * voltage); /* mV */

	//Vbase2 need to div 10^6, convert to V
	Vbase2 = GPU_VOLTAGE_BASE * GPU_VOLTAGE_BASE; /* mV*/

	//dyn_power = (Pdynperghz/1000)[mW] * (f_mhz/1000)[GHz] * v2[mV] * /Vbase2[mV]
	dyn_power = Pdynperghz * f_mhz / 1000 * v2 / (Vbase2 *1000);

	return (dyn_power);
}

static int model_online_cores_num(void)
{
	int result = 0;

	if (gpu_kbdev)
	{
		result = kbase_num_online_cores(gpu_kbdev);
	}

	return (result);
}

static int model_max_cores_num(void)
{
	int result = 0;

	if (gpu_kbdev)
	{
		result = kbase_num_max_cores(gpu_kbdev);
	}

	return (result);
}

static void model_set_limit_cores_num(int max_cores)
{
	if (gpu_kbdev)
	{
		kbase_set_max_cores(gpu_kbdev, max_cores);
	}
}
#endif

struct devfreq_cooling_ops power_model_simple_ops = {
	.get_static_power = model_static_power,
	.get_dynamic_power = model_dynamic_power,
#ifdef CONFIG_MALI_HOTPLUG
	.get_cluster_dynamic_power = model_cluster_dynamic_power,
	.get_core_dynamic_power = model_core_dynamic_power,
	.get_online_cores_num = model_online_cores_num,
	.get_max_cores_num = model_max_cores_num,
	.set_limit_cores_num = model_set_limit_cores_num,
#endif
};

int kbase_power_model_simple_init(struct kbase_device *kbdev)
{
	struct device_node *power_model_node;
	const char *tz_name;
	u32 static_power, dynamic_power;
	u32 voltage, voltage_squared, voltage_cubed, frequency;

	gpu_kbdev = kbdev;
	power_model_node = of_get_child_by_name(kbdev->dev->of_node,
			"power_model");
	if (!power_model_node) {
		dev_err(kbdev->dev, "could not find power_model node\n");
		return -ENODEV;
	}
	if (!of_device_is_compatible(power_model_node,
			"sprd,mali-power-model")) {
		dev_err(kbdev->dev, "power_model incompatible with simple power model\n");
		return -ENODEV;
	}

	if (of_property_read_string(power_model_node, "thermal-zone",
			&tz_name)) {
		dev_err(kbdev->dev, "ts in power_model not available\n");
		return -EINVAL;
	}

	gpu_tz = thermal_zone_get_zone_by_name(tz_name);
	if (IS_ERR(gpu_tz)) {
		pr_warn_ratelimited("Error getting gpu thermal zone (%ld), not yet ready?\n",
				PTR_ERR(gpu_tz));
		gpu_tz = NULL;

		return -EPROBE_DEFER;
	}

	if (of_property_read_u32(power_model_node, "static-power",
			&static_power)) {
		dev_err(kbdev->dev, "static-power in power_model not available\n");
		return -EINVAL;
	}
	if (of_property_read_u32(power_model_node, "dynamic-power",
			&dynamic_power)) {
		dev_err(kbdev->dev, "dynamic-power in power_model not available\n");
		return -EINVAL;
	}
	if (of_property_read_u32(power_model_node, "voltage",
			&voltage)) {
		dev_err(kbdev->dev, "voltage in power_model not available\n");
		return -EINVAL;
	}
	if (of_property_read_u32(power_model_node, "frequency",
			&frequency)) {
		dev_err(kbdev->dev, "frequency in power_model not available\n");
		return -EINVAL;
	}
	voltage_squared = (voltage * voltage) / 1000;
	voltage_cubed = voltage * voltage * voltage;
	static_coefficient = (static_power << 20) / (voltage_cubed >> 10);
	dynamic_coefficient = (((dynamic_power * 1000) / voltage_squared)
			* 1000) / frequency;

	if (of_property_read_u32_array(power_model_node, "ts", ts, 4)) {
		dev_err(kbdev->dev, "ts in power_model not available\n");
		return -EINVAL;
	}

	return 0;
}

