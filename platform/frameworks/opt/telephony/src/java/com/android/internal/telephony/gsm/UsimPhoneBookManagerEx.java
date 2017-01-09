package com.android.internal.telephony.gsm;

import android.util.SparseArray;
import android.util.SparseIntArray;
import android.os.AsyncResult;
import android.os.Handler;
import android.os.Message;
import android.telephony.Rlog;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import android.telephony.PhoneNumberUtils;
import android.text.TextUtils;
import java.util.HashSet;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.Set;

import com.android.internal.telephony.TeleUtils;
import com.android.internal.telephony.gsm.UsimPhoneBookManagerEx.SubjectIndexOfAdn;
import com.android.internal.telephony.uicc.AdnRecord;
import com.android.internal.telephony.uicc.IccFileHandler;
import com.android.internal.telephony.uicc.IccUtils;
import com.android.internal.telephony.uicc.AdnRecordEx;
import com.android.internal.telephony.uicc.AdnRecordCacheEx;
import com.android.internal.telephony.uicc.IccConstantsEx;
import com.android.internal.telephony.TeleFrameworkFactory;

/**
 * {@hide}
 */
public class UsimPhoneBookManagerEx extends UsimPhoneBookManager implements IccConstantsEx {
    private static final String TAG = "UsimPhoneBookManagerEx";
    private static final boolean DBG = true;

    private ArrayList<PbrRecord> mPbrRecords;
    private Boolean mIsPbrPresent;
    private IccFileHandler mFh;
    private AdnRecordCacheEx mAdnCache;
    private Object mLock = new Object();
    private boolean mEmailPresentInIap = false;
    private int mEmailTagNumberInIap = 0;
    // email list for each ADN record. The key would be
    // ADN's efid << 8 + record #
    private SparseArray<ArrayList<String>> mEmailsForAdnRec;
    private ArrayList<String> mAasList;
    private int mSneEfSize = 0;
    private boolean mSnePresentInIap = false;
    private int mSneTagNumberInIap = 0;
    protected int sneRecordSize[] = new int[3];
    private boolean mRefreshCache = false;
    private Object[] mIapFileRecordArray;
    public int[] mAdnRecordSizeArray;
    private int[] mEmailRecordSizeArray;
    private int[] mIapRecordSizeArray;
    private int[] mAnrRecordSizeArray;
    public int mAnrFileCount = 0;
    private int mDoneAdnCount = 0;
    public int mGrpCount = 0;
    private int changedCounter;
    protected int mTotalSize[] = null;
    protected int recordSize[] = new int[3];
    private boolean mAnrPresentInIap = false;
    private boolean mIsPbrFileExisting = true;
    private boolean mIsContainAdnInPbr = true;
    public HashMap<Integer, int[]> mRecordsSize;
    // SFI to ADN Efid mapping table
    private SparseIntArray mSfiEfidTable;
    private static final int INVALID_SFI = -1;
    private static final byte INVALID_BYTE = -1;

    private ArrayList<byte[]> mIapFileRecord;
    private ArrayList<byte[]> mEmailFileRecord;
    private ArrayList<byte[]> mAnrFileRecord;
    private ArrayList<byte[]> mGrpFileRecord;
    private ArrayList<byte[]> mPbcFileRecord;
    private ArrayList<byte[]> mGasFileRecord;
    private ArrayList<byte[]> mAasFileRecord;
    private ArrayList<byte[]> mSneFileRecord;
    private ArrayList<AdnRecordEx> mPhoneBookRecords;
    private ArrayList<String> mGasList;

    private AtomicInteger mPendingIapLoads = new AtomicInteger(0);
    private AtomicInteger mPendingGrpLoads = new AtomicInteger(0);
    private AtomicInteger mPendingAnrLoads = new AtomicInteger(0);
    private AtomicInteger mPendingPbcLoads = new AtomicInteger(0);
    private AtomicBoolean mIsNotify = new AtomicBoolean(false);

    private LinkedList<SubjectIndexOfAdn> mAnrInfoFromPBR = new LinkedList<SubjectIndexOfAdn>();
    private LinkedList<SubjectIndexOfAdn> mEmailInfoFromPBR = new LinkedList<SubjectIndexOfAdn>();
    private LinkedList<SubjectIndexOfAdn> mAasInfoFromPBR = new LinkedList<SubjectIndexOfAdn>();
    private LinkedList<SubjectIndexOfAdn> mSneInfoFromPBR = new LinkedList<SubjectIndexOfAdn>();

    public static final int USIM_SUBJCET_EMAIL = 0;
    public static final int USIM_SUBJCET_ANR = 1;
    public static final int USIM_SUBJCET_GRP = 2;
    public static final int USIM_SUBJCET_AAS = 3;
    public static final int USIM_SUBJCET_SNE = 4;

    public static final int USIM_TYPE1_TAG = 0xA8;
    public static final int USIM_TYPE2_TAG = 0xA9;
    private static final int USIM_TYPE3_TAG = 0xAA;
    private static final int USIM_EFADN_TAG = 0xC0;
    private static final int USIM_EFIAP_TAG = 0xC1;
    private static final int USIM_EFEXT1_TAG = 0xC2;
    public static final int USIM_EFSNE_TAG = 0xC3;
    public static final int USIM_EFANR_TAG = 0xC4;
    private static final int USIM_EFPBC_TAG = 0xC5;
    public static final int USIM_EFGRP_TAG = 0xC6;
    public static final int USIM_EFAAS_TAG = 0xC7;
    public static final int USIM_EFGAS_TAG = 0xC8;
    private static final int USIM_EFUID_TAG = 0xC9;
    public static final int USIM_EFEMAIL_TAG = 0xCA;
    private static final int USIM_EFCCP1_TAG = 0xCB;

    private static final int EVENT_PBR_LOAD_DONE = 1;
    private static final int EVENT_USIM_ADN_LOAD_DONE = 2;
    private static final int EVENT_IAP_LOAD_DONE = 3;
    private static final int EVENT_EMAIL_LOAD_DONE = 4;
    private static final int EVENT_ADN_RECORD_COUNT = 5;
    private static final int EVENT_AAS_LOAD_DONE = 6;
    private static final int EVENT_SNE_LOAD_DONE = 7;
    private static final int EVENT_GRP_LOAD_DONE = 8;
    private static final int EVENT_GAS_LOAD_DONE = 9;
    private static final int EVENT_ANR_LOAD_DONE = 10;
    private static final int EVENT_PBC_LOAD_DONE = 11;
    public static final int EVENT_EF_CC_LOAD_DONE = 12;
    private static final int EVENT_UPDATE_RECORD_DONE = 13;
    private static final int EVENT_LOAD_EF_PBC_RECORD_DONE = 14;
    private static final int EVENT_EF_PUID_LOAD_DONE = 15;
    private static final int EVENT_UPDATE_UID_DONE = 16;
    private static final int EVENT_EF_PSC_LOAD_DONE = 17;
    private static final int EVENT_UPDATE_CC_DONE = 18;
    private static final int EVENT_GET_RECORDS_COUNT = 19;
    private static final int EVENT_ANR_RECORD_LOAD_DONE = 20;
    private static final int EVENT_SNE_RECORD_COUNT = 21;

    // class File represent a PBR record TLV object which points to the rest of the phonebook EFs
    private class File {
        // Phonebook reference file constructed tag defined in 3GPP TS 31.102
        // section 4.4.2.1 table 4.1
        private final int mParentTag;
        // EFID of the file
        private final int mEfid;
        // SFI (Short File Identification) of the file. 0xFF indicates invalid SFI.
        private final int mSfi;
        // The order of this tag showing in the PBR record.
        private final int mIndex;

        File(int parentTag, int efid, int sfi, int index) {
            mParentTag = parentTag;
            mEfid = efid;
            mSfi = sfi;
            mIndex = index;
        }

        public int getParentTag() { return mParentTag; }
        public int getEfid() { return mEfid; }
        public int getSfi() { return mSfi; }
        public int getIndex() { return mIndex; }
    }

    public class SubjectIndexOfAdn {
        public int adnEfid;
        public int[] type;
        // <efid, record >
        public Map<Integer, Integer> recordNumInIap;
        // map <efid,ArrayList<byte[]>fileRecord >
        public Map<Integer, ArrayList<byte[]>> record;
        public int[] efids;
        // ArrayList<int[]> usedNumSet;
        Object[] usedSet;

    };

    public UsimPhoneBookManagerEx(IccFileHandler fh, AdnRecordCacheEx cache) {
        super(fh, cache);
        mFh = fh;
        mPhoneBookRecords = new ArrayList<AdnRecordEx>();
        mPbrRecords = null;
        // We assume its present, after the first read this is updated.
        // So we don't have to read from UICC if its not present on subsequent
        // reads.
        mIsPbrPresent = true;
        mAdnCache = cache;
        mGasList = new ArrayList<String>();
        mTotalSize = null;
        mAasList = new ArrayList<String>();
        mEmailsForAdnRec = new SparseArray<ArrayList<String>>();
        mSfiEfidTable = new SparseIntArray();
    }

    public void reset() {
        mPhoneBookRecords.clear();
        mGasList.clear();
        mIapFileRecord = null;
        mEmailFileRecord = null;
        mPbrRecords = null;
        mIsPbrPresent = true;
        mRefreshCache = false;
        mAnrFileRecord = null;
        mGrpFileRecord = null;
        mGasFileRecord = null;
        mPbcFileRecord = null;
        mTotalSize = null;
        mAnrFileCount = 0;
        mAnrInfoFromPBR = null;
        mEmailInfoFromPBR = null;
        mAasList.clear();
        mEmailsForAdnRec.clear();
        mSfiEfidTable.clear();
    }

    public ArrayList<AdnRecordEx> loadEfFilesFromUsimEx() {
        synchronized (mLock) {
            if (!mPhoneBookRecords.isEmpty()) {
                return mPhoneBookRecords;
            }

            if (!mIsPbrPresent)
                return null;

            // Check if the PBR file is present in the cache, if not read it
            // from the USIM.
            log("loadEfFilesFromUsim, mPbrRecords:" + mPbrRecords);
            if (mPbrRecords == null) {
                readPbrFileAndWait();
            }

            if (mPbrRecords == null) {
                mIsPbrFileExisting = false;
                log("mIsPbrFileExisting = false");
                return null;
            }

            int numRecs = mPbrRecords.size();

            mIapFileRecordArray = new Object[numRecs];
            mAdnRecordSizeArray = new int[numRecs];
            mEmailRecordSizeArray = new int[numRecs];
            mIapRecordSizeArray = new int[numRecs];
            mAnrRecordSizeArray = new int[numRecs];

            boolean isGotAdnSize = true;
            mDoneAdnCount = 0;
            log("loadEfFilesFromUsim, numRecs:" + numRecs);
            log("loadEfFilesFromUsim, mTotalSize:" + mTotalSize);
            if (mTotalSize == null) {
                mTotalSize = new int[3];
                isGotAdnSize = false;
            }

            for (int i = 0; i < numRecs; i++) {
                log("loadEfFilesFromUsim, the current record num is:" + i);
                readAdnFileAndWait(i);
                if (!isGotAdnSize) {
                    int[] size = readAdnFileSizeAndWait(i);

                    if (size != null) {
                        mTotalSize[0] = size[0];
                        mTotalSize[1] += size[1];
                        mTotalSize[2] += size[2];
                        log("mTotalSize[0]" + mTotalSize[0] + "mTotalSize[1]" + mTotalSize[1] + "mTotalSize[2]" + mTotalSize[2]);
                    }
                }
                readIapFile(i);
                readEmailFileAndWait(i);
                if (TeleFrameworkFactory.getInstance().isSupportOrange()) {
                    readSneFileAndWait(i);
                }
                readAnrFileAndWait(i);
                readGrpFileAndWait(i, mTotalSize[2]);
                updateAdnRecord(i);
            }

            CheckRepeatType2Ef();
            // All EF files are loaded, post the response.
            // update the adn recordNumber
            updateAdnRecordNum();
            updatePbcAndCc();
        }
        return mPhoneBookRecords;
    }

    private void readPbrFileAndWait() {
        mFh.loadEFLinearFixedAll(EF_PBR, obtainMessage(EVENT_PBR_LOAD_DONE));
        try {
            mLock.wait();
        } catch (InterruptedException e) {
            loge("Interrupted Exception in readAdnFileAndWait");
        }
    }

    private void readEmailFileAndWait(int recNum) {
        log("readEmailFileAndWait");

        synchronized (mLock) {
            if (mPbrRecords == null) {
                readPbrFileAndWait();
            }
        }

        if (mPbrRecords == null) {
            loge("Error: Pbr file is empty");
            return;
        }

        SparseArray<File> files;
        files = mPbrRecords.get(recNum).mFileIds;
        if (files == null || files.size() == 0)
            return;

        File email = files.get(USIM_EFEMAIL_TAG);
        if (email != null) {
            int emailEfid = email.getEfid();
            // Check if the EFEmail is a Type 1 file or a type 2 file.
            // If mEmailPresentInIap is true, its a type 2 file.
            // So we read the IAP file and then read the email records.
            // instead of reading directly.
            SubjectIndexOfAdn records = getSubjectIndex(USIM_SUBJCET_EMAIL, recNum);
            if (records == null) {
                log("readEmailFileAndWait  records == null ");
                return;
            }

            if (mEmailPresentInIap) {
                if (mIapFileRecord == null) {
                    loge("Error: IAP file is empty");
                    records = null;
                    setSubjectIndex(USIM_SUBJCET_EMAIL, recNum, records);
                    return;
                }
            }

            /**
             * Make sure this EF_EMAIL was never read earlier. Sometimes two PBR record points
             */
            // to the same EF_EMAIL
            for (int i = 0; i < recNum; i++) {
                if (mPbrRecords.get(i) != null) {
                    SparseArray<File> previousFileIds = mPbrRecords.get(i).mFileIds;
                    if (previousFileIds != null) {
                        File id = previousFileIds.get(USIM_EFEMAIL_TAG);
                        if (id != null && id.getEfid() == emailEfid) {
                            log("Skipped this EF_EMAIL which was loaded earlier");
                            return;
                        }
                    }
                }
            }
            // Read the EFEmail file.
            mFh.loadEFLinearFixedAll(emailEfid, obtainMessage(EVENT_EMAIL_LOAD_DONE));

            try {
                mLock.wait();
            } catch (InterruptedException e) {
                loge("Interrupted Exception in readEmailFileAndWait");
            }

            if (mEmailFileRecord == null) {
                loge("Error: Email file is empty");
                records = null;
                setSubjectIndex(USIM_SUBJCET_EMAIL, recNum, records);
                return;
            }


            records.record = new HashMap<Integer, ArrayList<byte[]>>();
            records.record.put(emailEfid, mEmailFileRecord);
            log("readEmailFileAndWait recNum " + recNum
                    + "  mEmailFileRecord  size " + mEmailFileRecord.size());
            setSubjectIndex(USIM_SUBJCET_EMAIL, recNum, records);
            setSubjectUsedNum(USIM_SUBJCET_EMAIL, recNum);

            // Build email list
            if (email.getParentTag() == USIM_TYPE2_TAG && mIapFileRecord != null) {
                // If the tag is type 2 and EF_IAP exists, we need to build tpe 2 email list
                buildType2EmailList(recNum, getSubjectIndex(USIM_SUBJCET_EMAIL, recNum), emailEfid);
            }
            else {
                // If one the followings is true, we build type 1 email list
                // 1. EF_IAP does not exist or it is failed to load
                // 2. ICC cards can be made such that they have an IAP file but all
                //    records are empty. In that case buildType2EmailList will fail and
                //    we need to build type 1 email list.

                // Build type 1 email list
                buildType1EmailList(recNum);
            }
            mEmailFileRecord = null;
        }
    }

