/*
 * Copyright (C) 2006 The Android Open Source Project
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

package com.sprd.incallui.hdvoice;

import android.app.AddonManager;
import android.content.Context;
import android.telecom.PhoneAccount;
import android.telecom.PhoneAccountHandle;
import android.telecom.TelecomManager;
import android.telecom.Call.Details;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.util.Log;
import android.view.View;
import android.widget.ImageView;
import com.android.incallui.Call;
import com.android.internal.telephony.TelephonyIntents;
import android.os.AsyncTask;
import android.os.UserHandle;
import android.content.Intent;
import android.graphics.drawable.Drawable;
import com.android.sprd.telephony.RadioInteractor;
import com.sprd.incallui.InCallUIHdVoicePlugin;
/**
 * Various utilities for dealing with phone number strings.
 */
public class AddonInCallUIHdVoicePlugin extends InCallUIHdVoicePlugin implements AddonManager.InitialCallback
{
    private Context mAddonContext;
    private static final String TAG = "[CUCC::AddonInCallUIHdVoicePlugin]";
    private boolean mHdVoiceState = false;
    private boolean mVolteHdVoiceState = false;
    private TelephonyManager mTelephonyManager;
    private TelecomManager mTelecomManager;

    public AddonInCallUIHdVoicePlugin() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        log("clazz: " + clazz);
        mAddonContext = context;
        mTelephonyManager = (TelephonyManager) mAddonContext
                .getSystemService(Context.TELEPHONY_SERVICE);
        mTelecomManager = (TelecomManager)
                mAddonContext.getSystemService(Context.TELECOM_SERVICE);
        return clazz;
    }

    public Drawable getCallStateIcon(Context context) {
        log("getCallStateIcon mVolteHdVoiceState = " + mVolteHdVoiceState + " mHdVoiceState = " + mHdVoiceState) ;
        // Return high definition audio icon if the capability is indicated.
        if (mVolteHdVoiceState) {
            return super.getCallStateIcon(context);
        } else if (mHdVoiceState) {
            return mAddonContext.getDrawable(R.drawable.ic_hd_voice_audio);
        }
        return super.getCallStateIcon(context);
    }

    public void showHdAudioIndicator(ImageView view, Call call, Context context) {
        if (call == null) {
            return;
        }
        log("showHdAudioIndicator") ;
        PhoneAccount account = mTelecomManager.getPhoneAccount(call.getAccountHandle());
        int subId = mTelephonyManager.getSubIdViaPhoneAccount(account);
        int slotId = SubscriptionManager.getSlotId(subId);

        mVolteHdVoiceState = (call != null && call.hasProperty(Details.PROPERTY_HIGH_DEF_AUDIO));
        HdVoiceAsyncTask task = new HdVoiceAsyncTask(context, view, slotId);
        task.execute();
    }

    public void removeHdVoiceIcon () {
        mHdVoiceState = false;
        log("removeHdVoiceIcon mHdVoiceState = " + mHdVoiceState);
        Intent intent = new Intent(TelephonyIntents.ACTION_HIGH_DEF_AUDIO_SUPPORT);
        intent.putExtra("isHdVoiceSupport", mHdVoiceState);
        mAddonContext.sendStickyBroadcastAsUser(intent, UserHandle.ALL);
    }

    private class HdVoiceAsyncTask extends AsyncTask<Void, Void, Boolean> {
        private Context context;
        private int slotId;
        private ImageView view;

        public HdVoiceAsyncTask(Context context, ImageView view, int id) {
            Log.d("HdVoiceAsyncTask", "new HdVoiceAsyncTask");
            this.context = context;
            this.slotId = id;
            this.view = view;
        }

        @Override
        protected Boolean doInBackground(Void... params) {
            boolean hdVoiceSupport;
            RadioInteractor radioInteractor = new RadioInteractor(context);
            hdVoiceSupport = radioInteractor.queryHdVoiceState(slotId);
            Log.d("HdVoiceAsyncTask", "doInBackground hdVoiceSupport = " + hdVoiceSupport);
            return hdVoiceSupport;
        }

        @Override
        protected void onPostExecute(Boolean result) {
            mHdVoiceState = result;
            Log.d("HdVoiceAsyncTask", "onPostExecute mHdVoiceState = " + mHdVoiceState);
            Intent intent = new Intent(TelephonyIntents.ACTION_HIGH_DEF_AUDIO_SUPPORT);
            intent.putExtra("isHdVoiceSupport", !mVolteHdVoiceState && mHdVoiceState);
            context.sendStickyBroadcastAsUser(intent, UserHandle.ALL);
            boolean shouldShowHdAudioIndicator = mHdVoiceState || mVolteHdVoiceState;
            if (view != null) {
                view.setBackground(getCallStateIcon(context));
                view.setVisibility(shouldShowHdAudioIndicator ? View.VISIBLE : View.GONE);
            }
        }
    }

    private static void log(String msg) {
        Log.d(TAG, msg);
    }
}
