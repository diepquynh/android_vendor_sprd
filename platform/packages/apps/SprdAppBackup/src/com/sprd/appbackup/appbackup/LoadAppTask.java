
package com.sprd.appbackup.appbackup;

import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.os.AsyncTask;
import android.util.Log;

import java.io.File;
import java.io.FileFilter;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import com.sprd.appbackup.AppInfo;
import com.sprd.appbackup.service.Config;
import com.sprd.appbackup.utils.StorageUtil;
import com.sprd.appbackup.RestoreFragment;
import com.sprd.appbackup.activities.MainActivity;
/* SPRD: 445202 import @{ */
import android.content.SharedPreferences;
/* @} */

public class LoadAppTask extends AsyncTask<Void, Void, List<AppInfo>> {

    private static final String TAG = "LoadAppTask";

    public static final int GET_INSTALLED_APPS = 0;

    public static final int GET_ARCHIVED_APPS = 1;

    private int mType;
    private Context mContext;

    public LoadAppTask(int mType, Context mContext) {
        this.mType = mType;
        this.mContext = mContext;
    }

    @Override
    protected void onPreExecute() {

    }

    @Override
    protected List<AppInfo> doInBackground(Void... params) {
        List<AppInfo> ret = null;
        switch (mType) {
            case GET_INSTALLED_APPS:
                ret = loadInstalledAppinfos();
                break;
            case GET_ARCHIVED_APPS:
                ret = loadRestoredAppinfos();
                Log.d(TAG,"doInBackground ret = " + ret);
                break;
            default:
                break;
        }

        return ret;
    }

    @Override
    protected void onPostExecute(List<AppInfo> result) {

    }

   public List<AppInfo> loadInstalledAppinfos() {
        List<AppInfo> ret = new ArrayList<AppInfo>();
        PackageManager pm = mContext.getPackageManager();

        Iterator<PackageInfo> it = pm.getInstalledPackages(0).iterator();
        while (it.hasNext()) {
            PackageInfo info = it.next();
            if ((info.applicationInfo != null)
                    && ((ApplicationInfo.FLAG_SYSTEM & info.applicationInfo.flags) == 0)
                    && !info.applicationInfo.packageName.equals(mContext.getApplicationInfo().packageName)
                    //SPRD: Bug_419733 load preload app
                    //&& !info.applicationInfo.sourceDir.startsWith("/system/preloadapp/")
                    && !info.applicationInfo.sourceDir.startsWith("/system/vital-app/")) {
                ret.add(Utils.packageInfoToAppInfo(pm, info, null));
            }
        }
        Log.i(TAG, "getInstalledAppInfos size : " + ret.size());
        return ret;
    }

