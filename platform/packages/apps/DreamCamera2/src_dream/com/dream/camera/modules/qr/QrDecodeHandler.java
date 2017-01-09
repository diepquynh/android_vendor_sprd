/*
 * Copyright (C) 2010 ZXing authors
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

package com.dream.camera.modules.qr;

import java.util.Hashtable;

import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.graphics.Rect;
import android.graphics.Bitmap;
import android.graphics.YuvImage;
import android.graphics.ImageFormat;
import android.graphics.BitmapFactory;
import java.io.ByteArrayOutputStream;

import com.dream.camera.modules.qr.QrCodePhotoModule;
import com.android.camera2.R;
import com.google.zxing.BinaryBitmap;
import com.google.zxing.DecodeHintType;
import com.google.zxing.MultiFormatReader;
import com.google.zxing.ReaderException;
import com.google.zxing.Result;
import com.google.zxing.common.HybridBinarizer;
import com.dream.camera.modules.qr.CameraManager;
import com.dream.camera.modules.qr.PlanarYUVLuminanceSource;

final class QrDecodeHandler extends Handler {

    private static final String TAG = QrDecodeHandler.class.getSimpleName();

    private final QrCodePhotoModule activity;
    private final MultiFormatReader multiFormatReader;
    private byte[] mYuvData = null;
    private int mPreviewWidth = 0;
    private int mPreviewHeight = 0;

    QrDecodeHandler(QrCodePhotoModule activity, Hashtable<DecodeHintType, Object> hints) {
        multiFormatReader = new MultiFormatReader();
        multiFormatReader.setHints(hints);
        this.activity = activity;
    }

    private void setYuvDataForFreeze(byte[] data, int width, int height) {
        synchronized (this) {
            if (mYuvData == null || (mYuvData.length != data.length)) {
                mYuvData = new byte[data.length];
            }
            System.arraycopy(data, 0, mYuvData, 0, data.length);
            mPreviewWidth = width;
            mPreviewHeight = height;
        }
    }

    public Bitmap getPreviewBitmap(Rect previewRect) {
        long startTime = System.currentTimeMillis();

        if (mYuvData == null)
            return null;

        Rect rect = null;
        if (previewRect == null) {
            rect = new Rect(0, 0, mPreviewWidth, mPreviewHeight);
        } else {
            rect = previewRect;
            mPreviewWidth = rect.right - rect.left;
            mPreviewHeight = rect.bottom - rect.top;
        }

        Log.i(TAG, "mPreviewWidth = " + mPreviewWidth + " mPreviewHeight = " + mPreviewHeight);
        ByteArrayOutputStream os = new ByteArrayOutputStream(mYuvData.length);

        Bitmap bitmap = null;
        YuvImage yuvImage = new YuvImage(mYuvData, ImageFormat.NV21, mPreviewWidth, mPreviewHeight,null);

        try {
            yuvImage.compressToJpeg(rect, 90, os);
            bitmap = BitmapFactory.decodeByteArray(os.toByteArray(), 0, os.toByteArray().length);
            os.close();
            Log.i(TAG, "getBitmap cost: " + (System.currentTimeMillis() - startTime));
        } catch (Exception e) {
            Log.i(TAG, "catch exception " + e.getMessage());
        }

        return bitmap;
    }

    @Override
    public void handleMessage(Message message) {
        switch (message.what) {
            case R.id.decode:
                decode((byte[]) message.obj, message.arg1, message.arg2);
                break;
            case R.id.quit:
                Looper.myLooper().quit();
                break;
        }
    }

    /**
     * Decode the data within the viewfinder rectangle, and time how long it took. For efficiency,
     * reuse the same reader objects from one decode to the next.
     *
     * @param data The YUV preview frame.
     * @param width The width of the preview frame.
     * @param height The height of the preview frame.
     */
    private void decode(byte[] data, int width, int height) {
        setYuvDataForFreeze(data, width, height);
        long start = System.currentTimeMillis();
        Result rawResult = null;

        byte[] rotatedData = new byte[data.length];
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++)
                rotatedData[x * height + height - y - 1] = data[x + y * width];
        }
        int tmp = width;
        width = height;
        height = tmp;
        PlanarYUVLuminanceSource source = CameraManager.get().buildLuminanceSource(rotatedData,
                width, height);
        BinaryBitmap bitmap = new BinaryBitmap(new HybridBinarizer(source));
        try {
            rawResult = multiFormatReader.decodeWithState(bitmap);
        } catch (ReaderException re) {
        } finally {
            multiFormatReader.reset();
        }

        if (rawResult != null) {
            long end = System.currentTimeMillis();
            Log.d(TAG, "Found barcode (" + (end - start) + " ms):\n" + rawResult.toString());
            Message message = Message.obtain(activity.getHandler(), R.id.decode_succeeded,
                    rawResult);
            Bundle bundle = new Bundle();
            bundle.putParcelable(QrDecodeThread.BARCODE_BITMAP,
                    source.renderCroppedGreyscaleBitmap());
            message.setData(bundle);
            message.sendToTarget();
        } else {
            Message message = Message.obtain(activity.getHandler(), R.id.decode_failed);
            message.sendToTarget();
        }
    }

}
