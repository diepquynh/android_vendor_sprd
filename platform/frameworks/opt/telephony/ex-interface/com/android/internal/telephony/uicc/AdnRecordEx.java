package com.android.internal.telephony.uicc;

import android.os.Parcelable;
import android.os.Parcel;
import android.telephony.PhoneNumberUtils;
import android.text.TextUtils;
import android.telephony.Rlog;

import java.util.Arrays;
import com.android.internal.telephony.EncodeException;
import com.android.internal.telephony.IccPhoneBookOperationException;
import com.android.internal.telephony.TeleFrameworkFactory;
/**
 * {@hide}
 */
public class AdnRecordEx implements Parcelable {
    static final String LOG_TAG = "AdnRecordEx";

    String mAlphaTag = null;
    String mNumber = null;
    String[] mEmails;
    int mExtRecord = 0xff;
    int mEfid;                   // or 0 if none
    int mRecordNumber;           // or 0 if none
    String mAnr = null;
    String mAas = null;
    String mSne = null;
    String mGrp = null;
    String mGas = null;
    int mIndex = -1;

    static final int FOOTER_SIZE_BYTES = 14;

    // Maximum size of the un-extended number field
    static final int MAX_NUMBER_SIZE_BYTES = 11;

    static final int EXT_RECORD_LENGTH_BYTES = 13;
    static final int EXT_RECORD_TYPE_ADDITIONAL_DATA = 2;
    static final int EXT_RECORD_TYPE_MASK = 3;
    static final int MAX_EXT_CALLED_PARTY_LENGTH = 0xa;

    // ADN offset
    static final int ADN_BCD_NUMBER_LENGTH = 0;
    static final int ADN_TON_AND_NPI = 1;
    static final int ADN_DIALING_NUMBER_START = 2;
    static final int ADN_DIALING_NUMBER_END = 11;
    static final int ADN_CAPABILITY_ID = 12;
    static final int ADN_EXTENSION_ID = 13;

    public static final int MAX_LENTH_ADN = (ADN_DIALING_NUMBER_END
            - ADN_DIALING_NUMBER_START + 1) * 2;
    public static final int MAX_LENTH_NUMBER = (ADN_DIALING_NUMBER_END
            - ADN_DIALING_NUMBER_START + 1) * 2
            + MAX_EXT_CALLED_PARTY_LENGTH * 2;

    public static final String ANR_SPLIT_FLG = ";";
    static final int TYPE1_DATA_LENGTH = 15;
    static final int ADN_SFI = 0;
    static final int ADN_REC_ID = 1;

    public static final Parcelable.Creator<AdnRecordEx> CREATOR
             = new Parcelable.Creator<AdnRecordEx>() {
        @Override
        public AdnRecordEx createFromParcel(Parcel source) {
            int efid;
            int recordNumber;
            String alphaTag;
            String number;
            String[] emails;
            String anr = null;
            String aas = null;
            String sne = null;
            String grp = null;
            String gas = null;

            efid = source.readInt();
            recordNumber = source.readInt();
            alphaTag = source.readString();
            number = source.readString();
            emails = source.readStringArray();
            anr = source.readString();
            aas = source.readString();
            sne = source.readString();
            grp = source.readString();
            gas = source.readString();

            return new AdnRecordEx(efid, recordNumber, alphaTag, number, emails,
                    anr, aas, sne, grp, gas);
        }

        @Override
        public AdnRecordEx[] newArray(int size) {
            return new AdnRecordEx[size];
        }
    };

    public AdnRecordEx (byte[] record) {
        this(0, 0, record);
    }

    public AdnRecordEx (int efid, int recordNumber, byte[] record) {
        this.mEfid = efid;
        this.mRecordNumber = recordNumber;
        parseRecord(record);
    }

    public AdnRecordEx (String alphaTag, String number) {
        this(0, 0, alphaTag, number);
    }

    public AdnRecordEx (String alphaTag, String number, String[] emails) {
        this(0, 0, alphaTag, number, emails);
    }

    public AdnRecordEx (int efid, int recordNumber, String alphaTag, String number, String[] emails) {
        this.mEfid = efid;
        this.mRecordNumber = recordNumber;
        this.mAlphaTag = alphaTag;
        this.mNumber = number;
        this.mEmails = emails;
    }

