package com.android.ex.camera2.portability;

public class SprdCameraActions {
    /* SPRD: fix bug 473462 add burst capture @{*/
    public static final int CAPTURE_BURST_PHOTO = 602;
    public static final int CANCEL_CAPTURE_BURST_PHOTO = 603;
    /* @}*/
    
    public static String stringifySprd(int action) {
        switch (action) {
            /* SPRD:fix bug 473462 add burst capture @{*/
            case CAPTURE_BURST_PHOTO:
                return "CAPTURE_BURST_PHOTO";
            case CANCEL_CAPTURE_BURST_PHOTO:
                return "CANCEL_CAPTURE_BURST_PHOTO";
            /* @} */
            default:
                return "UNKNOWN(" + action + ")";
        }
    }
}
