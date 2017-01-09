/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ugallery;

import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.net.Uri;
import android.widget.Toast;

import com.ucamera.ugallery.gallery.IImage;

import java.util.List;

public class DefaultImagePicker extends BaseImagePicker {

    private static final int MAX_LIMIT = 9;
    private static final int MIN_LIMIT = 1;
    private static final int NO_VALUE = -1;

    private int mMaxLimit = NO_VALUE;
    /* (non-Javadoc)
     * @see com.ucamera.ucam.common.BaseImagePicker#onSelectDone(java.util.List)
     */
    @Override
    protected void onSelectDone(List<IImage> selected) {
        Intent result = new Intent();
        Uri[] images = new Uri[selected.size()];
        for (int i=0; i<selected.size(); i++){
            images[i] = selected.get(i).fullSizeImageUri();
        }

        result.putExtra(ImageGallery.INTENT_EXTRA_IMAGES, images);
        setResult(RESULT_OK, result);
        finish();
    }

    @Override
    protected void onSelected(IImage image) {
        Intent result = new Intent();
        result.setAction("android.intent.action.UGALLERY_EDIT");
        result.setType("image/*");
        Uri[] images = new Uri[] {image.fullSizeImageUri()};
        result.putExtra(ImageGallery.INTENT_EXTRA_IMAGES, images);
        try {
            setResult(RESULT_OK, result);
        } catch(ActivityNotFoundException e) {
            Toast.makeText(this, R.string.text_activity_is_not_found, Toast.LENGTH_LONG).show();
        }
        finish();
    }

    @Override protected int getMaxLimit() {
        if (mMaxLimit == NO_VALUE) {
            Intent intent = getIntent();
            if (intent != null) {
               mMaxLimit = intent.getIntExtra(ImageGallery.INTENT_EXTRA_IMAGE_MAX_LIMIT, MAX_LIMIT);
            }

            if (mMaxLimit == NO_VALUE) {
               mMaxLimit = MAX_LIMIT;
            }
        }
        return mMaxLimit;
    }

    @Override protected int getMinLimit() {return MIN_LIMIT;}
}
