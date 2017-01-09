
package com.spreadtrum.ims.imsCmccPlugin;

import android.app.AddonManager;

import android.content.Context;
import com.spreadtrum.ims.ImsCmccHelper;
import android.util.Log;
import android.os.Message;
import com.spreadtrum.ims.ImsRIL;
import com.spreadtrum.ims.ImsCallSessionImpl;

/**
 * This class is used to manager ims CMCC Plugin
 */
public class ImsCmccPlugin extends ImsCmccHelper implements AddonManager.InitialCallback {

    private final String TAG = "ImsCmccPlugin";
    private Context mContext;

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        Log.d(TAG,"clazz: " + clazz);
        mContext = context;
        return clazz;
    }

    public ImsCmccPlugin() {
    }

    public boolean rejectMediaChange(ImsCallSessionImpl imsCallSessionImpl,
            final ImsRIL mCi, Message response) {
        Log.d(TAG, "is Cmcc project...... ");

        if (imsCallSessionImpl.isHasBackgroundCallAndActiveCall()) {

            Log.d(TAG, "reject change voice to video becase 1 hold 1 active");
            response.arg1 = Integer.parseInt(imsCallSessionImpl.getCallId());
            mCi.responseVolteCallMediaChange(false, response);
            return true;
        } else {

            Log.d(TAG, "user choose accept or reject");
            return false;
        }
    }
}
