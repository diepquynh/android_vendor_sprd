package com.thundersoft.advancedfilter;

import android.util.Log;


public class TsAdvancedFilterNative {

   public static native boolean takeFilterPicture(byte[] jpegBuffer,int pictureWidth, int pictureHeight,
           int effect, int cameraFacing, String params, String savedFileName, boolean isMultipleThread);

}