    // Build type 1 email list
    private void buildType1EmailList(int recNum) {
        /**
         * If this is type 1, the number of records in EF_EMAIL would be same as the record number
         * in the master/reference file.
         */
        if (mPbrRecords.get(recNum) == null)
            return;

        int numRecs = mPbrRecords.get(recNum).mMasterFileRecordNum;
        log("Building type 1 email list. recNum = "
                + recNum + ", numRecs = " + numRecs);

        byte[] emailRec;
        for (int i = 0; i < numRecs; i++) {
            try {
                emailRec = mEmailFileRecord.get(i);
            } catch (IndexOutOfBoundsException e) {
                loge("Error: Improper ICC card: No email record for ADN, continuing");
                break;
            }

            /**
             *  3GPP TS 31.102 4.4.2.13 EF_EMAIL (e-mail address)
             *
             *  The fields below are mandatory if and only if the file
             *  is not type 1 (as specified in EF_PBR)
             *
             *  Byte [X + 1]: ADN file SFI (Short File Identification)
             *  Byte [X + 2]: ADN file Record Identifier
             */
            int sfi = emailRec[emailRec.length - 2];
            int adnRecId = emailRec[emailRec.length - 1];

            String email = readEmailRecord(i);

            if (email == null || email.equals("")) {
                continue;
            }

            // Get the associated ADN's efid first.
            int adnEfid = 0;
            if (sfi == INVALID_SFI || mSfiEfidTable.get(sfi) == 0) {

                // If SFI is invalid or cannot be mapped to any ADN, use the ADN's efid
                // in the same PBR files.
                File file = mPbrRecords.get(recNum).mFileIds.get(USIM_EFADN_TAG);
                if (file == null)
                    continue;
                adnEfid = file.getEfid();
            } else {
                adnEfid = mSfiEfidTable.get(sfi);
            }
            /**
             * SIM record numbers are 1 based.
             * The key is constructed by efid and record index.
             */
            int index = (((adnEfid & 0xFFFF) << 8) | ((adnRecId - 1) & 0xFF));
            ArrayList<String> emailList = mEmailsForAdnRec.get(index);
            if (emailList == null) {
                emailList = new ArrayList<String>();
            }
            log("Adding email #" + i + " list to index 0x" +
                    Integer.toHexString(index).toUpperCase());
            emailList.add(email);
            mEmailsForAdnRec.put(index, emailList);
        }
    }

    // Build type 2 email list
    private boolean buildType2EmailList(int recNum, SubjectIndexOfAdn emailInfo, int efid) {

        if (mPbrRecords.get(recNum) == null)
            return false;

        int numRecs = mPbrRecords.get(recNum).mMasterFileRecordNum;
        log("Building type 2 email list. recNum = "
                + recNum + ", numRecs = " + numRecs);

        /**
         * 3GPP TS 31.102 4.4.2.1 EF_PBR (Phone Book Reference file) table 4.1

         * The number of records in the IAP file is same as the number of records in the master
         * file (e.g EF_ADN). The order of the pointers in an EF_IAP shall be the same as the
         * order of file IDs that appear in the TLV object indicated by Tag 'A9' in the
         * reference file record (e.g value of mEmailTagNumberInIap)
         */

        File adnFile = mPbrRecords.get(recNum).mFileIds.get(USIM_EFADN_TAG);
        if (adnFile == null) {
            loge("Error: Improper ICC card: EF_ADN does not exist in PBR files");
            return false;
        }
        int adnEfid = adnFile.getEfid();
        mIapFileRecordArray[recNum] = mIapFileRecord;

        for (int i = 0; i < numRecs; i++) {
            if (emailInfo != null){

                if (emailInfo.efids == null || emailInfo.efids.length == 0) {
                    log("getEmail emailInfo.efids == null ||emailInfo.efids.length == 0 ");
                    continue;
                }

                byte[] record;
                int emailRecId;
                try {
                    record = mIapFileRecord.get(i);
                    emailRecId =
                        record[mPbrRecords.get(recNum).mFileIds.get(USIM_EFEMAIL_TAG).getIndex()];
                } catch (IndexOutOfBoundsException e) {
                     loge("Error: Improper ICC card: Corrupted EF_IAP");
                     continue;
                }

                int emailIndex = -1;
                emailIndex = getUsedNumSetIndex(efid, emailInfo);
                if(emailIndex == -1){
                    continue;
                }
                mEmailTagNumberInIap = emailInfo.recordNumInIap.get(efid);
                mEmailFileRecord = emailInfo.record.get(efid);

                if (mEmailFileRecord == null) {
                    continue;
                }

                int recId = 0xFF;
                try {
                    recId = (int) (record[mEmailTagNumberInIap] & 0xFF);
                } catch (ArrayIndexOutOfBoundsException ex) {
                    log("ex :" + ex);
                }

                recId = ((recId == 0xFF) ? (-1) : recId);
                log("getType2Email  iap recNum == " + recId);

                String email = readEmailRecord(emailRecId - 1);

                if(recId != -1){
                    log("getType2Email,emails " + email);
                    if (TextUtils.isEmpty(email)) {
                        log("getType2Email,emails ==null");
                        setIapFileRecord(recNum, i, (byte) 0xFF, mEmailTagNumberInIap);
                        continue;
                    }

                   Set<Integer> set = (Set<Integer>) emailInfo.usedSet[emailIndex];
                   log("getType2Email  size (0)" + set.size() + " emailIndex " + emailIndex);
                   set.add(new Integer(recId));
                   emailInfo.usedSet[emailIndex] = set;
                   log("getType2Email  size (1)" + set.size());
                   setSubjectIndex(USIM_SUBJCET_EMAIL, recNum, emailInfo);
                }

                if (email != null && !email.equals("")) {
                    // The key is constructed by efid and record index.
                    int index = (((adnEfid & 0xFFFF) << 8) | (i & 0xFF));
                    ArrayList<String> emailList = mEmailsForAdnRec.get(index);
                    if (emailList == null) {
                        emailList = new ArrayList<String>();
                    }
                   emailList.add(email);
                   log("Adding email list to index 0x" +
                               Integer.toHexString(index).toUpperCase());
                   mEmailsForAdnRec.put(index, emailList);
               }
            }
        }
        return true;
    }

    private void readIapFileAndWait(int efid) {
        mFh.loadEFLinearFixedAll(efid, obtainMessage(EVENT_IAP_LOAD_DONE));
        try {
            mLock.wait();
        } catch (InterruptedException e) {
            loge("Interrupted Exception in readIapFileAndWait");
        }
    }

    private String readEmailRecord(int recNum) {
        byte[] emailRec = null;
        try {
            emailRec = mEmailFileRecord.get(recNum);
        } catch (IndexOutOfBoundsException e) {
            return null;
        }

        // The length of the record is X+2 byte, where X bytes is the email
        // address
        String email = IccUtils.adnStringFieldToString(emailRec, 0, emailRec.length - 2);
        return email;
    }

    // Read EF_ADN file
    private int[] readAdnFileAndWait(int recNum) {
        log("readAdnFileAndWait");
        synchronized (mLock) {
            if (mPbrRecords == null) {
                readPbrFileAndWait();
            }
        }
        if (mPbrRecords == null) {
            loge("Error: Pbr file is empty");
            mIsPbrFileExisting = false;
            log("mIsPbrFileExisting = false");
            return null;
        }

        SparseArray<File> files;
        files = mPbrRecords.get(recNum).mFileIds;
        if (files == null || files.size() == 0)
            return null;

        int extEf = 0;
        // Only call fileIds.get while EFEXT1_TAG is available
        if (files.get(USIM_EFEXT1_TAG) != null) {
            extEf = files.get(USIM_EFEXT1_TAG).getEfid();
        }

        if(files.get(USIM_EFADN_TAG) == null)
            return null;

        log("readAdnFileAndWait:efid = " + files.get(USIM_EFADN_TAG).getEfid() + "extEf =" + extEf);
        int previousSize = mPhoneBookRecords.size();
        mAdnCache.requestLoadAllAdnLike(files.get(USIM_EFADN_TAG).getEfid(), extEf,
                obtainMessage(EVENT_USIM_ADN_LOAD_DONE));
        try {
            mLock.wait();
        } catch (InterruptedException e) {
            loge("Interrupted Exception in readAdnFileAndWait");
        }
        /**
         * The recent added ADN record # would be the reference record size
         * for the rest of EFs associated within this PBR.
         */
        mPbrRecords.get(recNum).mMasterFileRecordNum = mPhoneBookRecords.size() - previousSize;
        return recordSize;
    }

    private void createPbrFile(ArrayList<byte[]> records) {
        if (records == null) {
            mPbrRecords = null;
            mIsPbrPresent = false;
            return;
        }

        mPbrRecords = new ArrayList<PbrRecord>();
        for (int i = 0; i < records.size(); i++) {
            // Some cards have two records but the 2nd record is filled with all invalid char 0xff.
            // So we need to check if the record is valid or not before adding into the PBR records.
            if (records.get(i)[0] != INVALID_BYTE) {
                mPbrRecords.add(new PbrRecord(records.get(i)));
            }
        }

        for (PbrRecord record : mPbrRecords) {
            File file = record.mFileIds.get(USIM_EFADN_TAG);
            // If the file does not contain EF_ADN, we'll just skip it.
            if (file != null) {
                int sfi = file.getSfi();
                if (sfi != INVALID_SFI) {
                    mSfiEfidTable.put(sfi, record.mFileIds.get(USIM_EFADN_TAG).getEfid());
                }
            }
        }
    }

