/*
 * Copyright (C) 2010,2011 Thundersoft Corporation
 */
package com.ucamera.ugallery.util;

import org.apache.http.HttpResponse;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.params.BasicHttpParams;
import org.apache.http.params.HttpConnectionParams;
import org.apache.http.params.HttpParams;
import org.apache.http.util.EntityUtils;
import org.json.JSONObject;

import java.io.File;
import java.net.URLEncoder;
import java.util.Locale;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.res.Resources;

import android.os.Handler;
import android.os.Message;
import android.preference.PreferenceManager;
import android.util.Log;

import com.ucamera.ugallery.R;
import com.ucamera.ugallery.UGalleryConst;

public class CheckVersion {
    private static final String TAG = "CheckVersion";
    private SharedPreferences mPreferences;
    private final static long updateTime=24 * 60 * 60 * 1000;
    private Handler mHandler;
    private static boolean mIsManualUpdate = false;
    private final static String mURLExternal = "http://www.u-camera.com/downloads/android/";
    private final static String mNewURL = "http://www.u-camera.com/api/version.php";
    public static String mUpdateServer = mURLExternal;

    public static enum Release {
        UGallery(".ugallery","UGallery",""),
        UGalleryTablet(".ugallerytablet","UGallery-Tablet","tablet-");

        private Release(String pkg, String m, String suffix) {
            pkgname    = "com.ucamera" + pkg;
            modulename = m;
            filesuffix = suffix;
        }

        public static Release fromPkg(String pkg) {
            for(Release v: values()) {
                if (v.pkgname.equals(pkg)) {
                    return v;
                }
            }
            throw new RuntimeException(pkg + " is not a valid Release enum");
        }

        /**
         * release package name, eg:
         *   <code>com.ucamera.ugallery</code>
         *   <code>com.ucamera.ugallerytablet</code>
         *   ...
         */
        public final String pkgname;

        /**
         * refer to the release, eg:
         *  UGallery       : for general release
         *  UGallery-Pro   : for Pro release
         *  UGallery-Tablet: for tablet release
         *  ...
         */
        public final String modulename;

        /**
         * some part of the release file name, eg
         *  <li><code>pro</code> part in <code>UGallery-pro-1.0.0.000000.apk</code>
         *  <li><code>tablet</code> part in <code>UGallery-tablet-1.0.0.000000.apk</code>
         *  ...
         */
        public final String filesuffix;
    }


    public void toUpdate(final Context context,Handler handler) {
        mHandler=handler;

        Runnable r = new Runnable() {
            public void run() {
                try {
                    boolean timeToUpdate=false;
                    if (mIsManualUpdate) {
                        timeToUpdate = true;
                    }else {
                        mPreferences = PreferenceManager.getDefaultSharedPreferences(context);
                        Util.upgradeGlobalPreferences(mPreferences);
                        long lastUpdateTime =  mPreferences.getLong("lastUpdateTime", 0);
                        if ((lastUpdateTime+updateTime) < System.currentTimeMillis()){
                            timeToUpdate=true;
                        }
                    }
                    String currentVersionString = Util.getPackageVersion(context);
                    String mLanguage = Locale.getDefault().getLanguage();
                    connectToNewServer(context, mLanguage, currentVersionString, timeToUpdate);
                } catch (Exception e) {
                    // failed to connect to network
                    if (mIsManualUpdate) {
                        Resources res= context.getResources();
                        Message m = mHandler.obtainMessage(UGalleryConst.GOTO_UPDATE,res.getString(R.string.text_no_network_available));
                        mHandler.sendMessage(m);
                        mIsManualUpdate = false;
                    }
                    e.printStackTrace();
                }
            }
        };
        new Thread(r).start();
    }

    @SuppressWarnings("deprecation")
    private static String makeCheckURL(Context context) {
        Release release = Release.fromPkg(context.getPackageName());
        return mNewURL + "?name=" + URLEncoder.encode(release.modulename);
    }

    private static String makeApkName(Context context, String version) {
        Release release = Release.fromPkg(context.getPackageName());
        return new StringBuilder()
                   .append(UGalleryConst.APKNAME)
                   .append(release.filesuffix)
                   .append(version.trim())
                   .append(UGalleryConst.EXTENTION)
                   .toString();
    }

    public static final String makeUpdatURL( Context context, String version) {
        return CheckVersion.mUpdateServer + makeApkName(context, version);
    }

    public void  toUpdataManually(final Context context,Handler handler) {
        mIsManualUpdate = true;
        toUpdate(context, handler);
    }

    // whether the former version string is newer than latter
    private static boolean isNewerVersion(String newVersion, String currentVersion ){
        return Version.fromString(newVersion).isNewerThan(Version.fromString(currentVersion));
    }

