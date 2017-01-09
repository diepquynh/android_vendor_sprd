package com.dream.camera.settings;

import android.content.Context;

import com.android.camera.debug.Log;
import com.android.camera.settings.Keys;

import java.io.File;
import java.util.ArrayList;

public class DataModuleManager {

    public static final Log.Tag TAG = new Log.Tag("DataModuleManager");

    private Context mContext;

    // handler for camera module
    private DataModuleCamera mDataModuleCamera;
    // handler for photo module
    private DataModulePhoto mDataModulePhoto;
    // handler for video module
    private DataModuleVideo mDataModuleVideo;
    // current module of app, which is photo module or videmodule
    private DataModuleBasic mCurrentDataModule;

    // temporary module used to get some settings
//    private DataModulePhoto mTempDataModulePhoto;
//    private DataModuleVideo mTempDataModuleVideo;
    private DataModuleTemp mDataModuleTemp;

    // current data setting
    private DataStructSetting mCurrentDataSetting;
    // temp data setting
    private DataStructSetting mTempDataSetting;

    private DataModuleManager(Context context) {
        mContext = context;
        // initialize three data module photo/video/camera
        mDataModuleCamera = new DataModuleCamera(mContext);
        mDataModulePhoto = new DataModulePhoto(mContext);
        mDataModuleVideo = new DataModuleVideo(mContext);
//        mTempDataModulePhoto = new DataModulePhoto(mContext);
//        mTempDataModuleVideo = new DataModuleVideo(mContext);
        mDataModuleTemp = new DataModuleTemp(mContext);

        mDataModuleCamera.initializeData(new DataStructSetting(
                DataConfig.CategoryType.CATEGORY_CAMERA, true, null, 0));
    }

    private static DataModuleManager mSingleInstance;

    public static DataModuleManager getInstance(Context context) {
        synchronized (mLock) {
            if (mSingleInstance == null) {
                mSingleInstance = new DataModuleManager(context.getApplicationContext());
            }
        }
        return mSingleInstance;
    }

    public void changeModuleStatus(DataStructSetting dataSetting) {

        if (dataSetting == null) {
            return;
        }

        if (mCurrentDataSetting != null
                && dataSetting.equals(mCurrentDataSetting)) {
            if (mDataModuleCamera != null) {
                mDataModuleCamera.resetResource();
            }
            if (mCurrentDataModule != null) {
                mCurrentDataModule.resetResource();
            }
            return;
        }

        mCurrentDataSetting = dataSetting;

        if (dataSetting.mCategory
                .equals(DataConfig.CategoryType.CATEGORY_PHOTO)) {
            mDataModulePhoto.changeCurrentModule(mCurrentDataSetting);
            mCurrentDataModule = mDataModulePhoto;
        } else if (dataSetting.mCategory
                .equals(DataConfig.CategoryType.CATEGORY_VIDEO)) {
            mDataModuleVideo.changeCurrentModule(mCurrentDataSetting);
            mCurrentDataModule = mDataModuleVideo;
        }

    }

//    public DataModuleBasic getTempModule(DataStructSetting dataSetting) {
//        if (dataSetting == null) {
//            return null;
//        }
//
//        if (dataSetting.mCategory
//                .equals(DataConfig.CategoryType.CATEGORY_PHOTO)) {
//            mTempDataModulePhoto.changeCurrentModule(dataSetting);
//            return mTempDataModulePhoto;
//        } else if (dataSetting.mCategory
//                .equals(DataConfig.CategoryType.CATEGORY_VIDEO)) {
//            mTempDataModuleVideo.changeCurrentModule(dataSetting);
//            return mTempDataModuleVideo;
//        }
//
//        return null;
//    }
    public DataModuleBasic getTempModule(DataStructSetting dataSetting) {
        if (dataSetting == null) {
            return null;
        }

        if (mTempDataSetting != null && mTempDataSetting.equals(dataSetting)){
            return mDataModuleTemp;
        }
        mTempDataSetting = dataSetting;
        mDataModuleTemp.changeCurrentModule(mTempDataSetting);
        return mDataModuleTemp;
    }


    public DataModuleBasic getDataModule(String category) {
        if (category.equals(DataConfig.CategoryType.CATEGORY_CAMERA)) {
            return mDataModuleCamera;
        } else if (category.equals(DataConfig.CategoryType.CATEGORY_PHOTO)) {
            return mDataModulePhoto;
        } else if (category.equals(DataConfig.CategoryType.CATEGORY_VIDEO)) {
            return mDataModuleVideo;
        }

        return null;
    }

    public DataModuleBasic getCurrentDataModule() {
        if (mCurrentDataModule instanceof DataModulePhoto) {
        } else if (mCurrentDataModule instanceof DataModuleVideo) {
        }
        return mCurrentDataModule;
    }

