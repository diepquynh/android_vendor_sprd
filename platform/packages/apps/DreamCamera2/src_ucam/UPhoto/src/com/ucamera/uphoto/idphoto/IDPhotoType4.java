/**
 * Copyright (C) 2014,2015 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto.idphoto;
import com.ucamera.uphoto.R;
public class IDPhotoType4 extends IDPhotoType{
    public IDPhotoType4() {
        mName = "type4";
        mWidth = 24;
        mHeight = 30;
        mColors = new int[] {0xffffffff, 0xffed1c24, 0xff6dcff6};
        mDrawableIDs = new int[] {R.drawable.idphoto_background_white,R.drawable.idphoto_background_red,R.drawable.idphoto_background_blue};
    }
}