    /**
     * deal with external url
     * @param context context
     * @param language language contains english and chinese
     * @param currentVersionString current version
     * @param timeToUpdate time
     */
    public void connectToNewServer(Context context, String language, String currentVersionString, boolean timeToUpdate){
        try{
            String newUrl = makeCheckURL(context);
            Log.d(TAG, "connectToNewServer(): newUrl = " + newUrl);
            HttpPost postRequest = new HttpPost(newUrl);
            HttpParams httpParameters;
            httpParameters = new BasicHttpParams();// Set the timeout in milliseconds until a connection is established.
            HttpConnectionParams.setConnectionTimeout(httpParameters, 5000);// Set the default socket timeout (SO_TIMEOUT) // in milliseconds which is the timeout for waiting for data.
            HttpConnectionParams.setSoTimeout(httpParameters, 5000);
            HttpResponse response = new DefaultHttpClient(httpParameters).execute(postRequest);
            if (response.getStatusLine().getStatusCode() == 200) {
                String retSrc = EntityUtils.toString(response.getEntity());
                JSONObject result = new JSONObject(retSrc);
                String updateinfo = null;
                // CID 109102 : DLS: Dead local store (FB.DLS_DEAD_LOCAL_STORE)
                // String name = result.getString("name");
                String newVersion = result.getString("version");
                boolean isThereNewVersion = isNewerVersion(newVersion.trim(), currentVersionString);
                String downloadUrl = result.getString("download_url");
                if(language.equals("en")) {
                    updateinfo = result.getString("desc_english");
                } else if(language.equals("zh")) {
                    updateinfo = result.getString("desc_chinese");
                }
                update(context, currentVersionString, downloadUrl, updateinfo, isThereNewVersion, timeToUpdate);
            } else {
                if (mIsManualUpdate) {
                    Resources res= context.getResources();
                    Message m = mHandler.obtainMessage(UGalleryConst.GOTO_UPDATE,res.getString(R.string.text_no_network_available));
                    mHandler.sendMessage(m);
                    mIsManualUpdate = false;
                }
            }
        } catch (Exception e) {
            if (mIsManualUpdate) {
                Resources res= context.getResources();
                Message m = mHandler.obtainMessage(UGalleryConst.GOTO_UPDATE,res.getString(R.string.text_no_network_available));
                mHandler.sendMessage(m);
                mIsManualUpdate = false;
            }
        }
    }

    /**
     * update info
     * @param context context
     * @param currentVersion current version
     * @param newVersion new version
     * @param updateinfo update info
     * @param isThereNewVersion is new version
     * @param timeToUpdate time update
     */
    private void update(Context context, String currentVersion, String newVersion, String updateinfo, boolean isThereNewVersion, boolean timeToUpdate) {
        if (UpdateService.mIsDownloading && mIsManualUpdate) {
            /**
             * FIX BUG: 224
             * BUG CAUSE: the update dialog pop-up when downloading
             * FIX COMMENT: according to the download state then decide whether to show the dialog
             * Date: 2011-12-15
             */
            mIsManualUpdate = false;
            Resources res= context.getResources();
            Message m = mHandler.obtainMessage(UGalleryConst.GOTO_UPDATE,res.getString(R.string.text_service_starting_notification));
            mHandler.sendMessage(m);
        } else if(timeToUpdate && isThereNewVersion && !UpdateService.mIsDownloading) {
            String download = newVersion;
            String fileName = null;
            if(newVersion.startsWith("http://")) {
                newVersion = newVersion.substring(newVersion.lastIndexOf("=") + 1, newVersion.length());
                fileName = UpdateService.DIRECTORY + newVersion.trim();
            } else {
                fileName = UpdateService.DIRECTORY + UGalleryConst.APKNAME + newVersion.trim() + UGalleryConst.EXTENTION;
            }
            if (new File(fileName).exists()) { // apk is exist,then send file path to install
                Message m = mHandler.obtainMessage(UGalleryConst.GOTO_UPDATE,fileName+"#" + updateinfo);
                mHandler.sendMessage(m);
            }else {                            // send message to start download
//                Message m = mHandler.obtainMessage(Camera.GOTO_UPDATE,newVersion + "#" + updateinfo);
                Message m = mHandler.obtainMessage(UGalleryConst.GOTO_UPDATE,download + "#" + updateinfo);
                mHandler.sendMessage(m);
            }
        }else if(mIsManualUpdate && !isThereNewVersion) {
            Resources res= context.getResources();  // current version is newest
            Message m = mHandler.obtainMessage(UGalleryConst.GOTO_UPDATE,res.getString(R.string.already_update));
            mHandler.sendMessage(m);
            mIsManualUpdate = false;
        }
    }

    public static boolean isInternalDownload() {
        return false;
    }
}

