/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ugallery;

import android.content.Intent;
import android.net.Uri;

import com.ucamera.ugallery.gallery.IImage;

public class UphotoImagePicker extends BaseImagePicker {
    private static final int MAX_LIMIT = 1;

    @Override
    protected void onSelected(IImage image) {
        Intent intent = new Intent();
        intent.setClassName(this, "com.ucamera.uphoto.ImageEditControlActivity");
        intent.setAction("android.intent.action.UGALLERY_EDIT");
//        intent.setType("image/*");
        intent.putExtra("extra_pick", "uphoto_pick");
        intent.putExtra(ViewImage.EXTRA_UPHOTO_MODULE_KEY, getUphotoModule());
//        Uri[] images = new Uri[] {image.fullSizeImageUri()};
//        intent.putExtra("extra_images", images);
        intent.setData(image.fullSizeImageUri());
        startActivity(intent);

        Intent result = new Intent(this, ImageGallery.class);
        setResult(RESULT_OK, result);
        finish();
    }

    @Override
    protected int getMaxLimit() {return MAX_LIMIT;}
}
