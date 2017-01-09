/** Created by spreadst */

package com.android.sprdlauncher2;

import com.android.sprdlauncher2.R;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Canvas;
import android.util.AttributeSet;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ImageView;

public class SprdCellLayout extends CellLayout {

    /*
     * SPRD: bug258558 2014-01-23 new requirement to allow delete in edit mode
     * @{
     */
    private int mMarginPixels;
    private static final int TOP_RIGHT_MARGIN_DIP = 15;

    /*
     * SPRD: bug258558 2014-01-23 new requirement to allow delete in edit mode
     * @}
     */

    public SprdCellLayout(Context context) {
        this(context, null);
    }

    public SprdCellLayout(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public SprdCellLayout(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        final Resources res = getResources();
        /* SPRD: Feature 254917,add new view in edit mode@{ */
        mAddBg = res.getDrawable(R.drawable.edit_add_bg);
        /* @} */
        /*
         * SPRD: bug258558 2014-01-23 new requirement to allow delete in edit @{
         * mode.
         */
        final float scale = context.getResources().getDisplayMetrics().density;
        mMarginPixels = (int) (TOP_RIGHT_MARGIN_DIP * scale + 0.5f);

        mDeleteView = new ImageView(context);
        mDeleteView.setImageResource(R.drawable.edit_mode_close_background);
        mDeleteView.setVisibility(View.GONE);
        mDeleteView.setAdjustViewBounds(true);
        mDeleteView.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                if (SprdCellLayout.this.getParent() instanceof SprdWorkspace) {
                    SprdWorkspace ws = (SprdWorkspace) SprdCellLayout.this.getParent();
                    ws.deleteCellLayoutInEditMode(SprdCellLayout.this);
                    ws.addPlusCellLayoutsIfNeeded();
                    ws.updateChildrenDeletable(true);
                }
            }
        });
        addView(mDeleteView);
        /* @} */
    }

    /*
     * SPRD: bug258558 2014-01-23 new requirement to allow delete in edit mode @{
     */
    public void setDeleteViewVisibility(boolean visible) {
        mDeleteView.setVisibility((visible) ? View.VISIBLE : View.GONE);
    }
    /* @} */

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        /* SPRD: Feature 254917,add new view in edit mode@{ */
        if (mEditAdd) {
            int left = (getWidth() - mAddBg.getIntrinsicWidth()) / 2;
            int top = (getHeight() - mAddBg.getIntrinsicHeight()) / 2;
            mAddBg.setBounds(left, top, left + mAddBg.getIntrinsicWidth(), top
                    + mAddBg.getIntrinsicHeight());
            mAddBg.draw(canvas);
        }
        /* @} */
    }

    /**
     * SPRD: Feature 255891,Porting this function from SprdLauncher(Android4.1). @{
     *
     * @param alpha
     */
    public void setFastAlpha(float alpha) {
        setFastChildrenAlpha(alpha);
        super.setAlpha(alpha);
    }

    /** @} */

    /**
     * SPRD: Feature 255891,Porting this function from SprdLauncher(Android4.1). @{
     *
     * @param alpha
     */
    private void setFastChildrenAlpha(float alpha) {
        final int childCount = getChildCount();
        for (int i = 0; i < childCount; i++) {
            getChildAt(i).setAlpha(alpha);
        }
    }
    /** @} */

    /* SPRD: add for UUI @{*/
    void updateCellInfoScreen(int screen){
        mCellInfo.screen = screen;
    }
    /* @} */
}