    @Override
    public void handleMessage(Message msg) {
        AsyncResult ar;
        byte data[];
        switch (msg.what) {
        case EVENT_PBR_LOAD_DONE:
            ar = (AsyncResult) msg.obj;
            if (ar.exception == null) {
                createPbrFile((ArrayList<byte[]>) ar.result);
            }
            synchronized (mLock) {
                mLock.notify();
            }
            break;
        case EVENT_USIM_ADN_LOAD_DONE:
            log("Loading USIM ADN records done");
            ar = (AsyncResult) msg.obj;
            // add by liguxiang 10-24-11 for NEWMS00132125 begin
            int size = 0;
            if ((ar != null) && ((ArrayList<AdnRecordEx>) ar.result != null)) {
                size = ((ArrayList<AdnRecordEx>) ar.result).size();
            }
            log("EVENT_USIM_ADN_LOAD_DONE size " + size);
            if ((ar != null) && ar.exception == null) {
                mPhoneBookRecords.addAll((ArrayList<AdnRecordEx>) ar.result);
            } else if (ar == null) {
                log("EVENT_USIM_ADN_LOAD_DONE exception ar is null");
            } else {
                log("EVENT_USIM_ADN_LOAD_DONE exception " + ar.exception);
            }
            synchronized (mLock) {
                mLock.notify();
            }
            break;
        case EVENT_IAP_LOAD_DONE:
            ar = (AsyncResult) msg.obj;
            int index = msg.arg1;
            if (ar.exception == null) {
                if (mIapFileRecord == null) {
                    mIapFileRecord = ((ArrayList<byte[]>) ar.result);
                }
                log("Loading USIM IAP record done " + index);
                mIapFileRecord.set(index, (byte[]) ar.result);
            }
            mPendingIapLoads.decrementAndGet();
            if (mPendingIapLoads.get() == 0) {
                if (mIsNotify.get()) {
                    mIsNotify.set(false);
                    synchronized (mLock) {
                        mLock.notify();
                    }
                    log("Loading USIM IAP records done notify");
                }
            }
            break;
        case EVENT_EMAIL_LOAD_DONE:
            log("Loading USIM Email records done");
            ar = (AsyncResult) msg.obj;
            if (ar.exception == null) {
                mEmailFileRecord = ((ArrayList<byte[]>) ar.result);
                mEmailFileRecord.addAll((ArrayList<byte[]>) ar.result);
                log("Loading USIM Email records done size "
                        + mEmailFileRecord.size());
            }

            synchronized (mLock) {
                mLock.notify();
            }
            break;
        case EVENT_ADN_RECORD_COUNT:
            log("Loading EVENT_ADN_RECORD_COUNT");
            ar = (AsyncResult) msg.obj;
            synchronized (mLock) {
                if (ar.exception == null) {
                    recordSize = (int[]) ar.result;
                    mIsContainAdnInPbr = true;
                    // recordSize[0] is the record length
                    // recordSize[1] is the total length of the EF file
                    // recordSize[2] is the number of records in the EF file
                    log("EVENT_ADN_RECORD_COUNT Size "
                            + recordSize[0] + " total " + recordSize[1]
                            + " #record " + recordSize[2]);
                } else {
                    Rlog.d(TAG, "Cannot load ADN records size in Pbr");
                    recordSize = null;
                    mIsContainAdnInPbr = false;
                }
                mLock.notify();
            }
            break;
        case EVENT_GET_RECORDS_COUNT:
            log("Loading EVENT_GET_RECORDS_COUNT");
            ar = (AsyncResult) msg.obj;
            synchronized (mLock) {
                if (ar.exception == null) {
                    if (mRecordsSize == null) {
                        mRecordsSize = new HashMap<Integer, int[]>();
                    }
                    mRecordsSize.put(msg.arg1, (int[]) ar.result);
                }
                mLock.notify();
            }
            break;
        case EVENT_ANR_RECORD_LOAD_DONE:
            ar = (AsyncResult) msg.obj;
            if (ar.exception == null) {
                if (mAnrFileRecord == null) {
                    mAnrFileRecord = new ArrayList<byte[]>();
                }
                log("Loading USIM ANR record done " + msg.arg1);
                if (msg.arg1 < mAnrFileRecord.size()) {
                    mAnrFileRecord.set(msg.arg1, (byte[]) ar.result);
                }
            }
            mPendingAnrLoads.decrementAndGet();
            if (mPendingAnrLoads.get() == 0) {
                if (mIsNotify.get()) {
                    mIsNotify.set(false);
                    synchronized (mLock) {
                        mLock.notify();
                    }
                    log("Loading USIM ANR records done notify");
                }
            }
            break;
        case EVENT_ANR_LOAD_DONE:
            log("Loading USIM ANR records done");
            ar = (AsyncResult) msg.obj;
            if (ar.exception == null) {
                if (mAnrFileRecord == null) {
                    mAnrFileRecord = new ArrayList<byte[]>();
                }
                mAnrFileRecord.addAll((ArrayList<byte[]>) ar.result);
                log("mAnrFileRecord.size() is " + mAnrFileRecord.size());
            }
            synchronized (mLock) {
                mLock.notify();
            }
            break;
        case EVENT_GRP_LOAD_DONE:
            ar = (AsyncResult) msg.obj;
            if (ar.exception == null) {
                if (mGrpFileRecord == null) {
                    mGrpFileRecord = new ArrayList<byte[]>();
                }
                int i = msg.arg1 + msg.arg2;
                log("Loading USIM Grp record done " + i);
                try {
                    mGrpFileRecord.set(i, (byte[]) ar.result);
                } catch (IndexOutOfBoundsException exc) {
                    log("IndexOutOfBoundsException readGrpFileAndWait i=" + i);
                }
            }
            mPendingGrpLoads.decrementAndGet();
            if (mPendingGrpLoads.get() == 0) {
                if (mIsNotify.get()) {
                    mIsNotify.set(false);
                    synchronized (mLock) {
                        mLock.notify();
                    }
                    log("Loading USIM Grp records done notify");
                }
            }
            break;
        case EVENT_GAS_LOAD_DONE:
            log("Loading USIM Gas records done, mGasFileRecord:"
                    + mGasFileRecord);
            ar = (AsyncResult) msg.obj;
            if (ar.exception == null) {
                mGasFileRecord = new ArrayList<byte[]>();
                mGasFileRecord.addAll((ArrayList<byte[]>) ar.result);
                log("Loading USIM Gas records done size "
                        + mGasFileRecord.size() + ", mGasFileRecord:"
                        + mGasFileRecord);
            }

            synchronized (mLock) {
                mLock.notify();
            }
            break;
        case EVENT_LOAD_EF_PBC_RECORD_DONE:
            log("Loading EVENT_LOAD_EF_PBC_RECORD_DONE");
            ar = (AsyncResult) (msg.obj);
            if (ar.exception == null) {
                if (mPbcFileRecord == null) {
                    mPbcFileRecord = new ArrayList<byte[]>();
                }
                mPbcFileRecord.set(msg.arg1, (byte[]) ar.result);
                log("Loading USIM PBC record done" + msg.arg1);
            } else {
                log("Loading USIM PBC records failed" + msg.arg1);
            }
            mPendingPbcLoads.decrementAndGet();
            if (mPendingPbcLoads.get() == 0) {
                if (mIsNotify.get()) {
                    mIsNotify.set(false);
                    synchronized (mLock) {
                        mLock.notify();
                    }
                    log("Loading USIM Pbc records done notify");
                }
            }
            break;
        case EVENT_EF_CC_LOAD_DONE:
            ar = (AsyncResult) (msg.obj);
            data = (byte[]) (ar.result);
            int temp = msg.arg1;
            int isUpdateUid = msg.arg2;
            if (ar.exception != null) {
                log("EVENT_EF_CC_LOAD_DONE has exception "
                        + ar.exception);
                break;
            }
            log("EVENT_EF_CC_LOAD_DONE data " + IccUtils.bytesToHexString(data));
            if (data == null) {
                log("EVENT_EF_CC_LOAD_DONE data is null");
                break;
            }
            // update EFcc
            byte[] counter = new byte[2];
            int cc = ((data[0] << 8) & 0xFF00) | (data[1] & 0xFF);
            changedCounter = cc;
            cc += temp;
            if (cc > 0xFFFF) {
                counter[0] = (byte) 0x00;
                counter[1] = (byte) 0x01;
            } else {
                counter[1] = (byte) (cc & 0xFF);
                counter[0] = (byte) (cc >> 8 & 0xFF);
            }
            log("EVENT_EF_CC_LOAD_DONE counter " + IccUtils.bytesToHexString(counter));
            if (isUpdateUid == 1) {
                mFh.updateEFTransparent(IccConstantsEx.EF_CC, counter,
                        obtainMessage(EVENT_UPDATE_CC_DONE));
            } else {
                mFh.updateEFTransparent(IccConstantsEx.EF_CC, counter,
                        obtainMessage(EVENT_UPDATE_RECORD_DONE));
            }
            break;
        case EVENT_EF_PUID_LOAD_DONE:
            ar = (AsyncResult) (msg.obj);
            data = (byte[]) (ar.result);
            int recNum = msg.arg1;
            int recordNum = msg.arg2;

            if (ar.exception != null) {
                log("EVENT_EF_PUID_LOAD_DONE has exception " + ar.exception);
                break;
            }
            // update EFuid and EFpuid
            byte[] uidData = new byte[2];
            byte[] newPuid = new byte[2];
            int puid = ((data[0] << 8) & 0xFF00) | (data[1] & 0xFF);
            if (changedCounter == 0xFFFF || puid == 0xFFFF) {
                // update psc
                synchronized (mLock) {
                    mFh.loadEFTransparent(IccConstantsEx.EF_PSC, 4,
                            obtainMessage(EVENT_EF_PSC_LOAD_DONE));
                }
                // regenerate uids
                int uid = 0;
                int[] adnRecordSizeArray = getAdnRecordSizeArray();
                for (int num = 0; num < getNumRecs(); num++) {
                    for (int i = 0; i < adnRecordSizeArray[num]; i++) {
                        int adnRecNum = i;
                        for (int j = 0; j < num; j++) {
                            adnRecNum += adnRecordSizeArray[j];
                        }
                        if (!TextUtils.isEmpty(mPhoneBookRecords.get(adnRecNum).getAlphaTag())
                                || !TextUtils.isEmpty(mPhoneBookRecords.get(adnRecNum).getNumber())) {
                            uid++;
                            uidData = TeleUtils.intToBytes(uid, 2);
                            synchronized (mLock) {

                            mFh.updateEFLinearFixed(getEfIdByTag(num, USIM_EFUID_TAG), i + 1, uidData,
                                        null, obtainMessage(EVENT_UPDATE_UID_DONE, -1));
                            }
                        }
                    }
                }
                // update puid
                newPuid = TeleUtils.intToBytes(uid, 2);
                log("update puid " + uid);
                mFh.updateEFTransparent(IccConstantsEx.EF_PUID, newPuid,
                        obtainMessage(EVENT_UPDATE_RECORD_DONE));
            } else {
                uidData[1] = (byte) ((puid + 1) & 0xFF);
                uidData[0] = (byte) ((puid + 1) >> 8 & 0xFF);
                newPuid[0] = uidData[0];
                newPuid[1] = uidData[1];
                log("updateEFPuid newPuid " + IccUtils.bytesToHexString(newPuid));
                synchronized (mLock) {
                    // update uid
                mFh.updateEFLinearFixed(getEfIdByTag(recNum, USIM_EFUID_TAG), recordNum,
                            uidData, null, obtainMessage(EVENT_UPDATE_UID_DONE, puid + 1));
                }
            }
            break;
        case EVENT_UPDATE_UID_DONE:
            // update puid
            ar = (AsyncResult) (msg.obj);
            int puid1 = (Integer) (ar.userObj);
            log("EVENT_UPDATE_UID_DONE");
            if (puid1 != -1) {
                log("EVENT_UPDATE_UID_DONE newPuid " + puid1);
                mFh.updateEFTransparent(IccConstantsEx.EF_PUID,
                        TeleUtils.intToBytes(puid1, 2), obtainMessage(EVENT_UPDATE_RECORD_DONE));
            }
            break;
        case EVENT_UPDATE_CC_DONE:
            mFh.loadEFTransparent(IccConstantsEx.EF_PUID, 2,
                    obtainMessage(EVENT_EF_PUID_LOAD_DONE));
            break;
        case EVENT_EF_PSC_LOAD_DONE:
            ar = (AsyncResult) (msg.obj);
            data = (byte[]) (ar.result);
            if (ar.exception != null) {
                log("EVENT_EF_PSC_LOAD_DONE has exception " + ar.exception);
                break;
            }
            log("EVENT_EF_PSC_LOAD_DONE data " + IccUtils.bytesToHexString(data));
            byte[] psc = new byte[4];
            int oldPsc = TeleUtils.bytesToInt(data);
            if (oldPsc != -1) {
                if (oldPsc == 0xFFFFFFFF) {
                    psc = TeleUtils.intToBytes(1, 4);
                } else {
                    psc = TeleUtils.intToBytes(oldPsc + 1, 4);
                }
            }
            log("update psc data " + IccUtils.bytesToHexString(psc));
            mFh.updateEFTransparent(IccConstantsEx.EF_PSC, psc,
                    obtainMessage(EVENT_UPDATE_RECORD_DONE));
            break;
        case EVENT_UPDATE_RECORD_DONE:
            ar = (AsyncResult) (msg.obj);
            if (ar.exception != null) {
                throw new RuntimeException("update EF records failed",
                        ar.exception);
            }
            log("update_record_success");
            break;
        case EVENT_AAS_LOAD_DONE:
            log("Loading USIM AAS records done");
            ar = (AsyncResult) msg.obj;
            if (ar.exception == null) {
                if (mAasFileRecord == null) {
                    mAasFileRecord = new ArrayList<byte[]>();
                }
                mAasFileRecord.clear();
                mAasFileRecord.addAll((ArrayList<byte[]>) ar.result);
                log("mAasFileRecord.size() is " + mAasFileRecord.size());
            }
            synchronized (mLock) {
                mLock.notify();
            }
            break;
        case EVENT_SNE_LOAD_DONE:
            log("Loading USIM SNE records done");
            ar = (AsyncResult) msg.obj;
            if (ar.exception == null) {
                if (mSneFileRecord == null) {
                    mSneFileRecord = new ArrayList<byte[]>();
                }
                mSneFileRecord.addAll((ArrayList<byte[]>) ar.result);
                log("mSneFileRecord.size() is " + mSneFileRecord.size());
            }

            synchronized (mLock) {
                mLock.notify();
            }
            break;
        case EVENT_SNE_RECORD_COUNT:
            ar = (AsyncResult) msg.obj;
            synchronized (mLock) {
                if (ar.exception == null) {
                    sneRecordSize = (int[]) ar.result;
                    // recordSize[0] is the record length
                    // recordSize[1] is the total length of the EF file
                    // recordSize[2] is the number of records in the EF file
                    log("EVENT_SNE_RECORD_COUNT Size "
                            + sneRecordSize[0] + " total " + sneRecordSize[1]
                            + " #record " + sneRecordSize[2]);
                }
                mLock.notify();
            }
            break;
        }
    }

    private class PbrRecord {
        // TLV tags
        private SparseArray<File> mFileIds;
        /**
         * 3GPP TS 31.102 4.4.2.1 EF_PBR (Phone Book Reference file)
         * If this is type 1 files, files that contain as many records as the
         * reference/master file (EF_ADN, EF_ADN1) and are linked on record number
         * bases (Rec1 -> Rec1). The master file record number is the reference.
         */
        private int mMasterFileRecordNum;

        PbrRecord(byte[] record) {
            mFileIds = new SparseArray<File>();
            SimTlv recTlv;
            log("before making TLVs, data is "
                        + IccUtils.bytesToHexString(record));
            if (IccUtils.bytesToHexString(record).startsWith("ffff")) {
                 return;
            }
            recTlv = new SimTlv(record, 0, record.length);
            parseTag(recTlv);
        }

        void parseTag(SimTlv tlv) {
            SimTlv tlvEfSfi;
            int tag;
            byte[] data;
            ArrayList<Integer> emailEfs = new ArrayList<Integer>();
            ArrayList<Integer> anrEfs = new ArrayList<Integer>();
            ArrayList<Integer> emailType = new ArrayList<Integer>();
            ArrayList<Integer> anrType = new ArrayList<Integer>();
            int i = 0;
            Map<Integer, Integer> val = new HashMap<Integer, Integer>();
            SubjectIndexOfAdn emailInfo = new SubjectIndexOfAdn();
            emailInfo.recordNumInIap = new HashMap<Integer, Integer>();
            SubjectIndexOfAdn anrInfo = new SubjectIndexOfAdn();
            anrInfo.recordNumInIap = new HashMap<Integer, Integer>();
            ArrayList<Integer> aasEfs = new ArrayList<Integer>();
            ArrayList<Integer> aasType = new ArrayList<Integer>();
            SubjectIndexOfAdn aasInfo = new SubjectIndexOfAdn();
            aasInfo.recordNumInIap = new HashMap<Integer, Integer>();
            ArrayList<Integer> sneEfs = new ArrayList<Integer>();
            ArrayList<Integer> sneType = new ArrayList<Integer>();
            SubjectIndexOfAdn sneInfo = new SubjectIndexOfAdn();
            sneInfo.recordNumInIap = new HashMap<Integer, Integer>();
            do {
                tag = tlv.getTag();
                switch (tag) {
                case USIM_TYPE1_TAG: // A8
                case USIM_TYPE3_TAG: // AA
                case USIM_TYPE2_TAG: // A9
                    data = tlv.getData();
                    tlvEfSfi = new SimTlv(data, 0, data.length);
                    parseEfAndSFI(tlvEfSfi, tag, emailInfo, anrInfo, emailEfs,
                            anrEfs, emailType, anrType, aasInfo, aasEfs,
                            aasType, sneInfo, sneEfs, sneType);
                    break;
                }
            } while (tlv.nextObject());
            if (emailEfs.size() > 0) {

                emailInfo.efids = new int[emailEfs.size()];
                emailInfo.type = new int[emailEfs.size()];

                for (i = 0; i < emailEfs.size(); i++) {

                    emailInfo.efids[i] = emailEfs.get(i);
                    emailInfo.type[i] = emailType.get(i);
                }
                log("parseTag email ef " + emailEfs + " types " + emailType);
            }
            if (anrEfs.size() > 0) {

                anrInfo.efids = new int[anrEfs.size()];
                anrInfo.type = new int[anrEfs.size()];

                for (i = 0; i < anrEfs.size(); i++) {

                    anrInfo.efids[i] = anrEfs.get(i);
                    anrInfo.type[i] = anrType.get(i);

                }
                log("parseTag anr ef " + anrEfs + " types " + anrType);
            }

            if (aasEfs.size() > 0) {
                aasInfo.efids = new int[aasEfs.size()];
                aasInfo.type = new int[aasEfs.size()];

                for (i = 0; i < aasEfs.size(); i++) {
                    aasInfo.efids[i] = aasEfs.get(i);
                    aasInfo.type[i] = aasType.get(i);
                }
                log("parseTag aas ef " + aasEfs + " types " + aasType);
            }

            if (sneEfs.size() > 0) {
                mSneEfSize = sneEfs.size();
                sneInfo.efids = new int[sneEfs.size()];
                sneInfo.type = new int[sneEfs.size()];

                for (i = 0; i < sneEfs.size(); i++) {
                    sneInfo.efids[i] = sneEfs.get(i);
                    sneInfo.type[i] = sneType.get(i);
                }
                log("parseTag sne ef " + sneEfs + " types " + sneType);
            }

            if (mPhoneBookRecords != null && mPhoneBookRecords.isEmpty()) {
                if (mAnrInfoFromPBR != null) {
                    mAnrInfoFromPBR.add(anrInfo);
                }
                if (mEmailInfoFromPBR != null) {
                    mEmailInfoFromPBR.add(emailInfo);
                }

                if (TeleFrameworkFactory.getInstance().isSupportOrange()) {
                    if (mAasInfoFromPBR != null) {
                        mAasInfoFromPBR.add(aasInfo);
                    }
                }

                if (TeleFrameworkFactory.getInstance().isSupportOrange()) {
                    if (mSneInfoFromPBR != null) {
                        mSneInfoFromPBR.add(sneInfo);
                        log("mSneInfoFromPBR.size() = "
                                + mSneInfoFromPBR.size());
                    }
                }
            }
        }

