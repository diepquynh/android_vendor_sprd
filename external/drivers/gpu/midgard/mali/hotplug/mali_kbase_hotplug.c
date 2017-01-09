/*
 *
 * (C) COPYRIGHT 2014 ARM Limited. All rights reserved.
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

#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/jiffies.h>
#include <linux/delay.h>

#include <mali_kbase.h>
#include <mali_kbase_config_defaults.h>
#include <backend/gpu/mali_kbase_pm_internal.h>

#define KBASE_GPU_BOOT_TIME     (50*HZ)     //s
#define KBASE_GPU_CHECK_TIME    (40)     //ms

#define KBASE_GPU_UP_MID_THRESHOLD      (80)
#define KBASE_GPU_UP_HIGH_THRESHOLD     (90)
#define KBASE_GPU_DOWN_MID_THRESHOLD    (30)
#define KBASE_GPU_DOWN_HIGH_THRESHOLD   (40)

#define KBASE_UTILISATION_ARRAY_MAX_SIZE    (10)
#define KBASE_UTILISATION_CAPACITY_SIZE     (3)

#define mod(n, div) ((n) % (div))

struct kbase_dbs_tuners {
	unsigned int hotplug_enable;
	unsigned int vailable_cores;
	unsigned int limit_cores;

	unsigned int up_mid_threshold;
	unsigned int up_high_threshold;
	unsigned int down_mid_threshold;
	unsigned int down_high_threshold;

	int utilisation_capacity_size;

	struct semaphore* hotplug_sem;
};

DEFINE_SEMAPHORE(s_kbase_hotplug_sem);
static unsigned long s_kbase_boot_done = 0;

//tuners
static struct kbase_dbs_tuners *s_kbase_tuners = NULL;

//utilisation array
static int s_kbase_utilisation_index = 0;
static int s_kbase_utilisation_array[KBASE_UTILISATION_ARRAY_MAX_SIZE] = {0};

struct workqueue_struct *s_kbase_plugin_work;
struct workqueue_struct *s_kbase_unplug_work;

//hotplug task
static struct task_struct *s_kbase_hotplug;
static DECLARE_WAIT_QUEUE_HEAD(s_kbase_hotplug_wait);

extern int gpu_support_hotplug;

int kbase_num_max_cores(struct kbase_device *kbdev)
{
	int result = 0;
	struct kbase_dbs_tuners *kbtuners = s_kbase_tuners;

	if (NULL != kbtuners)
	{
		//get pp cores
		down(kbtuners->hotplug_sem);
		result = hweight64(kbase_pm_get_present_cores(kbdev,KBASE_PM_CORE_SHADER));
		KBASE_DEBUG_PRINT(3, "MALI_HOTPLUG kbase_num_max_cores result=%d %llu %llu %llu %llu %llu\n",result,
			kbdev->shader_inuse_bitmap, kbdev->shader_needed_bitmap, kbdev->shader_available_bitmap,
			kbdev->shader_ready_bitmap, kbdev->shader_transitioning_bitmap);
		up(kbtuners->hotplug_sem);
	}
	else
	{
		KBASE_DEBUG_PRINT(2, "MALI_HOTPLUG kbase_num_max_cores kbtuners is NULL!\n");
	}

	return (result);
}

int kbase_num_online_cores(struct kbase_device *kbdev)
{
	int result = 0;
	struct kbase_dbs_tuners *kbtuners = s_kbase_tuners;

	if (NULL != kbtuners)
	{
		//get online pp cores
		down(kbtuners->hotplug_sem);
		result = hweight64(kbdev->pm.debug_core_mask_all);
		KBASE_DEBUG_PRINT(3, "MALI_HOTPLUG kbase_num_online_cores result=%d %llu %llu %llu %llu\n",
			result,	kbdev->pm.debug_core_mask[0], kbdev->pm.debug_core_mask[1],
			kbdev->pm.debug_core_mask[2],kbdev->pm.debug_core_mask_all);
		up(kbtuners->hotplug_sem);
	}
	else
	{
		KBASE_DEBUG_PRINT(2, "MALI_HOTPLUG kbase_num_online_cores kbtuners is NULL!\n");
	}

	return (result);
}

int kbase_num_limit_cores(struct kbase_device *kbdev)
{
	int result = 0;
	struct kbase_dbs_tuners *kbtuners = s_kbase_tuners;

	if (NULL != kbtuners)
	{
		//get pp cores
		down(kbtuners->hotplug_sem);
		result = kbtuners->limit_cores;
		up(kbtuners->hotplug_sem);
	}
	else
	{
		KBASE_DEBUG_PRINT(2, "MALI_HOTPLUG kbase_num_limit_cores kbtuners is NULL!\n");
	}

	return (result);
}

static int kbase_is_enable_hotplug(struct kbase_device *kbdev)
{
	int result = false;

	//if gpu cores more than one, permit hotplug
	if (1 < kbase_num_max_cores(kbdev))
	{
		//need to check chip

		result = true;
	}

	return (result);
}

static bool kbase_get_new_core_mask(u64 *core_mask_ptr, int core_num)
{
	bool result = true;

	KBASE_DEBUG_ASSERT(core_mask_ptr);
	switch (core_num)
	{
	case 1:
		*core_mask_ptr = 0x1;
		break;

	case 2:
		*core_mask_ptr = 0x3;
		break;

	case 3:
		*core_mask_ptr = 0x7;
		break;

	case 4:
		*core_mask_ptr = 0xF;
		break;

	case 5:
		*core_mask_ptr = 0x37;
		break;

	case 6:
		*core_mask_ptr = 0x77;
		break;

	case 7:
		*core_mask_ptr = 0x7F;
		break;

	case 8:
		*core_mask_ptr = 0xFF;
		break;

	default:
		result = false;
		break;
	}

	return (result);
}

static bool kbase_set_core_mask(struct kbase_device *kbdev,
		u64 new_core_mask_js0, u64 new_core_mask_js1,
		u64 new_core_mask_js2)
{
	bool result = false;

	if (kbdev->pm.debug_core_mask[0] != new_core_mask_js0 ||
		kbdev->pm.debug_core_mask[1] !=	new_core_mask_js1 ||
		kbdev->pm.debug_core_mask[2] !=	new_core_mask_js2)
	{
		unsigned long flags;

		spin_lock_irqsave(&kbdev->pm.power_change_lock, flags);

		kbase_pm_set_debug_core_mask(kbdev, new_core_mask_js0, new_core_mask_js1, new_core_mask_js2);

		spin_unlock_irqrestore(&kbdev->pm.power_change_lock, flags);

		result = true;
	}

	return (result);
}

static void kbase_plugin(struct work_struct *work)
{
	int num_online = 0;
	u64 new_core_mask = 0;
	struct kbase_device *kbdev = container_of(work, struct kbase_device, plugin_work);
	struct kbase_dbs_tuners *kbtuners = s_kbase_tuners;

	KBASE_DEBUG_PRINT(3, "MALI_HOTPLUG kbase_plugin kbdev=%p\n",kbdev);
	down(kbtuners->hotplug_sem);
	if (kbtuners->hotplug_enable)
	{
		//get online pp cores
		num_online = hweight64(kbdev->pm.debug_core_mask_all);
		KBASE_DEBUG_PRINT(3, "MALI_HOTPLUG kbase_plugin num_online=%d\n",num_online);
		if (num_online < kbtuners->vailable_cores)
		{
			u64 shader_present = kbdev->gpu_props.props.raw_props.shader_present;
			u64 group0_core_mask = kbdev->gpu_props.props.coherency_info.group[0].core_mask;

			//set new core mask
			num_online++;
			if (!kbase_get_new_core_mask(&new_core_mask, num_online))
			{
				KBASE_DEBUG_PRINT(2, "MALI_HOTPLUG kbase_plugin new_core_mask=%llu is not plugin!\n", new_core_mask);
				up(kbtuners->hotplug_sem);
				return ;
			}
			KBASE_DEBUG_PRINT(2, "MALI_HOTPLUG kbase_plugin old_core_mask=%llu new_core_mask=%llu\n", kbdev->pm.debug_core_mask_all, new_core_mask);

			if ((new_core_mask & shader_present) != new_core_mask ||
				!(new_core_mask & group0_core_mask))
			{
				KBASE_DEBUG_PRINT(2, "MALI_HOTPLUG kbase_plugin power_policy: invalid core specification!\n");
				up(kbtuners->hotplug_sem);
				return;
			}

			//set core mask
			kbase_set_core_mask(kbdev,new_core_mask,new_core_mask,new_core_mask);
		}
	}
	up(kbtuners->hotplug_sem);
}

static void kbase_unplug(struct work_struct *work)
{
	int num_online = 0;
	u64 new_core_mask = 0;
	struct kbase_device *kbdev = container_of(work, struct kbase_device, unplug_work);
	struct kbase_dbs_tuners *kbtuners = s_kbase_tuners;

	KBASE_DEBUG_PRINT(3, "MALI_HOTPLUG kbase_unplug kbdev=%p\n",kbdev);
	down(kbtuners->hotplug_sem);
	if (kbtuners->hotplug_enable)
	{
		//get online pp cores
		num_online = hweight64(kbdev->pm.debug_core_mask_all);
		KBASE_DEBUG_PRINT(3, "MALI_HOTPLUG kbase_unplug num_online=%d\n",num_online);
		if (1 < num_online)
		{
			u64 shader_present = kbdev->gpu_props.props.raw_props.shader_present;
			u64 group0_core_mask = kbdev->gpu_props.props.coherency_info.group[0].core_mask;

			//set new core mask
			num_online--;
			if (!kbase_get_new_core_mask(&new_core_mask, num_online))
			{
				KBASE_DEBUG_PRINT(2, "MALI_HOTPLUG kbase_unplug new_core_mask=%llu is not unplug!\n",new_core_mask);
				up(kbtuners->hotplug_sem);
				return ;
			}
			KBASE_DEBUG_PRINT(2, "MALI_HOTPLUG kbase_unplug old_core_mask=%llu new_core_mask=%llu\n", kbdev->pm.debug_core_mask_all, new_core_mask);

			if ((new_core_mask & shader_present) != new_core_mask ||
				!(new_core_mask & group0_core_mask))
			{
				KBASE_DEBUG_PRINT(2, "MALI_HOTPLUG kbase_unplug power_policy: invalid core specification!\n");
				up(kbtuners->hotplug_sem);
				return ;
			}

			//set core mask
			kbase_set_core_mask(kbdev,new_core_mask,new_core_mask,new_core_mask);
		}
	}
	up(kbtuners->hotplug_sem);
}

static void kbase_tuners_init(struct kbase_device *kbdev)
{
	//alloc memory for tuners
	s_kbase_tuners = kzalloc(sizeof(struct kbase_dbs_tuners), GFP_KERNEL);

	//set init value
	s_kbase_tuners->hotplug_sem = &s_kbase_hotplug_sem;
	s_kbase_tuners->hotplug_enable = kbase_is_enable_hotplug(kbdev);
	s_kbase_tuners->vailable_cores = s_kbase_tuners->limit_cores = kbase_num_max_cores(kbdev);
	s_kbase_tuners->up_mid_threshold = KBASE_GPU_UP_MID_THRESHOLD;
	s_kbase_tuners->up_high_threshold = KBASE_GPU_UP_HIGH_THRESHOLD;
	s_kbase_tuners->down_mid_threshold = KBASE_GPU_DOWN_MID_THRESHOLD;
	s_kbase_tuners->down_high_threshold = KBASE_GPU_DOWN_HIGH_THRESHOLD;

	s_kbase_tuners->utilisation_capacity_size = KBASE_UTILISATION_CAPACITY_SIZE;
}

static void kbase_work_init(struct kbase_device *kbdev)
{
	//init plug in/out work
	INIT_WORK(&kbdev->plugin_work, kbase_plugin);
	INIT_WORK(&kbdev->unplug_work, kbase_unplug);

	s_kbase_plugin_work = create_singlethread_workqueue("gpu_plugin");
	s_kbase_unplug_work = create_singlethread_workqueue("gpu_unplug");
}

static void kbase_work_exit(void)
{
	//destroy plug in/out work
	destroy_workqueue(s_kbase_plugin_work);
	destroy_workqueue(s_kbase_unplug_work);
}

static bool kbase_is_need_check(struct kbase_device *kbdev)
{
	bool    result = true;

	//check boot time
	if (time_before(jiffies, s_kbase_boot_done))
	{
		result = false;
	}
	else
	{
		if (gpu_support_hotplug)
		{
			//gpu is power on
			result = kbase_mali_is_powered();
		}
		else
		{
			result = false;
		}
	}
	KBASE_DEBUG_PRINT(3, "MALI_HOTPLUG kbase_is_need_check result=%d \n", result);

	return (result);
}

static int kbase_avg_utilisation(int utilisation, struct kbase_dbs_tuners *kbtuners)
{
	int avg_utilisation = 0, capacity_head = 0, capacity_tail = 0;
	int scale = 0, count = 0, sum_utilisation = 0, sum_scale = 0;

	//set load
	s_kbase_utilisation_array[s_kbase_utilisation_index] = utilisation;

	//set index
	s_kbase_utilisation_index++;
	s_kbase_utilisation_index = mod(s_kbase_utilisation_index, KBASE_UTILISATION_ARRAY_MAX_SIZE);

	//set capacity head and tail
	if (s_kbase_utilisation_index)
	{
		capacity_tail = s_kbase_utilisation_index - 1;
	}
	else
	{
		capacity_tail = KBASE_UTILISATION_ARRAY_MAX_SIZE - 1;
	}
	capacity_head = mod(KBASE_UTILISATION_ARRAY_MAX_SIZE + capacity_tail - 
		kbtuners->utilisation_capacity_size + 1, KBASE_UTILISATION_ARRAY_MAX_SIZE);

	for (scale = 1, count = 0; count < kbtuners->utilisation_capacity_size; scale += scale, count++)
	{
		sum_utilisation += (s_kbase_utilisation_array[capacity_head] * scale);
		sum_scale += scale;
		capacity_head++;
		capacity_head = mod(capacity_head, KBASE_UTILISATION_ARRAY_MAX_SIZE);
	}
	avg_utilisation = sum_utilisation / sum_scale;

	return (avg_utilisation);
}

static void kbase_check_gpu(struct kbase_device *kbdev, struct kbase_dbs_tuners *kbtuners)
{
	int utilisation = 0, avg_utilisation = 0, num_online = 0;

	//skip gpu hotplug check if hotplug is disabled
	if (kbtuners->hotplug_enable)
	{
		//get gpu load
		utilisation = kbase_pm_get_hotplug_utilisation(kbdev);

		//calculate average load
		avg_utilisation = kbase_avg_utilisation(utilisation, kbtuners);

		//get online pp cores
		num_online = kbase_num_online_cores(kbdev);
		KBASE_DEBUG_PRINT(3, "MALI_HOTPLUG kbase_check_gpu utilisation:%d avg_utilisation:%d num_online:%d vailable_cores:%d limit_cores:%d\n",
				utilisation, avg_utilisation, num_online, kbtuners->vailable_cores, kbtuners->limit_cores);

		//gpu plugin check
		if ((num_online < kbtuners->vailable_cores) &&
			(num_online < kbtuners->limit_cores))
		{
			int up_threshold = 0;

			if (1 == num_online)
			{
				up_threshold = kbtuners->up_mid_threshold;
			}
			else
			{
				up_threshold = kbtuners->up_high_threshold;
			}

			if (avg_utilisation > up_threshold)
			{
				KBASE_DEBUG_PRINT(3, "MALI_HOTPLUG kbase_check_gpu plugin kbdev=%p up_threshold:%d\n", kbdev, up_threshold);
				queue_work(s_kbase_plugin_work, &kbdev->plugin_work);
				return;
			}
		}

		//gpu unplug check
		if (1 < num_online)
		{
			int down_threshold = 0;

			if (2 < num_online)
			{
				down_threshold = kbtuners->down_high_threshold;
			}
			else
			{
				down_threshold = kbtuners->down_mid_threshold;
			}

			if (avg_utilisation < down_threshold)
			{
				KBASE_DEBUG_PRINT(3, "MALI_HOTPLUG kbase_check_gpu unplug kbdev=%p down_threshold:%d\n", kbdev, down_threshold);
				queue_work(s_kbase_unplug_work, &kbdev->unplug_work);
			}
		}
	}
}

static int kbase_hotplug(void *data)
{
	struct kbase_device *kbdev = (struct kbase_device*)data;

	while (1)
	{
		wait_event_interruptible(s_kbase_hotplug_wait,kbase_is_need_check(kbdev));

		kbase_check_gpu(kbdev, s_kbase_tuners);
		msleep(KBASE_GPU_CHECK_TIME);
	}

	return 0;
}

int kbase_hotplug_init(struct kbase_device *kbdev)
{
	int result = 0;

	s_kbase_boot_done = jiffies + KBASE_GPU_BOOT_TIME;

	//init tuners
	kbase_tuners_init(kbdev);

	//init work
	kbase_work_init(kbdev);

	//create thread
	s_kbase_hotplug = kthread_create(kbase_hotplug, (void *)kbdev, "mali_kbase_hotplug");

	//wake up thread
	wake_up_process(s_kbase_hotplug);

	return (result);
}

void kbase_hotplug_term(struct kbase_device *kbdev)
{
	//free tuners memory
	kfree(s_kbase_tuners);

	//free work
	kbase_work_exit();
}

void kbase_hotplug_wake_up(void)
{
	KBASE_DEBUG_PRINT(3, "MALI_HOTPLUG kbase_hotplug_wake_up!\n");
	wake_up_interruptible(&s_kbase_hotplug_wait);
}

void kbase_set_max_cores(struct kbase_device *kbdev, int max_cores)
{
	int num_online = 0;
	u64 new_core_mask = 0;
	struct kbase_dbs_tuners *kbtuners = s_kbase_tuners;

	if (NULL != kbtuners)
	{
		KBASE_DEBUG_PRINT(3, "MALI_HOTPLUG kbase_set_max_cores kbdev=%p is_need_check=%d vailable_cores=%d limit_cores=%d max_cores=%d\n",
				kbdev, kbase_is_need_check(kbdev), kbtuners->vailable_cores, kbtuners->limit_cores, max_cores);

		down(kbtuners->hotplug_sem);
		if ((kbase_is_need_check(kbdev)) &&
				(kbtuners->hotplug_enable) &&
				(max_cores <= kbtuners->vailable_cores) &&
				(0 < max_cores) &&
				(max_cores != kbtuners->limit_cores))
		{
			if (max_cores < kbtuners->limit_cores)
			{
				//get online pp cores
				num_online = hweight64(kbdev->pm.debug_core_mask_all);
				KBASE_DEBUG_PRINT(3, "MALI_HOTPLUG kbase_set_max_cores num_online=%d\n",num_online);

				//gpu unplug check
				if (max_cores < num_online)
				{
					u64 shader_present = kbdev->gpu_props.props.raw_props.shader_present;
					u64 group0_core_mask = kbdev->gpu_props.props.coherency_info.group[0].core_mask;

					//set new core mask
					if (!kbase_get_new_core_mask(&new_core_mask, max_cores))
					{
						KBASE_DEBUG_PRINT(2, "MALI_HOTPLUG kbase_set_max_cores new_core_mask=%llu is not unplug!\n",new_core_mask);
						up(kbtuners->hotplug_sem);
						return ;
					}
					KBASE_DEBUG_PRINT(2, "MALI_HOTPLUG kbase_set_max_cores old_core_mask=%llu new_core_mask=%llu\n", kbdev->pm.debug_core_mask_all, new_core_mask);

					if ((new_core_mask & shader_present) != new_core_mask ||
							!(new_core_mask & group0_core_mask))
					{
						KBASE_DEBUG_PRINT(2, "MALI_HOTPLUG kbase_set_max_cores power_policy: invalid core specification!\n");
						up(kbtuners->hotplug_sem);
						return ;
					}

					//set core mask
					kbase_set_core_mask(kbdev,new_core_mask,new_core_mask,new_core_mask);
				}
			}

			//set max cores
			kbtuners->limit_cores = max_cores;
		}
		up(kbtuners->hotplug_sem);
	}
	else
	{
		KBASE_DEBUG_PRINT(2, "MALI_HOTPLUG kbase_set_max_cores kbtuners is NULL!\n");
	}
}
