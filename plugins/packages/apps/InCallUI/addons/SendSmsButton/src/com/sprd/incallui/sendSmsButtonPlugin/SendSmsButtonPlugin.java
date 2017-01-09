package com.sprd.incallui.sendSmsButtonPlugin;

import com.android.incallui.Call;
import android.app.AddonManager;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import com.sprd.incallui.SendSmsButtonHelper;

public class SendSmsButtonPlugin extends SendSmsButtonHelper implements
        AddonManager.InitialCallback {

    private static final String TAG = "SendSmsButtonPlugin";
    private Context mContext;

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mContext = context;
        return clazz;
    }

    public void SendSmsButtonPlugin() {
    }

    @Override
    public void sendSms(Context context, Call mCall) {
        Uri uri = Uri.parse("smsto:" + mCall.getNumber());
        Intent intent = new Intent(Intent.ACTION_SENDTO, uri);
        context.startActivity(intent);
    }

    @Override
    public boolean isSupportSendSms() {
        return true;
    }

}