        void parseEfAndSFI(SimTlv tlv, int parentTag,
                SubjectIndexOfAdn emailInfo, SubjectIndexOfAdn anrInfo,
                ArrayList<Integer> emailEFS, ArrayList<Integer> anrEFS,
                ArrayList<Integer> emailTypes, ArrayList<Integer> anrTypes,
                SubjectIndexOfAdn aasInfo, ArrayList<Integer> aasEFS,
                ArrayList<Integer> aasTypes, SubjectIndexOfAdn sneInfo, ArrayList<Integer> sneEFS,
                ArrayList<Integer> sneTypes) {
            int tag;
            byte[] data;
            int tagNumberWithinParentTag = 0;
            boolean isNeedReadEmail = true;
            boolean isNeedReadAnr = true;

            do {
                tag = tlv.getTag();

                switch (tag) {
                case USIM_EFEMAIL_TAG:
                case USIM_EFADN_TAG:
                case USIM_EFEXT1_TAG:
                case USIM_EFANR_TAG:
                case USIM_EFPBC_TAG:
                case USIM_EFGRP_TAG:
                case USIM_EFAAS_TAG:
                case USIM_EFGAS_TAG:
                case USIM_EFUID_TAG:
                case USIM_EFCCP1_TAG:
                case USIM_EFIAP_TAG:
                case USIM_EFSNE_TAG:
                    /** 3GPP TS 31.102, 4.4.2.1 EF_PBR (Phone Book Reference file)
                    *
                    * The SFI value assigned to an EF which is indicated in EF_PBR shall
                    * correspond to the SFI indicated in the TLV object in EF_PBR.

                    * The primitive tag identifies clearly the type of data, its value
                    * field indicates the file identifier and, if applicable, the SFI
                    * value of the specified EF. That is, the length value of a primitive
                    * tag indicates if an SFI value is available for the EF or not:
                    * - Length = '02' Value: 'EFID (2 bytes)'
                    * - Length = '03' Value: 'EFID (2 bytes)', 'SFI (1 byte)'
                    */

                   int sfi = INVALID_SFI;
                   data = tlv.getData();

                   if (data.length < 2 || data.length > 3) {
                       log("Invalid TLV length: " + data.length);
                       break;
                   }

                   if (data.length == 3) {
                       sfi = data[2] & 0xFF;
                   }

                    int efid = ((data[0] & 0xFF) << 8) | (data[1] & 0xFF);

                    if (tag == USIM_EFADN_TAG) {

                        emailInfo.adnEfid = efid;
                        anrInfo.adnEfid = efid;
                        aasInfo.adnEfid = efid;
                        sneInfo.adnEfid = efid;

                    }

                    if (parentTag == USIM_TYPE2_TAG && tag == USIM_EFEMAIL_TAG) {
                        mEmailPresentInIap = true;
                        mEmailTagNumberInIap = tagNumberWithinParentTag;
                    }

                    if (tag == USIM_EFEMAIL_TAG && isNeedReadEmail) {

                        log("parseEf   email  efid "
                                        + Integer.toHexString(efid)
                                        + "  TAG  "
                                        + Integer.toHexString(parentTag)
                                        + "  tagNumberWithinParentTag "
                                        + Integer.toHexString(tagNumberWithinParentTag));

                        if (parentTag == USIM_TYPE2_TAG) {
                            emailInfo.recordNumInIap.put(efid, tagNumberWithinParentTag);

                        }

                        emailEFS.add(efid);
                        emailTypes.add(parentTag);
                        isNeedReadEmail = false;
                    }

                    if (tag == USIM_EFANR_TAG && isNeedReadAnr) {
                        log("parseEf   ANR  efid "
                                        + Integer.toHexString(efid)
                                        + "  TAG  "
                                        + Integer.toHexString(parentTag)
                                        + "  tagNumberWithinParentTag "
                                        + Integer.toHexString(tagNumberWithinParentTag));

                        if (parentTag == USIM_TYPE2_TAG) {

                            mAnrPresentInIap = true;
                            anrInfo.recordNumInIap.put(efid, tagNumberWithinParentTag);

                        }

                        anrEFS.add(efid);
                        anrTypes.add(parentTag);
                        isNeedReadAnr = false;

                    }

                    if (tag == USIM_EFAAS_TAG && TeleFrameworkFactory.getInstance().isSupportOrange()) {
                        log("parseEf   aas  efid "
                                        + Integer.toHexString(efid)
                                        + "  TAG  "
                                        + Integer.toHexString(parentTag)
                                        + "  tagNumberWithinParentTag "
                                        + Integer.toHexString(tagNumberWithinParentTag));
                        aasEFS.add(efid);
                        aasTypes.add(parentTag);
                    }

                    if (tag == USIM_EFSNE_TAG && TeleFrameworkFactory.getInstance().isSupportOrange()) {
                        log("parseEf   sne  efid "
                                        + Integer.toHexString(efid)
                                        + "  TAG  "
                                        + Integer.toHexString(parentTag)
                                        + "  tagNumberWithinParentTag "
                                        + Integer.toHexString(tagNumberWithinParentTag));
                        if (parentTag == USIM_TYPE2_TAG) {
                            mSnePresentInIap = true;
                            mSneTagNumberInIap = tagNumberWithinParentTag;
                            sneInfo.recordNumInIap.put(efid, tagNumberWithinParentTag);
                        }
                        sneEFS.add(efid);
                        sneTypes.add(parentTag);
                    }

                    log("parseTag tag " + tag + " efid   " + efid);

                    mFileIds.put(tag, new File(parentTag, efid, sfi, tagNumberWithinParentTag));
                    break;
                }
                tagNumberWithinParentTag++;
            } while (tlv.nextObject());
        }
    }

    private void log(String msg) {
        if (DBG)
            Rlog.d(TAG, msg);
    }

    private String readAnrRecord(int recNum) {
        byte[] anr1Rec = null;
        byte[] anr2Rec = null;
        byte[] anr3Rec = null;
        String anr1 = null;
        String anr2 = null;
        String anr3 = null;
        String anr = null;

        int firstAnrFileRecordCount = mAnrRecordSizeArray[0] / mAnrFileCount;
        if (mAnrFileCount == 0x1) {
            anr1Rec = mAnrFileRecord.get(recNum);
            anr = PhoneNumberUtils.calledPartyBCDToString(anr1Rec, 2,
                    (0xff & anr1Rec[2]));
            log("readAnrRecord anr:" + anr);
            return anr;
        } else {
            if (recNum < firstAnrFileRecordCount) {
                try {
                    anr1Rec = mAnrFileRecord.get(recNum);
                    anr2Rec = mAnrFileRecord.get(recNum
                            + firstAnrFileRecordCount);
                    if (mAnrFileCount > 0x2) {
                        anr3Rec = mAnrFileRecord.get(recNum + 2
                                * firstAnrFileRecordCount);
                    }
                } catch (IndexOutOfBoundsException e) {
                    return null;
                }
                anr1 = PhoneNumberUtils.calledPartyBCDToString(anr1Rec, 2,
                        (0xff & anr1Rec[2]));
                anr2 = PhoneNumberUtils.calledPartyBCDToString(anr2Rec, 2,
                        (0xff & anr2Rec[2]));
                if (mAnrFileCount > 0x2) {
                    anr3 = PhoneNumberUtils.calledPartyBCDToString(anr3Rec, 2,
                            (0xff & anr3Rec[2]));
                    anr = anr1 + ";" + anr2 + ";" + anr3;
                    log("readAnrRecord anr:" + anr);
                    return anr;
                }
            } else if (recNum >= firstAnrFileRecordCount && recNum < mAnrFileRecord.size() / mAnrFileCount) {
                int secondAnrFileRecordCount = (mAnrFileRecord.size() - mAnrRecordSizeArray[0])
                        / mAnrFileCount;
                try {
                    int secondAnrfileread = mAnrRecordSizeArray[0] + recNum % firstAnrFileRecordCount;
                    anr1Rec = mAnrFileRecord.get(secondAnrfileread);
                    anr2Rec = mAnrFileRecord.get(secondAnrfileread + secondAnrFileRecordCount);
                    if (mAnrFileCount > 0x2) {
                        anr3Rec = mAnrFileRecord.get(secondAnrfileread + 2 * secondAnrFileRecordCount);
                    }
                } catch (IndexOutOfBoundsException e) {
                    return null;
                }
                anr1 = PhoneNumberUtils.calledPartyBCDToString(anr1Rec, 2, (0xff & anr1Rec[2]));
                anr2 = PhoneNumberUtils.calledPartyBCDToString(anr2Rec, 2, (0xff & anr2Rec[2]));
                if (mAnrFileCount > 0x2) {
                    anr3 = PhoneNumberUtils.calledPartyBCDToString(anr3Rec, 2, (0xff & anr3Rec[2]));
                    anr = anr1 + ";" + anr2 + ";" + anr3;
                    log("readAnrRecord anr:" + anr);
                    return anr;
                }
            } else {
                log("the total anr size is exceed mAnrFileRecord.size()  " + mAnrFileRecord.size());
            }
            anr = anr1 + ";" + anr2;
            log("readAnrRecord anr:" + anr);
            return anr;
        }
    }

    private void readIapFileAndWait(int efid, int recNum) {
        log("readIapFileAndWait" + mPhoneBookRecords.size() + mTotalSize[2]);
        if (mIapFileRecord == null) {
            mIapFileRecord = new ArrayList<byte[]>();
        }
        int[] size;
        if (mRecordsSize != null && mRecordsSize.containsKey(efid)) {
            size = mRecordsSize.get(efid);
        } else {
            size = readFileSizeAndWait(efid);
        }
        log("readIapFileAndWait size" + size[0] + size[2]);

        for (int i = 0; i < size[2]; i++) {
            byte[] emptyValue = new byte[size[0]];
            for (byte value : emptyValue) {
                value = (byte) 0xFF;
            }
            mIapFileRecord.add(i, emptyValue);
        }
        int offSet = 0;
        offSet = mTotalSize[2] - mTotalSize[2] / (recNum + 1);

        log("readIapFileAndWait offSet " + offSet + mPhoneBookRecords.size());
        for (int i = offSet; i < mPhoneBookRecords.size() && i < (offSet + size[2]); i++) {
            if (!TextUtils.isEmpty(mPhoneBookRecords.get(i).getAlphaTag())
                    || !TextUtils.isEmpty(mPhoneBookRecords.get(i).getNumber())) {
                mPendingIapLoads.addAndGet(1);
                log("readIapFile index" + i);
                mFh.loadEFLinearFixed(efid, i + 1 - offSet, size[0],
                        obtainMessage(EVENT_IAP_LOAD_DONE, i - offSet, recNum));
            }
        }
        if (mPendingIapLoads.get() == 0) {
            mIsNotify.set(false);
            return;
        } else {
            mIsNotify.set(true);
        }
        try {
            mLock.wait();
        } catch (InterruptedException e) {
            loge("Interrupted Exception in readIapFileAndWait");
        }
    }

    public ArrayList<byte[]> getIapFileRecord(int recNum) {
        int efid = findEFIapInfo(recNum);
        if (efid < 0) {
            return null;
        }
        return (ArrayList<byte[]>) mIapFileRecordArray[recNum];
    }

    public int getNewSubjectNumber(int type, int num, int efid, int index,
            int adnNum, boolean isInIap) {

        log("getNewSubjectNumber  " + " adnNum " + adnNum
                + " isInIap " + isInIap + " efid " + efid + " index " + index);
        SubjectIndexOfAdn idx = getSubjectIndex(type, num);
        int newSubjectNum = -1;
        if (idx == null) {
            return -1;
        }

        if (idx.record == null || !idx.record.containsKey(efid)) {
            log("getNewSubjectNumber idx.record == null || !idx.record.containsKey(efid)  ");
            return -1;
        }
        int count = idx.record.get(efid).size();
        log("getNewSubjectNumber  count " + count + "adnNum " + adnNum);
        if (isInIap) {
            Set<Integer> set = (Set<Integer>) idx.usedSet[index];
            for (int i = 1; i <= count; i++) {
                Integer subjectNum = new Integer(i);
                if (!set.contains(subjectNum)) {
                    newSubjectNum = subjectNum;
                    log("getNewSubjectNumber  subjectNum(1) " + subjectNum);
                    set.add(subjectNum);
                    idx.usedSet[index] = set;
                    setSubjectIndex(type, num, idx);
                    SetRepeatUsedNumSet(type, efid, set);
                    break;
                }
            }
        } else {
            if (adnNum > count) {
                return newSubjectNum;
            } else {
                return adnNum;
            }
        }
        return newSubjectNum;
    }

    public void removeSubjectNumFromSet(int type, int num, int efid, int index,
            int anrNum) {
        Integer delNum = new Integer(anrNum);
        SubjectIndexOfAdn subject = getSubjectIndex(type, num);

        if (subject == null) {
            return;
        }
        int count = subject.record.get(efid).size();

        Set<Integer> set = (Set<Integer>) subject.usedSet[index];
        set.remove(delNum);
        log("removeSubjectNumFromSet  delnum(1) " + delNum);

        subject.usedSet[index] = set;
        setSubjectIndex(type, num, subject);
    }

    private int[] readAdnFileSizeAndWait(int recNum) {
        log("readAdnFileSizeAndWait, the current recNum is " + recNum);
        synchronized (mLock) {
            if (mPbrRecords == null) {
                readPbrFileAndWait();
            }
        }
        if (mPbrRecords == null) {
            loge("Error: Pbr file is empty");
            return null;
        }

        SparseArray<File> files;
        files = mPbrRecords.get(recNum).mFileIds;
        if (files == null || files.size() == 0)
            return null;

        if (files.get(USIM_EFADN_TAG) == null)
            return null;

        mFh.getEFLinearRecordSize(files.get(USIM_EFADN_TAG).getEfid(), obtainMessage(EVENT_ADN_RECORD_COUNT));
        try {
            mLock.wait();
        } catch (InterruptedException e) {
            loge("Interrupted Exception in readAdnFileAndWait");
        }

        return recordSize;
    }

    private void readIapFile(int recNum) {
        log("readIapFile recNum " + recNum);
        synchronized (mLock) {
            if (mPbrRecords == null) {
                readPbrFileAndWait();
            }
        }
        if (mPbrRecords == null) {
            loge("Error: Pbr file is empty");
            return;
        }

        SparseArray<File> files;
        files = mPbrRecords.get(recNum).mFileIds;
        if (files == null || files.size() == 0)
            return;

        log("readIapFile mAnrPresentInIap " + mAnrPresentInIap);
        log("readIapFile mEmailPresentInIap " + mEmailPresentInIap);
        if (mAnrPresentInIap || mEmailPresentInIap) {
            if(files.get(USIM_EFIAP_TAG) == null)
                return;
            readIapFileAndWait(files.get(USIM_EFIAP_TAG).getEfid(), recNum);
        }
    }

    public int[] readFileSizeAndWait(int efId) {
        log("readFileSizeAndWait" + efId);
        synchronized (mLock) {
            mFh.getEFLinearRecordSize(efId, obtainMessage(EVENT_GET_RECORDS_COUNT, efId, 0));
            try {
                mLock.wait();
            } catch (InterruptedException e) {
                loge("Interrupted Exception in readAdnFileAndWait");
            }
        }
        int[] size = mRecordsSize != null ? mRecordsSize.get(efId) : null;
        return size;
    }

    private SubjectIndexOfAdn getSubjectIndex(int type, int num) {
        LinkedList<SubjectIndexOfAdn> lst = null;
        SubjectIndexOfAdn index = null;
        switch (type) {
        case USIM_SUBJCET_EMAIL:
            lst = mEmailInfoFromPBR;
            break;
        case USIM_SUBJCET_ANR:
            lst = mAnrInfoFromPBR;
            break;
        case USIM_SUBJCET_AAS:
            lst = mAasInfoFromPBR;
            break;
        case USIM_SUBJCET_SNE:
            lst = mSneInfoFromPBR;
            break;
        default:
            break;
        }
        if (lst != null && lst.size() != 0) {
            index = lst.get(num);
            return index;
        }
        return null;
    }

