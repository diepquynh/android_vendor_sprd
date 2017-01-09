/*
 * Copyright (C) 2010,2013 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.ugallery;

import android.app.Activity;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.widget.Toast;

import com.ucamera.ugallery.gif.GifView;

public class GifPlayerActivity extends Activity {
    private static final String TAG = "GifPlayerActivity";

    private GifView mGifView = null;

    private boolean isFirstShow = true;

    /**
     * MSG_DECODE_GIF_ERROR
     */
    public static final int MSG_DECODE_GIF_ERROR = 0x2000;

    /**
     * mHandler
     */
    public Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_DECODE_GIF_ERROR:
                    if(isFirstShow) {
                        Toast.makeText(GifPlayerActivity.this,
                                getString(R.string.text_cannot_play_gif), Toast.LENGTH_SHORT).show();
                        isFirstShow = false;
                    }

                    if(mGifView != null) {
                        mGifView.recycleGifFrame();
                    }
                    finish();
                    break;
                default :
                    break;
            }
        }
    };

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Uri gifUri = getIntent().getData();
        Log.d(TAG, "gifUri: " + gifUri);
        try {
            mGifView = new GifView(this, gifUri, mHandler);
        } catch(OutOfMemoryError e) {
            e.printStackTrace();
        }
        if(mGifView !=null) {
            setContentView(mGifView);
        } else {
          mHandler.obtainMessage();
          mHandler.sendEmptyMessage(MSG_DECODE_GIF_ERROR);
        }
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if(mGifView != null) {
            mGifView.setDestroyed(true);
            mGifView.recycleGifFrame();
        }
    }
}
