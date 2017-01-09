
package com.android.internal.telephony.uicc;

public class Oplrecord {
    public int[] mOplplmn = {
            0, 0, 0, 0, 0, 0
    };
    public int mOpllac1;
    public int mOpllac2;
    public int PNNrecordnum;

    public Oplrecord(byte[] record) {
        Oplplmn(record);
        mOpllac1 = ((record[3] & 0xff) << 8) | (record[4] & 0xff);
        mOpllac2 = ((record[5] & 0xff) << 8) | (record[6] & 0xff);
        PNNrecordnum = (short) (record[7] & 0xff);
    }

    public void Oplplmn(byte[] record) {
        // getMCC
        mOplplmn[0] = record[0] & 0x0f;
        mOplplmn[1] = (record[0] >> 4) & 0x0f;
        mOplplmn[2] = record[1] & 0x0f;

        // GetMNC
        mOplplmn[3] = record[2] & 0x0f;
        mOplplmn[4] = (record[2] >> 4) & 0x0f;

        mOplplmn[5] = (record[1] >> 4) & 0x0f;
        if (0x0f == mOplplmn[5]) {
            mOplplmn[5] = 0;
        }
    }

    public int getPnnRecordNum() {
        return PNNrecordnum;
    }

    public String toString() {
        return "OPL Record mOplplmn = " + Integer.toHexString(mOplplmn[0])
                + Integer.toHexString(mOplplmn[1])
                + Integer.toHexString(mOplplmn[2]) + Integer.toHexString(mOplplmn[3])
                + Integer.toHexString(mOplplmn[4])
                + Integer.toHexString(mOplplmn[5]) + ", mOpllac1 =" + mOpllac1 + ", mOpllac2 ="
                + mOpllac2
                + " ,PNNrecordnum = " + PNNrecordnum;
    }

}
