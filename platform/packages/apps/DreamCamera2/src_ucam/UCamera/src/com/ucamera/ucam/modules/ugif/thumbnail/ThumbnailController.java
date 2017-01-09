/*
 * Copyright (C) 2011,2013 Thundersoft Corporation
 * All rights Reserved
 *
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.ucamera.ucam.modules.ugif.thumbnail;


import android.content.ContentResolver;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.TransitionDrawable;
import android.media.ThumbnailUtils;
import android.net.Uri;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.os.ParcelFileDescriptor;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup.LayoutParams;
import android.view.animation.Animation;
import android.view.animation.AnimationSet;
import android.view.animation.ScaleAnimation;
import android.widget.ImageView;
import android.widget.RelativeLayout;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;

import com.android.camera2.R;
import com.ucamera.ucam.modules.ui.PreviewFrameLayout;
import com.ucamera.ucam.modules.utils.LogUtils;
import com.ucamera.ucam.utils.UiUtils;
import com.ucamera.ucam.modules.utils.Utils;

/**
 * A controller shows thumbnail picture on a button. The thumbnail picture
 * corresponds to a URI of the original picture/video. The thumbnail bitmap
 * and the URI can be saved to a file (and later loaded from it).
 */
public class ThumbnailController {
    private static final String TAG = "ThumbnailController";
    private final ContentResolver mContentResolver;
    private Uri mUri;
    private Bitmap mThumb;
    private final ImageView mButton;
    private ImageView mLastButton;
    private RelativeLayout mThumbnailLayout;
    // CID 109202 : UrF: Unread field (FB.URF_UNREAD_FIELD)
    // private Drawable[] mThumbs;
    // CID 109234 : NP: Null pointer dereference (FB.NP_UNWRITTEN_FIELD)
    // private TransitionDrawable mThumbTransition;
    private boolean mShouldAnimateThumb;
    private final Resources mResources;

    // The "frame" is a drawable we want to put on top of the thumbnail.
    public ThumbnailController(Resources resources,
            ImageView button, ContentResolver contentResolver) {
        mResources = resources;
        mButton = button;
        mContentResolver = contentResolver;
    }

    public void setData(Uri uri, Bitmap original,boolean needAnim) {
        // Make sure uri and original are consistently both null or both
        // non-null.
        if (uri == null || original == null || original.isRecycled()) {
            uri = null;
            original = null;
        }
        mUri = uri;
        updateThumb(original,needAnim);
    }

    public void setData(Bitmap original,boolean needAnim) {
        // Make sure uri and original are consistently both null or both
        // non-null.
        if (original == null || original.isRecycled()) {
            original = null;
        }
        updateThumb(original,needAnim);
    }

    public void setUri(Uri uri) {
        mUri = uri;
    }

    public Uri getUri() {
        return mUri;
    }

    public Bitmap getThumbBitmap() {
        return mThumb;
    }

    private static final int BUFSIZE = 4096;

    // Stores the data from the specified file.
    // Returns true for success.
    public boolean storeData(File file) {
        if (mUri == null) {
            file.delete();
            return false;
        }

        FileOutputStream f = null;
        BufferedOutputStream b = null;
        DataOutputStream d = null;
        try {
            f = new FileOutputStream(file);
            b = new BufferedOutputStream(f, BUFSIZE);
            d = new DataOutputStream(b);
            d.writeUTF(mUri.toString());
            mThumb.compress(Bitmap.CompressFormat.PNG, 100, d);
            d.close();
        } catch (IOException e) {
            file.delete();
            return false;
        } finally {
            Utils.closeSilently(f);
            Utils.closeSilently(b);
            Utils.closeSilently(d);
        }
        return true;
    }

