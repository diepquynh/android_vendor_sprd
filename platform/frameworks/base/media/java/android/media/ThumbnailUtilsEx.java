/*
 * Copyright (C) 2009 The Android Open Source Project
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

package android.media;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.util.Log;

import java.io.FileInputStream;
import java.io.FileDescriptor;
import java.io.IOException;

import android.drm.DecryptHandle;
import android.drm.DrmManagerClientEx;
import android.graphics.BitmapFactoryEx;

public class ThumbnailUtilsEx extends ThumbnailUtils {
    private static final String TAG = "ThumbnailUtilsEx";

    private static DrmManagerClientEx mDrmClient = null;

    private static DrmManagerClientEx getDrmClient() {
        synchronized (ThumbnailUtils.class) {
        if (null == mDrmClient)
            mDrmClient = new DrmManagerClientEx(null);
        }
        return mDrmClient;
    }

    public static Bitmap createImageThumbnailEx(String filePath, int targetSize, int maxPixels, Bitmap bitmap) {
        boolean isDrm = false;
        try {
            isDrm = MediaFile.isDrmFileType(MediaFile.getFileType(filePath).fileType);
        } catch (NullPointerException e) {
            isDrm = false;
        }

        DrmManagerClientEx client = null;
        DecryptHandle handle = null;

       if (isDrm) {
           client = getDrmClient();
           handle = client.openDecryptSession(filePath);
           if (handle == null) {
               return null;
           }
        }
        FileInputStream stream = null;
            try {
                stream = new FileInputStream(filePath);
                FileDescriptor fd = stream.getFD();
                BitmapFactory.Options options = new BitmapFactory.Options();
                options.inSampleSize = 1;
                options.inJustDecodeBounds = true;
                if (isDrm) {
                    BitmapFactoryEx.decodeDrmStream(client, handle, options);
                } else {
                BitmapFactory.decodeFileDescriptor(fd, null, options);
                }
                if (options.mCancel || options.outWidth == -1
                        || options.outHeight == -1) {
                    return null;
                }

                ThumbnailUtils thumbnailutils = new ThumbnailUtils();
                options.inSampleSize = thumbnailutils.computeSampleSize(
                        options, targetSize, maxPixels);
                options.inJustDecodeBounds = false;

                options.inDither = false;
                options.inPreferredConfig = Bitmap.Config.ARGB_8888;

                if (isDrm) {
                    bitmap = BitmapFactoryEx.decodeDrmStream(client, handle, options);
                } else {
                    bitmap = BitmapFactory.decodeFileDescriptor(fd, null, options);
                }
            } catch (IOException ex) {
                Log.e(TAG, "", ex);
            } catch (OutOfMemoryError oom) {
                Log.e(TAG, "Unable to decode file " + filePath + ". OutOfMemoryError.", oom);
            } finally {
                try {
                   if (stream != null) {
                       stream.close();
                   }
                   if (handle != null) {
                       client.closeDecryptSession(handle);
                   }
               } catch (IOException ex) {
                   Log.e(TAG, "", ex);
               }
         }
         return bitmap;
    }
}
