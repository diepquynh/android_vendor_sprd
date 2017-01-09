/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.fmradio;

import android.os.SystemProperties;

/**
* @}
*/

/**
 * This class provider interface to compute station and frequency, get project
 * string
 */
public class FmUtils {
    public static final boolean support50ksearch = "1".equals(SystemProperties.get("persist.support.50ksearch", "0"));
    // maximum station frequency
    private static final int HIGHEST_STATION = support50ksearch ? 10800 : 1080;
    // convert rate
    public static final int CONVERT_RATE = support50ksearch ? 100 :10;

    public static float getHighestFrequency() {
        return computeFrequency(HIGHEST_STATION);
    }

    /**
     * Compute frequency value with given station
     *
     * @param station The station value
     *
     * @return station The frequency
     */
    public static float computeFrequency(int station) {
        return (float) station / CONVERT_RATE;
    }

    /**
     * Compute station value with given frequency
     *
     * @param frequency The station frequency
     *
     * @return station The result value
     */
    public static int computeStation(float frequency) {
        return (int) (frequency * CONVERT_RATE);
    }
}