    public List<AppInfo> loadRestoredAppinfos() {
        List<AppInfo> ret = new ArrayList<AppInfo>();
        File restoredDirFile = null;
        if (MainActivity.mRestoreSelectedFile.size() != 0) {
            Log.e(TAG, "loadRestoredAppinfos() scanSize = " + MainActivity.mRestoreSelectedFile.size());
            /* SPRD: for bug 442387 define the new ArrayList  @{ */
            ArrayList<String> newRestoreSelectedFile = (ArrayList) (((ArrayList) MainActivity.mRestoreSelectedFile)
                    .clone());
            for (String filePath : newRestoreSelectedFile) {
            /* @}  */
                if (MainActivity.mScanAllCancel) {
                    Log.e(TAG, "loadRestoredAppinfos() MainActivity.mScanAllCancel = false");
                    break;
                }

                File archive = new File(filePath);
                Log.e(TAG, "loadRestoredAppinfos() filePath = " + filePath);
//                ret = getRestoredAppinfos(archive);

                File[] apkFiles = null;
                if (archive.exists() && archive.isDirectory()) {
                    apkFiles = archive.listFiles(new FileFilter() {

                        @Override
                        public boolean accept(File file) {
                            if (file.getName().toLowerCase().endsWith(Config.SUFFIX_APK)) {
                                return true;
                            }
                            return false;
                        }
                    });
                }
                PackageManager pm = mContext.getPackageManager();

                if (apkFiles != null) {
                    for (File file : apkFiles) {
                        PackageInfo packageInfo = pm.getPackageArchiveInfo(file.getPath(), PackageManager.GET_ACTIVITIES);
                        Log.i(TAG,"loadRestoredAppinfos packageInfo = " + packageInfo);
                        if(packageInfo != null && packageInfo.applicationInfo != null) {
                            packageInfo.applicationInfo.sourceDir = file.getPath();
                            Log.d(TAG,"sourceDir: " + packageInfo.applicationInfo.sourceDir +
                                    "\npackageName: " + packageInfo.applicationInfo.packageName +
                                    "\nversionCode: " + packageInfo.versionCode +
                                    "\nversionName: " + packageInfo.versionName + "\n");
                            AppInfo info = Utils.packageInfoToAppInfo(pm, packageInfo, null);
                            info.setIcon(Utils.getUninstalledAPKIcon(mContext, file.getPath()));
                            info.setName(Utils.getUninstalledAPKName(mContext, file.getPath()));
                            info.setApkFileName(file.toString());
                            /*for bug 388383,cmcc new req,need default select all*/
                            info.setChecked(true);
                            if (info != null) {
                                Log.e(TAG, "loadRestoredAppinfos() info = " + info);
                                ret.add(info);
                            }
                        }
                    }
                }
                Log.e(TAG, "loadRestoredAppinfos() ret = " + ret);
                //return ret;
            }
        } else {
            /* SPRD: 445202 save defined backup path @{ */
            SharedPreferences sharedPreferences = mContext.getSharedPreferences(Config.SHAREPREFERENCE_FILE_NAME, Context.MODE_PRIVATE);
            String definedPath = sharedPreferences.getString(Config.DEFINED_BACKUP_PATH_KEY, "");
            if (!definedPath.isEmpty()) {
                definedPath = definedPath + "/backup/App/";
            }

            //make a traversal for internal, external and defined backup path
            for(int i = 0; i < 3; i++) {
            /* @} */
                if (0 == i) {
                    if (Config.IS_NAND || !StorageUtil.getInternalStorageState()) {
                        continue;
                    }
                    restoredDirFile = new File(Config.INTERNAL_APP_BACKUP_PATH);
                } else if(1 == i) {
                    if (!StorageUtil.getExternalStorageState()) {
                        continue;
                    }
                    restoredDirFile = new File(Config.EXTERNAL_APP_BACKUP_PATH);
                /* SPRD: 445202 save defined backup path @{ */
                } else if (2 == i){
                    if (definedPath.equalsIgnoreCase(Config.INTERNAL_APP_BACKUP_PATH)
                            || definedPath.equalsIgnoreCase(Config.EXTERNAL_APP_BACKUP_PATH)) {
                        continue;
                    }
                    restoredDirFile = new File(definedPath);
                /* @} */
                } else {
                    return ret;
                }

                File[] apkFiles = null;
                if (restoredDirFile.exists() && restoredDirFile.isDirectory()) {
                    apkFiles = restoredDirFile.listFiles(new FileFilter() {

                        @Override
                        public boolean accept(File file) {
                            if (file.getName().toLowerCase().endsWith(Config.SUFFIX_APK)) {
                                return true;
                            }
                            return false;
                        }
                    });
                }

//                ret = getRestoredAppinfos(restoredDirFile);
/*
                File[] apkFiles = null;
                if (restoredDirFile.exists() && restoredDirFile.isDirectory()) {
                    apkFiles = restoredDirFile.listFiles(new FileFilter() {

                        @Override
                        public boolean accept(File file) {
                            if (file.getName().toLowerCase().endsWith(Config.SUFFIX_APK)) {
                                return true;
                            }
                            return false;
                        }
                    });
                }
                */
                PackageManager pm = mContext.getPackageManager();

                if (apkFiles != null) {
                    for (File file : apkFiles) {
                        PackageInfo packageInfo = pm.getPackageArchiveInfo(file.getPath(), PackageManager.GET_ACTIVITIES);
                        Log.i(TAG,"packageInfo = " + packageInfo);
                        if(packageInfo != null && packageInfo.applicationInfo != null) {
                            packageInfo.applicationInfo.sourceDir = file.getPath();
                            Log.d(TAG,"sourceDir: " + packageInfo.applicationInfo.sourceDir +
                                    "\npackageName: " + packageInfo.applicationInfo.packageName +
                                    "\nversionCode: " + packageInfo.versionCode +
                                    "\nversionName: " + packageInfo.versionName + "\n");
                            AppInfo info = Utils.packageInfoToAppInfo(pm, packageInfo, null);
                            info.setIcon(Utils.getUninstalledAPKIcon(mContext, file.getPath()));
                            info.setName(Utils.getUninstalledAPKName(mContext, file.getPath()));
                            info.setApkFileName(file.toString());
                            /*for bug 388383,cmcc new req,need default select all*/

                            info.setChecked(true);
                            if (info != null) {
                                ret.add(info);
                            }
                        }
                    }
                }
            }
        }
        return ret;

    }
}
