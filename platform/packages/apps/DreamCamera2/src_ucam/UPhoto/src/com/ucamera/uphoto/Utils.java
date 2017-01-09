/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto;

import android.app.Activity;
import android.app.ActivityManager;
import android.app.AlertDialog;
import android.content.ActivityNotFoundException;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.Bitmap.Config;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.Debug;
import android.os.Environment;
import android.util.Log;
import android.view.View;
import android.view.animation.Animation;
import android.view.animation.Animation.AnimationListener;
import android.view.animation.TranslateAnimation;
import android.widget.Toast;

import java.io.Closeable;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import com.sprd.camera.storagepath.StorageUtil;
public class Utils {
    public static final String TAG = "Utils";

    public static Bitmap createBitmapByStream(InputStream is) throws IOException {
        Bitmap bitmap = null;
        BitmapFactory.Options options = getNativeAllocOptions();
        if (is instanceof FileInputStream){
            // use decodeFileDescriptor can use the inPurgeable option for file
            bitmap = BitmapFactory.decodeFileDescriptor(((FileInputStream)is).getFD(), null, options);
        }
        else {
            // the inPurgeable option only work on AssetStream when use decodeStream()
            bitmap = BitmapFactory.decodeStream(is, null, options);
        }
        return bitmap;
    }

    public static BitmapFactory.Options getNativeAllocOptions() {
        BitmapFactory.Options options = new BitmapFactory.Options();
        //options.inNativeAlloc = true;
        options.inInputShareable = true;
        options.inPurgeable = true;
        options.inPreferredConfig = Bitmap.Config.ARGB_8888;
        return options;
    }

    public static void closeSilently(Closeable c) {
        if (c == null) return;
        try {
            c.close();
        } catch (Throwable t) {
            // ignore
        }
    }

    public static void openResourceCenter(Activity activity, float screenDensity) {
        openResourceCenter(activity, screenDensity,Const.EXTRA_FRAME_VALUE);
    }

    public static void openResourceCenter(Activity activity, float screenDensity, String downloadType) {
        String density = Const.EXTRA_DENSITY_HDPI;
        Intent intent = new Intent();
        intent.setClassName(activity.getApplicationContext(), "com.ucamera.ucomm.downloadcenter.DownloadTabActivity");
        intent.putExtra(Const.ACTION_DOWNLOAD_TYPE, downloadType);
        intent.putExtra(Const.ACTION_SCREEN_DENSITY, density);
        try {
            activity.startActivity(intent);
        } catch(ActivityNotFoundException e) {
            Toast.makeText(activity, R.string.text_activity_is_not_found, Toast.LENGTH_LONG).show();
        }
    }

    public static boolean checkDangousMemory(Context context){
        if (context == null) {
            return true;
        }

        ActivityManager activityManager = (ActivityManager)context.getSystemService(Context.ACTIVITY_SERVICE);
        ActivityManager.MemoryInfo sysMemInfo = new ActivityManager.MemoryInfo();
        activityManager.getMemoryInfo(sysMemInfo);
        Log.d(TAG,"MEMINFO:sysMemInfo.availMem = " + sysMemInfo.availMem
                 + " threshold = " + sysMemInfo.threshold + " lowMemory = " + sysMemInfo.lowMemory);

        int[] pids = new int[1];
        pids[0] = android.os.Process.myPid();
        Debug.MemoryInfo[] memInfos = activityManager.getProcessMemoryInfo(pids);
        if(memInfos.length > 1) {
            Log.d(TAG,"MEMINFO:memInfo.getTotalPss = " + memInfos[0].getTotalPss() +
                    " getTotalPrivateDirty = " + memInfos[0].getTotalPrivateDirty() +
                    " getTotalSharedDirty = " + memInfos[0].getTotalSharedDirty() +
                    " dalvikPss = " + memInfos[0].dalvikPss +
                    " dalvikPrivateDirty = " + memInfos[0].dalvikPrivateDirty +
                    " dalvikSharedDirty = " + memInfos[0].dalvikSharedDirty +
                    " nativePss = " + memInfos[0].nativePss +
                    " nativePrivateDirty = " + memInfos[0].nativePrivateDirty +
                    " nativeSharedDirty = " + memInfos[0].nativeSharedDirty +
                    " otherPss = " + memInfos[0].otherPss +
                    " otherPrivateDirty = " + memInfos[0].otherPrivateDirty +
                    " otherSharedDirty = " + memInfos[0].otherSharedDirty);
        }

        Runtime runtime = Runtime.getRuntime();
        long dalvikMaxSize = runtime.totalMemory() / 1024;
        long dalvikFree = runtime.freeMemory() / 1024;
        long dalvikAllocated = dalvikMaxSize - dalvikFree;
        long nativeHeapSize = Debug.getNativeHeapSize() / 1024;
        long nativeAllocated = Debug.getNativeHeapAllocatedSize() / 1024;
        long nativeFree = Debug.getNativeHeapFreeSize() / 1024;
        Log.d(TAG, "MEMINFO:nativeHeapSize =" + nativeHeapSize +
                " nativeAllocated =" + nativeAllocated +
                " nativeFree =" + nativeFree +
                " dalvikMaxSize =" + dalvikMaxSize +
                " dalvikAllocated =" + dalvikAllocated +
                " dalvikFree =" + dalvikFree);
        /*
         * FIX BUG: 3378
         * FIX COMMENT: modify judgment of memory
         * DATE: 2013-04-02
         */
        if (sysMemInfo.lowMemory || (dalvikMaxSize > 20000 && (float)dalvikAllocated/dalvikMaxSize > 0.999) ||
                (nativeHeapSize > 20000 && (float)nativeAllocated/nativeHeapSize > 0.999)) {
            Log.d(TAG,"MEMINFO:judge to dangous!");
            return true;
        }else {
            return false;
        }
    }