    public AdnRecordEx(int efid, int recordNumber, String alphaTag, String number) {
        this.mEfid = efid;
        this.mRecordNumber = recordNumber;
        this.mAlphaTag = alphaTag;
        this.mNumber = number;
        this.mEmails = null;
    }

    public AdnRecordEx(String mAlphaTag, String mNumber, String[] mEmails,
            String anr, String aas, String sne, String grp, String gas) {
        this(0, 0, mAlphaTag, mNumber, mEmails, anr, aas, sne, grp, gas);
    }

    public AdnRecordEx(int efid, int recordNumber, String mAlphaTag,
            String mNumber, String[] mEmails, String anr, String aas, String sne,
            String grp, String gas) {
        this(efid, recordNumber, mAlphaTag, mNumber, mEmails);
        this.mAnr = anr;
        this.mAas = aas;
        this.mGrp = grp;
        this.mGas = gas;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeInt(mEfid);
        dest.writeInt(mRecordNumber);
        dest.writeString(mAlphaTag);
        dest.writeString(mNumber);
        dest.writeStringArray(mEmails);
        dest.writeString(mAnr);
        dest.writeString(mAas);
        dest.writeString(mSne);
        dest.writeString(mGrp);
        dest.writeString(mGas);
    }

    @Override
    public int describeContents() {
        return 0;
    }

    private void
    parseRecord(byte[] record) {
        try {
            mAlphaTag = IccUtils.adnStringFieldToString(
                    record, 0, record.length - FOOTER_SIZE_BYTES);
            int footerOffset = record.length - FOOTER_SIZE_BYTES;
            int numberLength = 0xff & record[footerOffset];
            if (numberLength > MAX_NUMBER_SIZE_BYTES) {
                // Invalid number length
                mNumber = "";
                return;
            }

            // Please note 51.011 10.5.1:
            // "If the Dialling Number/SSC String does not contain
            // a dialling number, e.g. a control string deactivating
            // a service, the TON/NPI byte shall be set to 'FF' by
            // the ME (see note 2)."

            mNumber = PhoneNumberUtils.calledPartyBCDToString(
                    record, footerOffset + 1, numberLength);
            mExtRecord = 0xff & record[record.length - 1];
            mEmails = null;
        } catch (RuntimeException ex) {
            Rlog.w(LOG_TAG, "Error parsing AdnRecord", ex);
            mNumber = "";
            mAlphaTag = "";
            mEmails = null;
        }
    }

    @Override
    public String toString() {
        return "ADN Record '" + mAlphaTag + "' '" + mNumber + " " + mEmails + "'";
    }

    public boolean isEmpty() {
        return TextUtils.isEmpty(mAlphaTag) && TextUtils.isEmpty(mNumber) 
                && mEmails == null && (isEmptyAnr(mAnr));
    }

    public boolean hasExtendedRecord() {
        return mExtRecord != 0 && mExtRecord != 0xff;
    }

    private static boolean stringCompareNullEqualsEmpty(String s1, String s2) {
        if (s1 == s2) {
            return true;
        }
        if (s1 == null) {
            s1 = "";
        }
        if (s2 == null) {
            s2 = "";
        }
        return (s1.trim().equals(s2.trim()));
    }

    public boolean isEqual(AdnRecordEx adn) {
        return (stringCompareNullEqualsEmpty(mAlphaTag, adn.mAlphaTag) &&
                stringCompareNullEqualsEmpty(mNumber, adn.mNumber) &&
                stringCompareEmails(mEmails, adn.mEmails) &&
                stringCompareAnr(mAnr, adn.mAnr));
    }

    public boolean isEmptyAnr(String s) {
        if (TextUtils.isEmpty(s)) {
            return true;
        }
        for (int i = 0; i < s.length(); i++) {
            if (s.charAt(i) != ';') {
                return false;
            }
        }
        return true;
    }

    public String getAlphaTag() {
        return mAlphaTag;
    }

    public String getNumber() {
        return mNumber;
    }

    public void setNumber(String number) {
        mNumber = number;
    }

    public String[] getEmails() {
        return mEmails;
    }

    public void setEmails(String[] emails) {
        this.mEmails = emails;
    }

    public String getAnr() {
        return mAnr;
    }

    public void setAnr(String anr) {
        this.mAnr = anr;
    }

    public String getAas() {
        return mAas;
    }

    public void setAas(String aas) {
        this.mAas = aas;
    }

    public String getSne() {
        return mSne;
    }

