/** Create by Spreadst */

package com.spreadst.lockscreen;

import android.content.Context;
import android.view.LayoutInflater;
import android.widget.ImageView;
import android.widget.RelativeLayout;
import android.widget.TextView;

public class ThumbView extends RelativeLayout {

    ImageView mImageView;
    TextView mTextView;
    ImageView mSelectImageView;

    public ThumbView(Context context) {
        super(context);
        LayoutInflater inflater = LayoutInflater.from(context);
        inflater.inflate(R.layout.thumbnail, this);
        mImageView = (ImageView) findViewById(R.id.thumbView);
        mTextView = (TextView) findViewById(R.id.thumbText);
        mSelectImageView = (ImageView) findViewById(R.id.selectedview);
    }

    public ImageView getImageView() {

        return mImageView;
    }

    public TextView getTextView() {

        return mTextView;
    }

    public ImageView getSelectImageView() {

        return mSelectImageView;
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
    }

}
