/*
 * Copyright (C) 2008 The Android Open Source Project
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

#include <stdint.h>
#include <errno.h>
#include <unistd.h>

#include <sys/cdefs.h>
#include <sys/types.h>

#include <cutils/log.h>

#include "SensorCoordinate.h"
/*****************************************************************************/

SensorCoordinate::SensorCoordinate(){
}

SensorCoordinate::~SensorCoordinate(){
}

void SensorCoordinate::coordinate_data_convert(float *vec_io, int dir){
	float tmp;

	if (!vec_io) {
		return;
	}

	switch (dir) {
		case OBVERSE_X_AXIS_FORWARD:
			//x'=-y y'=x z'=z
			tmp = vec_io[0];
			vec_io[0] = - vec_io[1];
			vec_io[1] = tmp;
			break;
		case OBVERSE_X_AXIS_RIGHTWARD:
			break;
		case OBVERSE_X_AXIS_BACKWARD:
			//x'=y y'=-x z'=z
			tmp = vec_io[0];
			vec_io[0] = vec_io[1];
			vec_io[1] = - tmp;
			break;
		case OBVERSE_X_AXIS_LEFTWARD:
			//x'=-x y'=-y z'=z
			vec_io[0] = - vec_io[0];
			vec_io[1] = - vec_io[1];
			break;
		case REVERSE_X_AXIS_FORWARD:
			//x'=y y'=x z'=-z
			tmp = vec_io[0];
			vec_io[0] = vec_io[1];
			vec_io[1] = tmp;
			vec_io[2] = - vec_io[2];
			break;
		case REVERSE_X_AXIS_RIGHTWARD:
			//x'=x y'=-y z'=-z
			vec_io[0] = vec_io[0];
			vec_io[1] = - vec_io[1];
			vec_io[2] = - vec_io[2];
			break;
		case REVERSE_X_AXIS_BACKWARD:
			//x'=-y y'=-x z'=-z
			tmp = vec_io[0];
			vec_io[0] = - vec_io[1];
			vec_io[1] = - tmp;
			vec_io[2] = - vec_io[2];
			break;
		case REVERSE_X_AXIS_LEFTWARD:
			//x'=-x y'=y z'=-z
			vec_io[0] = - vec_io[0];
			vec_io[1] = vec_io[1];
			vec_io[2] = - vec_io[2];
			break;
		default:
			break;
	}
}

