package com.sprd.incallui.inCallUICuccPlugin;

import java.util.List;

import android.app.AddonManager;
import android.content.Context;
import android.telecom.PhoneAccount;
import android.telecom.PhoneAccountHandle;
import android.telecom.Call.Details;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.util.Log;
import android.view.View;
import android.widget.ImageView;
import com.android.incallui.Call;
import com.android.incallui.CallList;
import com.android.internal.telephony.TelephonyIntentsEx;
import android.os.AsyncTask;
import android.os.UserHandle;
import android.content.Intent;
import android.graphics.drawable.Drawable;
import com.android.sprd.telephony.RadioInteractor;
import com.android.sprd.incallui.InCallUICuccHelper;
import com.android.sprd.incallui.InCallUiUtils;
/**
 * Various utilities for dealing with phone number strings.
 */
public class InCallUICuccPlugin extends InCallUICuccHelper implements AddonManager.InitialCallback
{
    private Context mAddonContext;
    private static final String TAG = "[CUCC::InCallUICuccPlugin]";
    private boolean mHdVoiceState = false;
    private boolean mVolteHdVoiceState = false;

    public InCallUICuccPlugin() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        log("clazz: " + clazz);
        mAddonContext = context;
        return clazz;
    }

    public Drawable getCallStateIcon(Context context) {
        log("getCallStateIcon mVolteHdVoiceState = " + mVolteHdVoiceState + " mHdVoiceState = " + mHdVoiceState) ;
        // Return high definition audio icon if the capability is indicated.
        if (mVolteHdVoiceState || mHdVoiceState) {
            return mAddonContext.getDrawable(R.drawable.ic_hd_voice_audio);
        }
        return super.getCallStateIcon(context);
    }

    public void showHdAudioIndicator(ImageView view, Call call, Context context) {
        if (call == null) {
            return;
        }
        log("showHdAudioIndicator") ;
        int slotId = InCallUiUtils.getPhoneIdByAccountHandle(context, call.getAccountHandle());

        mVolteHdVoiceState = (call != null && call.hasProperty(Details.PROPERTY_HIGH_DEF_AUDIO));
        HdVoiceAsyncTask task = new HdVoiceAsyncTask(context, view, slotId);
        task.execute();
    }

    public void removeHdVoiceIcon (Context context) {
        mHdVoiceState = false;
        log("removeHdVoiceIcon mHdVoiceState = " + mHdVoiceState);
        Intent intent = new Intent(TelephonyIntentsEx.ACTION_HIGH_DEF_AUDIO_SUPPORT);
        intent.putExtra("isHdVoiceSupport", mHdVoiceState);
        context.sendBroadcast(intent);
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
            Intent intent = new Intent(TelephonyIntentsEx.ACTION_HIGH_DEF_AUDIO_SUPPORT);
            intent.putExtra(TelephonyIntentsEx.EXTRA_HIGH_DEF_AUDIO, (mHdVoiceState || mVolteHdVoiceState));
            context.sendBroadcast(intent);
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
