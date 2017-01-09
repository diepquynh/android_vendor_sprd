
package com.android.camera.ui;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RectF;
import android.util.AttributeSet;
import android.view.View;
import android.graphics.BitmapFactory;
import com.android.camera2.R;
import android.graphics.Bitmap;
import android.graphics.Rect;

/**
 * GridLines is a view which directly overlays the preview and draws evenly spaced grid lines.
 */
public class GoldenGridLines extends View
        implements PreviewStatusListener.PreviewAreaChangedListener {

    private RectF mDrawBounds;
    private Bitmap mGolden;

    public GoldenGridLines(Context context, AttributeSet attrs) {
        super(context, attrs);
        mGolden = BitmapFactory.decodeResource(context.getResources(), R.drawable.helix);
    }

    @Override
    public void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        if (mDrawBounds != null) {
            float ratio = mDrawBounds.height() / mDrawBounds.width();
            if (Math.abs(ratio - 4.0 / 3.0) < Math.abs(ratio - 16.0 / 9.0)) {
                // 4:3
                Rect curRect = new Rect(0, 120, mGolden.getWidth(), mGolden.getHeight());
                canvas.drawBitmap(mGolden, curRect, mDrawBounds, null);
            } else {
                // 16:9
                RectF curRect = new RectF(0, 0, mDrawBounds.width(), mDrawBounds.height() - 170);
                canvas.drawBitmap(mGolden, null, curRect, null);
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
