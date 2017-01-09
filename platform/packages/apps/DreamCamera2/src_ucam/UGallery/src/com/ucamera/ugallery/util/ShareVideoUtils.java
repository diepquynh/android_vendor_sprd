/*
 * Copyright (C) 2014,2015 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ugallery.util;

import android.content.Context;
import android.content.Intent;
import android.widget.Toast;

import com.ucamera.ugallery.gallery.IImage;
import com.ucamera.ugallery.integration.Build;
import com.ucamera.ugallery.R;
public class ShareVideoUtils {
    private static boolean OtherVideoSns = false;
    public static void setOtherVideoSns(boolean sns) {
        OtherVideoSns = sns;
    }
    public static void shareVideo(Context context, IImage image) {
        Intent intent = new Intent();
        intent.setAction(Intent.ACTION_SEND);
        intent.setType(image.getMimeType());
        intent.putExtra(Intent.EXTRA_STREAM, image.fullSizeImageUri());
        if(OtherVideoSns) {
//            ShareUtils.shareVideo(context, intent);
        } else {
            try {
                context.startActivity(Intent.createChooser(intent, context.getText(R.string.sendVideo)));
            } catch (android.content.ActivityNotFoundException ex) {
                Toast.makeText(context, R.string.no_way_to_share_video, Toast.LENGTH_SHORT).show();
            }
        }
    }
}