    private void setSubjectIndex(int type, int num,
            SubjectIndexOfAdn subjectIndex) {
        SubjectIndexOfAdn index = null;
        switch (type) {
        case USIM_SUBJCET_EMAIL:
            if (mEmailInfoFromPBR == null) {
                return;
            }
            mEmailInfoFromPBR.set(num, subjectIndex);
            break;

        case USIM_SUBJCET_ANR:
            if (mAnrInfoFromPBR == null) {
                return;
            }
            mAnrInfoFromPBR.set(num, subjectIndex);
            break;

        case USIM_SUBJCET_AAS:
            if (mAasInfoFromPBR == null) {
                return;
            }
            log("aas num ==" + num + ",subjectIndex == " + subjectIndex);
            mAasInfoFromPBR.set(num, subjectIndex);
            break;

        case USIM_SUBJCET_SNE:
            if (mSneInfoFromPBR == null) {
                return;
            }
            log("sne num ==" + num + ",subjectIndex == " + subjectIndex);
            mSneInfoFromPBR.set(num, subjectIndex);
            break;
        default:
            break;
        }
    }

    public Set<Integer> getUsedNumSet(Set<Integer> set1, Set<Integer> set2,
            int count) {
        Set<Integer> totalSet = set1;
        for (int i = 1; i <= count; i++) {
            Integer subjectNum = new Integer(i);
            if (!totalSet.contains(subjectNum) && set2.contains(subjectNum)) {
                log("getUsedNumSet  subjectNum(1) " + subjectNum);
                totalSet.add(subjectNum);
            }
        }
        return totalSet;
    }

    public Set<Integer> getRepeatUsedNumSet(LinkedList<SubjectIndexOfAdn> lst,
            int idx, int efid, Set<Integer> set, int count) {
        SubjectIndexOfAdn index = null;
        Set<Integer> totalSet = set;
        for (int m = idx + 1; m < lst.size(); m++) {
            index = lst.get(m);
            if (index != null) {
                int num = getUsedNumSetIndex(efid, index);
                if (num >= 0) {
                    totalSet = getUsedNumSet((Set<Integer>) index.usedSet[num],
                            totalSet, count);
                }
            }
        }
        return totalSet;
    }

    private void setSubjectUsedNum(int type, int num) {
        SubjectIndexOfAdn index = getSubjectIndex(type, num);
        log(" setSubjectUsedNum num " + num);
        if (index == null || index.efids == null) {
            return;
        }
        int size = index.efids.length;
        log(" setSubjectUsedNum size " + size);
        index.usedSet = new Object[size];

        for (int i = 0; i < size; i++) {
            index.usedSet[i] = new HashSet<Integer>();
        }
        setSubjectIndex(type, num, index);
    }

    private void SetRepeatUsedNumSet(int type, int efid, Set<Integer> totalSet) {
        SubjectIndexOfAdn index = null;
        LinkedList<SubjectIndexOfAdn> lst = null;
        switch (type) {
        case USIM_SUBJCET_EMAIL:
            lst = mEmailInfoFromPBR;
            break;

        case USIM_SUBJCET_ANR:
            lst = mAnrInfoFromPBR;
            break;

        case USIM_SUBJCET_AAS:
            lst = mAasInfoFromPBR;
            break;

        case USIM_SUBJCET_SNE:
            lst = mSneInfoFromPBR;
            break;
        default:
            break;
        }
        if (lst == null) {
            return;
        }
        for (int m = 0; m < lst.size(); m++) {
            index = lst.get(m);
            if (index != null && index.recordNumInIap != null
                    && index.usedSet != null) {
                int num = getUsedNumSetIndex(efid, index);
                if (num >= 0) {
                    log(" SetRepeatUsedNumSet efid  " + efid + " num  " + num
                            + "  totalSet.size  " + totalSet.size());
                    index.usedSet[num] = totalSet;
                    setSubjectIndex(type, m, index);
                }
            }
        }
    }

    private void SetMapOfRepeatEfid(int type, int efid) {
        LinkedList<SubjectIndexOfAdn> lst = null;
        SubjectIndexOfAdn index = null;
        int efids[];
        Set<Integer> set;
        Set<Integer> totalSet = new HashSet<Integer>();
        switch (type) {
        case USIM_SUBJCET_EMAIL:
            lst = mEmailInfoFromPBR;
            break;
        case USIM_SUBJCET_ANR:
            lst = mAnrInfoFromPBR;
            break;
        case USIM_SUBJCET_AAS:
            lst = mAasInfoFromPBR;
            break;
        case USIM_SUBJCET_SNE:
            lst = mSneInfoFromPBR;
            break;
        default:
            break;
        }
        if (lst != null && lst.size() != 0) {
            int i = 0, j = 0;
            for (i = 0; i < lst.size(); i++) {
                index = lst.get(i);
                if (index != null && index.recordNumInIap != null
                        && index.record != null
                        && index.record.containsKey(efid)) {
                    int count = index.record.get(efid).size();
                    log("SetMapOfRepeatEfid  " + "count " + count);
                    int num = getUsedNumSetIndex(efid, index);
                    if (num >= 0) {
                        set = (Set<Integer>) index.usedSet[num];
                        if (set != null) {
                            log("SetMapOfRepeatEfid  size " + set.size());
                            totalSet = getUsedNumSet(totalSet, set, count);
                        }
                    }
                }
            }
        }
        if (totalSet != null) {
            log("SetMapOfRepeatEfid  size " + totalSet.size());
        }
        SetRepeatUsedNumSet(type, efid, totalSet);
    }

    public int getEfIdByTag(int recordNum, int fileTag) {
        if (mPbrRecords == null) {
            loge("getEfIdByTag error, Pbr file is empty");
            return -1;
        }
        SparseArray<File> files;
        files = mPbrRecords.get(recordNum).mFileIds;
        if (files == null || files.size() == 0){
            loge("getEfIdByTag error, files is empty");
            return -1;
        }
        if (files.get(fileTag) != null) {
            return files.get(fileTag).getEfid();
        }
        return 0;
    }

    public int findEFEmailInfo(int index) {
        synchronized (mLock) {
            if (mPbrRecords == null) {
                readPbrFileAndWait();
            }
        }
        if (mPbrRecords == null) {
            loge("Error: Pbr file is empty");
            return -1;
        }
        SparseArray<File> files;
        files = mPbrRecords.get(index).mFileIds;
        if (files == null || files.size() == 0){
            log("findEFEmailInfo  fileIds == null  index :" + index);
            return -1;
        }
        if (files.get(USIM_EFEMAIL_TAG) != null) {
            return files.get(USIM_EFEMAIL_TAG).getEfid();
        }
        return 0;
    }

    public int findEFAnrInfo(int index) {
        // for reload the pbr file when the pbr file is null
        synchronized (mLock) {
            if (mPbrRecords == null) {
                readPbrFileAndWait();
            }
        }
        if (mPbrRecords == null) {
            loge("Error: Pbr file is empty");
            return -1;
        }
        SparseArray<File> files;
        files = mPbrRecords.get(index).mFileIds;
        if (files == null || files.size() == 0){
            log("findEFAnrInfo  fileIds == null  index :" + index);
            return -1;
        }
        if (files.get(USIM_EFANR_TAG) != null) {
            return files.get(USIM_EFANR_TAG).getEfid();
        }
        return 0;
    }

    private String getType1Anr(int num, SubjectIndexOfAdn anrInfo, int adnNum,
            int efid, ArrayList<Integer> aasIndex) {
        String anr = "";
        int anrTagNumberInIap;
        ArrayList<byte[]> anrFileRecord;
        byte[] anrRec;
        anrFileRecord = anrInfo.record.get(efid);

        if (anrFileRecord == null) {
            return anr;
        }

        if (adnNum < anrFileRecord.size()) {
            anrRec = anrFileRecord.get(adnNum);
            anr = PhoneNumberUtils.calledPartyBCDToString(anrRec, 2,
                    (0xff & anrRec[2]));
            log("anrRec[0] == " + anrRec[0] + "anr" + anr);
            aasIndex.add((int) anrRec[0]);
        } else {
            anr = "";
        }
        // SIM record numbers are 1 based
        return anr;
    }

    private String getType2Anr(int num, SubjectIndexOfAdn anrInfo,
            byte[] record, int adnNum, int efid, ArrayList<Integer> aasIndex) {
        String anr = "";
        int anrTagNumberInIap;
        ArrayList<byte[]> anrFileRecord;
        byte[] anrRec;
        int index = 0;
        log(" getType2Anr  >> anrInfo.recordNumInIap.size() "
                + anrInfo.recordNumInIap.size() + "adnNum  " + adnNum);
        if (record == null) {
            return anr;
        }

        index = getUsedNumSetIndex(efid, anrInfo);
        if (index == -1) {
            return anr;
        }

        anrTagNumberInIap = anrInfo.recordNumInIap.get(efid);
        anrFileRecord = anrInfo.record.get(efid);
        if (anrFileRecord == null) {
            return anr;
        }
        log(" getType2Anr anrTagNumberInIap" + anrTagNumberInIap);
        int recNum = (int) (record[anrTagNumberInIap] & 0xFF);
        recNum = ((recNum == 0xFF) ? (-1) : recNum);
        log(" getType2Anr iap recNum == " + recNum);

        if (recNum > 0 && recNum <= anrFileRecord.size()) {
            anrRec = anrFileRecord.get(recNum - 1);
            anr = PhoneNumberUtils.calledPartyBCDToString(anrRec, 2,
                    (0xff & anrRec[2]));
            aasIndex.add((int) anrRec[0]);
            log("getAnrInIap anr:" + anr);
            // SIM record numbers are 1 based
            if (TextUtils.isEmpty(anr)) {
                log("getAnrInIap anr is emtry");
                setIapFileRecord(num, adnNum, (byte) 0xFF, anrTagNumberInIap);
                return anr;
            }

            Set<Integer> set = (Set<Integer>) anrInfo.usedSet[index];
            set.add(new Integer(recNum));
            anrInfo.usedSet[index] = set;
            setSubjectIndex(USIM_SUBJCET_ANR, num, anrInfo);

        }
        log("getType2Anr  >>>>>>>>>>>> anr " + anr);
        return anr;
    }

    private int getUsedNumSetIndex(int efid, SubjectIndexOfAdn index) {
        int count = -1;
        if (index != null && index.efids != null) {
            for (int k = 0; k < index.efids.length; k++) {
                log("getUsedNumSetIndex index.type[k] " + index.type[k]);
                if (index.type[k] == USIM_TYPE2_TAG) {
                    count++;
                    if (index.efids[k] == efid) {
                        log("getUsedNumSetIndex count " + count);
                        return count;
                    }
                }
            }
        }
        return -1;
    }

    public void setIapFileRecord(int recNum, int index, byte value, int numInIap) {
        log("setIapFileRecord >>  recNum: " + recNum + "index: " + index
                + " numInIap: " + numInIap + " value: " + value);
        ArrayList<byte[]> tmpIapFileRecord = (ArrayList<byte[]>) mIapFileRecordArray[recNum];
        byte[] record = tmpIapFileRecord.get(index);
        byte[] temp = new byte[record.length];
        for (int i = 0; i < temp.length; i++) {
            temp[i] = record[i];
        }
        temp[numInIap] = value;
        tmpIapFileRecord.set(index, temp);
        mIapFileRecordArray[recNum] = tmpIapFileRecord;
    }

    private String getAnr(int num, SubjectIndexOfAdn anrInfo,
            SubjectIndexOfAdn aasInfo, int index, byte[] record,
            int adnNum) {
        log("getAnr adnNum: " + adnNum + "num " + num);
        String anrGroup = null;
        String anr = null;
        String aasGroup = null;
        String aas = null;

        if (anrInfo.efids == null || anrInfo.efids.length == 0) {
            log("getAnr anrInfo.efids == null ||anrInfo.efids.length == 0 ");
            return null;
        }

        ArrayList<Integer> aasIndex = new ArrayList<Integer>();

        for (int i = 0; i < anrInfo.efids.length; i++) {
            if (anrInfo.type[i] == USIM_TYPE1_TAG) {
                anr = getType1Anr(num, anrInfo, adnNum, anrInfo.efids[i], aasIndex);
            }
            if (anrInfo.type[i] == USIM_TYPE2_TAG && anrInfo.recordNumInIap != null) {
                anr = getType2Anr(num, anrInfo, record, adnNum, anrInfo.efids[i], aasIndex);
            }

            if (i < aasIndex.size() && TeleFrameworkFactory.getInstance().isSupportOrange()) {
                aas = getAas(aasInfo, aasIndex.get(i));
            }

            if (i == 0) {
                anrGroup = anr;
                aasGroup = aas;
            } else {
                anrGroup = anrGroup + ";" + anr;
                aasGroup = aasGroup + ";" + aas;
            }

            log("anrGroup[" + i + "] == " + anrGroup + ", aasGroup[" + i
                    + "] == " + aasGroup + " ,mDoneAdnCount  += "
                    + mDoneAdnCount + " , record == " + record);
        }
        if (aasInfo != null && TeleFrameworkFactory.getInstance().isSupportOrange()) {
            setAas(index, aasGroup);
        }

        return anrGroup;
    }

    private void setEmailandAnr(int adnNum, String[] emails, String anr) {
        AdnRecordEx rec = mPhoneBookRecords.get(adnNum);
        if (rec == null && (emails != null || anr != null)) {
            rec = new AdnRecordEx("", "");
        }
        if (emails != null) {
            rec.setEmails(emails);
            log("setEmailandAnr AdnRecordEx  emails" + emails[0]);
        }
        if (anr != null) {
            log("setEmailandAnr AdnRecordEx  anr" + anr);
            rec.setAnr(anr);
        }
        mPhoneBookRecords.set(adnNum, rec);
    }

    private void setAnrIapFileRecord(int num, int index, byte value,
            int numInIap) {
        log("setAnrIapFileRecord >> num:" + num + "index: " + index + "value: "
                + value + " numInIap:" + numInIap);
        ArrayList<byte[]> tmpIapFileRecord = (ArrayList<byte[]>) mIapFileRecordArray[num];
        byte[] record = tmpIapFileRecord.get(index);
        record[numInIap] = value;
        tmpIapFileRecord.set(index, record);
        mIapFileRecordArray[num] = tmpIapFileRecord;
    }

    public int[] getAdnRecordSizeArray() {
        return mAdnRecordSizeArray;
    }

    public int getNumRecs() {
        synchronized (mLock) {
            if (mPbrRecords == null) {
                readPbrFileAndWait();
            }
            if (mPbrRecords == null) {
                loge("Error: Pbr file is empty");
                return 0;
            }
        }
        return mPbrRecords.size();
    }

    public int getAnrNum() {
        int num = 0;
        int[] efids = getSubjectEfids(USIM_SUBJCET_ANR, 0);
        if (efids == null) {
            return 0;
        }
        num = efids.length;
        log("getAnrNum " + num);
        return num;
    }

    public int getPhoneNumMaxLen() {
        return AdnRecordEx.MAX_LENTH_NUMBER;
    }

