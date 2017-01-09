package addon.sprd.downloadprovider;

import java.io.File;

import com.android.providers.downloads.DownloadThread.DownloadInfoDelta;
import com.android.providers.downloads.DownloadInfo;
import android.content.ContentUris;
import android.app.Activity;
import android.app.AddonManager;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.graphics.Color;
import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;
import android.widget.Toast;
import android.os.AsyncTask;
import com.android.providers.downloadsplugin.DownloadsDRMUtils;
import com.android.providers.downloadsplugin.OpenHelperDRMUtil;
import java.io.IOException;
import java.util.Random;
import java.util.Set;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import com.android.providers.downloads.DownloadThread;
import com.android.providers.downloads.Constants;
import android.provider.Downloads;

import com.android.providers.downloads.DownloadReceiver;

import android.app.DownloadManager;
import android.content.ContentValues;
import android.text.TextUtils;


public class AddonOpenHelperUtils extends OpenHelperDRMUtil implements AddonManager.InitialCallback {
    private Context mAddonContext;
    private static final String TAG = "AddonOpenHelperUtils";
    public static final int DRM_PLUGIN = 0;
    public static final int HELPER_FULLNAME_RETURN = 1;
    public static final int HELPER_SUQENCE_CONTINUE = 2;

    public static final String MIMETYPE_DRM_MESSAGE = "application/vnd.oma.drm.message";

    public AddonOpenHelperUtils() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    @Override
    public String getDRMPluginMimeType(Context context, File file, String mimeType, Cursor cursor) {
        Log.d(TAG, "getDRMPluginMimeType ==  ");
        return DownloadDrmHelper.getOriginalMimeType(context, file.toString(), mimeType);
    }

    @Override
    public Intent setDRMPluginIntent(File file, String mimeType) {
        final Intent intent = new Intent(Intent.ACTION_VIEW);

        Log.d(TAG, "setDRMPluginIntent ==  ");

        intent.putExtra("drmpath", file.getPath());
        intent.putExtra("drmtype", mimeType);

        return intent;
    }


//    @Override
//    public void setTextColor
}
