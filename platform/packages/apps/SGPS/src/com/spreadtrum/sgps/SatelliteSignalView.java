
package com.spreadtrum.sgps;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Paint.Align;
import android.graphics.Paint.Style;
import android.util.AttributeSet;
import android.view.View;

public class SatelliteSignalView extends View {

    private static final int TEXT_OFFSET = 8;
    private static final int BASELINE_OFFSET = 5;
    private static final double ROW_DEVIDER = 5.0;
    private static final float PERCENT = 100.0F;
    private static final int ONE_QUARTER = 25;
    private static final int TWO_QUARTER = 50;
    private static final int THREE_QUARTER = 75;
    private static final float TEXT_SIZE = 10.0f;
    private static final float THIN_LINE_STOKE_WIDTH = 0.5f;
    private static final int DIVIDER_MIN = 15;
    private static final int DIVIDER_MAX = 32;
    private static final int DIVIDER_1 = 20;
    private static final int DIVIDER_2 = 25;
    private static final int DIVIDER_3 = 30;
    private Paint mLinePaint = null;
    private Paint mThinLinePaint = null;
    private Paint mBarPaintUsed = null;
    private Paint mBarPaintUnused = null;
    private Paint mBarPaintNoFix = null;
    private Paint mBarOutlinePaint = null;
    private Paint mTextPaint = null;
    private Paint mBackground = null;

    private SatelliteDataProvider mProvider = null;
    private int mSatellites = 0;
    private int[] mPrns = new int[SatelliteDataProvider.MAX_SATELLITES_NUMBER];
    private float[] mSnrs = new float[SatelliteDataProvider.MAX_SATELLITES_NUMBER];
    private int[] mUsedInFixMask = new int[SatelliteDataProvider.SATELLITES_MASK_SIZE];

    public SatelliteSignalView(Context context) {
        this(context, null);
    }

    public SatelliteSignalView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public SatelliteSignalView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        Resources res = getResources();
        if (null != res) {
            mLinePaint = new Paint();
            mLinePaint.setColor(res.getColor(R.color.sigview_line_color));
            mLinePaint.setAntiAlias(true);
            mLinePaint.setStyle(Style.STROKE);
            mLinePaint.setStrokeWidth(1.0f);

            mThinLinePaint = new Paint(mLinePaint);
            mThinLinePaint.setStrokeWidth(THIN_LINE_STOKE_WIDTH);

            mBarPaintUsed = new Paint();
            mBarPaintUsed.setColor(res.getColor(R.color.bar_used));
            mBarPaintUsed.setAntiAlias(true);
            mBarPaintUsed.setStyle(Style.FILL);
            mBarPaintUsed.setStrokeWidth(1.0f);

            mBarPaintUnused = new Paint(mBarPaintUsed);
            mBarPaintUnused.setColor(res.getColor(R.color.bar_unused));

            mBarPaintNoFix = new Paint(mBarPaintUsed);
            mBarPaintNoFix.setStyle(Style.STROKE);

            mBarOutlinePaint = new Paint();
            mBarOutlinePaint.setColor(res.getColor(R.color.bar_outline));
            mBarOutlinePaint.setAntiAlias(true);
            mBarOutlinePaint.setStyle(Style.STROKE);
            mBarOutlinePaint.setStrokeWidth(1.0f);

            mTextPaint = new Paint();
            mTextPaint.setColor(res.getColor(R.color.sigview_text_color));
            mTextPaint.setTextSize(TEXT_SIZE);
            mTextPaint.setTextAlign(Align.CENTER);
            mBackground = new Paint();
            mBackground.setColor(res.getColor(R.color.sigview_background));
        }
    }

    public void setDataProvider(SatelliteDataProvider provider) {
        mProvider = provider;
    }

    private boolean isUsedInFix(int prn) {
        int innerPrn = prn;
        boolean result = false;
        if (0 >= innerPrn) {
            for (int mask : mUsedInFixMask) {
                if (0 != mask) {
                    result = true;
                    break;
                }
            }
        } else {
            innerPrn = innerPrn - 1;
            int index = innerPrn / SatelliteDataProvider.SATELLITES_MASK_BIT_WIDTH;
            int bit = innerPrn % SatelliteDataProvider.SATELLITES_MASK_BIT_WIDTH;
            result = (0 != (mUsedInFixMask[index] & (1 << bit)));
        }
        return result;
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        final int width = getWidth();
        final int height = getHeight();
        final float rowHeight = (float) Math.floor(height / ROW_DEVIDER);
        final float baseline = height - rowHeight + BASELINE_OFFSET;
        final float maxHeight = rowHeight * 4;
        final float scale = maxHeight / PERCENT;

        if (null != mProvider) {
            mSatellites = mProvider.getSatelliteStatus(mPrns, mSnrs, null,
                    null, 0, 0, mUsedInFixMask);
            for (int i = 0; i < mSatellites; i++) {
                if (mSnrs[i] < 0) {
                    mSnrs[i] = 0;
                }
            }
        }
        int devide = DIVIDER_MIN;
        if (mSatellites > DIVIDER_MAX) {
            devide = mSatellites;
        } else if (mSatellites > DIVIDER_3) {
            devide = DIVIDER_MAX;
        } else if (mSatellites > DIVIDER_2) {
            devide = DIVIDER_3;
        } else if (mSatellites > DIVIDER_1) {
            devide = DIVIDER_2;
        } else if (mSatellites > DIVIDER_MIN) {
            devide = DIVIDER_1;
        }
        final float slotWidth = (float) Math.floor(width / devide);
        final float barWidth = slotWidth / PERCENT * THREE_QUARTER;
        final float fill = slotWidth - barWidth;
        float margin = (width - slotWidth * devide) / 2;

        canvas.drawPaint(mBackground);
        canvas.drawLine(0, baseline, width, baseline, mLinePaint);
        float y = baseline - (PERCENT * scale);
        canvas.drawLine(0, y, getWidth(), y, mThinLinePaint);
        y = baseline - (TWO_QUARTER * scale);
        canvas.drawLine(0, y, getWidth(), y, mThinLinePaint);
        y = baseline - (ONE_QUARTER * scale);
        canvas.drawLine(0, y, getWidth(), y, mThinLinePaint);
        y = baseline - (THREE_QUARTER * scale);
        canvas.drawLine(0, y, getWidth(), y, mThinLinePaint);
        int drawn = 0;
        for (int i = 0; i < mSatellites; i++) {
            if (0 >= mPrns[i]) {
                continue;
            }
            final float left = margin + (drawn * slotWidth) + fill / 2;
            final float top = baseline - (mSnrs[i] * scale);
            final float right = left + barWidth;
            final float center = left + barWidth / 2;
            // if (0 == mUsedInFixMask[0]) {
            if (!isUsedInFix(0)) {
                canvas.drawRect(left, top, right, baseline, mBarPaintNoFix);
                // } else if (0 != (mUsedInFixMask[0] & (1 << (mPrns[i] - 1))))
                // {
            } else if (isUsedInFix(mPrns[i])) {
                canvas.drawRect(left, top, right, baseline, mBarPaintUsed);
            } else {
                canvas.drawRect(left, top, right, baseline, mBarPaintUnused);
            }
            canvas.drawRect(left, top, right, baseline, mBarOutlinePaint);
            String tmp = String.format("%2.0f", mSnrs[i]);
            canvas.drawText(tmp, center, top - fill, mTextPaint);
            canvas.drawText(Integer.toString(mPrns[i]), center, baseline
                    + TEXT_OFFSET + fill, mTextPaint);
            drawn += 1;
        }
    }

}