    public void setSne(String sne) {
        this.mSne = sne;
    }

    public String getGrp() {
        return mGrp;
    }

    public void setGrp(String grp) {
        this.mGrp = grp;
    }

    public String getGas() {
        return mGas;
    }

    public void setGas(String gas) {
        this.mGas = gas;
    }

    public int getIndex() {
        return mIndex;
    }

    public void setIndex(int index) {
        this.mIndex = index;
    }

    public void setRecordNumber(int sim_index) {
        mRecordNumber = sim_index;
    }

    public int getRecId() {
        return mRecordNumber;
    }

    public int getEfid() {
        return mEfid;
    }

    public byte[] buildExtString() {
        String extNumber="";
        if (mNumber != null && mNumber.length() > MAX_LENTH_ADN) {
            if (mNumber.charAt(0) == '+') {
                extNumber = mNumber.substring(MAX_LENTH_ADN + 1);
            } else {
                extNumber = mNumber.substring(MAX_LENTH_ADN);
            }
        }
        Rlog.d(LOG_TAG, "extNumber = "+extNumber);
        byte[] extBcdNumber = PhoneNumberUtils.numberToCalledPartyBCD(extNumber);
        byte[] extString = new byte[EXT_RECORD_LENGTH_BYTES];
        for (int i = 0; i < EXT_RECORD_LENGTH_BYTES; i++) {
            extString[i] = (byte) 0xFF;
        }
        if (!extNumber.isEmpty()) {
            extString[0] = (byte) EXT_RECORD_TYPE_ADDITIONAL_DATA;// ext record
            // type
            extBcdNumber[0] = (byte) (extBcdNumber.length - 1);
            if (extBcdNumber.length <= MAX_EXT_CALLED_PARTY_LENGTH + 1) {
                System.arraycopy(extBcdNumber, 0, extString, 1, extBcdNumber.length);
            }
            extString[12] = (byte) 0xFF; // only support one ext record
        }
        return extString;
    }

    public boolean extRecordIsNeeded() {
        String numberNoPlus = mNumber;
        if (!TextUtils.isEmpty(mNumber)) {
            if (mNumber.charAt(0) == '+') {
                numberNoPlus = numberNoPlus.substring(1);
            }
        }
        if (!TextUtils.isEmpty(numberNoPlus)
                && numberNoPlus.length() <= MAX_LENTH_NUMBER
                && numberNoPlus.length() > MAX_LENTH_ADN) {
            return true;
        }
        return false;
    }

    public boolean extRecord4DisplayIsNeeded() {
        String numberNoPlus = mNumber;
        if (!TextUtils.isEmpty(mNumber)) {
            if (mNumber.charAt(0) == '+') {
                numberNoPlus = numberNoPlus.substring(1);
            }
        }
        if (!TextUtils.isEmpty(numberNoPlus) && numberNoPlus.length() == MAX_LENTH_ADN) {
            return true;
        }
        return false;
    }

    public void
    appendExtRecord (byte[] extRecord) {
        try {
            if (extRecord.length != EXT_RECORD_LENGTH_BYTES) {
                return;
            }

            if ((extRecord[0] & EXT_RECORD_TYPE_MASK)
                    != EXT_RECORD_TYPE_ADDITIONAL_DATA) {
                return;
            }

            if ((0xff & extRecord[1]) > MAX_EXT_CALLED_PARTY_LENGTH) {
                // invalid or empty record
                return;
            }

            mNumber += PhoneNumberUtils.calledPartyBCDFragmentToString(extRecord, 2, 0xff & extRecord[1]);
            // We don't support ext record chaining.
        } catch (RuntimeException ex) {
            Rlog.w(LOG_TAG, "Error parsing AdnRecord ext record", ex);
        }
    }

    public boolean stringCompareEmails(String[] e1, String[] e2) {
        String e = "";
        if (e1 == null) {
            e1 = new String[1];
            e1[0] = e;
        }
        if (e2 == null) {
            e2 = new String[1];
            e2[0] = e;
        }
        return stringCompareNullEqualsEmpty(e1[0], e2[0]);
    }

    public boolean stringCompareAnr(String s1, String s2) {
        String[] pair;
        if (TextUtils.isEmpty(s1) || isEmptyAnr(s1)) {
            s1 = "";
        }
        if (TextUtils.isEmpty(s2) || isEmptyAnr(s2)) {
            s2 = "";
        }
        return stringCompareNullEqualsEmpty(s1, s2);

    }

