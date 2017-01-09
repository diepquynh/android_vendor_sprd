package com.sprd.xposed;

import java.util.List;

import android.app.AddonManager;
import android.content.Context;
import android.net.Uri;

import com.android.mail.R;
import com.android.mail.compose.AttachmentsView;
import com.android.mail.compose.ComposeActivity;
import com.android.mail.providers.Attachment;
import com.android.mail.providers.Account;

public class DrmXposedUtils {
    static DrmXposedUtils sInstance;

    public static DrmXposedUtils getInstance() {
        if (sInstance != null)
            return sInstance;
        sInstance = (DrmXposedUtils) AddonManager.getDefault().getAddon(
                R.string.feature_drm, DrmXposedUtils.class);
        return sInstance;
    }

    public DrmXposedUtils() {
    }

    public boolean isSDFile(Context context, Uri uri) {
        return true;
    }

    public boolean isDrmFile(String name, String contentType) {
        return false;
    }

    public boolean canOpenDrm(Context context, String name, String contentType) {
        return true;
    }

    public boolean drmPluginEnabled() {
        return false;
    }

    public long addAttachmentsXposed(ComposeActivity composeActivity,
            List<Attachment> attachments, AttachmentsView attachmentsView,
            Account account) {
        return 0;
    }

    public void sendContext(Context context) {
    }
}