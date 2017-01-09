/*
 * Copyright (C) 2006 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.sprd.contacts.common.widget;

import android.content.Context;
import android.content.res.ColorStateList;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import com.sprd.contacts.common.util.SystemProperties;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.widget.PopupWindow;
import android.widget.TextView;

import com.android.contacts.common.R;

public class BladeView extends View {

    private int mMinCharHeight = 1;
    /**
     * SPRD:Bug562091 Adjust the alphabet's size of the fast scroll bar.
     * Variable "mMaxCharHeight" limit the size of char,variable "mCharClickHeight"
     * decides the click position.
     */
    private int mMaxCharHeight = 60;
    private int mCharClickHeight = 5;
    private int mCharHeight = 5;
    private OnItemClickListener mOnItemClickListener;
    private PopupWindow mPopupWindow;
    private TextView mPopupText;
    private int mTextColor;
    private boolean mTouched = false;
    private boolean mIsTW = false;
    private int mCurItem = 0;
    private float mHeight = 0;
    private static int BLADE_BACKGROUND_WIDTH = 25;

    private int[] mTextColorSet;
    private final String[] mAlphabet = {
            "A", "B", "C", "D", "E", "F", "G", "H", "I", "J",
            "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "#"
    };

    private final String[] mTraditional = {
             "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16",
             "17", "18", "19", "20",
             "21", "22", "23", "24", "25", "26", "27", "28", "29", "30", "31", "32",
            "A", "B", "C", "D", "E", "F", "G", "H", "I", "J",
            "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "#"
    };

    private final String[] mTraditionalFull = {
            "1劃", "2劃", "3劃", "4劃", "5劃", "6劃", "7劃", "8劃", "9劃", "10劃", "11劃", "12劃", "13劃", "14劃", "15劃", "16劃",
            "17劃", "18劃", "19劃", "20劃",
            "21劃", "22劃", "23劃", "24劃", "25劃", "26劃", "27劃", "28劃", "29劃", "30劃", "31劃", "32劃",
           "A", "B", "C", "D", "E", "F", "G", "H", "I", "J",
           "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "#"
   };
    private String TAG = "BladeView";
    private Context mContext;

    public BladeView(Context context) {
        super(context);
        mContext = context;
        BLADE_BACKGROUND_WIDTH = mContext.getResources().getDimensionPixelSize(
                R.dimen.contact_bladeview_width);
        mIsTW = isTw(context);
        mTextColorSet = new int[(mIsTW ? mTraditional.length : mAlphabet.length)];
    }

    public BladeView(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;
        BLADE_BACKGROUND_WIDTH = mContext.getResources().getDimensionPixelSize(
                R.dimen.contact_bladeview_width);
        mIsTW = isTw(context);
        mTextColorSet = new int[(mIsTW ? mTraditional.length : mAlphabet.length)];
    }

    public BladeView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        mContext = context;
        ColorStateList textColor = null;
        BLADE_BACKGROUND_WIDTH = mContext.getResources().getDimensionPixelSize(
                R.dimen.contact_bladeview_width);
        if (textColor != null) {
            mTextColor = textColor.getDefaultColor();
        } else {
            mTextColor = Color.GRAY;
        }
        mIsTW = isTw(context);
        mTextColorSet = new int[(mIsTW ? mTraditional.length : mAlphabet.length)];
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        int heightSize = MeasureSpec.getSize(heightMeasureSpec);
        Log.e(TAG, "onMeasure heightSize = " + heightSize);
        mHeight = (float)heightSize;
        int charNumber = getAlphabet(mIsTW).length;
        int charHeigtht = heightSize / charNumber;
        Log.e(TAG, "onMeasure charHeigtht = " + charHeigtht);
        Log.e(TAG, "onMeasure mMinCharHeight = " + mMinCharHeight);
        /**
         * SPRD:Bug562091 Adjust the alphabet's size of the fast scroll bar
         *
         */
        if (charHeigtht < mMinCharHeight) {
            heightSize = mMinCharHeight * charNumber;
            mCharHeight = mMinCharHeight;
            mCharClickHeight = mCharHeight;
        } else if (charHeigtht <= mMaxCharHeight) {
            mCharHeight = charHeigtht;
            mCharClickHeight = mCharHeight;
        } else {
            mCharHeight = mMaxCharHeight;
            mCharClickHeight = charHeigtht;
        }
        Log.e(TAG, "onMeasure mCharHeight = " + mCharHeight);
        setMeasuredDimension(BLADE_BACKGROUND_WIDTH, heightSize);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        int textSize = 0;
        int charNumber = getAlphabet(mIsTW).length;
        super.onDraw(canvas);
        Paint paint = new Paint();
        paint.setAntiAlias(true);
        paint.setTextSize(mCharHeight - 6);
        textSize = mCharHeight - 6;
        paint.setColor(this.getResources().getColor(R.color.contact_bladeview_index_text_color));
        for (int i = 0; i < charNumber; ++i) {
            String currentChar = getAlphabet(mIsTW)[i];
            if (mTextColorSet[i] == Color.TRANSPARENT) {
                if (mTouched) {
                    paint.setColor(this.getResources().getColor(
                            R.color.contact_bladeview_index_pressed_text_color));
                } else {
                    paint.setColor(this.getResources().getColor(
                            R.color.contact_bladeview_index_text_color));
                }
            }
            float y = (i + 1) * mCharHeight;
            if (mContext.getResources().getConfiguration().orientation == Configuration.ORIENTATION_LANDSCAPE) {
                if (mIsTW) {
                    if (i % 8 != 0) {
                        currentChar = " ";
                    }
                    if (i == 0) {
                        currentChar = " ";
                    }
                    paint.setTextSize((mCharHeight) * 5);
                    textSize = (mCharHeight) * 5;
                    y = i * (mHeight / (float) mTraditional.length);
                    canvas.drawText(
                            currentChar,
                            ((float) BLADE_BACKGROUND_WIDTH - (float) paint
                                    .measureText(currentChar)) / (float) 2, y,
                            paint);
                } else {
                    if (i % 3 != 0) {
                        currentChar = " ";
                    }
                    paint.setTextSize((mCharHeight) * 2);
                    textSize = (mCharHeight) * 2;
                    y = (i + 2) * mCharHeight;
                    canvas.drawText(
                            currentChar,
                            ((float) BLADE_BACKGROUND_WIDTH - (float) paint
                                    .measureText(currentChar)) / (float) 2, y,
                            paint);
                }
            } else if (mIsTW) {
                if (i == 0 || i % 3 != 0) {
                    currentChar = " ";
                }
                paint.setTextSize((mCharHeight) * 2);
                textSize = (mCharHeight) * 2;
                y = i * (mHeight / (float) mTraditional.length);
                canvas.drawText(
                        currentChar,
                        ((float) BLADE_BACKGROUND_WIDTH - (float) paint
                                .measureText(currentChar)) / (float) 2, y,
                        paint);
            } else {
                paint.setTextSize(mCharHeight);
                textSize = mCharHeight;
                y = (i + 1) * (mHeight / (float) mAlphabet.length);
                canvas.drawText(
                        currentChar,
                        ((float) BLADE_BACKGROUND_WIDTH - (float) paint
                                .measureText(currentChar)) / (float) 2, y,
                        paint);
            }
        }
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        final int action = event.getActionMasked();
        final int charNumber = getAlphabet(mIsTW).length;
        if (action == MotionEvent.ACTION_DOWN
                || action == MotionEvent.ACTION_MOVE) {
            getParent().requestDisallowInterceptTouchEvent(true);
            /**
             * SPRD:Bug562091 Adjust the alphabet's size of the fast scroll bar.
             * Base on variable "mCharClickHeight" to show the information of "Popup".
             */
            int item = (int) (event.getY() / mCharClickHeight);
            if (item < 0 || item >= charNumber) {
                return true;
            }
            showPopup(item);
            performItemClicked(item);
            mTouched = true;
            mCurItem = item;
        } else {
            dismissPopup();
            mTouched = false;
        }
        return true;
        // return super.onTouchEvent(event);
    }

    private void showPopup(int item) {
        if (mPopupWindow == null) {
            Log.e(TAG,
                    "width = "
                            + this.getResources().getDimensionPixelSize(
                                    R.dimen.contact_bladeview_popup_width));
            mPopupWindow = new PopupWindow(this.getResources()
                    .getDimensionPixelSize(
                            R.dimen.contact_bladeview_popup_width), this
                    .getResources().getDimensionPixelSize(
                            R.dimen.contact_bladeview_popup_width));
            mPopupText = new TextView(getContext());
            /**
             * SPRD:Bug566697 when the system language is tw,the text size of popup should adjust.
             *
             */
            if (mIsTW) {
                mPopupText.setTextSize(this.getResources().getDimension(
                        R.dimen.contact_bladeview_popup_tw_text_size));
            } else {
                mPopupText.setTextSize(this.getResources().getDimension(
                        R.dimen.contact_bladeview_popup_text_size));
            }
            mPopupText
                    .setBackgroundResource(R.drawable.ic_contacts_index_backgroud_sprd);
            mPopupText.setTextColor(this.getResources().getColor(
                    R.color.contact_bladeview_popup_text_color));
            mPopupText.setGravity(Gravity.CENTER_HORIZONTAL
                    | Gravity.CENTER_VERTICAL);
            mPopupWindow.setContentView(mPopupText);
        }

        String text = getFullAlphabet(mIsTW)[item];
        mPopupText.setText(text);
        if (mPopupWindow.isShowing()) {
            // mPopupWindow.update();
        } else {
            mPopupWindow.showAtLocation(getRootView(), Gravity.CENTER_HORIZONTAL
                    | Gravity.CENTER_VERTICAL, 0, 0);
        }
    }

    private void dismissPopup() {
        if (mPopupWindow != null) {
            mPopupWindow.dismiss();
        }
    }

    public void setOnItemClickListener(OnItemClickListener listener) {
        mOnItemClickListener = listener;
    }

    private void performItemClicked(int item) {
        if (mOnItemClickListener != null) {
            mOnItemClickListener.onItemClick(item);
        }
    }

    public interface OnItemClickListener {
        void onItemClick(int item);
    }

    public void configCharacterColorToDefault() {

        for (int i = 0; i < mTextColorSet.length; i++) {
            mTextColorSet[i] = Color.TRANSPARENT;
        }
    }

    public boolean isTw(Context context) {
        /**
         * Bug568608 under the system language is tw, just use the alphabet in the fast scroll bar
         * the original code
        String country = context.getResources().getConfiguration().locale.getCountry();
        if ("TW".equals(country)) {
            return true;
        }
         */
        return false;
    }

    public String[] getAlphabet(boolean isTW) {
        if (isTW) {
            return mTraditional;
        } else {
            return mAlphabet;
        }
    }

    public String[] getFullAlphabet(boolean isTW) {
        if (isTW) {
            return mTraditionalFull;
        } else {
            return mAlphabet;
        }
    }
}
