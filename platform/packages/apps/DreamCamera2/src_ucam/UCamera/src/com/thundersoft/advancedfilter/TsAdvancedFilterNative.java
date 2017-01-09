package com.thundersoft.advancedfilter;

import android.util.Log;


public class TsAdvancedFilterNative {
    /*
     * Load Library
     * Must do this first
     */
    static {
        try {
            System.loadLibrary("tsadvancedfilterJni");
        } catch (UnsatisfiedLinkError e) {
            e.printStackTrace();
        }
    }
    private static OnReceiveBufferListener mListener = null;
    private static OnSavePictureCompletedListener mSavePicCompletedListener=null;
    /**
     * The facing of the camera is opposite to that of the screen.
     */
    public static final int CAMERA_FACING_BACK = 0;
    /**
     * The facing of the camera is the same as that of the screen.
     */
    public static final int CAMERA_FACING_FRONT = 1;
    /**
     * Filters types
     * Do not change the values
     */
    public static final int AdvancedFILTER_NONE                     = -1;
    public static final int ADVANCEDFILTER_BLACKWHITE               = 4;
    public static final int ADVANCEDFILTER_BLUECOLOR                = 5;//++
    public static final int ADVANCEDFILTER_GREEN                    = 6;
    public static final int ADVANCEDFILTER_INVERT                   = 12;//Invert
    public static final int ADVANCEDFILTER_INFRARED                 = 13;
    public static final int ADVANCEDFILTER_NIGHTVISION              = 18;//has res
    public static final int ADVANCEDFILTER_NEGATIVE                 = 20;
    public static final int ADVANCEDFILTER_POSTERIZE                = 21;
    public static final int ADVANCEDFILTER_NOSTALGIA                = 22;
    public static final int ADVANCEDFILTER_RED                      = 23;//++
    public static final int ADVANCEDFILTER_SYMMETRY_LEFT            = 100;
    public static final int ADVANCEDFILTER_SYMMETRY_RIGHT           = 101;
    public static final int ADVANCEDFILTER_SYMMETRY_UP              = 102;
    public static final int ADVANCEDFILTER_SYMMETRY_DOWN            = 103;
    public static final int ADVANCEDFILTER_FRESH                    = 140;//has res ++
    public static final int ADVANCEDFILTER_NOSTALGIA_INSTAGRAM      = 141;//has res
    public static final int ADVANCEDFILTER_FILM                     = 142;//has res ++
    public static final int ADVANCEDFILTER_REVERSAL                 = 143;//has res ++
    public static final int ADVANCEDFILTER_JAPAN                    = 146;
    public static final int ADVANCEDFILTER_BRIGHT                   = 300;//has res
    public static final int ADVANCEDFILTER_CUTE                     = 301;//has res
    public static final int ADVANCEDFILTER_BLUE                     = 302;//has res
    public static final int ADVANCEDFILTER_FLOWERINESS              = 303;//has res
    public static final int ADVANCEDFILTER_FLY                      = 304;//has res
    public static final int ADVANCEDFILTER_LOTUS                    = 305;//has res
    public static final int ADVANCEDFILTER_BLESS                    = 306;//has res
    public static final int ADVANCEDFILTER_SPARKLING                = 307;//has res
    public static final int ADVANCEDFILTER_HOPE                     = 308;//has res
    public static final int ADVANCEDFILTER_HAPPY                    = 309;//has res
    public static final int ADVANCEDFILTER_BLOOM                    = 310;//has res
    public static final int ADVANCEDFILTER_COLORFUL                 = 311;//has res
    public static final int ADVANCEDFILTER_1839                     = 318;
    public static final int ADVANCEDFILTER_MOSAIC                   = 321;
    public static final int ADVANCEDFILTER_MOVIED_COLOR             = 323;
    public static final int ADVANCEDFILTER_EDGE                     = 324;
    public static final int ADVANCEDFILTER_DREAMLAND                = 325;
    public static final int ADVANCEDFILTER_COLORPENCIL              = 400;//has res
    public static final int ADVANCEDFILTER_GRAYSKETCHPENCIL         = 401;//has res
    public static final int ADVANCEDFILTER_RAINBROW                 = 402;//has res
    public static final int ADVANCEDFILTER_AUTUMN                   = 403;//has res
    public static final int ADVANCEDFILTER_DUSK                     = 404;
    public static final int ADVANCEDFILTER_REFLECTION               = 405;
    public static final int ADVANCEDFILTER_SKETCH                   = 406;
    public static final int ADVANCEDFILTER_DREAMSPACE               = 407;
    public static final int ADVANCEDFILTER_NEON                     = 408;
    public static final int ADVANCEDFILTER_PENCIL1                  = 409;
    public static final int ADVANCEDFILTER_PENCIL2                  = 410;
    public static final int ADVANCEDFILTER_EMBOSS                   = 411;
    public static final int ADVANCEDFILTER_THERMAL                  = 412;
    public static final int ADVANCEDFILTER_CRAYON                   = 413;
    public static final int ADVANCEDFILTER_DREAM                    = 414;
    public static final int ADVANCEDFILTER_ENGRAVING                = 415;
    public static final int ADVANCEDFILTER_KOREA                    = 416;//has res
    public static final int ADVANCEDFILTER_AMERICA                  = 417;//has res
    public static final int ADVANCEDFILTER_FRANCE                   = 418;//has res
    public static final int ADVANCEDFILTER_DESERT                   = 419;//has res
    public static final int ADVANCEDFILTER_JIANGNAN                 = 420;//has res
    public static final int ADVANCEDFILTER_SHALLOWMEMORY            = 421;//has res
    public static final int ADVANCEDFILTER_WAVE                     = 422;
    public static final int ADVANCEDFILTER_PEEP1                    = 423;//has res
    public static final int ADVANCEDFILTER_PEEP2                    = 424;//has res
    public static final int ADVANCEDFILTER_PEEP3                    = 425;//has res
    public static final int ADVANCEDFILTER_PEEP4                    = 426;//has res
    public static final int ADVANCEDFILTER_PEEP5                    = 427;//has res
    public static final int ADVANCEDFILTER_PEEP6                    = 428;//has res
    public static final int ADVANCEDFILTER_PEEP7                    = 429;//has res
    public static final int ADVANCEDFILTER_PEEP8                    = 430;//has res
    public static final int ADVANCEDFILTER_PEEP9                    = 431;//has res
    public static final int ADVANCEDFILTER_PEEP10                   = 432;//has res
    public static final int ADVANCEDFILTER_PEEP11                   = 433;//has res
    public static final int ADVANCEDFILTER_PEEP12                   = 434;//has res
    public static final int ADVANCEDFILTER_PEEP13                   = 435;//has res

