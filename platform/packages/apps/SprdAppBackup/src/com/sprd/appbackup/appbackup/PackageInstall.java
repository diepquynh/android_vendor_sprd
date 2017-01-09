package com.sprd.appbackup.appbackup;

import java.io.File;
import java.util.List;

import android.content.Context;
import android.content.pm.IPackageDataObserver;
import android.content.pm.IPackageDeleteObserver;
import android.content.pm.IPackageInstallObserver;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.pm.PackageParser;
import android.net.Uri;
import android.os.RemoteException;
import android.util.DisplayMetrics;
import android.util.Log;
import com.sprd.appbackup.AppInfo;

public class PackageInstall {
    private static final String TAG = "PackageInstall";

    public static final int SUCCESS = 0;

    public static final int FAIL = -1;

    /* SPRD: Bug249244,PackageManager of Android4.4
     * changed the final variable DONT_DELETE_DATA
     * to be DELETE_KEEP_DATA, but the value of this variable
     * did not changed,so we just use the value of it. @{ */
    public static final int DELETE_KEEP_DATA = 0x00000001;
    /* @} */

    private Context mContext;
    private File mApkFile;
    private PackageParser.Package mPkgInfo;

    private INSTALL_STATUS mInstallFlag;
    private DELETE_STATUS mDeleteFlag;
    private DATA_STATUS mDataFlag;

    private Object mOperLock = new Object();

    public PackageInstall(Context _context, File _apkFile){
        mContext = _context;
        mApkFile = _apkFile;

        mInstallFlag = INSTALL_STATUS.INSTALL_INIT;
        mDeleteFlag = DELETE_STATUS.DELETE_INIT;
        mDataFlag = DATA_STATUS.DATA_INIT;

    }

    private enum INSTALL_STATUS{
        INSTALL_INIT,
        INSTALL_ING,
        INSTALL_SUCCESSED,
        INSTALL_FAILED
    }

    private enum DELETE_STATUS{
        DELETE_INIT,
        DELETE_ING,
        DELETE_SUCCESSED,
        DELETE_FAILED
    }

    private enum DATA_STATUS{
        DATA_INIT,
        DATA_ING,
        DATA_SUCCESSED,
        DATA_FAILED
    }

    private void waiting() {
        synchronized (mOperLock) {
            while (true) {
                if (mInstallFlag == INSTALL_STATUS.INSTALL_ING
                        || mDataFlag == DATA_STATUS.DATA_ING
                        || mDeleteFlag == DELETE_STATUS.DELETE_ING) {
                    try {
                        mOperLock.wait();
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }

                } else {
                    break;
                }
            }
        }
    }

    public int install(){
       if(mApkFile==null || !mApkFile.exists() || !mApkFile.getName().endsWith(".apk")){
           return FAIL;
       }
       int status = SUCCESS;
       mPkgInfo = getPackageInfo(Uri.fromFile(mApkFile));

       /* SPRD: bug_412000 judge whether the object is null. @{ */
       if((mPkgInfo == null) || (mPkgInfo.packageName == null)) {
           return FAIL;
       }
       /* @} */
       // uninstall apk
       if(isPackageAlreadyInstalled(mContext,mPkgInfo.packageName)){
           deletePackage(mPkgInfo.packageName);
           waiting();
           if(mDeleteFlag == DELETE_STATUS.DELETE_FAILED){
               status = FAIL;
           }
       }

       // install apk
       if(status == SUCCESS){
           installPackage(Uri.fromFile(mApkFile));
           waiting();
           if(mInstallFlag == INSTALL_STATUS.INSTALL_FAILED){
               status = FAIL;
           }
       }

       return status;
    }

    private void installPackage(Uri packageUri){
        mInstallFlag = INSTALL_STATUS.INSTALL_ING;
        int installFlags = 0;

        PackageManager mPm = mContext.getPackageManager();
        try {
            PackageInfo pi = mPm.getPackageInfo(mPkgInfo.packageName,
                    PackageManager.GET_UNINSTALLED_PACKAGES);
            if(pi != null) {
                installFlags |= PackageManager.INSTALL_REPLACE_EXISTING;
            }
        } catch (NameNotFoundException e) {
            e.printStackTrace();
        }
        PackageInstallObserver observer = new PackageInstallObserver();
        mPm.installPackage(packageUri, observer, installFlags, mContext.getPackageName());

    }

