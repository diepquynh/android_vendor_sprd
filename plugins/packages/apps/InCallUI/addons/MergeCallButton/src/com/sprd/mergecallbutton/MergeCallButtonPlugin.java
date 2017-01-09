package com.sprd.mergecallbutton;

import android.app.AddonManager;
import android.content.Context;
import android.widget.Toast;

import android.util.Log;
import com.android.incallui.Call;
import com.android.incallui.InCallPresenter;
import com.android.incallui.ShowMergeOptionUtil;

import java.lang.Class;
import java.lang.Override;

public class MergeCallButtonPlugin extends ShowMergeOptionUtil
                implements AddonManager.InitialCallback {

    private static final String TAG = "MergeButtonPlugin";
    private static final int MAX_CONFERENCE_NUMS = 5;

    private Context mContext;
    private int mConferenceSize;
    private Call mBackgroundCall;

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mContext = context;
        return clazz;
    }

    public void MergeButtonPlugin() {

    }

    public boolean showMergeButton (Call call) {
        if (call.getChildCallIds() == null || InCallPresenter.getInstance().getCallList() == null) {
            Log.d(TAG,"showMergeButton  null error");
            return false;
        }
        mBackgroundCall = InCallPresenter.getInstance().getCallList().getBackgroundCall();

        if (call.isConferenceCall()) {
            mConferenceSize = call.getChildCallIds().size();
            Log.d(TAG," foreground conference call Size == " + mConferenceSize);
        } else if(mBackgroundCall != null)  {
            mConferenceSize = mBackgroundCall.getChildCallIds().size();
            Log.d(TAG," background conference call Size == " + mConferenceSize);
        } else {
            mConferenceSize = 0;
        }

        return mConferenceSize < MAX_CONFERENCE_NUMS ? call.can(
                    android.telecom.Call.Details.CAPABILITY_MERGE_CONFERENCE) :
                    mBackgroundCall != null;
    }

    public void showToast(Context context) {
        if(mConferenceSize == MAX_CONFERENCE_NUMS) {
            Toast.makeText(context, mContext.getString(R.string.exceed_limit),
                    Toast.LENGTH_SHORT).show();
        }
    }

    public boolean shouldBreak() {
        return mConferenceSize == MAX_CONFERENCE_NUMS;
    }
}

