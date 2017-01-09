/** Created by SPRD */
package com.android.internal.telephony.cat.bip;

import android.os.Parcel;
import android.os.Parcelable;
import com.android.internal.telephony.cat.CatLog;
import android.telephony.TelephonyManager;
import android.text.TextUtils;

public class OpenChannelData extends ChannelData implements Parcelable{
    public String NetAccessName;
    public int bufferSize;
    public byte BearerType;
    public String BearerParam;
    public byte OtherAddressType;
    public String OtherAddress;
    public String LoginStr;
    public String PwdStr;
    public byte transportType;
    public int portNumber;
    public byte DataDstAddressType;
    public String DataDstAddress;

    // constants
    public static final int BEARER_TYPE_CSD = 1;
    public static final int BEARER_TYPE_GPRS = 2;
    public static final int BEARER_TYPE_DEFAULT = 3;
    public static final int BEARER_TYPE_UTRAN_PACKET_SERVICE = 11;
    public static final int ADDRESS_TYPE_IPV4 = 0X21;
    public static final int ADDRESS_TYPE_IPV6 = 0X57;
    public static final int TRANSPORT_TYPE_UDP = 1;
    public static final int TRANSPORT_TYPE_TCP = 2;

    public OpenChannelData() {
        super();
        NetAccessName = null;
        bufferSize = 0;
        BearerType = BEARER_TYPE_DEFAULT;
        BearerParam = null;
        OtherAddressType = 0;;
        OtherAddress = null;
        LoginStr = null;
        PwdStr = null;
        transportType = TRANSPORT_TYPE_UDP;
        portNumber = 0;
        DataDstAddressType = 0;
        DataDstAddress = null;
    }

    public OpenChannelData(Parcel in) {
        super(in);
        NetAccessName = in.readString();
        bufferSize = in.readInt();
        BearerType = in.readByte();
        BearerParam = in.readString();
        OtherAddressType = in.readByte();
        OtherAddress = in.readString();
        LoginStr = in.readString();
        PwdStr = in.readString();
        transportType = in.readByte();
        portNumber = in.readInt();
        DataDstAddressType = in.readByte();
        DataDstAddress = in.readString();
    }

    public void writeToParcel(Parcel dest, int flags) {
        super.writeToParcel(dest,flags);
        dest.writeString(NetAccessName);
        dest.writeInt(bufferSize);
        dest.writeByte(BearerType);
        dest.writeString(BearerParam);
        dest.writeByte(OtherAddressType);
        dest.writeString(OtherAddress);
        dest.writeString(LoginStr);
        dest.writeString(PwdStr);
        dest.writeByte(transportType);
        dest.writeInt(portNumber);
        dest.writeByte(DataDstAddressType);
        dest.writeString(DataDstAddress);
    }

    public static final Parcelable.Creator<OpenChannelData> CREATOR = new Parcelable.Creator<OpenChannelData>() {
        public OpenChannelData createFromParcel(Parcel in) {
            return new OpenChannelData(in);
        }

        public OpenChannelData[] newArray(int size) {
            return new OpenChannelData[size];
        }
    };

    @Override
    public int describeContents() {
        return 0;
    }

}
