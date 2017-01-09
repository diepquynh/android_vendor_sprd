/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.ugallery;

import android.content.ContentResolver;
import android.content.Context;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.provider.MediaStore.Images.Media;
import android.util.Log;

import com.ucamera.ugallery.util.ImageManager;
import com.ucamera.ugallery.util.Util;

import java.io.IOException;
import java.io.InputStream;

public class BitmapUtil {
    private Context mContext;

    private BitmapUtil(Context context) {
        mContext = context;
    }

    public static BitmapUtil create(Context context) {
        return new BitmapUtil(context);
    }

    public Bitmap createBitmap(Uri uri, int maxWidth, int maxHeight) {
        InputStream is = null;

        // decode bitmap bounds
        int bitmapWidth = 0;
        int bitmapHeight = 0;
        try {
            BitmapFactory.Options options = new BitmapFactory.Options();
            options.inJustDecodeBounds = true;
            is = mContext.getContentResolver().openInputStream(uri);
            BitmapFactory.decodeStream(is,null,options);
            bitmapWidth = options.outWidth;
            bitmapHeight = options.outHeight;
        } catch (IOException e) {
            Log.e("BitmapUtil", "fail get bounds info",e);
            // fail get bounds info
            return null;
        } finally {
            Util.closeSilently(is);
        }
        //calc sampleSize accounding width/height
        int sampleSize = 1;
        while (maxWidth < (bitmapWidth / sampleSize) || maxHeight < (bitmapHeight / sampleSize) ) {
            sampleSize <<= 1;
        }

        //decode the bitmap
        try {
            Bitmap tmp = null;
            BitmapFactory.Options options = new BitmapFactory.Options();
            options.inSampleSize = sampleSize;
            is = mContext.getContentResolver().openInputStream(uri);
            tmp = BitmapFactory.decodeStream(is,null,options);

            float scale = Math.min(1,Math.min((float)maxWidth/options.outWidth, (float)maxHeight/options.outHeight));
            int w = (int)(options.outWidth * scale);
            int h = (int)(options.outHeight*scale);
            Bitmap bitmap = Bitmap.createScaledBitmap(tmp,w,h, false);
            if (bitmap != tmp){
                /*
                 * BUG FIX: 1487
                 * FIX COMMENT: some case bitmap == tmp, so dont recycle
                 * DATE: 2012-08-21
                 */
                Util.recyleBitmap(tmp);
            }

            int rotateDegree = getImageDegreeByUri(uri);
            if (rotateDegree != 0) {
                tmp = Util.rotate(bitmap, rotateDegree);
                /*
                 * BUG FIX: 1486
                 * FIX COMMENT: some case bitmap == tmp, so dont recycle
                 * DATE: 2012-08-21
                 */
                if (tmp != bitmap){
                    Util.recyleBitmap(bitmap);
                    bitmap = tmp;
                }
            }
            return bitmap;
        } catch (Exception e) {
            Log.e("BitmapUtil","Failt create bitmap",e);
            // some error occure
        }finally {
            Util.closeSilently(is);
        }
        return null;
    }

    private int getImageDegreeByUri(Uri target) {
        if (target == null) {
            return 0;
        }
        String filepath = getDefaultPathAccordUri(target);
        if (filepath != null) {
            return ImageManager.getExifOrientation(filepath);
        } else {
            Log.d("BitmapUtil", "The path in uri: " + target + " is null");
            return 0;
        }
    }

    private String getDefaultPathAccordUri(Uri uri) {
        String strPath = null;
        if (ContentResolver.SCHEME_FILE.equals(uri.getScheme())) {
            strPath = uri.getPath();
        } else if (ContentResolver.SCHEME_CONTENT.equals(uri.getScheme())) {
            final String[] IMAGE_PROJECTION = new String[] {Media.DATA};
            Cursor cr = mContext.getContentResolver().query(uri, IMAGE_PROJECTION, null, null, null);
            if(cr != null && cr.getCount() > 0) {
                if(cr.isBeforeFirst()) {
                    cr.moveToFirst();
                    strPath = cr.getString(cr.getColumnIndex(Media.DATA));
                }
            /* SPRD: CID 109007 : Resource leak (RESOURCE_LEAK) @{ */
            }
            if(cr != null) {
                cr.close();
            }
            /**
                cr.close();
            }
            */
                /* @} */
        }
        return strPath;
    }
}
