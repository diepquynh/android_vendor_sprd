/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.ucomm.puzzle;

import android.app.ActivityManager;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.drawable.BitmapDrawable;
import android.net.Uri;
import android.os.AsyncTask;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.Toast;

import com.ucamera.ucomm.puzzle.util.PuzzleBitmapManager;
import com.ucamera.ugallery.UCamToast;


public abstract class PuzzleView extends ViewGroup {

    private static final String TAG = "PuzzleView";
    protected Puzzle mPuzzle = Puzzle.EMPTY_PUZZLE;
    protected PuzzleSpec mPuzzleSpec = Puzzle.EMPTY_PUZZLE.getSpec();
    protected PuzzlePiece mCurrentPiece = null;
    protected boolean mInitialized = false;

    public PuzzleView(Context context) {
        super(context);
    }

    public PuzzleView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    public PuzzleView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public boolean isInitializeFinish() {
        return mInitialized;
    }
    /**
     * Tricky to avoid casting
     */
    @Override
    public PuzzlePiece getChildAt(int index) {
        return (PuzzlePiece) super.getChildAt(index);
    }

    /**
     *  ONLY {@link PuzzlePiece} can be as child, IGNORE other types!
     */
    @Override
    public void addView(View child, int index, LayoutParams params) {
        if (child instanceof PuzzlePiece){
            super.addView(child, index, params);
        }
    }

    ////////////////////////////////////////////////////////////////////
    //      ABSTRACT METHODS
    protected abstract void createPuzzle(int count);
    public abstract PuzzleView addPiece(Bitmap bitmap, int num, Uri uri);

    ///////////////////////////////////////////////////////////////////
    //      USER DEFINED METHODS

    public void reset() {
        mInitialized = false;
        removeAllViews();
    }

    public void shuffle(int index) {
        mPuzzle.random(index);
        for (int i = 0, max = getChildCount(); i < max; i++) {
            getChildAt(i).scrollTo(0, 0);
        }
        requestLayout();
    }

    /*public void shuff(Puzzle puzzle) {
        mPuzzle = puzzle;
        for (int i = 0, max = getChildCount(); i < max; i++) {
            getChildAt(i).scrollTo(0, 0);
        }
        requestLayout();
    }*/

    public void shuff(PuzzleSpec puzzle) {
        mPuzzleSpec = puzzle;
        for (int i = 0, max = getChildCount(); i < max; i++) {
            getChildAt(i).scrollTo(0, 0);
        }
        requestLayout();
    }
    public void borrowBitmapsFrom(PuzzleView src){
        if ( !mInitialized || !src.mInitialized){
            return;
        }

        for (int i=0; i<src.getChildCount(); i++){
            getChildAt(i).borrowBitmap(src.getChildAt(i));
        }
    }

    public  void initPuzzleByCopy(PuzzleView src) {
        /*
         * BUG Fix: 1394
         * FIX COMMENT: add check before call some method which may raise NullPointException
         * Date: 2012-07-31
         */
        if (mInitialized == true || src == null || !src.mInitialized) {
            return;
        }

        final int count = src.getChildCount();
        createPuzzle(count);
        for ( int i=0; i<count; i++){
            final BitmapDrawable drawable = (BitmapDrawable)src.getChildAt(i).getDrawable();
            final int num = src.getChildAt(i).getNum();
            final Uri uri = src.getChildAt(i).getUri();
            if (drawable != null) {
                addPiece(drawable.getBitmap(), i, uri);
            }
        }
        mInitialized = true;
    }