    public byte[] buildAdnString(int recordSize) {
        byte[] bcdNumber;
        byte[] byteTag = null;
        byte[] adnString;
        int footerOffset = recordSize - FOOTER_SIZE_BYTES;

        // create an empty record
        adnString = new byte[recordSize];
        for (int i = 0; i < recordSize; i++) {
            adnString[i] = (byte) 0xFF;
        }

        String numberNoPlus = mNumber;
        if (!TextUtils.isEmpty(mNumber)) {
            if (mNumber.charAt(0) == '+') {
                numberNoPlus = numberNoPlus.substring(1);
            }
        }
        Rlog.i("AdnRecord", "buildAdnString mNumber:" + mNumber + ", mAlphaTag:"
                + mAlphaTag+"numberNoplus = "+ numberNoPlus);
        if (TextUtils.isEmpty(mNumber) && TextUtils.isEmpty(mAlphaTag) && mEmails != null && mEmails.length != 0) {
            throw new IccPhoneBookOperationException(IccPhoneBookOperationException.WRITE_OPREATION_FAILED, "number and alphaTag is null but emails is not null ");
        }
        if (TextUtils.isEmpty(mNumber) && TextUtils.isEmpty(mAlphaTag) && mGrp != null && mGrp.length() !=0) {
            throw new IccPhoneBookOperationException(IccPhoneBookOperationException.WRITE_OPREATION_FAILED, "number and alphaTag is null but grp is not null ");
        }
        if (TextUtils.isEmpty(mNumber) && TextUtils.isEmpty(mAlphaTag) && mAnr != null && mAnr.length() != 0) {
            throw new IccPhoneBookOperationException(IccPhoneBookOperationException.WRITE_OPREATION_FAILED, "number and alphaTag is null but anr is not null ");
        }
        if (TextUtils.isEmpty(mNumber) && TextUtils.isEmpty(mAlphaTag)) {
            Rlog.w(LOG_TAG, "[buildAdnString] Empty dialing mNumber");
            return adnString;   // return the empty record (for delete)
        } else if (!TextUtils.isEmpty(numberNoPlus) && numberNoPlus.length() > MAX_LENTH_NUMBER){
            Rlog.w(LOG_TAG,
                    "[buildAdnString] Max length of dialing mNumber is "+MAX_LENTH_NUMBER);
            throw new IccPhoneBookOperationException(IccPhoneBookOperationException.OVER_NUMBER_MAX_LENGTH,
                    "Max length of dialing mNumber is "+MAX_LENTH_NUMBER);
        } else if (mAlphaTag != null && mAlphaTag.length() > footerOffset) {
            Rlog.w(LOG_TAG,
                    "[buildAdnString] Max length of tag is " + footerOffset);
            throw new IccPhoneBookOperationException(IccPhoneBookOperationException.OVER_NAME_MAX_LENGTH,
                    "Max length of name is "+footerOffset);
        }else {
            if (!TextUtils.isEmpty(numberNoPlus) && numberNoPlus.length() <= MAX_LENTH_ADN) {
                Rlog.d(LOG_TAG, "mNumber.length < " + MAX_LENTH_ADN);
                bcdNumber = PhoneNumberUtils.numberToCalledPartyBCD(mNumber);
                System.arraycopy(bcdNumber, 0, adnString, footerOffset
                        + ADN_TON_AND_NPI, bcdNumber.length);

                adnString[footerOffset + ADN_BCD_NUMBER_LENGTH] = (byte) (bcdNumber.length);
                adnString[footerOffset + ADN_CAPABILITY_ID] = (byte) 0xFF; // Capacility
                // Id
                adnString[footerOffset + ADN_EXTENSION_ID] = (byte) 0xFF; // Extension
                // Record Id
            } else if (!TextUtils.isEmpty(numberNoPlus)
                    && numberNoPlus.length() <= MAX_LENTH_NUMBER) {
                String adnNumber;
                if (mNumber.charAt(0) == '+') {
                    adnNumber = mNumber.substring(0, MAX_LENTH_ADN + 1);
                } else {
                    adnNumber = mNumber.substring(0, MAX_LENTH_ADN);
                }
                Rlog.d(LOG_TAG, "adnNumber=" + adnNumber);
                if (!TextUtils.isEmpty(adnNumber)) {
                    byte[] adnBcdNumber = PhoneNumberUtils.numberToCalledPartyBCD(adnNumber);
                    System.arraycopy(adnBcdNumber, 0, adnString, footerOffset
                            + ADN_TON_AND_NPI, adnBcdNumber.length);
                    adnString[footerOffset + ADN_BCD_NUMBER_LENGTH] = (byte) (adnBcdNumber.length);
                }
                adnString[footerOffset + ADN_CAPABILITY_ID] = (byte) 0xFF; // Capacility
                adnString[footerOffset + ADN_EXTENSION_ID] = (byte) mExtRecord; // Extension
                Rlog.d(LOG_TAG, "mNumber.length >20 , mExtRecord = " + mExtRecord);
            }

            // mAlphaTag format
            if (!TextUtils.isEmpty(mAlphaTag)) {
                try {
                    byteTag = TeleFrameworkFactory.getInstance().stringToGsmAlphaSS(mAlphaTag);
                    System.arraycopy(byteTag, 0, adnString, 0, byteTag.length);
                } catch (EncodeException ex) {
                    try {
                        byteTag = mAlphaTag.getBytes("utf-16be");
                        if(byteTag != null && byteTag.length < adnString.length){
                           System.arraycopy(byteTag, 0, adnString, ADN_TON_AND_NPI, byteTag.length);
                        }
                        adnString[0] = (byte) 0x80;
                    } catch (java.io.UnsupportedEncodingException ex2) {
                        Rlog.e(LOG_TAG,
                                "[AdnRecord]mAlphaTag convert byte excepiton");
                    }
                }
                Rlog.w(LOG_TAG, "mAlphaTag length="
                        + (byteTag != null ? byteTag.length : "null")
                        + " footoffset =" + footerOffset);
                if (byteTag != null && byteTag.length > footerOffset) {
                    Rlog.w(LOG_TAG, "[buildAdnString] Max length of tag is " + footerOffset);
                    throw new IccPhoneBookOperationException(
                            IccPhoneBookOperationException.OVER_NAME_MAX_LENGTH,
                            "Max length of name is " + footerOffset);
                }
            }
            return adnString;
        }
    }

