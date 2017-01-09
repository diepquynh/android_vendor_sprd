package com.sprdroid.note;

import android.widget.CheckBox;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.ImageView;

/*
 * ListView中每一行中包含的组件
 * 这个类把ListView中的每一行都包装成一个ListItemView对象
 */
public final class ListItemView {
    public LinearLayout linearlayout;
    public TextView tv_left;
    public TextView tv_right;
    public ImageView clock_image;
    public CheckBox check;
    //wangsl
    //为了判断checkbox选中项位置，是否同adapter中getview的position一致
    public int position;  
    //wangsl
}
