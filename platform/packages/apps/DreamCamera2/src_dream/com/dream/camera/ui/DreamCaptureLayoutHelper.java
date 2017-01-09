
package com.dream.camera.ui;

import android.app.Activity;
import android.view.View;

import com.android.camera.CaptureLayoutHelper;
import com.android.camera.TextureViewHelper;
import com.android.camera.CaptureLayoutHelper.PositionConfiguration;

import com.android.camera2.R;

public class DreamCaptureLayoutHelper extends CaptureLayoutHelper {

    private Activity mActivity;

    public DreamCaptureLayoutHelper(int bottomBarMinHeight, int bottomBarMaxHeight,
            int bottomBarOptimalHeight) {
        super(bottomBarMinHeight, bottomBarMaxHeight, bottomBarOptimalHeight);
    }

    @Override
    protected PositionConfiguration getPositionConfiguration(int width, int height,
            float previewAspectRatio, int rotation) {
        PositionConfiguration config = new PositionConfiguration();

        //SPRD: Add landscape for preview.the preview UI appear chaotic when from the filmstrip back with horizontal screen
        boolean landscape = width > height;
        int topHeight = 0;
        int bottomHeight = 0;
        int slideHeight = 0;

        if (mActivity != null && mActivity.getResources() != null) {
            topHeight = mActivity.getResources()
                    .getDimensionPixelSize(R.dimen.top_panel_height);
            bottomHeight = mActivity.getResources()
                    .getDimensionPixelSize(R.dimen.bottom_bar_height);
            slideHeight = mActivity.getResources()
                    .getDimensionPixelSize(R.dimen.slide_panel_height);
        }

        config.mBottomBarRect.set(0, height - bottomHeight + slideHeight, width, height);

        if (previewAspectRatio == TextureViewHelper.MATCH_SCREEN) {
            config.mPreviewRect.set(0, 0, width, height);
        } else {
            if (previewAspectRatio < 1) {
                previewAspectRatio = 1 / previewAspectRatio;
            }
            // Get the bottom bar width and height.
            float barSize;
            int longerEdge = Math.max(width, height);
            int shorterEdge = Math.min(width, height);

            // Check the remaining space if fit short edge.
            float spaceNeededAlongLongerEdge = shorterEdge * previewAspectRatio;
            float remainingSpaceAlongLongerEdge = longerEdge - spaceNeededAlongLongerEdge;

            float previewShorterEdge;
            float previewLongerEdge;

            if (remainingSpaceAlongLongerEdge <= 0) {
                // Preview aspect ratio > screen aspect ratio: fit longer edge.
                previewLongerEdge = longerEdge;
                previewShorterEdge = longerEdge / previewAspectRatio;
                barSize = bottomHeight - slideHeight;
                config.mBottomBarOverlay = true;

                if (landscape) {
                    config.mPreviewRect.set(0, height / 2 - previewShorterEdge / 2, previewLongerEdge,
                            height / 2 + previewShorterEdge / 2);
                } else {
                    config.mPreviewRect.set(width / 2 - previewShorterEdge / 2, 0,
                            width / 2 + previewShorterEdge / 2, previewLongerEdge);
                }
            } else if (previewAspectRatio > 14f / 9f) {
                // If the preview aspect ratio is large enough, simply offset the
                // preview to the bottom/right.
                barSize = bottomHeight - slideHeight;
                previewShorterEdge = shorterEdge;
                previewLongerEdge = shorterEdge * previewAspectRatio;
                config.mBottomBarOverlay = true;

                if (landscape) {
                    config.mPreviewRect.set(0, 0, previewLongerEdge, previewShorterEdge);
                } else {
                    config.mPreviewRect.set(0, 0, previewShorterEdge, previewLongerEdge);
                }
            } else {
                // Fit shorter edge.
                barSize = remainingSpaceAlongLongerEdge <= (bottomHeight + topHeight) ?
                        remainingSpaceAlongLongerEdge : bottomHeight;
                previewShorterEdge = shorterEdge;
                previewLongerEdge = shorterEdge * previewAspectRatio;
                config.mBottomBarOverlay = false;

                // SPRD: remove checkui 79
//                if(barSize == remainingSpaceAlongLongerEdge){
//                    config.mPreviewRect.set(0, 0, previewShorterEdge,previewLongerEdge);
//                }else{
//                    config.mPreviewRect.set(0, topHeight, previewShorterEdge, topHeight
//                            + previewLongerEdge);
//                }
                    if (landscape) {
                        config.mPreviewRect.set(topHeight, 0, topHeight
                                + previewLongerEdge, previewShorterEdge);
                    } else {
                        config.mPreviewRect.set(0, topHeight, previewShorterEdge, topHeight
                                + previewLongerEdge);
                    }
            }
        }

        round(config.mBottomBarRect);
        round(config.mPreviewRect);

        return config;
    }

    public void setActivity(Activity activity) {
        mActivity = activity;
    }
}
