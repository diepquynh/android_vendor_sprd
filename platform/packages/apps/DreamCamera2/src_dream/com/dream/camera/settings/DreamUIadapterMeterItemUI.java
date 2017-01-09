package com.dream.camera.settings;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.CheckedTextView;
import android.widget.ImageView;
import android.widget.LinearLayout;
import com.android.camera2.R;

public class DreamUIadapterMeterItemUI extends LinearLayout {

    CheckedTextView checkedText;
    ImageView imageView;

    public DreamUIadapterMeterItemUI(Context context, AttributeSet attrs,
            int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        // TODO Auto-generated constructor stub
    }

    public DreamUIadapterMeterItemUI(Context context, AttributeSet attrs) {
        super(context, attrs);
        // TODO Auto-generated constructor stub
    }

    public DreamUIadapterMeterItemUI(Context context) {
        super(context);
        // TODO Auto-generated constructor stub
    }

    @Override
    protected void onFinishInflate() {
        checkedText = (CheckedTextView) findViewById(android.R.id.text1);
        imageView = (ImageView) findViewById(R.id.image);
    }

    public void setData(CharSequence text, int imageId) {
        checkedText.setText(text);
        imageView.setImageResource(imageId);
    }

    public void setChecked(boolean b) {
        checkedText.setChecked(b);
    }

}
