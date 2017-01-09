package com.android.camera.ui;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RectF;
import android.util.AttributeSet;
import android.view.View;

import com.android.camera2.R;

/**
 * GridLines is a view which directly overlays the preview and draws
 * evenly spaced grid lines.
 */
public class ReticleGridLines extends View
    implements PreviewStatusListener.PreviewAreaChangedListener {

    private RectF mDrawBounds;
    Paint mPaint = new Paint();

    public ReticleGridLines(Context context, AttributeSet attrs) {
        super(context, attrs);
        mPaint.setStrokeWidth(getResources().getDimensionPixelSize(R.dimen.grid_line_width));
        mPaint.setColor(getResources().getColor(R.color.grid_line));
    }

    @Override
    public void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        if (mDrawBounds != null) {
            float secondWidth = mDrawBounds.width() / 2;
            float secondHeight = mDrawBounds.height() / 2;
            for (int i = 1; i < 2; i++) {
                // Draw the vertical lines.
                final float x = secondWidth * i;
                canvas.drawLine(mDrawBounds.left + x, mDrawBounds.top,
                        mDrawBounds.left + x, mDrawBounds.bottom, mPaint);
                // Draw the horizontal lines.
                final float y = secondHeight * i;
                canvas.drawLine(mDrawBounds.left, mDrawBounds.top + y,
                        mDrawBounds.right, mDrawBounds.top + y, mPaint);
            }
        }
    }

    @Override
    public void onPreviewAreaChanged(final RectF previewArea) {
        setDrawBounds(previewArea);
    }

    private void setDrawBounds(final RectF previewArea) {
        mDrawBounds = new RectF(previewArea);
        invalidate();
    }
}
