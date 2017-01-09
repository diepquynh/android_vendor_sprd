
package com.sprd.cmccvideoplugin;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Handler;
import android.os.Message;
import android.util.Log;

import com.android.gallery3d.app.MoviePlayer;

public class CmccMessagingUtils {

    private static final String TAG = "CmccMessagingUtils";

    /** SPRD:Bug 474641 add reciver MMS&SMS Control @{ */
    private static final String TRANSACTION_COMPLETED_ACTION = "android.intent.action.TRANSACTION_COMPLETED_ACTION";
    private static final String SMS_RECEIVED = "android.provider.Telephony.SMS_RECEIVED";
    private static final int MOVIEVIEW_END_MMS_CONNECTIVITY = 4;
    private static final int MOVIEVIEW_SMS_RECEIVED = 7;
    private final Handler mHandler = new MovePlayerHandler();
    private AlertDialog mMMSCompletedDialog;
    private AlertDialog mSMSReceivedDialog;
    private MessageBroadcastReceiver mMessageReceiver;
    private Context mPluginContext;
    private Activity mActivity;
    private MoviePlayer mPlayer;
    private static CmccMessagingUtils mInstance;
    /** @} */

    public static CmccMessagingUtils getInstance(){
        if(mInstance == null){
            mInstance = new CmccMessagingUtils();
        }
        return mInstance;
    }

    public void initMessagingUtils(final Activity activity, Context context) {
        mPluginContext = context;
        mActivity = activity;
        if(mMessageReceiver == null){
            mMessageReceiver = new MessageBroadcastReceiver();
        }
        IntentFilter messageFilter = new IntentFilter();
        /** SPRD:Bug 474641 add reciver MMS&SMS Control @{ */
        messageFilter.addAction(TRANSACTION_COMPLETED_ACTION);
        messageFilter.addAction(SMS_RECEIVED);
        /** @} */
        Log.d(TAG,"initMessagingUtils regist broadcast");
        mActivity.registerReceiver(mMessageReceiver, messageFilter);
    }

    public void releaseMessagingUtils() {
        /** SPRD:Bug 474641 add reciver MMS&SMS Control @{ */
        if (mMessageReceiver != null) {
            mActivity.unregisterReceiver(mMessageReceiver);
        }
        /** @} */
    }

    public void initPlayer(MoviePlayer player){
        mPlayer = player;
    }

    public void destoryMessagingDialog() {
        /** SPRD:Bug 474641 add reciver MMS&SMS Control @{ */
        if (mMMSCompletedDialog != null) {
            mMMSCompletedDialog.cancel();
            mMMSCompletedDialog = null;
        }
        if (mSMSReceivedDialog != null) {
            mSMSReceivedDialog.cancel();
            mSMSReceivedDialog = null;
        }
    }

    private AlertDialog initCompleteDialog(int flag) {
        int titleId = flag == MOVIEVIEW_END_MMS_CONNECTIVITY ?
                R.string.movie_view_mms_process_title : R.string.movie_view_sms_process_title;
        return new AlertDialog.Builder(mActivity)
                .setTitle(mPluginContext.getText(titleId))
                .setCancelable(false)
                .setPositiveButton(mPluginContext.getText(R.string.movie_view_mms_ok),
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog,
                                    int which) {
                                dialog.dismiss();
                                mPlayer.onPlayPause();
                            }
                        }).create();
    }

    /** @} */

    private class MovePlayerHandler extends Handler {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MOVIEVIEW_END_MMS_CONNECTIVITY:
                    if (mMMSCompletedDialog == null) {
                        mMMSCompletedDialog = initCompleteDialog(MOVIEVIEW_END_MMS_CONNECTIVITY);
                    } else {
                        mMMSCompletedDialog.dismiss();
                    }
                    pauseIfNeed();
                    mMMSCompletedDialog.setMessage(mPluginContext.getText(R.string.movie_view_mms_processing_ok));
                    mMMSCompletedDialog.show();
                    break;
                case MOVIEVIEW_SMS_RECEIVED:
                    if (mSMSReceivedDialog == null) {
                        mSMSReceivedDialog = initCompleteDialog(MOVIEVIEW_SMS_RECEIVED);
                    } else {
                        mSMSReceivedDialog.dismiss();
                    }
                    pauseIfNeed();
                    mSMSReceivedDialog.setMessage(mPluginContext.getText(R.string.movie_view_sms_received_ok));
                    mSMSReceivedDialog.show();
                    break;
                default:
                    break;

            }
        }
    }

    private class MessageBroadcastReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            Log.e(TAG, "MessageBroadcastReceiver onReceive " + intent);
            if (mPlayer == null || mHandler == null || !mPlayer.isStreamUri())
                return;

            if (mPlayer.misLiveStreamUri) {
                Log.d(TAG, "When mms receiving, can't stop live streaming");
                return;
            }
            if (intent.getAction().equals(TRANSACTION_COMPLETED_ACTION)) {
                Message handldermsg = mHandler.obtainMessage();
                handldermsg.what = MOVIEVIEW_END_MMS_CONNECTIVITY;
                handldermsg.arg1 = intent.getIntExtra("state", 0);
                mHandler.sendMessage(handldermsg);
            } else if (intent.getAction().equals(SMS_RECEIVED)) {
                Message handldermsg = mHandler.obtainMessage();
                handldermsg.what = MOVIEVIEW_SMS_RECEIVED;
                mHandler.sendMessage(handldermsg);
            }
        }
    }

    private boolean pauseIfNeed() {
        if (mPlayer == null)
            return false;
        if (!mPlayer.isPlaying()) {
            mPlayer.pause();
            return false;
        }
        mPlayer.onPlayPause();
        return true;
    }
}
