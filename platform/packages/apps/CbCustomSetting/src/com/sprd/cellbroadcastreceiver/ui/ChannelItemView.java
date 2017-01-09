package com.sprd.cellbroadcastreceiver.ui;

import com.sprd.cellbroadcastreceiver.R;
import com.sprd.cellbroadcastreceiver.data.ChannelItemData;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

public class ChannelItemView extends LinearLayout {

    public ChannelItemView(Context context) {
        super(context);
    }

    public ChannelItemView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public ChannelItemView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    public ChannelItemView(Context context, AttributeSet attrs,
            int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);

    }

    protected void onFinishInflate() {
        mIcon = (ImageView) findViewById(R.id.icon);
//        mEnable = (TextView) findViewById(R.id.on_off);
//        mCanSave = (TextView) findViewById(R.id.save);
        mChannelID = (TextView) findViewById(R.id.channel_id);
        mChannelName = (TextView) findViewById(R.id.channel_name);

    }

    public void init() {
        getIcon().setBackgroundResource(R.drawable.cb_item);
        //modify for bug 549170 begin
        getChannel().setText(getUserData() != null ? String.valueOf(getUserData().getChannelId()) : "");
        getChannelName().setText(getUserData() != null ? getUserData().getChannelName() : "");
        //modify for bug 549170 end
//        String szEnable = getUserData().getEnabled() == 1 ? "ON" : "OFF";
//        getEnable().setText(szEnable);
//        String szSave = getUserData().getSave() == 1 ? "Save" : "Discard";
//        getSave().setText(szSave);
    }

    public ChannelItemData getUserData() {
        Object obj = getTag();
        if (obj instanceof ChannelItemData) {
            return (ChannelItemData) obj;
        } else {
            return null;
        }
    }

    private ImageView getIcon() {
        return mIcon;
    }

    private TextView getChannel() {
        return mChannelID;
    }

    private TextView getChannelName() {
        return mChannelName;
    }

//    private TextView getEnable() {
//        return mEnable;
//    }
//
//    private TextView getSave() {
//        return mCanSave;
//    }

    private ImageView mIcon;
    private TextView mChannelID;
    private TextView mChannelName;
//    private TextView mEnable;
//    private TextView mCanSave;

}
