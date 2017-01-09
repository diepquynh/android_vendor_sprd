package com.sprd.common;

import com.android.browser.R;
import android.app.AddonManager;
import com.sprd.common.BrowserPlugInDrm;
import com.sprd.common.StorageAddonStub;

public class Common {
    private static volatile BrowserPlugInDrm sBrowserDrmPlugIn = null;
    private static volatile StorageAddonStub sStoragePlugIn = null;

    private Common() {
    }

    public static BrowserPlugInDrm getBrowserDrmPlugIn() {
        if (sBrowserDrmPlugIn != null)
            return sBrowserDrmPlugIn;
        sBrowserDrmPlugIn = (BrowserPlugInDrm) AddonManager.getDefault().getAddon(R.string.feature_browserplugdrm, BrowserPlugInDrm.class);
        return sBrowserDrmPlugIn;
    }

    public static StorageAddonStub getStoragePlugIn() {
        if (sStoragePlugIn != null)
            return sStoragePlugIn;
        sStoragePlugIn = (StorageAddonStub) AddonManager.getDefault().getAddon(R.string.feature_storageplugin, StorageAddonStub.class);
        return sStoragePlugIn;
    }
}
