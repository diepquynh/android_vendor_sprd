/**
 *   Copyright (C) 2010,2011 Thundersoft Corporation
 *   All rights Reserved
 */

package com.ucamera.ugallery.preference;

import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.net.Uri;
import android.text.SpannableStringBuilder;
import android.text.method.LinkMovementMethod;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.TextView;
import android.widget.Toast;

import com.ucamera.ugallery.R;
import com.ucamera.ugallery.util.Util;

public class MyAboutusDialogStub extends MyDialogStub {

    @Override
    public int getContentView() {
        return R.layout.settings_aboutus;
    }

    @Override
    protected void onBind() {
        //Version
        ((TextView)mContext.findViewById(R.id.tv_app_version)).setText(getAppVersion());
        //FAQ
        ((TextView)mContext.findViewById(R.id.aboutus_faq)).setMovementMethod(LinkMovementMethod.getInstance());
        //User manual
        ((TextView)mContext.findViewById(R.id.aboutus_user_manual)).setMovementMethod(LinkMovementMethod.getInstance());
        //Support Email
        /*
         * FIX BUG: 1377 1849
         * BUG CAUSE: Email application is not installed.
         * FIX COMMENT: Hint user instead of crashing UCam.
         * DATE: 2012-07-31
         */
        mContext.findViewById(R.id.aboutus_support).setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                try {
                    mContext.startActivity(new Intent().
                            setAction(Intent.ACTION_SENDTO).
                            setData(Uri.parse("mailto:support@u-camera.com")));
                } catch (ActivityNotFoundException e) {
                    Toast.makeText(mContext, R.string.text_activity_is_not_found, Toast.LENGTH_SHORT).show();
                }
            }
        });
        //CopyRight
        ((TextView)mContext.findViewById(R.id.aboutus_copyright)).setMovementMethod(LinkMovementMethod.getInstance());
    }

    private CharSequence getAppVersion() {
        return new SpannableStringBuilder().append("UGallery ").append(Util.getPackageVersion(mContext));
    }
}
