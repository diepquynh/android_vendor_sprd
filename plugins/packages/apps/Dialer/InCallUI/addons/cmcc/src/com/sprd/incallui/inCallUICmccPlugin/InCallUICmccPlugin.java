package com.sprd.incallui.inCallUICmccPlugin;

import android.app.AddonManager;
import android.content.Context;
import android.util.Log;
import android.widget.TextView;
import android.widget.Toast;
import android.content.Intent;
import android.net.Uri;

import com.android.incallui.InCallPresenter;
import com.android.incallui.InCallPresenter.InCallState;
import com.android.phone.common.animation.AnimUtils;
import com.android.sprd.incallui.InCallUICmccHelper;

import com.android.incallui.Call;
import com.android.incallui.CallList;

/**
 * This class is used to manager InCallUI CMCC Plugin
 */
public class InCallUICmccPlugin extends InCallUICmccHelper implements AddonManager.InitialCallback {
    private Context mContext;
    private static final String TAG = "InCallUICmccPlugin";
    private static final int MAX_CONFERENCE_NUMS = 5;

    private int mConferenceSize;
    private Call mBackgroundCall;
    private String mEmergencyNumber = ""; // Show emergency number when dial emergency call

    public InCallUICmccPlugin() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        log("clazz: " + clazz);
        mContext = context;
        return clazz;
    }

    /* duration time should not fadeout when hangup call. */
    public void setPrimaryCallElapsedTime(TextView elapsedTimeT) {
        log("setPrimaryCallElapsedTime");
        final InCallState newState = InCallPresenter.getInstance().getInCallState();
        if (newState != InCallState.INCALL) {
            AnimUtils.fadeOut(elapsedTimeT, -1);
        }
    }

    /* SPRD: Toast information when the number of conference call is over limit for cmcc case @{ */
    public boolean showMergeButton(Call call) {
        if (call.getChildCallIds() == null || InCallPresenter.getInstance().getCallList() == null) {
            log("showMergeButton null error");
            return false;
        }
        mBackgroundCall = InCallPresenter.getInstance().getCallList().getBackgroundCall();

        if (call.isConferenceCall()) {
            mConferenceSize = call.getChildCallIds().size();
            log("foreground conference call size == " + mConferenceSize);
        } else if (mBackgroundCall != null) {
            mConferenceSize = mBackgroundCall.getChildCallIds().size();
            log("background conference call size == " + mConferenceSize);
        } else {
                mConferenceSize = 0;
        }

        return mConferenceSize < MAX_CONFERENCE_NUMS ? call.can(
                android.telecom.Call.Details.CAPABILITY_MERGE_CONFERENCE) :
                mBackgroundCall != null;
    }

    public void showToast(Context context) {
        if (mConferenceSize == MAX_CONFERENCE_NUMS) {
            Toast.makeText(context, mContext.getString(R.string.exceed_limit),
                    Toast.LENGTH_SHORT).show();
        }
    }

    public boolean isSupportClickMergeButton() {
        return !(mConferenceSize == MAX_CONFERENCE_NUMS);
    }
    /* @} */

    /* SPRD: Enable send sms on InCallUI for cmcc case @{ */
    @Override
    public void sendSms(Context context, Call call, CallList callList) {
        String multicallnNumberList = "";
        Uri uri = null;
        String[] numberArray = CallList.getInstance().getConferenceCallNumberArray();

        if (!callList.hasValidGroupCall() || callList.getAllConferenceCall() == null) {
            uri = Uri.parse("smsto:" + call.getNumber());
            log("Send sms.");
        } else {
            for (int i = 0; i < numberArray.length; i++) {
                multicallnNumberList += numberArray[i] + ",";
            }
            uri = Uri.parse("smsto:" + multicallnNumberList);
            log("Send sms when multi call.");
        }

        Intent intent = new Intent(Intent.ACTION_SENDTO, uri);
        context.startActivity(intent);
    }

    @Override
    public boolean isSupportSendSms() {
        return true;
    }
    /* @} */

    /* Show emergency number when dial emergency call feature for cmcc case @{ */
    public void setEmergencyNumber(String number) {
        mEmergencyNumber = number;
    }

    public String getEmergencyNumber() {
        log("Display emergency number =" + mEmergencyNumber);
        return mEmergencyNumber;
    }
    /* @} */

    private static void log(String msg) {
        Log.d(TAG, msg);
    }

    /* SPRD: Add for CMCC requirement bug574817 */
    public boolean shouldDisplayUpdateToVideo() {
        log("shouldDisplayUpdateToVideo");
        CallList calllist = CallList.getInstance();
        if (calllist != null) {
            return calllist.getBackgroundCall() == null;
        }
        log("shouldDisplayUpdateToVideo true");
        return true;
    }
    /* @} */
    /* SPRD: Not show hold on button bug 632499 */
    public boolean showHoldOnButton() {
        log("showHoldOnButton false");
        return false;
    }

    /* SPRD:fix for bug639207 not show add call button when call is video */
    public boolean showAddCallButton(){
        log("showAddCallButton false");
        return false;
    }
    /* @} */

}