    public DataModuleCamera getDataModuleCamera() {
        return mDataModuleCamera;
    }

    public DataModulePhoto getDataModulePhoto() {
        return mDataModulePhoto;
    }

    public DataModuleVideo getDataModuleVideo() {
        return mDataModuleVideo;
    }

    public DataStructSetting getCurrentDataSetting() {
        return mCurrentDataSetting;
    }

    public void reset() {

        Log.e(TAG, "reset start..............");

        savePreference();

        //Bug: 656307 camera can not reset if operated by new user
        String dataFile = mContext.getFilesDir().toString();
        dataFile = dataFile.substring(0, dataFile.lastIndexOf("/"));
        File file = new File(dataFile + "/shared_prefs");

        if (file.isDirectory()) {
            String[] childFiles = file.list();
            for (String child : childFiles) {

                child.lastIndexOf(".");

                DreamSettingUtil
                        .openPreferences(
                                mContext,
                                child.subSequence(0, child.lastIndexOf("."))
                                        .toString()).edit().clear().apply();
            }
        }

        restorePreference();
        getCurrentDataModule().clearRestoreData();
        // SPRD: Fix Bug614967 GIF mode, restore the default settings, click Add photos, will not find the file
        mDataModuleCamera.fillEntriesAndSummaries();
        synchronized (mLock) {

            for (int i = 0; i < mSettingResetListeners.size(); i++) {
                ResetListener listener = mSettingResetListeners.get(i);
                Log.e(TAG, " reset listener class = "
                        + listener.getClass().getName());
                listener.onSettingReset();
            }

        }
        Log.e(TAG, "reset end..............");
    }

    // **************************** listener *************************

    private void restorePreference() {
        mDataModuleCamera.changeSettings(Keys.KEY_CAMERA_ID, mCameraId);
        mDataModuleCamera.changeSettings(Keys.KEY_CAMERA_WELCOME, false);
        // SPRD: add for proting bug 567394 from trunk branch
        String defaultQuickCaptureValue = mDataModuleCamera
                .getStringDefault(Keys.KEY_QUICK_CAPTURE);
        mDataModuleCamera.changeSettings(Keys.KEY_QUICK_CAPTURE,
                defaultQuickCaptureValue);
    }

    String mCameraId;

    private void savePreference() {
        mCameraId = mDataModuleCamera.getString(Keys.KEY_CAMERA_ID);
    }

    public interface ResetListener {
        public void onSettingReset();
    }

    private ArrayList<ResetListener> mSettingResetListeners = new ArrayList<ResetListener>();
    private static Object mLock = new Object();

    public void removeListener(ResetListener listener) {
        synchronized (mLock) {
            if (listener == null || mSettingResetListeners == null) {
                throw new IllegalArgumentException();
            }

            if (!mSettingResetListeners.contains(listener)) {
                return;
            }
            Log.e(TAG, "remove listener = " + listener.getClass().getName());
            mSettingResetListeners.remove(listener);
        }
    }

    public void addListener(final ResetListener listener) {
        synchronized (mLock) {
            if (listener == null || mSettingResetListeners == null) {
                throw new IllegalArgumentException(
                        "OnSettingChangedListener cannot be null.");
            }

            if (mSettingResetListeners.contains(listener)) {
                return;
            }
            Log.e(TAG, "add listener = " + listener.getClass().getName());
            mSettingResetListeners.add(listener);
        }
    }

    public void resetResource() {
        if (mDataModuleCamera != null) {
            mDataModuleCamera.resetResource();
        }
        if (mCurrentDataModule != null) {
            mCurrentDataModule.resetResource();
        }

    }

    /**
     * clear all listeners
     */
    public void removeAllSettingResetListeners() {
        Log.d(TAG, "removeAllSettingResetListeners");
        if (mSettingResetListeners != null) {
            mSettingResetListeners.clear();
        }
    }

    public void destory() {
        removeAllSettingResetListeners();
        if (mDataModuleCamera != null) {
            mDataModuleCamera.removeAllListeners();
        }
        if (mDataModulePhoto != null) {
            mDataModulePhoto.removeAllListeners();
        }
        if (mDataModuleVideo != null) {
            mDataModuleVideo.removeAllListeners();
        }
//        if (mTempDataModulePhoto != null) {
//            mTempDataModulePhoto.removeAllListeners();
//        }
//        if (mTempDataModuleVideo != null) {
//            mTempDataModuleVideo.removeAllListeners();
//        }
        if (mDataModuleTemp != null) {
            mDataModuleTemp.removeAllListeners();
        }
    }
}
