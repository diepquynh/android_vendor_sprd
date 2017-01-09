/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "vcm_drv.h"


vcm_info_tab_t driver_dw9714;
vcm_info_tab_t driver_dw9714a;
vcm_info_tab_t driver_dw9806;
vcm_info_tab_t driver_dw8714;
vcm_info_tab_t driver_ov8825;

vcm_info_tab_t*vcm_info_tab[]=
{
	&driver_dw9714,
	&driver_dw9714a,
	&driver_dw9806,
	&driver_dw8714,
	&driver_ov8825,
	PNULL
};

uint32_t vcm_identify(uint32_t id)
{
	uint32_t count = VCM_DRIVER_MAX;
	uint32_t ret_value = SENSOR_SUCCESS;
	uint32_t i;
	vcm_info_tab_t *vcm_temp;
	for (i = 0;i < count;i ++) {
		vcm_temp = (vcm_info_tab_t *)vcm_info_tab[i];
		if (id == vcm_temp->vcm_cur_id) {
			ret_value = i;
			break;
		}
	}
	if (i == VCM_DRIVER_MAX) {
		ret_value = VCM_UNIDENTIFY;
	}
	return ret_value;
}

uint32_t vcm_init(uint32_t id,uint32_t mode)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint32_t vcm_id_num = vcm_identify(id);
	if (vcm_id_num == VCM_UNIDENTIFY) {
		ret_value = SENSOR_FAIL;
	}
	else {
		vcm_info_tab_t *vcm_identifyed = (vcm_info_tab_t *)vcm_info_tab[vcm_id_num];
		ret_value = vcm_identifyed->vcm_func->vcm_init(mode);
	}
	return ret_value;
}
uint32_t vcm_go_position(uint32_t id,uint32_t pos,uint32_t slewrate)
{

	uint32_t ret_value = SENSOR_SUCCESS;
	SENSOR_PRINT_HIGH("CCFF vcm_pos  %d",pos);
	uint32_t vcm_id_num = vcm_identify(id);
	if (vcm_id_num == VCM_UNIDENTIFY) {
		ret_value = SENSOR_FAIL;
	}
	else {
		vcm_info_tab_t *vcm_identifyed = (vcm_info_tab_t *)vcm_info_tab[vcm_id_num];
		ret_value = vcm_identifyed->vcm_func->vcm_set_pos(pos,slewrate);
	}
	return ret_value;

}


