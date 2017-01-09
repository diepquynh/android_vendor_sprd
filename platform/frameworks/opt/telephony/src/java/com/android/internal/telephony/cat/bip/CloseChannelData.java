/** Created by SPRD */
package com.android.internal.telephony.cat.bip;

import android.os.Parcel;
import android.os.Parcelable;

public class CloseChannelData extends ChannelData implements Parcelable {

    public CloseChannelData() {
        super();
    }

    public CloseChannelData(Parcel in) {
        super(in);
    }

    public static final Parcelable.Creator<ChannelData> CREATOR = new Parcelable.Creator<ChannelData>() {
        public ChannelData createFromParcel(Parcel in) {
            return new CloseChannelData(in);
        }

        public CloseChannelData[] newArray(int size) {
            return new CloseChannelData[size];
        }
    };

    @Override
    public int describeContents() {
        return 0;
    }
}