    public byte[] buildIapString(int recordSize, int recNum) {
        byte[] iapString = null;

        // create an empty record
        iapString = new byte[recordSize];
        for (int i = 0; i < recordSize; i++) {
            iapString[i] = (byte) 0xFF;
        }
        iapString[0] = (byte) recNum;

        return iapString;
    }

    public byte[] buildEmailString(int recordSize, int recordSeq, int efid,
            int adnNum) {
        byte[] byteTag;
        byte[] emailString;
        String emailRecord;
        int footerOffset = recordSize - 2;

        // create an empty record
        emailString = new byte[recordSize];
        for (int i = 0; i < recordSize; i++) {
            emailString[i] = (byte) 0xFF;

        }
        emailString[footerOffset + ADN_SFI] = (byte) efid; // Adn Sfi
        emailString[footerOffset + ADN_REC_ID] = (byte) adnNum; // Adn Record Id

        if (mEmails == null) {
            return emailString;
        } else {
            emailRecord = mEmails[recordSeq];
        }
        if (!TextUtils.isEmpty(emailRecord)) {
            try {
                byteTag = TeleFrameworkFactory.getInstance().isAsciiStringToGsm8BitUnpackedField(emailRecord);
                System.arraycopy(byteTag, 0, emailString, 0, byteTag.length);
            } catch (EncodeException ex) {
                try {
                    byteTag = emailRecord.getBytes("utf-16be");
                    System.arraycopy(byteTag, 0, emailString, ADN_TON_AND_NPI,
                            byteTag.length);
                    emailString[0] = (byte) 0x80;
                } catch (java.io.UnsupportedEncodingException ex2) {
                    Rlog.e(LOG_TAG, "[AdnRecord]emailRecord convert byte exception");
                }
            }
        }
        Rlog.e(LOG_TAG, "emailRecord for adn[" + adnNum + "]==" + emailRecord);
        return emailString;
    }