    // the email ef may be type1 or type2
    public int getEmailType() {
        if (mEmailPresentInIap == true) {
            return 2;
        } else {
            return 1;
        }
    }

    public int getEmailNum() {
        int[] efids = getSubjectEfids(USIM_SUBJCET_EMAIL, 0);
        if (efids == null) {
            log("efids is NULL");
            return -2;
        }
        int num = efids.length;
        log("getEmailNum " + num);
        return num;
    }

    public int[] getValidNumToMatch(AdnRecordEx adn, int type, int[] subjectNums) {
        int[] ret = null;
        int efid = 0;
        for (int num = 0; num < getNumRecs(); num++) {
            efid = findEFInfo(num);
            if (efid <= 0) {
                return null;
            }
            log("getEfIdToMatch ");
            ArrayList<AdnRecordEx> oldAdnList;
            loge("efid is " + efid);
            oldAdnList = mAdnCache.getRecordsIfLoadedEx(efid);
            if (oldAdnList == null) {
                return null;
            }
            log("getEfIdToMatch (2)");
            int count = 1;
            int adnIndex = 0;
            for (Iterator<AdnRecordEx> it = oldAdnList.iterator(); it.hasNext();) {
                if (adn.isEqual(it.next())) {
                    log("we got the index " + count);
                    adnIndex = count;
                    ret = getAvalibleSubjectCount(num, type, efid, adnIndex,
                            subjectNums);
                    if (ret != null) {
                        return ret;
                    }
                }
                count++;
            }
        }
        return null;
    }

    private int getAvalibleAdnCount() {
        List<AdnRecordEx> adnRecords = mPhoneBookRecords;
        int totalCount = 0;
        int count = 0;
        if (mPhoneBookRecords == null) {
            return 0;
        }
        totalCount = mPhoneBookRecords.size();
        AdnRecordEx adnRecord;
        for (int i = 0; i < totalCount; i++) {
            adnRecord = mPhoneBookRecords.get(i);
            if (adnRecord.isEmpty()) {
                count++;
            }
        }
        return count;
    }

    public int[] getAvalibleSubjectCount(int num, int type, int efid,
            int adnNum, int[] subjectNums) {
        SubjectIndexOfAdn index = null;
        int count = 0;
        int avalibleNum = 0;
        int[] ret = null;
        int n = 0;
        log("getAvalibleSubjectCount efid " + efid + " num " + num);
        log("getAvalibleSubjectCount  " + " type " + type + " adnNum " + adnNum
                + "  subjectNums " + subjectNums);
        index = getSubjectIndex(type, num);
        if (index == null) {
            return null;
        }
        ret = new int[subjectNums.length];
        log("getAvalibleSubjectCount adnEfid " + index.adnEfid);
        if (index != null && index.adnEfid == efid && index.record != null
                && index.efids != null && index.type != null) {
            for (int j = 0; j < index.efids.length; j++) {
                log("getAvalibleSubjectCount efid " + index.efids[j]);
                for (int l = 0; l < subjectNums.length; l++) {
                    log("getAvalibleSubjectCount efid " + subjectNums[l]);
                    if (subjectNums[l] == 1 && index.record.containsKey(index.efids[j])
                            && index.record.get(index.efids[j]) != null) {
                        count = index.record.get(index.efids[j]).size();
                        if (index.type[j] == USIM_TYPE1_TAG) {
                            ret[n] = getAvalibleAdnCount();
                            n++;
                            break;
                        } else if (index.type[j] == USIM_TYPE2_TAG) {
                            int idx = getUsedNumSetIndex(index.efids[j], index);
                            log("getAvalibleSubjectCount idx " + idx);
                            if (idx >= 0) {
                                Set<Integer> usedSet = (Set<Integer>) index.usedSet[idx];
                                avalibleNum = count - usedSet.size();
                                ret[n] = avalibleNum;
                                n++;
                                break;
                            }
                        }
                    }
                }
            }
        }
        log("getAvalibleSubjectCount  n " + n);
        if (n == 0) {
            ret = null;
            return ret;
        }
        for (int i = 0; i < ret.length; i++) {
            log("getAvalibleSubjectCount  ret[] " + ret[i]);
        }
        return ret;
    }

    public int[] getAvalibleAnrCount(String name, String number,
            String[] emails, String anr, int[] anrNums) {
        AdnRecordEx adn = new AdnRecordEx(name, number, emails, anr, "", "",
                "", "");
        return getValidNumToMatch(adn, USIM_SUBJCET_ANR, anrNums);
    }

    public int[] getAvalibleEmailCount(String name, String number,
            String[] emails, String anr, int[] emailNums) {
        AdnRecordEx adn = new AdnRecordEx(name, number, emails, anr, "", "",
                "", "");
        return getValidNumToMatch(adn, USIM_SUBJCET_EMAIL, emailNums);
    }

    private String getGrp(int adnNum) {
        if (mGrpFileRecord == null) {
            return null;
        }
        String retGrp = composeGrpString(mGrpFileRecord.get(adnNum));
        return retGrp;
    }

    private String composeGrpString(byte data[]) {
        String groupInfo = null;
        mGrpCount = data.length;
        for (int i = 0; i < mGrpCount; i++) {
            int temp = data[i] & 0xFF;
            if (temp == 0 || temp == 0xFF)
                continue; // 0X'00' -- no group indicated;
            if (groupInfo == null) {
                groupInfo = Integer.toString(temp);
            } else {
                groupInfo = groupInfo + AdnRecordEx.ANR_SPLIT_FLG + temp;
            }
        }
        return groupInfo;
    }

    public int[] getSubjectEfids(int type, int num) {
        SubjectIndexOfAdn index = getSubjectIndex(type, num);
        if (index == null) {
            return null;
        }
        int[] result = index.efids;
        if (result != null) {
            log("getSubjectEfids  " + "length " + result.length);
        }
        return result;
    }

    public int[][] getSubjectTagNumberInIap(int type, int num) {

        Map<Integer, Integer> anrTagMap = null;
        SubjectIndexOfAdn index = getSubjectIndex(type, num);
        boolean isInIap = false;
        if (index == null) {
            return null;
        }
        int[][] result = new int[index.efids.length][2];
        anrTagMap = index.recordNumInIap;
        if (anrTagMap == null || anrTagMap.size() == 0) {
            log("getSubjectTagNumberInIap recordNumInIap == null");
            return null;
        }
        for (int i = 0; i < index.efids.length; i++) {
            if (anrTagMap.containsKey(index.efids[i])) {
                result[i][1] = anrTagMap.get(index.efids[i]);
                result[i][0] = index.efids[i];
                isInIap = true;
            }
        }
        if (!isInIap) {
            result = null;
            log("getSubjectTagNumberInIap isInIap == false");
        }
        return result;
    }

    public int[][] getAnrTagNumberInIap(int num) {
        Map<Integer, Integer> anrTagMap;
        int[][] result = new int[mAnrInfoFromPBR.get(num).efids.length][2];
        anrTagMap = mAnrInfoFromPBR.get(num).recordNumInIap;
        for (int i = 0; i < mAnrInfoFromPBR.get(num).efids.length; i++) {
            result[i][0] = mAnrInfoFromPBR.get(num).efids[i];
            result[i][1] = anrTagMap.get(mAnrInfoFromPBR.get(num).efids[i]);
        }
        return result;
    }

    public boolean isSubjectRecordInIap(int type, int num, int indexOfEfids) {
        SubjectIndexOfAdn index = getSubjectIndex(type, num);
        if (index == null) {
            return false;
        }
        if (index.type[indexOfEfids] == USIM_TYPE2_TAG
                && index.recordNumInIap.size() > 0) {
            return true;
        } else if (index.type[indexOfEfids] == USIM_TYPE1_TAG) {
            return false;
        }
        return false;
    }

    public int findEFInfo(int index) {
        synchronized (mLock) {
            if (mPbrRecords == null) {
                readPbrFileAndWait();
            }
        }
        if (mPbrRecords == null) {
            loge("Error: Pbr file is empty");
            return -1;
        }
        SparseArray<File> files;
        files = mPbrRecords.get(index).mFileIds;
        if (files == null || files.size() == 0){
            loge("Error: files is empty");
            return -1;
        }

        if (files.get(USIM_EFADN_TAG) != null) {
            return files.get(USIM_EFADN_TAG).getEfid();
        }

        return -1;
    }

    public int findExtensionEFInfo(int index) {
        synchronized (mLock) {
            if (mPbrRecords == null) {
                readPbrFileAndWait();
            }
        }
        if (mPbrRecords == null) {
            loge("Error: Pbr file is empty");
            return -1;
        }
        SparseArray<File> files;
        files = mPbrRecords.get(index).mFileIds;
        if (files == null || files.size() == 0){
            loge("Error: files is empty");
            return -1;
        }
        log("findExtensionEFInfo files " + files);
        if (files.get(USIM_EFEXT1_TAG) != null) {
            return files.get(USIM_EFEXT1_TAG).getEfid();
        }
        return 0;
    }

    public int findEFIapInfo(int index) {
        synchronized (mLock) {
            if (mPbrRecords == null) {
                readPbrFileAndWait();
            }
        }
        if (mPbrRecords == null) {
            loge("Error: Pbr file is empty");
            return -1;
        }
        SparseArray<File> files;
        files = mPbrRecords.get(index).mFileIds;
        if (files == null || files.size() == 0){
            log("findEFIapInfo  fileIds == null  index :" + index);
            return -1;
        }

        if (files.get(USIM_EFIAP_TAG) != null) {
            return files.get(USIM_EFIAP_TAG).getEfid();
        }
        return 0;
    }

    public int findEFGasInfo() {
        // for reload the pbr file when the pbr file is null
        synchronized (mLock) {
            if (mPbrRecords == null) {
                readPbrFileAndWait();
            }
        }
        if (mPbrRecords == null) {
            loge("Error: Pbr file is empty");
            return -1;
        }
        SparseArray<File> files;
        files = mPbrRecords.get(0).mFileIds;
        if (files == null || files.size() == 0){
            log("findEFGasInfo fileIds == null");
            return -1;
        }

        if (files.get(USIM_EFGAS_TAG) != null) {
            return files.get(USIM_EFGAS_TAG).getEfid();
        }
        return 0;
    }

    public void updateGasList(String groupName, int groupId) {
        if (mGasList.isEmpty())
            return;
        mGasList.set(groupId - 1, groupName);
    }

    private void readAnrFileAndWait(int recNum) {
        log("readAnrFileAndWait recNum " + recNum);
        synchronized (mLock) {
            if (mPbrRecords == null) {
                readPbrFileAndWait();
            }
        }
        if (mPbrRecords == null) {
            loge("Error: Pbr file is empty");
            return;
        }

        SparseArray<File> files;
        files = mPbrRecords.get(recNum).mFileIds;
        if (files == null || files.size() == 0){
            log("readAnrFileAndWait  fileIds == null");
            return;
        }

        log("readAnrFileAndWait  mAnrInfoFromPBR !=null fileIds.size()" + files.size());
        if (files.get(USIM_EFANR_TAG) != null) {
            SubjectIndexOfAdn records = getSubjectIndex(USIM_SUBJCET_ANR, recNum);
            if (records == null) {
                log("readAnrFileAndWait  records == null ");
                return;
            }
            records.record = new HashMap<Integer, ArrayList<byte[]>>();
            if (records.efids == null || records.efids.length == 0) {
                log("readAnrFileAndWait  records.efids == null || records.efids.length == 0");
                return;
            }
            mAnrFileCount = records.efids.length;
            boolean isFail = false;
            log("readAnrFileAndWait mAnrFileCount " + mAnrFileCount);
            for (int i = 0; i < mAnrFileCount; i++) {
                log("readAnrFileAndWait type" + records.type[i]);
                if (records.type[i] == USIM_TYPE1_TAG) {
                    if (mAnrFileRecord == null) {
                        mAnrFileRecord = new ArrayList<byte[]>();
                    }
                    int[] size;
                    if (mRecordsSize != null && mRecordsSize.containsKey(records.efids[i])) {
                        size = mRecordsSize.get(records.efids[i]);
                    } else {
                        size = readFileSizeAndWait(records.efids[i]);
                    }
                    log("readAnrFileAndWait size[0] = " + size[0] + " size[2] = " + size[2]);

                    for (int j = 0; j < size[2]; j++) {
                        byte[] emptyValue = new byte[size[0]];
                        for (byte value : emptyValue) {
                            value = (byte) 0xFF;
                        }
                        mAnrFileRecord.add(j, emptyValue);
                    }

                    int offSet = 0;
                    offSet = mTotalSize[2] - mTotalSize[2] / (recNum + 1);

                    log("readAnrFileAndWait offSet " + offSet + mPhoneBookRecords.size());
                    for (int j = offSet; j < mPhoneBookRecords.size() && j < (offSet + size[2]); j++) {
                        if (!TextUtils.isEmpty(mPhoneBookRecords.get(j).getAlphaTag())
                                || !TextUtils.isEmpty(mPhoneBookRecords.get(j).getNumber())) {
                            mPendingAnrLoads.addAndGet(1);
                            log("readAnrFile index" + j);
                            mFh.loadEFLinearFixed(records.efids[i], j + 1 - offSet, size[0], 
                                    obtainMessage(EVENT_ANR_RECORD_LOAD_DONE, j - offSet, recNum));
                        }
                    }

                    if (mPendingAnrLoads.get() == 0) {
                        mIsNotify.set(false);
                    } else {
                        mIsNotify.set(true);
                        try {
                            mLock.wait();
                        } catch (InterruptedException e) {
                            loge("Interrupted Exception in readEmailFileAndWait");
                        }
                    }
                } else if (records.type[i] == USIM_TYPE2_TAG) {
                    mFh.loadEFLinearFixedAll(records.efids[i],
                            obtainMessage(EVENT_ANR_LOAD_DONE));
                    try {
                        mLock.wait();
                    } catch (InterruptedException e) {
                        loge("Interrupted Exception in readEmailFileAndWait");
                    }
                }

                log("load ANR times ...... " + (i + 1));

                if (mAnrFileRecord == null) {
                    loge("Error: ANR file is empty");
                    records.efids[i] = 0;
                    isFail = true;
                    continue;
                }

                records.record.put(records.efids[i], mAnrFileRecord);
                mAnrFileRecord = null;
            }
            handleReadFileResult(records);
            setSubjectIndex(USIM_SUBJCET_ANR, recNum, records);
            setSubjectUsedNum(USIM_SUBJCET_ANR, recNum);
        }
    }

    private void handleReadFileResult(SubjectIndexOfAdn records) {
        log("handleReadFileResult  ");
        int i = 0;
        ArrayList<Integer> efs = new ArrayList<Integer>();
        if (records == null || records.efids == null) {
            log("handleReadFileResult records == null ||records.efids == null ");
            return;
        }
        for (i = 0; i < records.efids.length; i++) {
            if (records.efids[i] != 0) {
                efs.add(records.efids[i]);
            } else {
                log("handleReadFileResult err efid " + records.efids[i]);
                if (records.recordNumInIap != null && records.recordNumInIap.containsKey(records.efids[i])) {
                    records.recordNumInIap.remove(records.efids[i]);
                }
            }
        }
        log("handleReadFileResult  efs " + efs);
        int[] validEf = new int[efs.size()];
        for (i = 0; i < efs.size(); i++) {
            validEf[i] = efs.get(i);
        }
        records.efids = validEf;
    }

