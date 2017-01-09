/**
 * Copyright (C) 2014,2015 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto.idphoto;
import com.ucamera.uphoto.R;
public class IDPhotoType5 extends IDPhotoType{
    public IDPhotoType5() {
        mName = "type5";
        mWidth = 40;
        mHeight = 50;
        mColors = new int[] {0xffffffff, 0xffed1c24, 0xff6dcff6};
        mDrawableIDs = new int[] {R.drawable.idphoto_background_white,R.drawable.idphoto_background_red,R.drawable.idphoto_background_blue};
    }
}
