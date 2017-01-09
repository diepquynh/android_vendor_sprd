/*
 * Copyright (C) 2010,2012 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.uphoto;

import android.widget.BaseAdapter;

public abstract class DragGridViewAdapter extends BaseAdapter {
    public abstract boolean checkBeforeDragItem(int pos);
    public abstract void exchangeItem(int position0, int position1);
}
