/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.uphoto;

import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.RectF;
import android.util.FloatMath;
import android.util.Log;

import java.util.Vector;

public class TextBubbleUtil {
    private float mFontHeight;
    private int mLineNumber;
    private float mTextRectHeight;
    private float mTextPositionX;
    private float mTextPositionY;
    // CID 109258 : UrF: Unread field (FB.URF_UNREAD_FIELD)
    // private int mTextSize;
    private float mTextRectWidth;
    private Paint mTextPaint;
    private String mTextString;
    private Vector<String> mStrVector;

    public TextBubbleUtil(String textString, Paint textPaint) {
        mTextString = textString;
        mTextPaint = textPaint;
        mStrVector = new Vector<String>();

        // CID 109258 : UrF: Unread field (FB.URF_UNREAD_FIELD)
        // mTextSize = (int)textPaint.getTextSize();
    }

    public void setText(String textString) {
        mTextString = textString;
    }

    public void drawText(Canvas canvas, Matrix matrix, BubbleVertex bubbleVertex) {
        int index = 0;
        /* FIX BUG: 3939
         * FIX COMMENT: calculate start position again when the text more than one line
         * Date: 2013-06-06
         */
        int size = mStrVector.size();
        mTextPositionY = mTextPositionY - ((size -1) * mFontHeight) / 2;
        do {
            if(index >= mLineNumber) {
                break;
            }
            String text = mStrVector.elementAt(index);
            float textWidth = mTextPaint.measureText(text);
            canvas.drawText(text, mTextPositionX - textWidth / 2, mTextPositionY + mFontHeight * index, mTextPaint);

            index++;
        } while(true);
    }

    public void setTextRect(RectF rectf) {
        mTextRectWidth = rectf.right - rectf.left;
        mTextRectHeight = rectf.bottom - rectf.top;
        mTextPositionX = rectf.left + mTextRectWidth / 2;
        mTextPositionY = rectf.top + mTextRectHeight / 2;

        prepareToDrawText();
    }

    private void prepareToDrawText() {
//        Paint.FontMetrics fontMetrics = mTextPaint.getFontMetrics();
//        float descent = fontMetrics.descent;
//        float fontTop = fontMetrics.top;
//        mFontHeight = FloatMath.ceil(descent - fontTop);
//        mTextPositionY = mTextPositionY - Math.abs(FloatMath.ceil(fontTop) / 2);

        int strLen = mTextString.length();
        Rect bounds = new Rect();
        mTextPaint.getTextBounds(mTextString, 0, strLen, bounds);
        mFontHeight = bounds.height();
        mTextPositionY = mTextPositionY + mFontHeight / 2;

        mStrVector.clear();
        mLineNumber = 0;

        float tempLineWidth = 0;
        int index = 0;
        for(int i = 0; i < strLen; i++) {
            char character = mTextString.charAt(i);
            String singleStr = String.valueOf(character);
            float singleStrWidth = mTextPaint.measureText(singleStr);
            tempLineWidth += singleStrWidth;

            if(i < strLen - 1) {
                char nextChar = mTextString.charAt(i + 1);
                String nextSingleStr = String.valueOf(nextChar);
                float nextSingleStrWidth = mTextPaint.measureText(nextSingleStr);

                if((mTextRectWidth -tempLineWidth) < nextSingleStrWidth) {
                    mLineNumber++;
                    tempLineWidth = 0;
                    if((mTextRectHeight - mLineNumber * mFontHeight) < mFontHeight) {
                        String subString = mTextString.substring(index, i);
                        String tempStr = (new StringBuilder(subString).append("...")).toString();
                        mStrVector.addElement(tempStr);
                        break;
                    }
                    mStrVector.addElement(mTextString.substring(index, i + 1));
                    index = i + 1;
                }
            } else if(i == strLen - 1) {
                if(tempLineWidth <= mTextRectWidth) {
                    mLineNumber++;
                    tempLineWidth = 0;
                    String subString = mTextString.substring(index, i + 1);
                    mStrVector.addElement(subString);
                    index = i + 1;
                }
            }
        }
    }
}
