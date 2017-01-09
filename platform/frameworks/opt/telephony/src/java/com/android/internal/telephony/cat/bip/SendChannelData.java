/** Created by SPRD */
package com.android.internal.telephony.cat.bip;

import android.os.Parcel;
import android.os.Parcelable;
import com.android.internal.telephony.cat.CatLog;

public class SendChannelData extends ChannelData implements Parcelable{
    public String sendDataStr;

    public SendChannelData() {
        super();
        sendDataStr = null;
    }

    public SendChannelData(Parcel in) {
        super(in);
        sendDataStr = in.readString();
    }

    public void writeToParcel(Parcel dest, int flags) {
        super.writeToParcel(dest,flags);
        dest.writeString(sendDataStr);
    }

    public static final Parcelable.Creator<SendChannelData> CREATOR = new Parcelable.Creator<SendChannelData>() {
       public SendChannelData createFromParcel(Parcel in) {
           return new SendChannelData(in);
       }

       public SendChannelData[] newArray(int size) {
           return new SendChannelData[size];
       }
    };

    @Override
    public int describeContents() {
        return 0;
    }
}
