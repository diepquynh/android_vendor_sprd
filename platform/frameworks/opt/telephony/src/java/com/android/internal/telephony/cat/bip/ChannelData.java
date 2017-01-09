/** Created by SPRD */
package com.android.internal.telephony.cat.bip;

import android.graphics.Bitmap;
import android.os.Parcel;
import android.os.Parcelable;

//Base ChannelData
public class ChannelData {

    public int openChannelType = 0;
    public String text = null;
    public Bitmap icon = null;
    public boolean iconSelfExplanatory = false;
    public boolean isNullAlphaId = false;

    public ChannelData() {
    }

    public int getChannelType() {
        return openChannelType;
    }

    public void setChannelType(int type) {
        openChannelType = type;
    }

    public ChannelData(Parcel in) {
        openChannelType = in.readInt();
        text = in.readString();
        icon = in.readParcelable(null);
        iconSelfExplanatory = in.readInt() == 1 ? true : false;
        isNullAlphaId = in.readInt() == 1 ? true : false;
    }

    public void writeToParcel(Parcel dest, int flags) {
        dest.writeInt(openChannelType);
        dest.writeString(text);
        dest.writeParcelable(icon, 0);
        dest.writeInt(iconSelfExplanatory ? 1 : 0);
        dest.writeInt(isNullAlphaId ? 1 : 0);
    }

    public boolean setIcon(Bitmap Icon) {
        return true;
    }
}
