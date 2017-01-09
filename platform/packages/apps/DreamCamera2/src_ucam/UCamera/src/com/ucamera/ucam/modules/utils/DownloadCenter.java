/**
 * Copyright (C) 2010,2013 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.ucam.modules.utils;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.concurrent.Executor;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.drawable.Drawable;
import android.os.AsyncTask;
import android.os.Environment;
import android.util.Log;

import com.android.camera2.R;

public class DownloadCenter {

    public static boolean RESOURCE_DOWNLOAD_ON = false;
    public final static String DOWNLOAD_DIRECTORY = Environment
            .getExternalStorageDirectory().toString() + "/UCam/download/";
    public static final String TAG = "DownloadCenter";
    public final static String COPIED_FROM_ASSETS_DIRECTORY =
        Environment.getExternalStorageDirectory().toString() + "/DCIM/" + ".UCam";

    private static int mThumbnailWidth = -1;
    private static int mThumbnailHeight = -1;
    private static String mFileName = null;

    /*SPRD:fix bug624552 scenery picture not show on time @{ */
    public static final Executor THREAD_POOL_EXECUTOR = new ThreadPoolExecutor(1, 2, 1L,
            TimeUnit.SECONDS, new LinkedBlockingQueue<Runnable>());
    /* @} */

    public static void setResouceDownloadOn(boolean on) {
        RESOURCE_DOWNLOAD_ON = on;
    }

    public static void loadAssetsFrame(final Context context, final OnLoadCallback callback) {
        loadAssetsResource(context, callback, Constants.EXTRA_FRAME_THUMB_VALUE);
    }

    private static void loadAssetsResource(final Context context, final OnLoadCallback callback, final String type) {
        new AsyncTask<Void, Void, String[]>() {
            @Override
            protected String[] doInBackground(Void... params) {
                Log.i(TAG, "loadAssetsResource");
                return loadAssetsResource(context, type);
            }

            @Override
            protected void onPostExecute(String[] result) {
                if (callback != null) {
                    callback.onLoadComplete(result);
                }
            }
        }.executeOnExecutor(THREAD_POOL_EXECUTOR);
    }

    private static String[] loadAssetsResource(Context context, String type) {
        try {
            String[] resNames = context.getAssets().list(type);
            return resNames;
        } catch (Exception e) {
        }
        return null;
    }

    public static Bitmap assetsThumbNameToBitmap(Context context, String fileName ,String type){
        InputStream inputStream = null;
        mFileName = null;
        try {
            mFileName = fileName;
            inputStream = context.getAssets().open(type+"/"+mFileName);
            Log.d(TAG, "Assets fileName : " +mFileName);
            Bitmap bitmap = BitmapFactory.decodeStream(inputStream, null, getOptions());
            return bitmap;
        } catch (OutOfMemoryError e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        // CID 124810 : Resource leak (RESOURCE_LEAK)
        } finally {
            Utils.closeSilently(inputStream);
        }
        return null;
    }

    public static Bitmap assetsThumbNameToThumbBitmap(Context context, String fileName ,String type){
        InputStream inputStream = null;
        try {
            try {
                inputStream = context.getAssets().open(type+"/"+fileName);
            } catch(FileNotFoundException e) {
                if(fileName.endsWith(".png")) {
                    inputStream = context.getAssets().open(type+"/"+fileName);
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
            Utils.closeSilently(inputStream);
        }
        return null;
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
        callback.onLoadComplete(loadResource(context, Constants.EXTRA_MANGA_FRAME_VALUE));
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
            Log.d("SSSS", "type=" + type + ", resNames=" + resNames);
            int assetLen = resNames.length;
            File resDownloaDir = new File(DOWNLOAD_DIRECTORY + resBaseDir + "/");
            Log.d("SSSS", DOWNLOAD_DIRECTORY + resBaseDir + "/");
            Log.d("SSSS", "resDownloaDir.exists()=" + resDownloaDir.exists());
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
            return bitmap;
        } catch (OutOfMemoryError e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        // CID 111058 : Resource leak (RESOURCE_LEAK)
        } finally {
            Utils.closeSilently(inputStream);
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
            Utils.closeSilently(inputStream);
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
//        final Drawable drawable = context.getResources().getDrawable(R.drawable.collage_thumbnail_normal);
        final Drawable drawable = context.getResources().getDrawable(R.drawable.ucam_ic_menu_item_bg_normal);
        mThumbnailWidth = (int) (drawable.getIntrinsicWidth() - UiUtils.dpToPixel(14));
        mThumbnailHeight = (int) (drawable.getIntrinsicHeight() - UiUtils.dpToPixel(14));
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

    public static void openResourceCenter(Context activity, String downloadType) {

    }
}