    public byte[] buildAnrString(int recordSize, int anrCount, int efid,
            int adnNum, int aasIndex) {
        byte[] anrString;
        String anrRecord = null;
        byte[] anrNumber;
        Rlog.e(LOG_TAG, "enter buildAnrString");
        // create an empty record
        anrString = new byte[recordSize];
        for (int i = 0; i < recordSize; i++) {
            anrString[i] = (byte) 0xFF;
        }
        if (TextUtils.isEmpty(mAnr) || mAnr.equals(";") || mAnr.equals(";;")) {
            Rlog.e(LOG_TAG, "[buildAnrString] anr mNumber is empty. ");
            return anrString;
        } else {
            Rlog.e(LOG_TAG, "anr = " + mAnr);
            String[] ret = null;
            if (!TextUtils.isEmpty(mAnr)) {
                ret = (mAnr + "1").split(";");
                ret[ret.length - 1] = ret[ret.length - 1].substring(0,
                        ret[ret.length - 1].length() - 1);
            }
            if (anrCount >= ret.length) {
                return anrString;
            } else {
                anrRecord = ret[anrCount];
            }
            Rlog.e(LOG_TAG, "anrRecord = " + anrRecord);
            if (TextUtils.isEmpty(anrRecord)) {
                Rlog.e(LOG_TAG, "[buildAnrString] anrRecord is empty. ");
            } else if (anrRecord.length() > 20) {
                Rlog.e(LOG_TAG,
                        "[buildAnrString] Max length of dailing mNumber is 20,throw exception");
                throw new IccPhoneBookOperationException(
                        IccPhoneBookOperationException.OVER_NUMBER_MAX_LENGTH,
                        "Max length of dialing mNumber is " + MAX_LENTH_ADN);
            } else {
                if(TeleFrameworkFactory.getInstance().isSupportOrange()){
                    anrString[0] = (byte) aasIndex;//aas id
                } else {
                    anrString[0] = (byte) 0x01;
                }
                anrNumber = PhoneNumberUtils.numberToCalledPartyBCD(anrRecord);
                if (anrNumber == null) {
                    return anrString;
                }
                anrString[ADN_BCD_NUMBER_LENGTH + 1] = (byte) (anrNumber.length);
                System.arraycopy(anrNumber, 0, anrString, 2, anrNumber.length);
                if (recordSize > TYPE1_DATA_LENGTH) {
                    anrString[recordSize - 4] = (byte) 0xFF; // Capacility Id
                    anrString[recordSize - 3] = (byte) 0xFF; // Extension Record
                    // Id
                    anrString[recordSize - 2] = (byte) efid; // Adn Sfi
                    anrString[recordSize - 1] = (byte) adnNum; // Adn Record Id
                } else {
                    anrString[recordSize - 2] = (byte) 0xFF; // Capacility Id
                    anrString[recordSize - 1] = (byte) 0xFF; // Extension Record
                    // Id
                }
            }
            return anrString;
        }
    }

    public byte[] buildSneString(int recordSize, int recordSeq, int efid, int adnNum) {
        byte[] byteTag;
        byte[] sneString;
        String sneRecord;
        int footerOffset = recordSize - 2;
        // create an empty record
        sneString = new byte[recordSize];
        for (int i = 0; i < recordSize; i++) {
            sneString[i] = (byte) 0xFF;
        }
        sneString[footerOffset + ADN_SFI] = (byte) efid; // Adn Sfi
        sneString[footerOffset + ADN_REC_ID] = (byte) adnNum; // Adn Record Id

        if (mSne == null) {
            return sneString;
        } else {
            sneRecord = mSne;
        }
        if (!TextUtils.isEmpty(sneRecord)) {
            try {
                byteTag = TeleFrameworkFactory.getInstance().isAsciiStringToGsm8BitUnpackedField(sneRecord);
                System.arraycopy(byteTag, 0, sneString, 0, byteTag.length);
            } catch (EncodeException ex) {
                try {
                    byteTag = sneRecord.getBytes("utf-16be");
                    System.arraycopy(byteTag, 0, sneString, ADN_TON_AND_NPI,byteTag.length);
                    sneString[0] = (byte) 0x80;
                } catch (java.io.UnsupportedEncodingException ex2) {
                    Rlog.e(LOG_TAG,"[AdnRecord]sneRecord convert byte exception");

                } catch (ArrayIndexOutOfBoundsException e) {
                    Rlog.e(LOG_TAG, "over the length of aas");
                    return null;
                }
            } catch (ArrayIndexOutOfBoundsException ex) {
                Rlog.e(LOG_TAG, "over the length of aas");
                return null;
            }
        }
        return sneString;
    }
}