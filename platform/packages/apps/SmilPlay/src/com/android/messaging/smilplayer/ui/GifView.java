/*
 * Copyright (c) 2012 Thunder Software Technology Co.,Ltd.
 * All Rights Reserved.
 * Thundersoft Confidential and Proprietary.
 * Developed by Thundersoft Engineering team.
 */
package com.android.messaging.smilplayer.ui;

//import com.android.mms.ui.MessageUtils;
import com.android.messaging.smilplayer.ui.DecodeGif.GifFrame;

import android.net.Uri;
import android.os.Handler;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RectF;

/**
 * @author owen
 */
public class GifView extends View implements Runnable {
    private DecodeGif.GifFrame mGifFrame;

    private DecodeGif.GifFrame mGifFrameReserved;

    private DecodeGif mDecoder;
    private Handler mHandler;

    private static final String TAG = "GifPlayerActivity_GifView";

    private boolean isOsDebug(){
        return true;
    }

    /**
     * @param context context
     * @param uri
     * @param handler handler
     */
    public GifView(Context context, Uri uri, Handler handler) {
        super(context);
        if (isOsDebug()) {
            Log.d("=====GifView====", "uri : " + uri);
        }
        mHandler = handler;
        mDecoder = new DecodeGif(context, uri);
        new Thread(new Runnable() {

            @Override
            public void run() {
                mDecoder.init();
                mDecoder.loadGif();
            }
        }).start();

        new Thread(this).start();
    }

    /**
     * @param context context
     * @param aSet aSet
     */
    public GifView(Context context, AttributeSet aSet) {
        super(context, aSet);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        if (bExit) {
            return;
        }

        if (mGifFrame == null) {
            mGifFrame = mDecoder.getFirstFrame();
        }

        if (mGifFrame == null) {
            //maybe the data not be loaded yet, retry at 1s later
            return;
        }

        try {
            resizeBitmap(canvas);
            mGifFrameReserved = mGifFrame;
            mGifFrame = mDecoder.next();

            if (mGifFrame == null) {
                mGifFrame = mGifFrameReserved;
            }
        } catch (NullPointerException e) {
            e.printStackTrace();
//            if (!bExit) {
//                mHandler.obtainMessage();
//                mHandler.sendEmptyMessage(DisplayGifActivity.MSG_DECODE_GIF_ERROR);
//            }
        } catch (Exception e) {
            e.printStackTrace();
//            if (!bExit) {
//                mHandler.obtainMessage();
//                mHandler.sendEmptyMessage(DisplayGifActivity.MSG_DECODE_GIF_ERROR);
//            }
        }
    }

    protected void resizeBitmap(Canvas canvas) {
        if (isOsDebug()) {
        Log.d(TAG, "resizeBitmap  mGifFrame:" + mGifFrame + " image:" + mGifFrame.image);
        }
        float gifWidth = mGifFrame.image.getWidth();
        float gifHeight = mGifFrame.image.getHeight();
        float windowWidth = getWidth();
        float windowHeight = getHeight();
        float scaleWidth;
        float scaleHight;
        float left;
        float top;
        double scale;

        if (gifWidth > windowWidth || gifHeight > windowHeight) {
            if ((gifWidth / windowWidth) <= (gifHeight / windowHeight)) {
                scale = gifHeight / windowHeight;
                scaleWidth = (float) (gifWidth / scale);
                left = (windowWidth - scaleWidth) / 2;
                scaleWidth = left + scaleWidth;
                canvas.drawBitmap(mGifFrame.image, null, new RectF(left, 0, scaleWidth,
                        windowHeight), new Paint());
            } else {
                scale = gifWidth / windowWidth;
                scaleHight = (float) (gifHeight / scale);
                top = (windowHeight - scaleHight) / 2;
                scaleHight = top + scaleHight;
                canvas.drawBitmap(mGifFrame.image, null,
                        new RectF(0, top, windowWidth, scaleHight), new Paint());
            }
        } else {
            top = (windowHeight - gifHeight) / 2;
            left = (windowWidth - gifWidth) / 2;
            canvas.drawBitmap(mGifFrame.image, left, top, new Paint());
        }
    }

    private boolean bExit = false;

    public void run() {
        if (isOsDebug()) {
        Log.d(TAG, "run() enter: bExit = " + bExit);
        }
        while (!bExit) {
            try {
                this.postInvalidate();
                if (mGifFrame == null || mGifFrame.delay < 0 || mGifFrame.delay == 0) {
                    Thread.sleep(100);
                } else {
                    Thread.sleep((long) mGifFrame.delay);
                }
            } catch (InterruptedException e) {
                e.printStackTrace();

            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    public void stopPlayGif() {
        bExit = true;

        //notify gif decode thread to exit
        mDecoder.setbExit(true);
    }
}
