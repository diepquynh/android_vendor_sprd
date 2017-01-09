/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucomm.puzzle.free;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.drawable.BitmapDrawable;
import android.net.Uri;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.widget.ImageView;
import android.widget.ImageView.ScaleType;
import android.widget.TextView;

import com.ucamera.ucomm.downloadcenter.Constants;
import com.ucamera.ucomm.downloadcenter.DownloadCenter;
import com.ucamera.ucomm.puzzle.Puzzle;
import com.ucamera.ucomm.puzzle.R;
import com.ucamera.ucomm.puzzle.PuzzlePiece;
import com.ucamera.ucomm.puzzle.PuzzleSpec;
import com.ucamera.ucomm.puzzle.PuzzleUtils;
import com.ucamera.ucomm.puzzle.PuzzleView;
import com.ucamera.ucomm.puzzle.util.PuzzleBitmapManager;

public class FreePuzzleView extends PuzzleView {
    private static final String TAG = "FreePuzzleView";
    private MultiTouchListener mMultiTouchListener = new MultiTouchListener();
    public FreePuzzleView(Context context) {
        super(context);
    }

    public FreePuzzleView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public FreePuzzleView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

   public void shuffle(int index) {
       mPuzzle.random(index);
       refreshPuzzlePieceInfo();
       requestLayout();
    }

   private void refreshPuzzlePieceInfo() {
        int count = getChildCount();
        for (int i = 0; i < count; i++) {
            FreePuzzlePiece piece = (FreePuzzlePiece) getChildAt(i);
            piece.refreshInfo(mPuzzle.getSpecInfo(i));
            piece.invalidate();
        }
    }

    @Override
    public void borrowBitmapsFrom(PuzzleView src) {
        super.borrowBitmapsFrom(src);
        for (int i = 0; i < src.getChildCount(); i++) {
            getChildAt(i).updateUri(src.getChildAt(i).getUri());
        }
    }

    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b) {
        PuzzleSpec spec = mPuzzle.getSpec();
        final int childCount = getChildCount();
        for (int i = 0; i < spec.length() && i < childCount; i++) {
            getChildAt(i).layout(0, 0, r - l, b - t);
        }
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        int width = getDefaultSize(getSuggestedMinimumWidth(), widthMeasureSpec);
        int height = getDefaultSize(getSuggestedMinimumHeight(), heightMeasureSpec);
        if((float)width / height < (float)3 / 4) {
            height = width * 4 /3;
        }else {
            width = height * 3 / 4;
        }
        setMeasuredDimension(width, height);
    }

    public void setWidthAndHeight(int width, int height) {
        int count = getChildCount();
        for (int i = 0; i < count; i++) {
            FreePuzzlePiece piece = (FreePuzzlePiece) getChildAt(i);
            piece.setParentWidthAndHeight(width, height);
        }
    }

    @Override
    public void createPuzzle(int count) {
        count = Math.min(9, Math.max(2, count));
        mPuzzle = Puzzle.create(Puzzle.Type.FREE, count).random(0);
    }

    @Override
    public PuzzleView addPiece(Bitmap bitmap, int num, Uri uri) {
        FreePuzzlePiece piece = new FreePuzzlePiece(getContext(), num, uri);
        piece.setImageBitmap(bitmap);
        piece.setScaleType(ScaleType.MATRIX);
        if (getHeight() > 0) {
            piece.setParentWidthAndHeight(getWidth(), getHeight());
            if(mPuzzle != null)
                piece.refreshInfo(mPuzzle.getSpecInfo(getChildCount()));
        }
        addView(piece);
        return this;
    }

    @Override
    public PuzzlePiece getPieceAt(int x, int y) {
        // SPRD: Fix bug 542626 ClassCastException happens when cast mCurrentPiece to FreePuzzlePiece
        if (mCurrentPiece != null && mCurrentPiece.isPointInView(x, y)) {
            return mCurrentPiece;
        }

        FreePuzzlePiece piece = null;
        for (int i = 0, max = getChildCount(); i < max; i++) {
            FreePuzzlePiece p1 = (FreePuzzlePiece) getChildAt(i);
            if (p1.isPointInView(x, y)) {
                piece = p1;
                break;
            }
        }

        return piece;
    }

    @Override
    public void setCurrentPiece(PuzzlePiece piece) {
        if(piece == mCurrentPiece) {
            return;
        }
        super.setCurrentPiece(piece);
        if (mCurrentPiece != null) {
            mCurrentPiece.bringToFront();
            PuzzleBitmapManager.getInstance().clear();
            requestLayout();
            for(int i = 0; i < this.getChildCount(); i++) {
                PuzzleBitmapManager.getInstance().addBitmap(i, ((FreePuzzlePiece)this.getChildAt(i)).getImageBitmap());
            }
        }
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (mCurrentPiece != null) {
            return mMultiTouchListener.onTouch(mCurrentPiece, event);
        }
        return false;
    }

    @SuppressWarnings("deprecation")
    public void setBackgroundFile(Context context, String fileName ) {
        if (fileName == null)
            return;
        Log.d(TAG, "fileName" + fileName);
        Bitmap bitmap =  DownloadCenter.thumbNameToBitmap(context, fileName, Constants.EXTRA_PUZZLE_VALUE);
        BitmapDrawable bitmapDrawable = new BitmapDrawable(getResources(), bitmap);
        this.setBackgroundDrawable(bitmapDrawable);
    }

    public ArrayList<? extends View> createAdapterItems(Context context, String[] puzzleImages) {
        if(puzzleImages == null) {
            return null;
        }
        ArrayList<View> lists = new ArrayList<View>();
        for (int i = 0; i < puzzleImages.length; i++) {
            View view = View.inflate(context, R.layout.menu_free_puzzle_item, null);
            if(view == null) {
                continue;
            }
            ImageView imageView = (ImageView) view.findViewById(R.id.iv_free_puzzle_item_bk);
            TextView ivFress = (TextView)view.findViewById(R.id.tv_free_puzzle_free);
            if(imageView == null) {
                continue;
            }
            Bitmap bitmap = DownloadCenter.thumbNameToThumbBitmap(context, puzzleImages[i], Constants.EXTRA_PUZZLE_VALUE);
            if(bitmap != null) {
                if(i == 0 && DownloadCenter.RESOURCE_DOWNLOAD_ON) {
                    ivFress.setVisibility(View.VISIBLE);
                    imageView.setBackgroundResource(R.drawable.puzzle_bg_download_normal);
                }else{
                    ivFress.setVisibility(View.GONE);
                    imageView.setImageBitmap(bitmap);
                }
            }
            lists.add(view);
        }
        return lists;
    }
}