    public static void recyleBitmap(Bitmap bitmap){
        if (bitmap != null && !bitmap.isRecycled()) {
            bitmap.recycle();
        }
    }

    private static long exitTime = 0;

    /* return exit judgement.
     * true : finish activity
     * false : wait the second back in 2000 ms
     */
    public static boolean onClickBackToExit(Context context){
        if (System.currentTimeMillis() -exitTime > 2000) {
            Toast.makeText(context, context.getResources().getString(R.string.click_back_to_exit), Toast.LENGTH_SHORT).show();
            exitTime = System.currentTimeMillis();
            return false;
        }
        else {
//            if (context instanceof VideoCamera){
//                if (Camera.gCameraActivity != null) {
//                    Camera.gCameraActivity.finish();
            // }
            // }
            ((Activity)context).finish();
            return true;
        }
    }

    public static void showAnimation(View animView, AnimationListener listener) {
        if (animView == null)
            return;
        TranslateAnimation labelAnimShow = new TranslateAnimation(Animation.RELATIVE_TO_SELF, 0,
                Animation.RELATIVE_TO_SELF, 0, Animation.RELATIVE_TO_SELF, -1,
                Animation.RELATIVE_TO_SELF, 0);
        labelAnimShow.setDuration(500);
        labelAnimShow.setAnimationListener(listener);
        animView.startAnimation(labelAnimShow);
        animView.setVisibility(View.VISIBLE);
    }

    public static void showViewAnimation(View view) {
        if (view == null)
            return;
        TranslateAnimation topbarAnim = new TranslateAnimation(Animation.RELATIVE_TO_SELF, 0,
                Animation.RELATIVE_TO_SELF, 0, Animation.RELATIVE_TO_SELF, -1,
                Animation.RELATIVE_TO_SELF, 0);
        topbarAnim.setDuration(300);
        view.startAnimation(topbarAnim);
        view.setVisibility(View.VISIBLE);
    }

    public static void dismissViewAnimation(final View view, boolean beTop,
            AnimationListener animListener) {
        if (view == null)
            return;
        // to prevent the view starting animation twice
        Animation currViewAnimation = view.getAnimation();
        if (currViewAnimation != null && !currViewAnimation.hasEnded()) {
            view.setVisibility(View.GONE);
            return;
        }

        if (beTop) {
            // construct and start the animation
            TranslateAnimation topAnimationDis = new TranslateAnimation(Animation.RELATIVE_TO_SELF,
                    0, Animation.RELATIVE_TO_SELF, 0, Animation.RELATIVE_TO_SELF, 0,
                    Animation.RELATIVE_TO_SELF, -1);
            if (animListener == null) {
                animListener = new SimpleAnimationListener() {
                    @Override
                    public void onAnimationEnd(Animation animation) {
                        view.setVisibility(View.GONE);
                    }
                };
            }
            topAnimationDis.setAnimationListener(animListener);
            topAnimationDis.setDuration(300);
            view.startAnimation(topAnimationDis);
        } else {
            view.setVisibility(View.GONE);
        }
    }

    public static class SimpleAnimationListener implements AnimationListener {
        @Override public void onAnimationStart(Animation animation) {}
        @Override public void onAnimationEnd(Animation animation) {}
        @Override public void onAnimationRepeat(Animation animation) {}
    }

    public static boolean checkSDStatus(Context context) {
        //SPRD:fix bug537451 pull sd card, edit and puzzle can not work.
        //String state = Environment.getExternalStorageState();
        String state = StorageUtil.getInstance().getStorageState();
        if(Environment.MEDIA_MOUNTED.equals(state)) {
            return true;
        } else {
            int msgId = 0;
            if(Environment.MEDIA_CHECKING.equals(state)) {
                msgId = R.string.preparing_sd;
            } else {
                msgId = R.string.no_storage;
            }
            Toast.makeText(context, msgId, Toast.LENGTH_LONG).show();
        }

        return false;
    }
    public static Bitmap makeMosaicBitmap(Bitmap src, Bitmap cover) {
      final int block = 18;
//      if(cover!=null) {
//            Util.Assert(cover.getWidth()==block && block==cover.getHeight());
//      }
      int bmpWidth = src.getWidth();
      int bmpHeight = src.getHeight();
      Bitmap dst = Bitmap.createBitmap(bmpWidth, bmpHeight, Config.ARGB_8888);
      Canvas c = new Canvas(dst);
      Paint p = new Paint();
      int blockX = (bmpWidth+block-1)/block;
      int blockY = (bmpHeight+block-1)/block;
      int step = block/4;
      step = Math.max(step, 1);
      for(int y=0; y<blockY; y++) {
            for(int x=0; x<blockX; x++) {
                  int r = 0;
                  int g = 0;
                  int b = 0;
                  int count = 0;
                  int startX = x*block;
                  int endX = startX+block;
                  endX = Math.min(endX, bmpWidth);
                  int startY = y*block;
                  int endY = startY+block;
                  endY = Math.min(endY, bmpHeight);
                  for(int y1=startY; y1<endY; y1+=step) {
                        for(int x1=startX; x1<endX; x1+=step) {
                              int color = src.getPixel(x1, y1);
                              r += Color.red(color);
                              g += Color.green(color);
                              b += Color.blue(color);
                              count ++;
                        }
                  }
                  r /= count;
                  g /= count;
                  b /= count;
                  p.setColor(Color.rgb(r, g, b));
                  c.drawRect(startX, startY, endX, endY, p);
                  if(cover!=null) {
                        c.drawBitmap(cover, null, new Rect(startX, startY, endX, endY), null);
                  }
            }
      }
      return dst;
    }
}