    public void updateDisplayIfNeeded() {
        if (mUri == null) {
            // fix bug 30122. when no pictures or records show the default image.
            //mButton.setImageResource(R.drawable.photos_preview_p);
            return;
        }

        if (mShouldAnimateThumb && (mButton != null)) {
            mShouldAnimateThumb = false;
            // CID 109234 : NP: Null pointer dereference (FB.NP_UNWRITTEN_FIELD)
            // CID 109361 : UwF: Unwritten field (FB.UWF_UNWRITTEN_FIELD)
            // mThumbTransition.startTransition(0);
            AnimationSet animSet = new AnimationSet(true);
            animSet.setInterpolator(mButton.getContext(),android.R.anim.accelerate_decelerate_interpolator);
            ScaleAnimation anim = new ScaleAnimation(0f, 1.5f, 0f, 1.5f,Animation.RELATIVE_TO_SELF,0.5f,Animation.RELATIVE_TO_SELF, 0.5f);
            anim.setDuration(180);
            animSet.addAnimation(anim);

            anim = new ScaleAnimation(1.5f, 1f, 1.5f, 1f,Animation.RELATIVE_TO_SELF,0.5f,Animation.RELATIVE_TO_SELF, 0.5f);
            anim.setDuration(180);
            animSet.addAnimation(anim);
            mButton.startAnimation(animSet);
        }
    }

    public void setLastImageButton(ImageView imageView){
        mLastButton = imageView;
    }

    public void setLayout(RelativeLayout layout){
        mThumbnailLayout = layout;
    }

    private void updateThumb(Bitmap original,boolean needAnim) {
        if (original == null) {
            mThumb = null;
            // CID 109202 : UrF: Unread field (FB.URF_UNREAD_FIELD)
            // mThumbs = null;
            return;
        }

        final int miniThumbWidth = 86;
        final int miniThumbHeight = 78;
        Bitmap bitmapOrg = null;
        try{
            bitmapOrg = ThumbnailUtils.extractThumbnail(original, miniThumbWidth, miniThumbHeight);
        }catch(OutOfMemoryError oom) {
            bitmapOrg = original;
        }
        mThumb = Utils.roundRectBitmap(bitmapOrg,0);
        bitmapOrg.recycle();
        bitmapOrg = null;
        /* FIX BUG: OOM in log of bug 4117
         * BUG CAUSE: the original bitmap is never recycled, and introduces a memory leak
         * FIX COMMENT: recycle the original bitmap
         * DATE: 2011-09-15
         */
        original.recycle();
        original = null;

        final Drawable drawable = new BitmapDrawable(mResources, mThumb);
        /*if (mThumbs == null) {
            mThumbs = new Drawable[2];
            mThumbs[1] = new BitmapDrawable(mResources, mThumb);
            drawable = mThumbs[1];
            mShouldAnimateThumb = false;
        } else {
            mThumbs[0] = mThumbs[1];
            mThumbs[1] = new BitmapDrawable(mResources, mThumb);
            mThumbTransition = new TransitionDrawable(mThumbs);
            drawable = mThumbTransition;
            mShouldAnimateThumb = true;
        }*/
        /*
         * FIX BUG: 4861
         * BUG COMMENT:set delay is 0 if don't need animation
         * DATE: 2013-11-06
         */
        long delayTime = 0;
        if(needAnim){
            delayTime = 1200;
            if(mThumbnailLayout != null){
                mThumbnailLayout.setBackgroundResource(R.drawable.bg_control_thumbnail_press);
            }
            if(mButton != null) {
                Animation animation = UiUtils.createTranslateAnimation(mButton.getContext(), 0, 0, -100, 0, 500);
                mButton.setVisibility(View.VISIBLE);
                mButton.setImageDrawable(drawable);
                mButton.startAnimation(animation);
            }
        }
        new Handler().postDelayed(new Runnable() {
            @Override
            public void run() {
                if(mLastButton != null){
                    mLastButton.setImageDrawable(drawable);
                    mLastButton.setVisibility(View.VISIBLE);
                    if(mButton != null){
                        mButton.setVisibility(View.GONE);
                    }
                }
                if(mThumbnailLayout != null){
                    mThumbnailLayout.setBackgroundResource(R.drawable.bg_control_thumbnail);
                }
            }
        }, delayTime);
    }

    public boolean isUriValid() {
        if (mUri == null) {
            Log.e("PanoramaModule", "mUri == null");
            return false;
        }
        try {
            ParcelFileDescriptor pfd =
                    mContentResolver.openFileDescriptor(mUri, "r");
            if (pfd == null) {
                Log.e("PanoramaModule", "Fail to open URI.");
                Log.e(TAG, "Fail to open URI.");
                return false;
            }
            pfd.close();
        } catch (IOException ex) {
            return false;
        }
        return true;
    }
}
