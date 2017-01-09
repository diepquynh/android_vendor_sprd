package com.sprd.autoauth;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import android.webkit.WebView;
import android.webkit.WebSettings;
import java.lang.reflect.Method;

public class AutoAuthReceiver extends BroadcastReceiver {
    private static final String TAG = "AutoAuthReceiver";

    public AutoAuthReceiver() {
    }
    WebView mWebView;
    @Override
    public void onReceive(Context context, Intent intent) {
        Log.d(TAG, "onReceive ction=" + intent.getAction());
        if (mWebView == null){
            mWebView = new WebView(context);
            mWebView.getSettings().setJavaScriptEnabled(true);
            mWebView.getSettings().setCacheMode(WebSettings.LOAD_NO_CACHE);
        }
        String url = "http://www.baidu.com";
        try {
            Method m = WebView.class.getMethod("doAutoAuthentication",String.class);
            m.invoke(mWebView, url);
        } catch(Exception e) {
            Log.i(TAG, "no doAutoAuthentication in webview");
        }
    }
}
