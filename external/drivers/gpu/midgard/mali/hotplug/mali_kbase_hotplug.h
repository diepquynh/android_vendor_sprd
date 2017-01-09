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



#ifndef _BASE_HOTPLUG_H_
#define _BASE_HOTPLUG_H_

int kbase_num_max_cores(struct kbase_device *kbdev);
int kbase_num_online_cores(struct kbase_device *kbdev);
int kbase_num_limit_cores(struct kbase_device *kbdev);
int kbase_hotplug_init(struct kbase_device *kbdev);
void kbase_hotplug_term(struct kbase_device *kbdev);
void kbase_hotplug_wake_up(void);
void kbase_set_max_cores(struct kbase_device *kbdev, int max_cores);

#endif /* _BASE_HOTPLUG_H_ */
