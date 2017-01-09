/** Created by SPRD */
package com.android.internal.telephony.cat.bip;

import android.os.Parcel;
import android.os.Parcelable;
import com.android.internal.telephony.cat.CatLog;

public class ReceiveChannelData extends ChannelData implements Parcelable{
    public int channelDataLength = 0;

    public ReceiveChannelData() {
        super();
    }

    public ReceiveChannelData(Parcel in) {
        super(in);
        channelDataLength = in.readInt();
    }

    public void writeToParcel(Parcel dest, int flags) {
        super.writeToParcel(dest,flags);
        dest.writeInt(channelDataLength);
    }

    public static final Parcelable.Creator<ReceiveChannelData> CREATOR = new Parcelable.Creator<ReceiveChannelData>() {
       public ReceiveChannelData createFromParcel(Parcel in) {
           return new ReceiveChannelData(in);
       }

       public ReceiveChannelData[] newArray(int size) {
           return new ReceiveChannelData[size];
       }
    };

    @Override
    public int describeContents() {
        return 0;
    }
}
