/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ugallery;

import android.content.Intent;
import android.net.Uri;

import com.ucamera.ugallery.gallery.IImage;

public class IdPhotoImagePicker extends BaseImagePicker {
    private static final int MAX_LIMIT = 1;

    @Override
    protected void onSelected(IImage image) {
        Intent intent = new Intent();
        intent.setClassName(this, "com.ucamera.ugallery.crop.CropImageActivity");
        intent.setData(image.fullSizeImageUri());
        intent.putExtra("idphoto_type", getUphotoModule());
        startActivity(intent);

        Intent result = new Intent(this, ImageGallery.class);
        setResult(RESULT_OK, result);
        finish();
    }

    @Override
    protected int getMaxLimit() {return MAX_LIMIT;}
}
