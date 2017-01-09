
package com.sprd.contacts.common.widget;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.graphics.Xfermode;
import android.util.AttributeSet;
import android.widget.ImageView;


public class CircleImageView extends ImageView {

    private static final Xfermode MASK_XFERMODE = new PorterDuffXfermode(
            PorterDuff.Mode.DST_IN);

    private Bitmap mBitmapMask;
    private Paint mMaskPaint;
    private Paint mPaint;

    private int mWidth;
    private int mHeight;

    public CircleImageView(Context paramContext) {
        this(paramContext, null);
    }

    public CircleImageView(Context paramContext, AttributeSet paramAttributeSet) {
        this(paramContext, paramAttributeSet, 0);
    }

    public CircleImageView(Context paramContext,
            AttributeSet paramAttributeSet, int paramInt) {
        super(paramContext, paramAttributeSet, paramInt);

        mMaskPaint = new Paint();
        mMaskPaint.setFilterBitmap(false);
        mMaskPaint.setAntiAlias(true);
        mMaskPaint.setXfermode(MASK_XFERMODE);

        mPaint = new Paint();
        mPaint.setAntiAlias(true);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        final int w = getWidth();
        final int h = getHeight();

        int layer = canvas.saveLayer(0, 0, w, h, null, Canvas.ALL_SAVE_FLAG);
        super.onDraw(canvas);
        if (mWidth != w || mHeight != h) {
            if (mBitmapMask != null && !mBitmapMask.isRecycled()) {
                mBitmapMask.recycle();
            }
            mBitmapMask = createOvalBitmap(w, h);
        }
        mWidth = w;
        mHeight = h;
        canvas.drawBitmap(mBitmapMask, 0.0F, 0.0F, mMaskPaint);
        canvas.restoreToCount(layer);
    }

    public Bitmap createOvalBitmap(final int width, final int height) {
        Bitmap bitmap = Bitmap.createBitmap(width, height,
                Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(bitmap);
        canvas.drawCircle(width / 2, height / 2, width / 2, mPaint);
        return bitmap;
    }
}
