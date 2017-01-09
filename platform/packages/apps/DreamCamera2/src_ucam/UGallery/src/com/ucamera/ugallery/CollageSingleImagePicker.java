/*
 * Copyright (C) 2014,2015 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ugallery;

import android.content.Intent;
import android.net.Uri;

import com.ucamera.ugallery.gallery.IImage;

public class CollageSingleImagePicker extends BaseImagePicker {
    private static final int MAX_LIMIT = 1;

    @Override
    protected void onSelected(IImage image) {
        Intent intent = new Intent();
        Uri[] images = new Uri[] {image.fullSizeImageUri()};
        intent.putExtra(ImageGallery.INTENT_EXTRA_IMAGES, images);
        setResult(RESULT_OK, intent);
        finish();
    }

    @Override
    protected int getMaxLimit() {return MAX_LIMIT;}
}
