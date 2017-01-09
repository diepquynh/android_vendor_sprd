/*
 * Copyright (C) 2008 ZXing authors
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
import java.util.Vector;
import java.util.concurrent.CountDownLatch;

import android.os.Handler;
import android.os.Looper;
import com.dream.camera.modules.qr.QrCodePhotoModule;

import com.google.zxing.BarcodeFormat;
import com.google.zxing.DecodeHintType;
import com.google.zxing.ResultPointCallback;

/**
 * This thread does all the heavy lifting of decoding the images.
 */
final class QrDecodeThread extends Thread {

    public static final String BARCODE_BITMAP = "barcode_bitmap";
    private final QrCodePhotoModule activity;
    private final Hashtable<DecodeHintType, Object> hints;
    private Handler handler;
    private final CountDownLatch handlerInitLatch;

    QrDecodeThread(QrCodePhotoModule activity,
            Vector<BarcodeFormat> decodeFormats,
            String characterSet,
            ResultPointCallback resultPointCallback) {

        this.activity = activity;
        handlerInitLatch = new CountDownLatch(1);

        hints = new Hashtable<DecodeHintType,Object>();
        Vector<BarcodeFormat> formats = new Vector<BarcodeFormat>();
        formats.add(BarcodeFormat.QR_CODE);
        hints.put(DecodeHintType.POSSIBLE_FORMATS, formats);

        if (characterSet != null) {
            hints.put(DecodeHintType.CHARACTER_SET, characterSet);
        }

        hints.put(DecodeHintType.NEED_RESULT_POINT_CALLBACK, resultPointCallback);
    }

    Handler getHandler() {
        try {
            handlerInitLatch.await();
        } catch (InterruptedException ie) {
         // continue?
        }
        return handler;
    }

    @Override
    public void run() {
        Looper.prepare();
        handler = new QrDecodeHandler(activity, hints);
        handlerInitLatch.countDown();
        Looper.loop();
    }

}
