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

package com.android.fmradio;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.Intent;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.KeyEvent;

public class MediaButtonIntentReceiver extends BroadcastReceiver {

    private static final String TAG = "FmMediaButtonIntentReceiver";
    private static final int MSG_LONGPRESS_TIMEOUT = 1;

    private static long mLastClickTime = 0;
    private static boolean mDown = false;
    private static boolean mLaunched = false;
    public static final String ACTION_FM_HEADSET_PAUSE = "com.android.fmradio.HEADSET.PAUSE";
    public static final String ACTION_FM_HEADSET_NEXT = "com.android.fmradio.HEADSET.NEXT";

    private static Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_LONGPRESS_TIMEOUT:
                    if (!mLaunched) {
                        mLaunched = true;
                    }
                    break;
            }
        }
    };

    @Override
    public void onReceive(Context context, Intent intent) {
        String intentAction = intent.getAction();
        Log.d(TAG, "onReceive intentAction :" + intentAction);
        if (Intent.ACTION_MEDIA_BUTTON.equals(intentAction)) {
            KeyEvent event = (KeyEvent)
                    intent.getParcelableExtra(Intent.EXTRA_KEY_EVENT);
            if (event == null) {
                return;
            }
            int keycode = event.getKeyCode();
            int action = event.getAction();
            long eventtime = event.getEventTime();

            if ((keycode == KeyEvent.KEYCODE_HEADSETHOOK)
                    || (keycode == KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE)) {
                if (action == KeyEvent.ACTION_DOWN) {
                    if (mDown) {
                    } else if (event.getRepeatCount() == 0) {
                        // only consider the first event in a sequence, not the repeat events,
                        // so that we don't trigger in cases where the first event went to
                        // a different app (e.g. when the user ends a phone call by
                        // long pressing the headset button)

                        // The service may or may not be running, but we need to send it
                        // a command.
                        if (eventtime - mLastClickTime < 300) {
                            // double click play next
                            Intent fmIntent = new Intent(ACTION_FM_HEADSET_NEXT);
                            new ContextWrapper(context).sendBroadcast(fmIntent);
                            mLastClickTime = 0;
                        } else {
                            // single click mute/unmute
                            Intent fmIntent = new Intent(ACTION_FM_HEADSET_PAUSE);
                            new ContextWrapper(context).sendBroadcast(fmIntent);
                            mLastClickTime = eventtime;
                        }

                        mLaunched = false;
                        mDown = true;
                    }
                } else {
                    mHandler.removeMessages(MSG_LONGPRESS_TIMEOUT);
                    mDown = false;
                }
                if (isOrderedBroadcast()) {
                    abortBroadcast();
                }
            }
        }
    }
}