    private void deletePackage(String pkgName){
        int unInstallFlags = 1;
        mDeleteFlag = DELETE_STATUS.DELETE_ING;

        /* SPRD: Bug249244,PackageManager of Android4.4
         * changed the final variable DONT_DELETE_DATA
         * to be DELETE_KEEP_DATA, but the value of this variable
         * did not changed,so we just use the value of it. @{ */
        unInstallFlags = DELETE_KEEP_DATA;
        /* @} */
        PackageManager mPm = mContext.getPackageManager();

        PackageDeleteObserver observer = new PackageDeleteObserver();
        mPm.deletePackage(pkgName, observer, unInstallFlags);

    }

    private void clearUserDate(String pkgName){
        PackageManager mPm = mContext.getPackageManager();

        IPackageDataObserver observer = new PackageDataObserver();
        mPm.clearApplicationUserData(pkgName,observer);

    }


    class PackageInstallObserver extends IPackageInstallObserver.Stub{

        public void packageInstalled(String packageName, int returnCode) {
            switch(returnCode){
            case PackageManager.INSTALL_SUCCEEDED:
                mInstallFlag = INSTALL_STATUS.INSTALL_SUCCESSED;
                synchronized (mOperLock) {
                    mOperLock.notifyAll();
                }
                break;
            default:
                mInstallFlag = INSTALL_STATUS.INSTALL_FAILED;
                synchronized (mOperLock) {
                    mOperLock.notifyAll();
                }
                break;
            }
        }
    }

    class PackageDeleteObserver extends IPackageDeleteObserver.Stub{

        @Override
        public void packageDeleted(String packageName, int returnCode)
                throws RemoteException {
            if(returnCode == PackageManager.DELETE_SUCCEEDED){
                mDeleteFlag = DELETE_STATUS.DELETE_SUCCESSED;
                synchronized (mOperLock) {
                    mOperLock.notifyAll();
                }
            }else{
                mDeleteFlag = DELETE_STATUS.DELETE_FAILED;
                synchronized (mOperLock) {
                    mOperLock.notifyAll();
                }
            }
        }
    }

    class PackageDataObserver extends IPackageDataObserver.Stub {

        public void onRemoveCompleted(String packageName, boolean succeeded) throws RemoteException {
            Log.w(TAG, "=====clear succeeded:" + succeeded);
            if (succeeded) {
                mDataFlag = DATA_STATUS.DATA_SUCCESSED;
                synchronized (mOperLock) {
                    mOperLock.notifyAll();
                }
            } else {
                mDataFlag = DATA_STATUS.DATA_FAILED;
                synchronized (mOperLock) {
                    mOperLock.notifyAll();
                }
            }

        }

    }

    public static PackageParser.Package getPackageInfo(Uri packageURI) {
        final String archiveFilePath = packageURI.getPath();

        PackageParser packageParser = new PackageParser();
        File sourceFile = new File(archiveFilePath);
        DisplayMetrics metrics = new DisplayMetrics();
        metrics.setToDefaults();

        PackageParser.Package pkg = null;
        try {
            pkg = packageParser.parsePackage(sourceFile, 0);
            if(pkg == null){
                return null;
            }
        } catch (PackageParser.PackageParserException e) {
            return null;
        }
        // Nuke the parser reference.
        packageParser = null;
        return pkg;
    }

    public static boolean isPackageAlreadyInstalled(Context context, String pkgName) {
        List<PackageInfo> installedList = context.getPackageManager().getInstalledPackages(
                PackageManager.GET_UNINSTALLED_PACKAGES);
        int installedListSize = installedList.size();
        for(int i = 0; i < installedListSize; i++) {
            PackageInfo tmp = installedList.get(i);
            if(pkgName.equalsIgnoreCase(tmp.packageName)) {
                return true;
            }
        }
        return false;
    }

}
