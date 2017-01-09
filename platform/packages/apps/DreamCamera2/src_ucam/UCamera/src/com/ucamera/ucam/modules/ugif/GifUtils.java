/*
 *   Copyright (C) 2010,2013 Thundersoft Corporation
 *   All rights Reserved
 */

package com.ucamera.ucam.modules.ugif;

import android.content.Context;
import android.database.Cursor;
import android.provider.MediaStore.Images;
import android.provider.MediaStore.Images.Media;

public class GifUtils {
    /*
     * SPRD: Fix bug 538832 When decoding large size GIF pictures, the camera
     * appears no response @{
     */
    private static final String WHERE_CLAUSE = "(" + Media.MIME_TYPE + " in ('image/gif'))"
            + " AND " + Media.SIZE + "> 0 " + " AND " + Media.DATA + " LIKE " + "('%DCIM/Camera%')";
    /* @} */
    public static Cursor getGifListFromDB(Context context) {
        final String[] IMAGE_PROJECTION = new String[] {
                Media._ID, Media.DATA, Media.SIZE
        };
        Cursor rc = context.getContentResolver().query(Images.Media.EXTERNAL_CONTENT_URI,
                IMAGE_PROJECTION, WHERE_CLAUSE, null, null);
        return rc;
    }
}