    public TsAdvancedFilterNative() {
    };
    /**
     * Function setOnReceiveBufferListener
     * The OnReceiveBufferListener instance as listener
     * @param listener, OnReceiveBufferListener instance
     */
    public static void setOnReceiveBufferListener(OnReceiveBufferListener listener) {
        mListener = listener;
    }
    /**
     * Function setOnSavePictureCompletedListener
     * The OnSavePictureCompletedListener instance as listener
     * @param listener, OnSavePictureCompletedListener instance
     */
    public static void setOnSavePictureCompletedListener(OnSavePictureCompletedListener listener){
        mSavePicCompletedListener=listener;
    }
    /**
     * Function nativeFilterCaptureCompleted
     *
     * This method is callback and called by JNI C++
     * Do change the method signature
     */
    private static void nativeFilterCaptureCompleted(boolean result) {
        if (mSavePicCompletedListener != null) {
            mSavePicCompletedListener.onCompleted(result);
        }
    }
    /**
     * Function nativeSenderMessage
     *
     * @param what, filter index
     * @param colors, color image pixels array
     * @param width, color image width
     * @param height color image height
     *
     * This method is callback and called by JNI C++
     * Do change the method signature
     */
    private static void nativeSenderMessage(int what, int[] colors, int width, int height) {
        if (mListener != null) {
            mListener.onReceiveBuffer(what, colors, width, height);
        }
    }
    /**
     * Function init
     * initialize context
     *
     * @param arrSupportType, the array which includes filter types of the library supports. The array values MUST be
     * AdvancedFILTER_NONE,ADVANCEDFILTER_BLACKWHITE,ADVANCEDFILTER_BLUECOLOR ......ADVANCEDFILTER_PEEP13
     */
    public static native void init(int[] arrSupportType);
    /**
     * Function destroy
     * destroy context
     */
    public static native void destroy();
    /**
     * Function  setResPath
     * set the directory path of filter resources
     * @param path, resource directory path. Can not include "/" in the path end
     */
    public static native void setResPath(String path);
    /**
     * Function  releaseEffectRes
     * release memory of resource which is used in filter
     */
    public static native void releaseEffectRes();
    /**
     * Function  doEffect
     * process effects.lenght number filters,at the same time
     *
     * @param yuvBuf, the source yuv420sp data which will be processed.yuvBuf Must not null.
     * @param frameWidth, the width of YUV Frame. frameWidth Must > 0
     * @param frameHeight, the height of YUV Frame. frameHeight Must > 0
     * @param dstW, the width of target which you need. dstW Must > 0 && dstW Must < frameWidth
     * @param dstH, the height of target which you need. dstH Must > 0 && dstH Must < frameHeight
     *
     * @param matrix, 4 x 4 matrix, has 16 elements float array. matrix Must not null.
     * use matrix to change camera preview orientation, when switch camera facing or surface changed.
     * @see android.opengl.Matrix
     *
     * @param cameraFacing, camera facing.
     * @see the defination of CAMERA_FACING_BACK CAMERA_FACING_FRONT
     *
     * @param effects, the filter ids array which you expect to deal with.
     * @see the defination of ADVANCEDFILTER_MOVED_COLOR
         ADVANCEDFILTER_EDGE
         ADVANCEDFILTER_MIRROR_RIGHT2LEFT
         ADVANCEDFILTER_BROWN
         etc
     */
    public static native void doEffect(byte[] yuvBuf, int frameWidth, int frameHeight, int dstW, int dstH,
            float[]matrix, int cameraFacing,
            int[] effects);
    /**
     * Function  setEffectType
     * set filter type
     *
     * @param effect, the filter type  which you expect to deal with.
     * @see the defination of ADVANCEDFILTER_MOVED_COLOR
         ADVANCEDFILTER_EDGE
         ADVANCEDFILTER_MIRROR_RIGHT2LEFT
         ADVANCEDFILTER_BROWN
         etc
     */
    public static native void setEffectType(int effect);
    /**
     * Function  setEffectParam
     * set filter parameter
     *
     * @param param, filter parameter
     * eg String.format("selectedRGB=%f,%f,%f", (float)0.1, (float)0.9, (float)0.1), the ADVANCEDFILTER_MOVED_COLOR filter
     */
    public static native void setEffectParam(String param);
    /**
     * FUNCTION: surfaceChanged. MUST be called in render thread.
     *
     * @param width, surface width
     * @param height,  surface height
     *
     * @see android.opengl.GLSurfaceView  android.opengl.GLSurfaceView.Renderer
     */
    public static native void surfaceChanged(int width, int height);
    /**
     * FUNCTION: onDrawFrame
     *
     * @param yuvBuf,  YUV frame buffer.yuvBuf Must not null.
     * @param frameWidth, the width of YUV Frame.frameWidth Must > 0
     * @param frameHeight, the height of YUV Frame.frameHeight Must > 0
     *
     * @param matrix, 4 x 4 matrix, has 16 elements float array.matrix Must not null.
     * use matrix to change camera preview orientation, when switch camera facing or surface changed.
     * @see android.opengl.Matrix
     *
     * @param pictureWidth, picture width
     * @param pictureHeight, picture height
     *
     * @param cameraFacing, camera facing
     * @see the defination of CAMERA_FACING_BACK CAMERA_FACING_FRONT
     *
     * MUST be called, in render thread
     * @see android.opengl.GLSurfaceView  android.opengl.GLSurfaceView.Renderer
     *
     */
    public static native void onDrawFrame(byte[]yuvBuf, int frameWidth, int frameHeight, float[]matrix, int cameraFacing,
            int pictureWidth, int pictureHeight);
    /**
     * FUNCTION: yuv2RGB888
     *
     * @param yuvBuf,  YUV frame buffer.yuvBuf MUST not null
     * @param frameWidth, the width of YUV Frame.frameWidth MUST > 0
     * @param frameHeight, the height of YUV Frame.frameHeight MUST > 0
     *
     * @return int array includes rgb8888 pixels.
     */
    public static native int[] yuv2RGB888(byte[] yuvBuf, int frameWidth, int frameHeight);