    private void readGrpFileAndWait(int recNum, int totalSize) {
        log("readGrpFileAndWait");
        synchronized (mLock) {
            if (mPbrRecords == null) {
                readPbrFileAndWait();
            }
        }
        if (mPbrRecords == null) {
            loge("Error: Pbr file is empty");
            return;
        }

        SparseArray<File> files;
        files = mPbrRecords.get(recNum).mFileIds;
        if (files == null || files.size() == 0)
            return;

        if (files.get(USIM_EFGRP_TAG) == null)
            return;

        int efId = files.get(USIM_EFGRP_TAG).getEfid();
        if (mGrpFileRecord == null) {
            mGrpFileRecord = new ArrayList<byte[]>();
        }
        int[] size;
        if (mRecordsSize != null && mRecordsSize.containsKey(efId)) {
            size = mRecordsSize.get(efId);
        } else {
            size = readFileSizeAndWait(efId);
        }

        int offSet = 0;
        offSet = totalSize - totalSize / (recNum + 1);
        log("readGrpFileAndWait size" + size[2] + " offSet "
                + offSet + " totalSize " + totalSize);
        for (int i = offSet; i < offSet + size[2]; i++) {
            byte[] emptyValue = new byte[size[0]];
            for (byte value : emptyValue) {
                value = (byte) 0xFF;
            }
            mGrpFileRecord.add(i, emptyValue);
        }
        log("readGrpFileAndWait  " + mPhoneBookRecords.size());
        for (int i = offSet; i < mPhoneBookRecords.size() && i < (offSet + size[2]); i++) {
            if (!TextUtils.isEmpty(mPhoneBookRecords.get(i).getAlphaTag())
                    || !TextUtils.isEmpty(mPhoneBookRecords.get(i).getNumber())) {
                mPendingGrpLoads.addAndGet(1);
                log("readGrpFile index" + i);
                mFh.loadEFLinearFixed(efId, i + 1 - offSet, size[0],
                        obtainMessage(EVENT_GRP_LOAD_DONE, i - offSet, offSet));
            }
        }
        if (mPendingGrpLoads.get() == 0) {
            mIsNotify.set(false);
            return;
        } else {
            mIsNotify.set(true);
        }
        try {
            mLock.wait();
        } catch (InterruptedException e) {
            loge("Interrupted Exception in readGrpFileAndWait");
        }
    }

    private void updateAdnRecord(int num) {
        SubjectIndexOfAdn emailInfo = null;
        int emailType = 0;
        SubjectIndexOfAdn anrInfo = null;
        int anrType = 0;
        String anr = null;
        int numAdnRecs = mPhoneBookRecords.size();
        SubjectIndexOfAdn aasInfo = null;
        String aas = null;
        SubjectIndexOfAdn sneInfo = null;
        String snes = null;

        mAdnRecordSizeArray[num] = mPhoneBookRecords.size();
        log("updateAdnRecord numAdnRecs : " + numAdnRecs + " num " + num);
        for (int i = 0; i < num; i++) {
            mAdnRecordSizeArray[num] -= mAdnRecordSizeArray[i];
        }

        log("updateAdnRecord mAdnRecordSizeArray[num] : "
                + mAdnRecordSizeArray[num] + " num " + num);

        int numIapRec = 0;
        int efid = 0;
        byte[] record = null;

        emailInfo = getSubjectIndex(USIM_SUBJCET_EMAIL, num);

        anrInfo = getSubjectIndex(USIM_SUBJCET_ANR, num);
        if (TeleFrameworkFactory.getInstance().isSupportOrange()) {
            aasInfo = getSubjectIndex(USIM_SUBJCET_AAS, num);
        }
        if (TeleFrameworkFactory.getInstance().isSupportOrange()) {
            sneInfo = getSubjectIndex(USIM_SUBJCET_SNE, num);
        }
        log("emailInfo == " + emailInfo + ",anrInfo == " + anrInfo
                + ", sneInfo == " + sneInfo + ",aasInfo == " + aasInfo
                + ", mIapFileRecord == " + mIapFileRecord);

        if (mIapFileRecord != null) {
            // The number of records in the IAP file is same as the number of
            // records in ADN file.
            // The order of the pointers in an EFIAP shall be the same as the
            // order of file IDs
            // that appear in the TLV object indicated by Tag 'A9' in the
            // reference file record.
            // i.e value of mEmailTagNumberInIap

            numIapRec = mIapFileRecord.size();
            log("updateAdnRecord mIapRecordSizeArray[num] : "
                    + mIapFileRecord.size());

            mIapFileRecordArray[num] = mIapFileRecord;
            mIapRecordSizeArray[num] = mIapFileRecord.size();

            log("updateAdnRecord,numIapRec  " + numIapRec);
            numIapRec = ((numAdnRecs - mDoneAdnCount) > numIapRec) ? numIapRec
                    : (numAdnRecs - mDoneAdnCount);
        } else {
            numIapRec = numAdnRecs - mDoneAdnCount;
        }
        log("updateAdnRecord,numIapRec  " + numIapRec + " mDoneAdnCount "
                + mDoneAdnCount);
        for (int i = mDoneAdnCount; i < (mDoneAdnCount + numIapRec); i++) {
            record = null;
            if (mIapFileRecord != null) {
                try {
                    record = mIapFileRecord.get((i - mDoneAdnCount));
                } catch (IndexOutOfBoundsException e) {
                   loge("Error: Improper ICC card: No IAP record for ADN, continuing");
                }
            }

            if (mEmailsForAdnRec != null) {
                AdnRecordEx rec = mPhoneBookRecords.get(i);
                int adnEfid = rec.getEfid();
                int adnRecId = rec.getRecId();

                int index = (((adnEfid & 0xFFFF) << 8) | ((adnRecId - 1) & 0xFF));

                ArrayList<String> emailList = null;
                try {
                    emailList = mEmailsForAdnRec.get(index);
                } catch (IndexOutOfBoundsException e) {

                }

                if (emailList != null){
                    String[] emails = new String[emailList.size()];
                    System.arraycopy(emailList.toArray(), 0, emails, 0, emailList.size());
                    setEmailandAnr(i, emails, null);
                }
            }
            if (anrInfo != null) {
                anr = getAnr(num, anrInfo, aasInfo, i, record, (i - mDoneAdnCount));
                setEmailandAnr(i, null, anr);
            }

            if (sneInfo != null && TeleFrameworkFactory.getInstance().isSupportOrange()) {
                snes = getSne(num, sneInfo, record, (i - mDoneAdnCount));
                log("updateAdnRecord,snes =" + snes);
                setSne(i, snes);
            }
        }
        mIapFileRecord = null;
        mDoneAdnCount += numAdnRecs;
    }

    private void CheckRepeatType2Ef() {
        ArrayList<Integer> efs = getType2Ef(USIM_SUBJCET_EMAIL);
        int i = 0;
        log("CheckRepeatType2Ef ");
        for (i = 0; i < efs.size(); i++) {
            SetMapOfRepeatEfid(USIM_SUBJCET_EMAIL, efs.get(i));
        }
        efs = getType2Ef(USIM_SUBJCET_ANR);
        for (i = 0; i < efs.size(); i++) {
            SetMapOfRepeatEfid(USIM_SUBJCET_ANR, efs.get(i));
        }
    }

    private ArrayList<Integer> getType2Ef(int type) {
        ArrayList<Integer> efs = new ArrayList<Integer>();
        LinkedList<SubjectIndexOfAdn> lst = null;
        SubjectIndexOfAdn index = null;
        boolean isAdd = false;
        switch (type) {
        case USIM_SUBJCET_EMAIL:
            lst = mEmailInfoFromPBR;
            break;

        case USIM_SUBJCET_ANR:
            lst = mAnrInfoFromPBR;
            break;

        case USIM_SUBJCET_AAS:
            lst = mAasInfoFromPBR;
            break;

        case USIM_SUBJCET_SNE:
            lst = mSneInfoFromPBR;
            break;
        default:
            break;
        }

        if (lst != null && lst.size() != 0) {
            log("getType2Ef size " + lst.size());
            for (int i = 0; i < lst.size(); i++) {
                index = lst.get(i);
                if (index != null && index.efids != null && index.type != null) {
                    for (int j = 0; j < index.efids.length; j++) {
                        if (index.type[j] == USIM_TYPE2_TAG) {
                            isAdd = true;
                            for (int k = 0; k < efs.size(); k++) {
                                if (efs.get(k) == index.efids[j]) {
                                    isAdd = false;
                                }
                            }
                            if (isAdd) {
                                efs.add(index.efids[j]);
                            }
                        }
                    }
                }
            }
        }
        log("getType2Ef  type " + type + " efs " + efs);
        return efs;
    }

    private void setUsedNumOfEfid(int type, int idx, int efid, Object obj) {
        LinkedList<SubjectIndexOfAdn> lst = null;
        SubjectIndexOfAdn index = null;
        switch (type) {
        case USIM_SUBJCET_EMAIL:
            lst = mEmailInfoFromPBR;
            break;

        case USIM_SUBJCET_ANR:
            lst = mAnrInfoFromPBR;
            break;
        default:
            break;
        }

        if (lst != null && lst.size() != 0) {
            log("setUsedNumOfEfid size " + lst.size());
            for (int i = 0; i < lst.size(); i++) {
                index = lst.get(i);
                if (index != null && index.efids != null) {
                    for (int j = 0; j < index.efids.length; j++) {
                        if (index.efids[j] == efid) {
                            index.usedSet[idx] = obj;
                            setSubjectIndex(type, i, index);
                            break;
                        }
                    }
                }
            }
        }
    }

    private void updateAdnRecordNum() {
        int numAdnRecs = mPhoneBookRecords.size();
        log("updateAdnRecord Num and grp info, adn size:" + numAdnRecs);
        for (int i = 0; i < numAdnRecs; i++) {
            AdnRecordEx adn = mPhoneBookRecords.get(i);
            if (adn == null)
                continue;
            adn.setRecordNumber(i + 1);
            if (mGrpFileRecord != null && i < mGrpFileRecord.size()) {
                adn.setGrp(getGrp(i));
            }
        }
    }

    private void updatePbcAndCc() {
        log("update EFpbc begin");
        SparseArray<File> files;
        files = mPbrRecords.get(0).mFileIds;
        if (files == null || files.size() == 0)
            return;

        if (files.get(USIM_EFPBC_TAG) == null)
            return;
        int efPbcId = files.get(USIM_EFPBC_TAG).getEfid();
        log(" USIM_EFPBC_TAG = 0x" + Integer.toHexString(efPbcId).toUpperCase());
        int changeCounter = 0;
        if (mPbcFileRecord == null) {
            mPbcFileRecord = new ArrayList<byte[]>();
        }
        int[] size;
        if (mRecordsSize != null && mRecordsSize.containsKey(efPbcId)) {
            size = mRecordsSize.get(efPbcId);
        } else {
            size = readFileSizeAndWait(efPbcId);
        }
        log("readPbcFileAndWait size" + size[0] + size[2]);

        for (int i = 0; i < size[2]; i++) {
            byte[] emptyValue = new byte[size[0]];
            for (byte value : emptyValue) {
                value = (byte) 0xFF;
            }
            mPbcFileRecord.add(i, emptyValue);
        }

        for (int i = 0; (i < mPbcFileRecord.size() && i < mPhoneBookRecords.size()); i++) {
            if (!TextUtils.isEmpty(mPhoneBookRecords.get(i).getAlphaTag())
                    || !TextUtils.isEmpty(mPhoneBookRecords.get(i).getNumber())) {
                mPendingPbcLoads.addAndGet(1);
                log("readPbcFile index" + i);
                mFh.loadEFLinearFixed(efPbcId, i + 1, size[0],
                        obtainMessage(EVENT_LOAD_EF_PBC_RECORD_DONE, i, 0));
            }
        }
        if (mPendingPbcLoads.get() == 0) {
            mIsNotify.set(false);
        } else {
            mIsNotify.set(true);
            try {
                mLock.wait();
            } catch (InterruptedException e) {
                loge("Interrupted Exception in updatePbcAndCc");
            }
        }

        for (int i = 0; i < mPbcFileRecord.size(); i++) {
            byte[] temp = null;
            temp = mPbcFileRecord.get(i);
            if (temp != null && ((temp[0] & 0xFF) == 0x01)) {
                changeCounter++;
                byte[] data = new byte[2];
                data[0] = (byte) 0x00;
                data[1] = (byte) 0x00;
                // udpate EF pbc
                mFh.updateEFLinearFixed(efPbcId, i + 1, data, null,
                        obtainMessage(EVENT_UPDATE_RECORD_DONE));
            }
        }
        log("update EFpbc end, changeCounter " + changeCounter);
        // update EFcc
        if (changeCounter > 0) {
            // get Change Counter
            mFh.loadEFTransparent(IccConstantsEx.EF_CC, 2,
                    obtainMessage(EVENT_EF_CC_LOAD_DONE, changeCounter, 0));
        }
        return;
    }

    public ArrayList<String> loadGasFromUsim() {
        log("loadGasFromUsim");
        if (!mGasList.isEmpty())
            return mGasList;

        if (mGasFileRecord == null) {
            // the gas file is Type3 in Pbr
            readGasFileAndWait(0);
        }

        if (mGasFileRecord == null) {
            loge("Error: mGasFileRecord file is empty");
            return null;
        }

        int gasSize = mGasFileRecord.size();
        log("getGas size " + gasSize);

        byte[] gasRec = null;
        for (int i = 0; i < gasSize; i++) {
            gasRec = mGasFileRecord.get(i);
            mGasList.add(IccUtils.adnStringFieldToString(gasRec, 0,
                    gasRec.length));
        }
        log("loadGasFromUsim mGasList: " + mGasList);
        return mGasList;
    }

    private void readGasFileAndWait(int recNum) {
        log("readGasFileAndWait");
        synchronized (mLock) {
            if (mPbrRecords == null) {
                readPbrFileAndWait();
            }
        }
        if (mPbrRecords == null) {
            loge("Error: Pbr file is empty");
            mIsPbrFileExisting = false;
            log("mIsPbrFileExisting = false");
            return;
        }

        SparseArray<File> files;
        files = mPbrRecords.get(recNum).mFileIds;
        if (files == null || files.size() == 0)
            return;

        if(files.get(USIM_EFGAS_TAG) == null)
            return;
        mFh.loadEFLinearFixedAll(files.get(USIM_EFGAS_TAG).getEfid(), obtainMessage(EVENT_GAS_LOAD_DONE));

        synchronized (mLock) {
            try {
                mLock.wait();
            } catch (InterruptedException e) {
                loge("Interrupted Exception in readGasFileAndWait");
            }
        }
    }

    public int[] getEmailRecordSizeArray() {
        return mEmailRecordSizeArray;
    }

    public int[] getIapRecordSizeArray() {
        return mIapRecordSizeArray;
    }

    public void setPhoneBookRecords(int index, AdnRecordEx adn) {
        mPhoneBookRecords.set(index, adn);
    }

    public int getPhoneBookRecordsNum() {
        return mPhoneBookRecords.size();
    }

