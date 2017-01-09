package com.android.phone;

import com.android.internal.telephony.Phone;
import com.android.internal.telephony.gsm.SuppServiceNotification;

import android.content.Context;
import android.content.Intent;
import android.os.AsyncResult;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.widget.Toast;
import com.android.phone.R;

public class SuppServiceConsumer extends Handler {
    private static final String TAG = "SuppServiceConsumer";
    private static final int MO_CALL = 0;
    private static final int MT_CALL = 1;

    protected static final int EVENT_SSN = 1;
    protected static final int MSG_SUPP_SERVICE_FAILED = 2;
    // SPRD: Porting dealing with SS notification. Same defined in InCallUI.
    private static final String ACTION_SUPP_SERVICE_FAILURE =
            "org.codeaurora.ACTION_SUPP_SERVICE_FAILURE";

    private Context mContext;
    private static SuppServiceConsumer mInstance;

    // TODO Register this handler to gsmphone.
    private SuppServiceConsumer(Context context) {
        mContext = context;
    }

    public static SuppServiceConsumer getInstance(Context context, Phone phone) {
        if (mInstance == null) {
            mInstance = new SuppServiceConsumer(context);
        }
        Log.d(TAG, "getInstance");
        phone.registerForSuppServiceNotification(mInstance, EVENT_SSN, null);
        // it can not handle the old one still or the activity will be not
        // release
        phone.registerForSuppServiceFailed(mInstance, MSG_SUPP_SERVICE_FAILED, null);
        mInstance.setContext(context);
        return mInstance;
    }

    private void setContext(Context context) {
        mContext = context;
    }

    @Override
    public void handleMessage(Message msg) {
        // TODO Auto-generated method stub
        AsyncResult ar;
        Log.d(TAG, "handleMessage msg.what = " + msg.what);
        switch (msg.what) {
        case EVENT_SSN:
            ar = (AsyncResult)msg.obj;
            CharSequence cs = null;
            SuppServiceNotification not = (SuppServiceNotification) ar.result;
            if (not.notificationType == MO_CALL) {
                switch(not.code) {
                    case SuppServiceNotification.MO_CODE_UNCONDITIONAL_CF_ACTIVE:
                    cs = mContext.getString(R.string.ActiveUnconCf);
                    break;
                    case SuppServiceNotification.MO_CODE_SOME_CF_ACTIVE:
                    cs = mContext.getString(R.string.ActiveConCf);
                    break;
                    case SuppServiceNotification.MO_CODE_CALL_FORWARDED:
                    cs = mContext.getString(R.string.CallForwarded);
                    break;
                    case SuppServiceNotification.MO_CODE_CALL_IS_WAITING:
                    cs = mContext.getString(R.string.CallWaiting);
                    break;
                    case SuppServiceNotification.MO_CODE_OUTGOING_CALLS_BARRED:
                    cs = mContext.getString(R.string.OutCallBarred);
                    break;
                    case SuppServiceNotification.MO_CODE_INCOMING_CALLS_BARRED:
                    cs = mContext.getString(R.string.InCallBarred);
                    break;
                    //case SuppServiceNotification.MO_CODE_CLIR_SUPPRESSION_REJECTED:
                    //cs = mContext.getText(com.android.internal.R.string.ClirRejected);
                    //break;
                }
            } else if (not.notificationType == MT_CALL) {
                switch(not.code) {
                    case SuppServiceNotification.MT_CODE_FORWARDED_CALL:
                    cs = mContext.getString(R.string.ForwardedCall);
                    break;
                    /* case SuppServiceNotification.MT_CODE_CUG_CALL:
                    cs = mContext.getText(com.android.internal.R.string.CugCall);
                    break;*/
                    //Fix Bug 4182 phone_01
                    case SuppServiceNotification.MT_CODE_CALL_ON_HOLD:
                    cs = mContext.getString(R.string.CallHold);
                    break;
                    case SuppServiceNotification.MT_CODE_CALL_RETRIEVED:
                    cs = mContext.getString(R.string.CallRetrieved);
                    break;
                    /*case SuppServiceNotification.MT_CODE_MULTI_PARTY_CALL:
                    cs = mContext.getText(R.string.MultiCall);
                    break;
                    case SuppServiceNotification.MT_CODE_ON_HOLD_CALL_RELEASED:
                    cs = mContext.getText(R.string.HoldCallReleased);
                    break;
                    case SuppServiceNotification.MT_CODE_CALL_CONNECTING_ECT:
                    cs = mContext.getText(R.string.ConnectingEct);
                    break;
                    case SuppServiceNotification.MT_CODE_CALL_CONNECTED_ECT:
                    cs = mContext.getText(R.string.ConnectedEct);
                    break;*/
                    case SuppServiceNotification.MT_CODE_ADDITIONAL_CALL_FORWARDED:
                    cs = mContext.getString(R.string.IncomingCallForwarded);
                    break;
                }
            }
            if (cs!=null) {
                Toast.makeText(mContext, cs, Toast.LENGTH_LONG).show();
            }
            break;
        case MSG_SUPP_SERVICE_FAILED:
            if (mContext != null) {
                AsyncResult r = (AsyncResult) msg.obj;
                Phone.SuppService service = (Phone.SuppService) r.result;
                int val = service.ordinal();
                Intent intent = new Intent();
                intent.setAction(ACTION_SUPP_SERVICE_FAILURE);
                intent.putExtra("supp_serv_failure", val);
                mContext.sendBroadcast(intent);
            }
            break;
        }
        super.handleMessage(msg);
    }
}
