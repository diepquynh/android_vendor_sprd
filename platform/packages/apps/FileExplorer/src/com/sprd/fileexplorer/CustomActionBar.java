package com.sprd.fileexplorer;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.util.Log;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

public class CustomActionBar implements View.OnClickListener {

    public static final String TAG = "CustomActionBar";
    
    private Context mContext;
    private View mView;
    private ImageView mLogo;
    private TextView mTitle;
    private ImageView mSearch;
    private ImageView mOverflow;
    private static CustomActionBar mActionBar;

    private CustomActionBar(Context context, View view) {
        if (view == null) {
            Log.e(TAG, "Init CustomActionBar failed! view == null");
            throw new NullPointerException();
        }
        mContext = context;
        mView = view;
        mLogo = (ImageView) mView.findViewById(R.id.custom_actionbar_logo);
        mTitle = (TextView) mView.findViewById(R.id.custom_actionbar_title);
        mSearch = (ImageView) mView.findViewById(R.id.custom_actionbar_search);
        mOverflow = (ImageView) mView
                .findViewById(R.id.custom_actionbar_overflow);
        mLogo.setOnClickListener(this);
        mTitle.setOnClickListener(this);
        mSearch.setOnClickListener(this);
        mOverflow.setOnClickListener(this);
    }

    public static CustomActionBar init(Context context, View view) {
        if (mActionBar == null) {
            mActionBar = new CustomActionBar(context, view);
        }
        return mActionBar;
    }

    public CustomActionBar getActionBar() {
        return mActionBar;
    }

    public void setUpVisible(boolean visible) {
        // TODO: Add home view
    }

    public void setLogo(Drawable icon) {
        mLogo.setImageDrawable(icon);
    }

    public void setLogo(int resId) {
        mLogo.setImageDrawable(mContext.getResources().getDrawable(resId));
    }

    public void setTitle(String text) {
        mTitle.setText(text);
    }

    public void setTitle(int resourceId) {
        mTitle.setText(mContext.getText(resourceId));
    }

    public void setTitleVisible(boolean visible) {
        mTitle.setVisibility(visible ? View.VISIBLE : View.GONE);
    }

    public void setTitleVisisble(int visible) {
        mTitle.setVisibility(visible);
    }

    @Override
    public void onClick(View v) {
        switch (v.getId()) {
        case R.id.custom_actionbar_logo:
            break;
        case R.id.custom_actionbar_title:
            break;
        case R.id.custom_actionbar_search:
            break;
        case R.id.custom_actionbar_overflow:
            break;
        }

    }

    // TODO: IN FUTURE VERSION, THERE WILL BE ACTIONMODE.
    public void onAcionMode() {

    }

    public void onNormalMode() {

    }
}