    private class UpdateResult{
        public int num;
        public Uri uri;
        public Bitmap bitmap;
        public UpdateResult(Bitmap bitmap, int num, Uri uri) {
            this.num = num;
            this.uri = uri;
            this.bitmap = bitmap;
        }
    }
    /**
     * Be sure to call this before show!
     */
    public void initPuzzleWithUris(Uri[] imageUris, final OnInitListener initListener, PuzzleSpec mspec) {
        if (mInitialized == true)
            return;

        this.mPuzzleSpec = mspec;
        createPuzzle(imageUris.length);
        new AsyncTask<Uri, UpdateResult, Void>() {
            private ProgressDialog mSpinner;
            @Override
            protected void onPreExecute() {
                mSpinner = new ProgressDialog(getContext());
                mSpinner.requestWindowFeature(Window.FEATURE_NO_TITLE);
                mSpinner.setMessage(getContext().getString(R.string.puzzle_text_waiting));
                mSpinner.setOnCancelListener(new DialogInterface.OnCancelListener() {
                    @Override public void onCancel(DialogInterface dialog) {
                        cancel(true);
                    }
                });
                mSpinner.setCancelable(false);
                mSpinner.show();
            }

            @Override
            protected Void doInBackground(Uri... params) {
                for (int i = 0, count=params.length; i < count; i++) {
                    if (isCancelled()) break;
                    if(PuzzleBitmapManager.getInstance().containsBitmap(i)) {
                        publishProgress(new UpdateResult(PuzzleBitmapManager.getInstance().getBitmap(i), i, params[i]));
                    } else {
//                        publishProgress(createBitmap(params[i], count));
                        publishProgress(new UpdateResult(createBitmap(params[i], count, i), i, params[i]));
                    }
                }
                return null;
            }

            @Override
            protected void onProgressUpdate(UpdateResult... values) {
                if (values.length > 0 && values[0] != null && values[0].bitmap != null) {
                    addPiece(values[0].bitmap, values[0].num, values[0].uri);
                } else {
                    UCamToast.showToast(getContext(), R.string.edit_operation_fail,
                            Toast.LENGTH_SHORT);
                    cancel(true);
                }
            }

            @Override
            protected void onCancelled() {
                removeAllViews();
                mInitialized = false;
                mSpinner.dismiss();
                mSpinner = null;
                if (initListener != null) {
                    initListener.onFail();
                }
            }

            @Override
            protected void onPostExecute(Void result) {
                if (initListener != null){
                    initListener.onInit();
                }
                mInitialized = true;
                requestLayout();
                try {
                    mSpinner.dismiss();
                } catch (Exception e) {
                }
                mSpinner = null;
            }

            private Bitmap createBitmap(Uri uri, int count, int index) {
                Bitmap bitmap = null;
                int size = getMaxBitmapSize(count);
                android.util.Log.d("PuzzleView", "max bitmap size=" + size);
                while(bitmap == null && size > 50) {
                    try {
                        bitmap = PuzzleUtils.createBitmap(getContext(), uri,size,size);
                        if (bitmap != null) {
                            PuzzleBitmapManager.getInstance().addBitmap(index, bitmap);
                            break;
                        }
                        size /= 2;
                    } catch(OutOfMemoryError oom) {
                        android.util.Log.e("PuzzleView","Fail create bitmap",oom);
                        size /= 2;
                        bitmap = null;
                    }
                }
                return bitmap;
            }

            private int getMaxBitmapSize(int number) {
                final ActivityManager activityManager =
                    (ActivityManager) getContext().getSystemService(Context.ACTIVITY_SERVICE);
                ActivityManager.MemoryInfo memInfo = new ActivityManager.MemoryInfo();
                activityManager.getMemoryInfo(memInfo);
                Log.d("PuzzleView", "availMem:" + memInfo.availMem
                            + "threshold:" + memInfo.threshold);
                double availMem = memInfo.availMem * 0.5;
                double maxSize = (availMem/4) / number;
                int size=1024;

                /* FIX BUG: 3770
                 * BUG CAUSE: Memory is not enough after editing collage photo
                 * FIX COMMENT: reduce the bitmap size
                 * DATE: 2013-05-08
                 */
                if("sp7710ga".equals(PuzzleUtils.getModle())) {
                    size = 320;
                }else if("QRD_8625DSDS".equals(PuzzleUtils.getModle())){
                    size = 640;
                }
                return (int)Math.min(Math.sqrt(maxSize), size);
            }
        }.execute(imageUris);
    }

    public Bitmap exportScaledBitmap(int width, int height, boolean keepRatio) {
        // SPRD: fix bug 548755 cause crash when width or height less than zero
        if (getWidth() <= 0 || getHeight() <=0 || width <= 0 || height <= 0){
            Log.e(TAG, "exportScaledBitmap: width or height less than zero !");
            return null;
        }
        float scaleX = width / (float) getWidth();
        float scaleY = height / (float) getHeight();
        if (keepRatio) {
            scaleX = scaleY = Math.min(scaleX, scaleY);
            width = (int) (scaleX * getWidth());
            height = (int) (scaleY * getHeight());
        }
        Bitmap bitmap = Bitmap.createBitmap(width, height, Config.ARGB_8888);
        if (bitmap == null) {
            return null;
        }
        Canvas canvas = new Canvas(bitmap);
        Matrix m = canvas.getMatrix();
        m.setScale(scaleX, scaleY);
        canvas.setMatrix(m);
        draw(canvas);
        return bitmap;
    }

    public Bitmap exportBitmap() {
        if (getWidth() <= 0 || getHeight() <=0){
            return null;
        }
        Bitmap bitmap = Bitmap.createBitmap(getWidth(), getHeight(), Config.ARGB_8888);
        /* FIX BUG: 1820
         * BUG CAUSE: null point exception
         * FIX COMMENT: do not create canvas if bitmap is null
         * DATE: 2012-11-02
         */
        if(bitmap == null) return null;
        Canvas canvas = new Canvas(bitmap);
        draw(canvas);
        return bitmap;
    }

    public PuzzlePiece getPieceAt(int x, int y){
        for (int i=0, max=getChildCount(); i<max; i++) {
            PuzzlePiece piece = getChildAt(i);
            if (piece.isPointInView(x, y)){
                return piece;
            }
        }
        return null;
    }

    public void setCurrentPiece(PuzzlePiece piece){
        if (piece == mCurrentPiece) {
            return;
        }

        if (mCurrentPiece != null) {
            mCurrentPiece.setSelected(false);
        }
        if (piece != null){
            piece.setSelected(true);
        }
        mCurrentPiece = piece;
    }

    public PuzzlePiece getCurrentPiece(){
        return mCurrentPiece;
    }

    //////////////////////////////////
    public interface OnInitListener{
        void onInit();
        void onFail();
    }
}