    public synchronized int[] getAdnRecordsSize() {
        int size[] = new int[3];
        int totalSize[] = new int[3];
        log("getAdnRecordsSize");
        synchronized (mLock) {
            if (mTotalSize != null) {
                return mTotalSize;
            }
            if (mPbrRecords == null) {
                readPbrFileAndWait();
            }
            if (mPbrRecords == null)
                return null;
            int numRecs = mPbrRecords.size();

            for (int i = 0; i < numRecs; i++) {
                size = readAdnFileSizeAndWait(i);
                if (size != null) {
                    totalSize[0] = size[0];
                    totalSize[1] += size[1];
                    totalSize[2] += size[2];
                    log("getAdnRecordsSize totalSize[0]"
                            + totalSize[0] + "totalSize[1]" + totalSize[1]
                            + "totalSize[2]" + totalSize[2]);
                } else {
                    Rlog.d(TAG, "getAdnRecordsSize size == null");
                    return null;
                }
            }
        }
        return totalSize;
    }

    public int[] getEfFilesFromUsim() {
        int[] efids = null;
        int len = 0;
        len = mPbrRecords.size();
        log("getEfFilesFromUsim" + len);
        efids = new int[len];
        for (int i = 0; i < len; i++) {
            SparseArray<File> files;
            files = mPbrRecords.get(i).mFileIds;
            efids[i] = files.get(USIM_EFADN_TAG).getEfid();
            log("getEfFilesFromUsim" + efids[i]);
        }
        return efids;
    }

    public boolean isPbrFileExisting() {
        log("mIsPbrFileExisting" + mIsPbrFileExisting);
        return mIsPbrFileExisting;
    }

    public boolean isContainAdnInPbr(){
        Rlog.i(TAG, "isContainAdnInPbr" + mIsContainAdnInPbr);
        return mIsContainAdnInPbr;
    }

    public HashMap<Integer, int[]> getRecordsSize() {
        return mRecordsSize;
    }

    public int getGrpCount() {
        return mGrpCount;
    }

    public ArrayList<String> loadAasFromUsim() {
        log("loadAasFromUsim");
        if (!mAasList.isEmpty()) {
            return mAasList;
        }

        if (mAasFileRecord == null) {
            readAasFileAndWait(0);
        }

        if (mAasFileRecord == null) {
            loge("Error: mAasFileRecord file is empty");
            return null;
        }

        int aasSize = mAasFileRecord.size();
        log("getAas size " + aasSize);

        byte[] aasRec = null;
        for (int i = 0; i < aasSize; i++) {
            aasRec = mAasFileRecord.get(i);
            mAasList.add(IccUtils.adnStringFieldToString(aasRec, 0, aasRec.length));
        }
        log("loadAasFromUsim mAasList: " + mAasList);
        return mAasList;
    }

    public void readAasFileAndWait(int recNum) {
        log("readAasFileAndWait recNum " + recNum);
        synchronized (mLock) {
            if (mPbrRecords == null) {
                readPbrFileAndWait();
            }
        }
        if (mPbrRecords == null) {
            loge("Error: Pbr file is empty");
            return;
        }

        SparseArray<File> files;
        files = mPbrRecords.get(recNum).mFileIds;
        if (files == null || files.size() == 0){
            log("readAasFileAndWait  fileIds == null");
            return;
        }

        log("readAasFileAndWait mAasInfoFromPBR !=null fileIds.size()  " + files.size());
        if (files.get(USIM_EFAAS_TAG) != null) {
            SubjectIndexOfAdn records = getSubjectIndex(USIM_SUBJCET_AAS, recNum);
            if (records == null) {
                log("readAasFileAndWait  records == null ");
                return;
            }

            records.record = new HashMap<Integer, ArrayList<byte[]>>();
            if (records.efids == null || records.efids.length == 0) {
                log("readAasFileAndWait  records.efids == null || records.efids.length == 0");
                return;
            }

            mFh.loadEFLinearFixedAll(records.efids[0], obtainMessage(EVENT_AAS_LOAD_DONE));
            synchronized (mLock) {
                try {
                    mLock.wait();
                } catch (InterruptedException e) {
                    Rlog.e(TAG,
                            "Interrupted Exception in readAasFileAndWait");
                }
            }

            if (mAasFileRecord == null) {
                loge("Error: Aas file is empty");
                records.efids[0] = 0;
            } else {
                records.record.put(records.efids[0], mAasFileRecord);
            }

            handleReadFileResult(records);
            setSubjectIndex(USIM_SUBJCET_AAS, recNum, records);
            setSubjectUsedNum(USIM_SUBJCET_AAS, recNum);
        }
    }

    public int findEFAasInfo() {
        synchronized (mLock) {
            if (mPbrRecords == null) {
                readPbrFileAndWait();
            }
        }
        if (mPbrRecords == null) {
            loge("Error: Pbr file is empty");
            return -1;
        }
        SparseArray<File> files;
        files = mPbrRecords.get(0).mFileIds;
        if (files == null || files.size() == 0) {
            log("findEFAasInfo fileIds == null");
            return -1;
        }
        if (files.get(USIM_EFAAS_TAG) != null) {
            return files.get(USIM_EFAAS_TAG).getEfid();
        }
        return 0;
    }

    public void updateAasList(String aas, int aasIndex) {
        if (mAasList.isEmpty())
            return;
        mAasList.set(aasIndex - 1, aas);
        for (String aasStr : mAasList) {
            Rlog.d("dory", "updateAasList aasStr== " + aasStr);
        }
    }

    private String getAas(SubjectIndexOfAdn aasInfo, int adnNum) {
        log("getAas adnNum: " + adnNum);
        adnNum = adnNum - 1;
        String aas = null;
        if (aasInfo == null) {
            return null;
        }
        if (aasInfo.efids == null || aasInfo.efids.length == 0) {
            log("getAas aasInfo.efids == null ||aasInfo.efids.length == 0 ");
            return null;
        }
        log("aasInfo.efids == " + aasInfo.efids);
        aas = getTypeAas(aasInfo, adnNum, aasInfo.efids[0]);
        return aas;
    }

    private String getTypeAas(SubjectIndexOfAdn aasInfo, int adnNum, int efid) {

        String aas = null;
        if (aasInfo == null || aasInfo.record == null) {
            return null;
        }
        mAasFileRecord = aasInfo.record.get(efid);
        if (mAasFileRecord == null) {
            return null;
        }
        log("getTypeAas size " + mAasFileRecord.size());
        aas = readAasRecord(adnNum);
        log("getTypeAas,aas " + aas);

        if (TextUtils.isEmpty(aas)) {
            log("getTypeAas,aas==null");
            return null;
        }
        return aas;
    }

    private String readAasRecord(int recNum) {
        byte[] aasRec = null;
        try {
            aasRec = mAasFileRecord.get(recNum);
        } catch (IndexOutOfBoundsException e) {
            return null;
        }
        String aas = IccUtils.adnStringFieldToString(aasRec, 0, aasRec.length - 2);
        return aas;
    }

    private void setAas(int adnNum, String aas) {
        AdnRecordEx rec = mPhoneBookRecords.get(adnNum);
        log("setAas,rec name:" + rec.getAlphaTag() + "num " + rec.getNumber() + " adnNum " + adnNum);

        if (rec == null && aas != null) {
            rec = new AdnRecordEx("", "");
        }
        if (aas != null) {
            rec.setAas(aas);
            log("setAas AdnRecordEx  aas" + aas);
        }
        mPhoneBookRecords.set(adnNum, rec);
    }

    private void readSneFileAndWait(int recNum) {
        log("readSnelFileAndWait" + "recNum = " + recNum);
        synchronized (mLock) {
            if (mPbrRecords == null) {
                readPbrFileAndWait();
            }
        }
        if (mPbrRecords == null) {
            loge("Error: Pbr file is empty");
            return;
        }
        SparseArray<File> files;
        files = mPbrRecords.get(recNum).mFileIds;
        if (files == null || files.size() == 0)
            return;

        File sne = files.get(USIM_EFEMAIL_TAG);
        if (sne != null) {
            int efid = sne.getEfid();
            log("EF_SNE exists in PBR. efid = 0x" +
                    Integer.toHexString(efid).toUpperCase());

            SubjectIndexOfAdn records = getSubjectIndex(USIM_SUBJCET_SNE, recNum);
            if (records == null) {
                log("readSnelFileAndWait  records == null ");
                return;
            }

            mFh.loadEFLinearFixedAll(efid, obtainMessage(EVENT_SNE_LOAD_DONE));
            try {
                mLock.wait();
            } catch (InterruptedException e) {
                loge("Interrupted Exception in readSneFileAndWait");
            }

            if (mSneFileRecord == null) {
                loge("Error: sne file is empty");
                records = null;
                setSubjectIndex(USIM_SUBJCET_SNE, recNum, records);
                return;
            }

            records.record = new HashMap<Integer, ArrayList<byte[]>>();
            records.record.put(efid, mSneFileRecord);
            log("readSnelFileAndWait recNum " + recNum
                    + "  mSneFileRecord  size " + mSneFileRecord.size());
            setSubjectIndex(USIM_SUBJCET_SNE, recNum, records);
            setSubjectUsedNum(USIM_SUBJCET_SNE, recNum);
            mSneFileRecord = null;
        }
    }

    private String getSne(int num, SubjectIndexOfAdn sneInfo, byte[] record,
            int adnNum) {
        log("getSne sneNum: " + adnNum + "num " + num);

        String sneGroup = null;
        String sne = null;
        if (sneInfo == null || sneInfo.record == null) {
            return null;
        }
        if (sneInfo.efids == null || sneInfo.efids.length == 0) {
            log("getSne sneInfo.efids == null ||sneInfo.efids.length == 0 ");
            return null;
        }
        for (int i = 0; i < sneInfo.efids.length; i++) {
            if (sneInfo.type[i] == USIM_TYPE1_TAG) {
                sne = getType1Sne(num, sneInfo, adnNum, sneInfo.efids[i]);
            }
            if (sneInfo.type[i] == USIM_TYPE2_TAG && sneInfo.recordNumInIap != null) {
                sne = getType2Sne(num, sneInfo, record, adnNum, sneInfo.efids[i]);
            }
            if (i == 0) {
                sneGroup = sne;
            } else {
                sneGroup = sneGroup + ";" + sne;
            }
        }
        return sneGroup;
    }

    private String getType1Sne(int num, SubjectIndexOfAdn sneInfo, int adnNum,
            int efid) {
        String snes = null;
        mSneFileRecord = sneInfo.record.get(efid);
        if (mSneFileRecord == null) {
            return null;
        }
        log("getType1Sne size " + mSneFileRecord.size());
        snes = readSneRecord(adnNum);
        log("getType1Sne,snes " + snes);

        if (TextUtils.isEmpty(snes)) {
            log("getType1Sne,snes==null");
            return null;
        }
        return snes;
    }

    private String getType2Sne(int num, SubjectIndexOfAdn sneInfo,
            byte[] record, int adnNum, int efid) {
        String snes = null;
        int index = -1;
        log(" getType2Sne >>  sneInfo.recordNumInIap.size() "
                + sneInfo.recordNumInIap.size() + " adnNum " + adnNum
                + " efid " + efid);
        if (record == null) {
            return snes;
        }
        index = getUsedNumSetIndex(efid, sneInfo);
        if (index == -1) {
            return snes;
        }

        mSneTagNumberInIap = sneInfo.recordNumInIap.get(efid);
        mSneFileRecord = sneInfo.record.get(efid);
        if (mSneFileRecord == null) {
            return snes;
        }

        int recNum = (int) (record[mSneTagNumberInIap] & 0xFF);
        recNum = ((recNum == 0xFF) ? (-1) : recNum);
        log("getType2Sne  iap recNum == " + recNum);
        if (recNum != -1) {
            snes = readSneRecord(recNum - 1);
            log("getType2Sne,snes " + snes);
            if (TextUtils.isEmpty(snes)) {
                log("getType2Sne,snes ==null");
                setIapFileRecord(num, adnNum, (byte) 0xFF, mSneTagNumberInIap);
                return null;
            }
            Set<Integer> set = (Set<Integer>) sneInfo.usedSet[index];
            log("getType2Sne  size (0)" + set.size() + " index " + index);
            set.add(new Integer(recNum));
            sneInfo.usedSet[index] = set;
            log("getType2Sne  size (1)" + set.size());
            setSubjectIndex(USIM_SUBJCET_SNE, num, sneInfo);
        }
        return snes;
    }

    private String readSneRecord(int recNum) {
        byte[] sneRec = null;
        try {
            sneRec = mSneFileRecord.get(recNum);
        } catch (IndexOutOfBoundsException e) {
            return null;
        }
        // The length of the record is X+2 byte, where X bytes is the sne
        // address
        String sne = IccUtils.adnStringFieldToString(sneRec, 0, sneRec.length - 2);
        return sne;
    }

    private void setSne(int adnNum, String snes) {
        AdnRecordEx rec = mPhoneBookRecords.get(adnNum);
        log("setSne,rec name:" + rec.getAlphaTag() + "num " + rec.getNumber()
                + " adnNum " + adnNum);

        if (rec == null && snes != null) {
            rec = new AdnRecordEx("", "");
        }
        if (snes != null) {
            rec.setSne(snes);
            log("setSne AdnRecordEx  sne" + snes);
        }
        mPhoneBookRecords.set(adnNum, rec);
    }

    public int getSneSize() {
        return mSneEfSize;
    }

    public int[] getSneLength() {
        int[] sneSize = null;
        synchronized (mLock) {
            sneSize = readSneFileSizeAndWait();
        }
        return sneSize;
    }

    private int[] readSneFileSizeAndWait() {
        log("readSneFileSizeAndWait");
        synchronized (mLock) {
            if (mPbrRecords == null) {
                readPbrFileAndWait();
            }
        }
        if (mPbrRecords == null) {
            loge("Error: Pbr file is empty");
            return null;
        }

        SparseArray<File> files;
        files = mPbrRecords.get(0).mFileIds;
        if (files == null || files.size() == 0)
            return null;

        if (files.get(USIM_EFSNE_TAG) == null) {
            return null;
        }

        mFh.getEFLinearRecordSize(files.get(USIM_EFSNE_TAG).getEfid(), obtainMessage(EVENT_SNE_RECORD_COUNT));
        try {
            mLock.wait();
        } catch (InterruptedException e) {
            loge("Interrupted Exception in readSneFileSizeAndWait");
        }
        return sneRecordSize;
    }

    public int findEFSneInfo(int index) {
        synchronized (mLock) {
            if (mPbrRecords == null) {
                readPbrFileAndWait();
            }
        }
        if (mPbrRecords == null) {
            loge("Error: Pbr file is empty");
            return -1;
        }

        SparseArray<File> files;
        files = mPbrRecords.get(index).mFileIds;
        if (files == null || files.size() == 0) {
            log("findEFSNEInfo  fileIds == null  index :" + index);
            return -1;
        }
        if (files.get(USIM_EFSNE_TAG) != null) {
            return files.get(USIM_EFSNE_TAG).getEfid();
        }
        return 0;
    }

    private void loge(String msg) {
        if(DBG) Rlog.e(TAG, msg);
    }

    public void updateUidForAdn(int adnEf, int recNum, int adnIndex, AdnRecordEx adn) {
        if (adn.isEmpty()) {
        } else {
            if (getEfIdByTag(recNum,USIM_EFUID_TAG) <= 0) {
                log("get EfUID failed,EFUID is not exist");
                return;
            }
            mFh.loadEFTransparent(IccConstantsEx.EF_CC, 2,
                    obtainMessage(EVENT_EF_CC_LOAD_DONE, 1,1));
        }
    }
}