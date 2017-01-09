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
public class BoxGridLines extends View
    implements PreviewStatusListener.PreviewAreaChangedListener {

    private RectF mDrawBounds;
    Paint mPaint = new Paint();

    public BoxGridLines(Context context, AttributeSet attrs) {
        super(context, attrs);
        mPaint.setStrokeWidth(getResources().getDimensionPixelSize(R.dimen.grid_line_width));
        mPaint.setColor(getResources().getColor(R.color.grid_line));
    }

    @Override
    public void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        if (mDrawBounds != null) {
            float boxMargin = 8;
            float boxWidth = mDrawBounds.width() - boxMargin;
            float boxOutHeight = (mDrawBounds.height() - boxWidth) / 2;
            for (int i = 1; i < 3; i++) {
                final float x = boxOutHeight;
                final float y = boxOutHeight;
                if (i == 1) {
                    // Draw the vertical lines.
                    canvas.drawLine(mDrawBounds.left + boxMargin, mDrawBounds.top + x,
                            mDrawBounds.left + boxMargin, mDrawBounds.bottom - x, mPaint);
                    // Draw the horizontal lines.
                    canvas.drawLine(mDrawBounds.left + boxMargin, mDrawBounds.top + y,
                            mDrawBounds.right - boxMargin, mDrawBounds.top + y, mPaint);
                }
                if (i == 2) {
                    // Draw the vertical lines.
                    canvas.drawLine(mDrawBounds.right - boxMargin, mDrawBounds.top + x,
                            mDrawBounds.right - boxMargin, mDrawBounds.bottom - x, mPaint);
                    // Draw the horizontal lines.
                    canvas.drawLine(mDrawBounds.left + boxMargin, mDrawBounds.bottom - y,
                            mDrawBounds.right - boxMargin, mDrawBounds.bottom - y, mPaint);
                }
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
