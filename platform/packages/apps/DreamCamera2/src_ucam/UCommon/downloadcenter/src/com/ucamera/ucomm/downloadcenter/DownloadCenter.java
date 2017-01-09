/**
 * Copyright (C) 2010,2013 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.ucomm.downloadcenter;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.drawable.Drawable;
import android.os.AsyncTask;
import android.os.Environment;
import android.preference.PreferenceManager;
import android.util.Log;

import com.sprd.camera.storagepath.StorageUtilProxy;

public class DownloadCenter {
    public static boolean RESOURCE_DOWNLOAD_ON = false; // SPRD: Closed the RESOURCE_DOWNLOAD.
    //SPRD:Fix bug 561234 The edit is invalid.
    public final static String DOWNLOAD_DIRECTORY = StorageUtilProxy
            .getInternalStoragePath().toString() + "/UCam/download/";
    public static final String TAG = "DownloadCenter";
    public final static String COPIED_FROM_ASSETS_DIRECTORY =
            StorageUtilProxy.getInternalStoragePath().toString() + "/DCIM/" + ".UCam";

    private static int mThumbnailWidth = -1;
    private static int mThumbnailHeight = -1;
    private static String mFileName = null;

    private static SharedPreferences mSharedPref;

    public static void setResouceDownloadOn(boolean on) {
        RESOURCE_DOWNLOAD_ON = on;
    }
    public static void loadFrame(final Context context, final OnLoadCallback callback) {
        loadResource(context, callback, Constants.EXTRA_FRAME_VALUE);
    }

    public static void loadDecor(final Context context, final OnLoadCallback callback) {
        loadResource(context, callback, Constants.EXTRA_DECOR_VALUE);
    }

    public static void loadPhotoFrame(final Context context, final OnLoadCallback callback) {
        loadResource(context, callback, Constants.EXTRA_PHOTO_FRAME_VALUE);
    }
    public static void loadTexture(final Context context, final OnLoadCallback callback) {
        loadResource(context, callback, Constants.EXTRA_TEXTURE_VALUE);
    }
    public static void loadMosaic(final Context context, final OnLoadCallback callback) {
        loadResource(context, callback, Constants.EXTRA_MOSAIC_VALUE);
    }
    public static void loadFont(final Context context, final OnLoadCallback callback) {
        loadResource(context, callback, Constants.EXTRA_FONT_VALUE);
    }
    public static void loadPuzzle(final Context context, final OnLoadCallback callback) {
        loadResource(context, callback, Constants.EXTRA_PUZZLE_VALUE);
    }
    public static void loadMangaFrame(final Context context, final OnLoadCallback callback) {
//        loadResource(context, callback, Constants.EXTRA_MANGA_FRAME_VALUE);
        /* SPRD:  CID 108990 : Dereference null return value (NULL_RETURNS) @{ */
        String[] val = loadResource(context, Constants.EXTRA_MANGA_FRAME_VALUE);
        if(val == null) {
            val = new String[]{};
        }
        callback.onLoadComplete(val);
        // callback.onLoadComplete(loadResource(context, Constants.EXTRA_MANGA_FRAME_VALUE));
        /* @} */
    }
    private static void loadResource(final Context context, final OnLoadCallback callback, final String type) {
        new AsyncTask<Void, Void, String[]>() {
            @Override
            protected String[] doInBackground(Void... params) {
                return loadResource(context, type);
            }

            @Override
            protected void onPostExecute(String[] result) {
                if (callback != null) {
                    callback.onLoadComplete(result);
                }
            }
        }.execute();
    }

    private static String[] loadResource(Context context, String type) {
        try {
            String resBaseDir = type + "/" + type;
            String[] resNames = context.getAssets().list(resBaseDir);
            int assetLen = resNames.length;
            File resDownloaDir = new File(DOWNLOAD_DIRECTORY + resBaseDir + "/");
            Log.d("SSSS", DOWNLOAD_DIRECTORY + resBaseDir + "/");
            if (resDownloaDir.exists()) {
                String[] sdcardFrame = resDownloaDir.list(new FileAccept(".ucam"));
                int sdcardLen = sdcardFrame.length;
                if (sdcardLen > 0) {
                    ArrayList<String> existFrame = new ArrayList<String>();
                    for (int i = 0; i < sdcardLen; i++) {
                        String fileName = sdcardFrame[i];
                        String thumbPath = DOWNLOAD_DIRECTORY + resBaseDir+ "_hdpi/" + fileName;
                        if (!new File(thumbPath).exists()) {
                            new File(resDownloaDir, fileName).delete();
                            continue;
                        }
                        existFrame.add(thumbPath);
                    }
                    if (!existFrame.isEmpty()) {
                        sdcardFrame = existFrame.toArray(new String[existFrame
                                .size()]);
                        existFrame.clear();
                        existFrame = null;
                        sdcardLen = sdcardFrame.length;
                        String[] allFrames = new String[assetLen + sdcardLen];
                        System.arraycopy(resNames, 0, allFrames, 0, assetLen);
                        System.arraycopy(sdcardFrame, 0, allFrames, assetLen,
                                sdcardLen);
                        resNames = allFrames;
                    }
                }
            }

            assetLen = resNames.length;
            String[] displayFrames;
            if(RESOURCE_DOWNLOAD_ON) {
                String[] downloadIcon = new String[] { "download_center_icon.png" };
                displayFrames = new String[assetLen + 1];
                System.arraycopy(downloadIcon, 0, displayFrames, 0, 1);
                System.arraycopy(resNames, 0, displayFrames, 1, assetLen);
            } else {
                displayFrames = new String[assetLen];
                System.arraycopy(resNames, 0, displayFrames, 0, assetLen);
            }
            resNames = displayFrames;
            return resNames;
        } catch (Exception e) {
        }
        return null;
    }

    public interface OnLoadCallback {
        public void onLoadComplete(String[] result);
    }
    /**
     * start download center
     * @param activity context
     * @param screenDensity density contains 1.0 and more than 1.5
     * @param downloadType type contains frame and decor
     */
    public static void openResourceCenter(Context activity, String downloadType) {
        if(mSharedPref == null) {
            mSharedPref = PreferenceManager.getDefaultSharedPreferences(activity);
        }

        if(DownloadTabActivity.isNeedNetworkPermission() &&
                (!mSharedPref.contains(UiUtils.KEY_CONFIRM_NETWORK_PERMISSION) || mSharedPref.getString(UiUtils.KEY_CONFIRM_NETWORK_PERMISSION, "").equals("off"))) {
            UiUtils.confirmNetworkPermission(activity, new String[]{activity.getString(R.string.download_text_item_name_resources)}, null, null, 1, downloadType);
        }else {
//        String density = Constants.EXTRA_DENSITY_HDPI;
            Intent intent = new Intent();
            intent.setClass(activity.getApplicationContext(), DownloadTabActivity.class);
            intent.putExtra(Constants.ACTION_DOWNLOAD_TYPE, downloadType);
//        intent.putExtra(Constants.ACTION_SCREEN_DENSITY, density);
            activity.startActivity(intent);
        }
    }

    public static Bitmap thumbNameToBitmap(Context context, String fileName ,String type){
        Log.d(TAG, "fileName" + fileName);
        InputStream inputStream = null;
        mFileName = null;
        try {
            if (fileName.startsWith(DownloadCenter.DOWNLOAD_DIRECTORY + type +"/"+type+"_hdpi")) {
                mFileName = fileName.replace(type+"_hdpi", type);
                Log.d(TAG, "Download fileName : "+mFileName);
                inputStream = new FileInputStream(mFileName);
            } else {
                mFileName = type+"/"+type+"/" + fileName;
                inputStream = context.getAssets().open(mFileName);
                Log.d(TAG, "Assets fileName : " +mFileName);
            }
            Bitmap bitmap = BitmapFactory.decodeStream(inputStream, null, getOptions());
            /* SPRD:
             * CID 110910 : Resource leak (RESOURCE_LEAK)
             * CID 110911 : Resource leak (RESOURCE_LEAK) @{ */
            if(inputStream != null){
                inputStream.close();
            }
            /* @} */
            return bitmap;
        } catch (OutOfMemoryError e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }
        return null;
    }

    private static BitmapFactory.Options getOptions() {
        BitmapFactory.Options options = new BitmapFactory.Options();
        options.inInputShareable = true;
        options.inPurgeable = true;
        options.inPreferredConfig = Bitmap.Config.ARGB_8888;
        return options;
    }

    public static Bitmap thumbNameToThumbBitmap(Context context, String fileName ,String type){
        InputStream inputStream = null;
        Log.d(TAG, "filename:" + fileName);
        try {
            try {
                if (fileName.startsWith(DOWNLOAD_DIRECTORY + type +"/"+type+"_hdpi")) {
                    inputStream = new FileInputStream(fileName);
                } else {
                    inputStream = context.getAssets().open(type +"/"+type+"_hdpi/" + fileName);
                }
            } catch(FileNotFoundException e) {
                if(fileName.endsWith(".png")) {
                    inputStream = context.getAssets().open(fileName);
                }
            }
            Bitmap bitmap = BitmapFactory.decodeStream(inputStream);
            bitmap = createFitBitmap(context, bitmap);
            return bitmap;
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            DownLoadUtil.closeSilently(inputStream);
        }
        return null;
    }

    public static String getFullResourcePath(String fileName, String type) {
        String strReturn = null;
        if (fileName.startsWith(DOWNLOAD_DIRECTORY + type +"/"+type+"_hdpi")) {
            strReturn = fileName.replace(type+"_hdpi", type);
        }
        else {
            strReturn = COPIED_FROM_ASSETS_DIRECTORY + "/" + type + "/" + type + "/" + fileName;
        }

        return strReturn;
    }

    private static Bitmap createFitBitmap(Context context, Bitmap bitmap) {
        if(bitmap == null) return null;
        Bitmap bm;
        if (bitmap.getWidth() != getThumbnailWidth(context) || bitmap.getHeight() != getThumbnailHeight(context)) {
            bm = Bitmap.createScaledBitmap(bitmap, getThumbnailWidth(context), getThumbnailHeight(context), true);
        } else {
            bm = Bitmap.createBitmap(bitmap);
        }
        if(!bitmap.sameAs(bm)) {
            bitmap.recycle();
        }

        return bm;
    }

    private static void initThumbnailSize(Context context) {
        final Drawable drawable = context.getResources().getDrawable(R.drawable.collage_thumbnail_normal);
        mThumbnailWidth = (int) (drawable.getIntrinsicWidth() - UiUtils.dpToPixel(context, 14));
        mThumbnailHeight = (int) (drawable.getIntrinsicHeight() - UiUtils.dpToPixel(context, 14));
    }

    public static int getThumbnailWidth(Context context) {
        if (mThumbnailWidth == -1) {
            initThumbnailSize(context);
        }
        return mThumbnailWidth;
    }

    public static int getThumbnailHeight(Context context) {
        if (mThumbnailHeight == -1) {
            initThumbnailSize(context);
        }
        return mThumbnailHeight;
    }

    public static String getFileName(){
        return mFileName;
    }

    public static void setFileName(String name){
        mFileName = name;
    }
}