    /**
     *FUNCTION: getSupportedMaxPictureSize
     *
     * @return int array. The length of the array is 2.array[0] is max picture width,array[1] is max picture height
     */
    public static native int[] getSupportedMaxPictureSize();

    /**
    *FUNCTION: takeFilterPicture
    *
    * @param jpegBuffer,  jpegBuffer jpeg buffer.jpegBuffer MUST not null
    * @param pictureWidth, the width of picture.pictureWidth MUST > 0 and < supported max picture width
    * @param pictureHeight, the height of picture.pictureHeight MUST > 0 and < supported max picture height
    * @param effect, the filter type which you expect to take picture
    * @param cameraFacing, camera facing
    * @see the defination of CAMERA_FACING_BACK CAMERA_FACING_FRONT
    * @param params, filter parameter
    * eg String.format("selectedRGB=%f,%f,%f", (float)0.1, (float)0.9, (float)0.1), the ADVANCEDFILTER_MOVED_COLOR filter
    * @param savedFileName, complete path name for the file to be saved.savedFileName MUST not null
    * eg /sdcard/camera/picname.jpeg
    *@param isMultipleThread true saving picture in asynchronous thread and call OnSavePictureCompletedListener.onCompleted when saving picture completed.
    *                        false saving picture in the same thread and NOT call OnSavePictureCompletedListener.onCompleted
    *@see OnSavePictureCompletedListener.onCompleted
    *
    * @return true take picture successfull,otherwise take picture failed
    */
   public static native boolean takeFilterPicture(byte[] jpegBuffer,int pictureWidth, int pictureHeight,
           int effect, int cameraFacing, String params, String savedFileName, boolean isMultipleThread);

    /**
     * Interface: OnReceiveBufferListener
     */
    public static interface OnReceiveBufferListener {
        public void onReceiveBuffer(int what, int[] colors, int width, int height);
    }

    /**
     * Interface: OnSavePictureCompletedListener
     * You can implements this interface and override onCompleted
     */
    public static interface OnSavePictureCompletedListener {
        /**
         * FUNCTION: onCompleted
         *
         * When saving picture complete,the method is called.
         *
         * The method is ONLY called in using multiple thread to take filter picture
         * @see com.thundersoft.advancedfilter.TsAdvancedFilterNative.takeFilterPicture
         *
         *@param result true save picture successfull,otherwise save picture failed.
         */
        public void onCompleted(boolean result);
    }

    public static void removeOnReceiveBufferListener(OnReceiveBufferListener listener) {
        if (mListener == listener) {
            mListener = null;
        }
    }
}
