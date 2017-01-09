/** Created by Spreadst */
package com.sprd.fileexplorer.util;

public class PastePathUtil {

    private OnPastePathChangedListener mOnPastePathChangedListener;

    public void addPastePathChangeListener(OnPastePathChangedListener onpathchengedlistener) {
        mOnPastePathChangedListener = onpathchengedlistener;
    }

    public void removePastePathChangeListener(OnPastePathChangedListener onpathchengedlistener) {
        mOnPastePathChangedListener = null;
    }

    /*
     * SPRD: add for bug513485. monkey click is too quick, so add null pointer
     * protect for mOnPastePathChangedListener@{
     */
    public void notifyPathChanged(String path) {
        if (mOnPastePathChangedListener != null) {
            mOnPastePathChangedListener.onPasteFinsish(path);
        }
    }
    /* @} */
}
