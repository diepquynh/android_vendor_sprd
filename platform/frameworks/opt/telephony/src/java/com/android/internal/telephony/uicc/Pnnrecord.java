
package com.android.internal.telephony.uicc;

import com.android.internal.telephony.gsm.SimTlv;

public class Pnnrecord {
    private String PNNLongName;
    private String PNNShortName;

    static final int TAG_FULL_NETWORK_NAME = 0x43;
    static final int TAG_SHORT_NETWORK_NAME = 0x45;

    public Pnnrecord(byte[] data) {
        SimTlv tlv = new SimTlv(data, 0, data.length);

        for (; tlv.isValidObject(); tlv.nextObject()) {
            // PNN long name
            if (tlv.getTag() == TAG_FULL_NETWORK_NAME) {
                PNNLongName = IccUtils.networkNameToString(tlv.getData(), 0, tlv.getData().length);
            }
            // PNN short name
            if (tlv.getTag() == TAG_SHORT_NETWORK_NAME) {
                PNNShortName = IccUtils.networkNameToString(tlv.getData(), 0, tlv.getData().length);
                break;
            }
        }
    }

    public String getLongname() {
        return PNNLongName;
    }

    public String getShortName() {
        return PNNShortName;
    }

    public String toString() {
        return "PNNLongName = " + PNNLongName + ", PNNShortName = " + PNNShortName;
    }
}
