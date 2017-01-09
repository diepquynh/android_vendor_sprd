package com.sprd.cellbroadcastreceiver.ui;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.sprd.cellbroadcastreceiver.R;
import com.sprd.cellbroadcastreceiver.data.LanguageItemData;
import com.sprd.cellbroadcastreceiver.util.LanguageIds;

public class LanguageItemView extends RelativeLayout {

    public LanguageItemView(Context context) {
        super(context);
    }

    public LanguageItemView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public LanguageItemView(Context context, AttributeSet attrs,
            int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    public LanguageItemView(Context context, AttributeSet attrs,
            int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
    }

    public LanguageItemData getUserData() {
        Object obj = getTag();
        if (obj instanceof LanguageItemData) {
            return (LanguageItemData) obj;
        } else {
            return null;
        }
    }

    protected void onFinishInflate() {
        mLanguageName = (TextView) findViewById(R.id.language_name);
        mDeleteImg = (ImageView) findViewById(R.id.delete_icon);
    }

    public void init(){
        //modify for bug 549170 begin
        int languageBit = getUserData() != null ? getUserData().getLanguageBit() : 0;
        getLanguageName().setText(LanguageIds.LangMap[languageBit - 1]);
        //modify for bug 549170 end
        //getDeleteImg().setBackground(android.R.drawable.ic_menu_delete);
        getDeleteImg().setBackgroundResource(android.R.drawable.ic_menu_delete);
    }

    private TextView getLanguageName() {
        return mLanguageName;
    }

    public ImageView getDeleteImg() {
        return mDeleteImg;
    }

    private TextView mLanguageName;
    private ImageView mDeleteImg;
}
