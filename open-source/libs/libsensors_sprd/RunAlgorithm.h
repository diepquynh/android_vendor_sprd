/****************************************************************************
 * FileIO.h  2012-09-27 10:03:13
 *
 * -- Copyright Notice --
 *
 * Copyright (c) 2012 Senodia Corporation,
 * All Rights Reserved.
 *
 * This software program is proprietary program of Senodia Corporation
 * licensed to authorized Licensee under Software License
 * Agreement (SLA) executed between the Licensee and Senodia.
 *
 * Use of the software by unauthorized third party, or use of the software
 * beyond the scope of the SLA is strictly prohibited.
 *
 * -- End Senodia Copyright Notice --
 *
 * AUTHOR: Tori <xuezhi_xu@senodia.com>
 *
 ****************************************************************************/

#ifndef RUN_ALGORITHM_H
#define RUN_ALGORITHM_H

extern "C" {
typedef struct {
     float  acc_x,
            acc_y,
            acc_z,
            mag_x,
            mag_y,
            mag_z;
}_st480;

int run_library(_st480 st480, signed char level, int time_diff_ms);

void get_magnetic_offset(float offset[3]);

void get_orientation_angles(float angles[3]);

void get_magnetic_values(float values[3]);
}

#endif
