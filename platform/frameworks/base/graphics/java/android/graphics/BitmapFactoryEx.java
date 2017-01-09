/*
 * Copyright (C) 2007 The Android Open Source Project
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

package android.graphics;

import android.drm.DrmManagerClientEx;
import android.drm.DecryptHandle;
import android.util.Log;

/**
 * Creates Bitmap objects from various sources, including files, streams,
 * and byte-arrays.
 * @hide
 */
public class BitmapFactoryEx {
    private static final String TAG = "BitmapFactoryEx";

    public static Bitmap decodeDrmStream(DrmManagerClientEx client, DecryptHandle handle, BitmapFactory.Options opts) {
        Bitmap bm = null;
        try {
            if (opts == null || (opts.inScaled && opts.inBitmap == null)) {
                float scale = 1.0f;
                int targetDensity = 0;
                if (opts != null) {
                    final int density = opts.inDensity;
                    targetDensity = opts.inTargetDensity;
                    if (density != 0 && targetDensity != 0) {
                        scale = targetDensity / (float) density;
                    }
                }

                bm = nativeDecodeDrmStream(client, handle, opts, true, scale);
                if (bm != null && targetDensity != 0) bm.setDensity(targetDensity);
            } else {
                bm = nativeDecodeDrmStream(client, handle, opts, false, 0);
        }
        return bm;
        } catch (Exception e) {
            Log.e(TAG, "decodeDrmStream: failed!", e);
        }
        return bm;
    }

    private static native Bitmap nativeDecodeDrmStream(DrmManagerClientEx client, DecryptHandle handle, BitmapFactory.Options opts, boolean applyScale, float scale);
}
