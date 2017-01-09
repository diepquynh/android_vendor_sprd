/*******************************************************************************
 *
 *  Copyright (C) 2009-2013 Broadcom Corporation
 *
 *  This program is the proprietary software of Broadcom Corporation and/or its
 *  licensors, and may only be used, duplicated, modified or distributed
 *  pursuant to the terms and conditions of a separate, written license
 *  agreement executed between you and Broadcom (an "Authorized License").
 *  Except as set forth in an Authorized License, Broadcom grants no license
 *  (express or implied), right to use, or waiver of any kind with respect to
 *  the Software, and Broadcom expressly reserves all rights in and to the
 *  Software and all intellectual property rights therein.
 *  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS
 *  SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE
 *  ALL USE OF THE SOFTWARE.
 *
 *  Except as expressly set forth in the Authorized License,
 *
 *  1.     This program, including its structure, sequence and organization,
 *         constitutes the valuable trade secrets of Broadcom, and you shall
 *         use all reasonable efforts to protect the confidentiality thereof,
 *         and to use this information only in connection with your use of
 *         Broadcom integrated circuit products.
 *
 *  2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
 *         "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
 *         REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY,
 *         OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
 *         DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
 *         NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
 *         ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
 *         CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT
 *         OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 *  3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
 *         ITS LICENSORS BE LIABLE FOR
 *         (i)   CONSEQUENTIAL, INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY
 *               DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY RELATING TO
 *               YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
 *               HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR
 *         (ii)  ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
 *               SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE
 *               LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY FAILURE OF
 *               ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 *
 *******************************************************************************/

package com.broadcom.bt.app.fm.rx;

import com.broadcom.bt.app.fm.R;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;

public class FmRadioFrequencySlider extends View {

    private int width, height;
    Bitmap slider = null;
    Bitmap bg;
    int sliderX;
    int freq;
    private int minX, maxX;
    private int touchDownStartSliderX;
    private FmRadioFrequencySliderEventListener listener;

    Context context;

    // TODO: get the correct value
    public final static int FREQ_STEP = 20;

    private int minFreq = 0;
    private int maxFreq = 0;

    public FmRadioFrequencySlider(Context context) {
        super(context);
        this.context = context;
    }

    public FmRadioFrequencySlider(Context context, AttributeSet attrs) {
        super(context, attrs);
        this.context = context;
    }

    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        width = w;
        height = h;
        if (minFreq > 0 && maxFreq > 0) {
            drawBackground();
            sliderX = freqToX(freq);
            invalidate();
        }
    }

    public void setFrequencyRange(int minFreq, int maxFreq) {
        this.minFreq = minFreq;
        this.maxFreq = maxFreq;
        if (width > 0 && height > 0) {
            drawBackground();
            sliderX = freqToX(freq);
            invalidate();
        }
    }

    private void drawBackground() {
        if (slider == null)
            slider = BitmapFactory.decodeResource(context.getResources(), R.drawable.slider);

        minX = slider.getWidth() / 2;
        maxX = width - minX;

        sliderX = minX;

        bg = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
        Canvas c = new Canvas(bg);

        // draw numbers every 5 MHz
        Paint p = new Paint();
        p.setColor(0xFFFFFFFF); // white
        p.setTextSize(24);
        p.setTextAlign(Paint.Align.CENTER);

        int drawFreq = (minFreq / 500) * 500;
        if (drawFreq < minFreq)
            drawFreq += 500;

        while (drawFreq <= maxFreq) {
            c.drawText(Integer.toString(drawFreq / 100), freqToX(drawFreq), 24, p);
            drawFreq += 500;
        }

        // draw lines every 0.5 MHz

        drawFreq = (minFreq / 50) * 50;
        if (drawFreq < minFreq)
            drawFreq += 50;

        int largeDivisionTopY = 30;
        int largeDivisionBottomY = height - slider.getHeight() / 2 - 6;
        int largeDivisionSize = largeDivisionBottomY - largeDivisionTopY;
        int largeDivisionCenter = (largeDivisionBottomY + largeDivisionTopY) / 2;
        int smallDivisionTopY = largeDivisionCenter - largeDivisionSize / 3;
        int smallDivisionBottomY = largeDivisionCenter + largeDivisionSize / 3;

        while (drawFreq <= maxFreq) {
            boolean largeDivision = (drawFreq % 500 == 0);
            int x = freqToX(drawFreq);
            if (largeDivision) {
                c.drawLine(x, largeDivisionTopY, x, largeDivisionBottomY, p);
            } else {
                c.drawLine(x, smallDivisionTopY, x, smallDivisionBottomY, p);
            }
            drawFreq += 50;
        }

    }

    protected void onDraw(Canvas canvas) {
        if (bg == null)
            return; // not yet initialized

        canvas.drawBitmap(bg, 0, 0, null);
        canvas.drawBitmap(slider, sliderX - slider.getWidth() / 2, height - slider.getHeight(),
                null);
    }

    public void setFreq(int freq) {
        this.freq = freq;
        sliderX = freqToX(freq);
        if (sliderX < minX)
            sliderX = minX;
        else if (sliderX > maxX)
            sliderX = maxX;
        invalidate();

    }

    public int getMinimumHeight() {
        return 75;
    }

    public boolean onTouchEvent(MotionEvent event) {
        if (bg == null)
            return true; // not yet initialized

        int action = event.getAction();
        if (action == MotionEvent.ACTION_DOWN) {
            touchDownStartSliderX = sliderX;
            sliderX = (int) event.getX();

            if (sliderX < minX)
                sliderX = minX;
            else if (sliderX > maxX)
                sliderX = maxX;
            if (listener != null)
                listener.onSliderDown();

        } else if (action == MotionEvent.ACTION_MOVE) {
            sliderX = (int) event.getX();

            if (sliderX < minX)
                sliderX = minX;
            else if (sliderX > maxX)
                sliderX = maxX;
            if (listener != null)
                listener.onSliderDrag(xToFreq(sliderX));

        } else if (action == MotionEvent.ACTION_UP) {
            freq = xToFreq(sliderX);
            if (listener != null)
                listener.onSliderSet(freq);
        } else if (action == MotionEvent.ACTION_CANCEL) {
            // cancel
            sliderX = touchDownStartSliderX;
            if (listener != null)
                listener.onSliderCancel();

        }
        invalidate();
        return true;
    }

    private int xToFreq(int x) {
        int freqRange = maxFreq - minFreq;
        int xRange = maxX - minX;

        int freq = minFreq + (x - minX) * freqRange / xRange;

        // round to step
        freq = ((freq - minFreq + FREQ_STEP / 2) / FREQ_STEP) * FREQ_STEP + minFreq;

        return freq;
    }

    private int freqToX(int freq) {
        int freqRange = maxFreq - minFreq;
        int xRange = maxX - minX;

        int x = minX + (freq - minFreq) * xRange / freqRange;

        return x;
    }

    public void setListener(FmRadioFrequencySliderEventListener listener) {
        this.listener = listener;
    }
}
