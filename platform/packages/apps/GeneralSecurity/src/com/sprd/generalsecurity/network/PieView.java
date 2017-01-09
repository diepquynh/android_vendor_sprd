package com.sprd.generalsecurity.network;

import android.graphics.BlurMaskFilter;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.view.View;
import android.content.Context;

import android.graphics.RectF;
import android.util.Log;

import com.sprd.generalsecurity.R;

public class PieView extends View {

    int mHeight, mWidth, mRadius;
    int mAngles = 0;
    int mPercentUsed = -1;
    public PieView(Context cxt, int percentUsed) {
        super(cxt);
        mPercentUsed = percentUsed;
        if (mPercentUsed > 0) {
            mAngles = (percentUsed * 360) / 100;
        }
    }

    public void onDraw(Canvas canvas) {
        mHeight = getMeasuredHeight() -10;

        RectF rectF = new RectF(25, 25, mHeight, mHeight);
        Paint p = new Paint();
        p.setColor(getResources().getColor(R.color.yellow));

        BlurMaskFilter blurMaskFilter = new BlurMaskFilter(1, BlurMaskFilter.Blur.INNER);
        p.setMaskFilter(blurMaskFilter);
        p.setAntiAlias(true);

        canvas.drawArc(rectF, -90, mAngles, true, p);

        if (mPercentUsed == -1) {
            p.setColor(Color.LTGRAY);
        } else {
            p.setColor(getResources().getColor(R.color.blue));
        }
        canvas.drawArc(rectF, (mAngles-90), 360 - mAngles, true, p);
    }
}