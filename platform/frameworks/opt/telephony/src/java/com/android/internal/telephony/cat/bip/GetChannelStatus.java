/** Created by SPRD */
package com.android.internal.telephony.cat.bip;

import android.os.Parcel;
import android.os.Parcelable;

public class GetChannelStatus extends ChannelData implements Parcelable {

    public GetChannelStatus() {
        super();
    }

    public GetChannelStatus(Parcel in) {
        super(in);
    }

    public static final Parcelable.Creator<GetChannelStatus> CREATOR = new Parcelable.Creator<GetChannelStatus>() {
        public GetChannelStatus createFromParcel(Parcel in) {
            return new GetChannelStatus(in);
        }

        public GetChannelStatus[] newArray(int size) {
            return new GetChannelStatus[size];
        }
    };

    @Override
    public int describeContents() {
        return 0;
    }
}
