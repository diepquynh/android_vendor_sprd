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


#ifndef __SENSORS_COORDINATE_H__
#define __SENSORS_COORDINATE_H__

#include <stdint.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>
/*****************************************************************************/
/*
 * Definition of sensor placement on target board
 * 
 *               up
 *       +---------------+
 *       |+-------------+|
 *       ||             ||
 * left  ||             || right
 *       ||             ||
 *       ||   screen    ||
 *       ||             ||
 *       ||             ||
 *       |+-------------+|
 *       |---------------|
 *       |[ ] [ ] [ ] [ ]|
 *       | 1   2   3   * |
 *       | 4   5   6   0 |
 *       | 7   8   9   # |
 *       +---------------+
 *              down
 *
 *                                left    right   up      down
 * OBVERSE_X_AXIS_FORWARD          Y+      Y-      X+      X-     ->     0
 * OBVERSE_X_AXIS_RIGHTWARD        X-      X+      Y+      Y-     ->     1
 * OBVERSE_X_AXIS_BACKWARD         Y-      Y+      X-      X+     ->     2
 * OBVERSE_X_AXIS_LEFTWARD         X+      X-      Y-      Y+     ->     3
 * REVERSE_X_AXIS_FORWARD          Y-      Y+      X+      X-     ->     4
 * REVERSE_X_AXIS_RIGHTWARD        X-      X+      Y-      Y+     ->     5
 * REVERSE_X_AXIS_BACKWARD         Y+      Y-      X-      X+     ->     6
 * REVERSE_X_AXIS_LEFTWARD         X+      X-      Y+      Y-     ->     7
 */

class SensorCoordinate
{
public:
	/**
	* @brief
	* Sensor placement on target board
	*/
	enum sensors_placement_t {
		OBVERSE_X_AXIS_FORWARD,		/*!< Sensor on the same side of screen, x-axis point forward */
		OBVERSE_X_AXIS_RIGHTWARD,	/*!< Sensor on the same side of screen, x-axis point rightward */
		OBVERSE_X_AXIS_BACKWARD,	/*!< Sensor on the same side of screen, x-axis point backward */
		OBVERSE_X_AXIS_LEFTWARD,	/*!< Sensor on the same side of screen, x-axis point leftward */
		REVERSE_X_AXIS_FORWARD,		/*!< Sensor on the reverse side of screen, x-axis point forward */
		REVERSE_X_AXIS_RIGHTWARD,	/*!< Sensor on the reverse side of screen, x-axis point rightward */
		REVERSE_X_AXIS_BACKWARD,	/*!< Sensor on the reverse side of screen, x-axis point backward */
		REVERSE_X_AXIS_LEFTWARD		/*!< Sensor on the reverse side of screen, x-axis point leftward */
	};
	/**
	 * @brief Convert sensor raw data vector to target coordinate.
	 * @param vec_io is the raw data vector.
	 * @param dir is the sensor placement on target board.
	 */
	void coordinate_data_convert(float *vec_io, int dir);
	SensorCoordinate();
	~SensorCoordinate();
};
 #endif /* __SENSORS_COORDINATE_H__ */
