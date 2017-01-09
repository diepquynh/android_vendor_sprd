/*
 *   Copyright (C) 2011,2012 Thundersoft Corporation
 *   All rights Reserved
 */
package com.ucamera.ucomm.puzzle;

import android.content.Intent;
import android.net.Uri;

import com.ucamera.ugallery.BaseImagePicker;
import com.ucamera.ugallery.ImageGallery;
import com.ucamera.ugallery.gallery.IImage;

import java.util.List;

public class PuzzleImagePicker extends BaseImagePicker {

    private static final int MAX_LIMIT = 9;
    private static final int MIN_LIMIT = 2;

    @Override
    protected void onSelectDone(List<IImage> selected) {
        Intent intent = new Intent(this, PuzzleActivity.class);
        Uri[] images = new Uri[selected.size()];
        for (int i=0; i<selected.size(); i++){
            images[i] = selected.get(i).fullSizeImageUri();
        }
        intent.putExtra(PuzzleActivity.INTENT_EXTRA_IMAGES, images);
        startActivity(intent);

        /*
         * FIX BUG 1188
         * BUG COMMENT: finsh current activity when start PuzzleActivity.
         * DATE: 2012-07-03
         */
        Intent result = new Intent(this, ImageGallery.class);
        setResult(RESULT_OK, result);
        finish();
    }

    @Override
    protected int getMaxLimit() {return MAX_LIMIT;}
    @Override
    protected int getMinLimit() {return MIN_LIMIT;}
}
