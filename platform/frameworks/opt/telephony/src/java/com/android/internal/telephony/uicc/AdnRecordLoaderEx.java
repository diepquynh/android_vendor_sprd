package com.android.internal.telephony.uicc;

import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.AsyncResult;
import android.telephony.Rlog;

import com.android.internal.telephony.IccPhoneBookOperationException;
import com.android.internal.telephony.uicc.IccUtils;

import java.util.ArrayList;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * {@hide}
 */
public class AdnRecordLoaderEx extends Handler {
    final static String LOG_TAG = "AdnRecordLoaderEx";
    final static boolean VDBG = false;

    private IccFileHandler mFh;
    int mEf;
    int mExtensionEF;
    int mPendingExtLoads;
    Message mUserResponse;
    String mPin2;
    int mIapEF;
    int mAdnNum;
    byte[] mIapRec;
    int mFileCount;
    protected final Object mLock = new Object();
    int[] mRecordSize;
    // For "load one"
    int mRecordNumber;

    // for "load all"
    ArrayList<AdnRecordEx> mAdns; // only valid after EVENT_ADN_LOAD_ALL_DONE

    // Either an AdnRecord or a reference to adns depending
    // if this is a load one or load all operation
    Object mResult;

    //***** Event Constants

    static final int EVENT_ADN_LOAD_DONE = 1;
    static final int EVENT_EXT_RECORD_LOAD_DONE = 2;
    static final int EVENT_ADN_LOAD_ALL_DONE = 3;
    static final int EVENT_EF_LINEAR_RECORD_SIZE_DONE = 4;
    static final int EVENT_UPDATE_RECORD_DONE = 5;
    static final int EVENT_UPDATE_ANR_RECORD_DONE = 10;
    static final int EVENT_EF_CYCLIC_LINEAR_RECORD_SIZE_DONE = 18;
    static final int EVENT_UPDATE_EXT_RECORD_DONE = 19;
    static final int EVENT_EXT_LOAD_ALL_DONE = 20;
    static final int EVENT_GET_SIZE_DONE = 21;
    /* SPRD: PhoneBook for AAS {@ */
    static final int EVENT_UPDATE_AAS_RECORD_DONE = 14;
    /* @} */
    /* SPRD: PhoneBook for SNE {@ */
    ArrayList<Integer> sneEfids;
    ArrayList<Integer> sneNums;
    ArrayList<Integer> sneEfIndex;
    static final int EVENT_EF_PBR_SNE_LINEAR_RECORD_SIZE_DONE = 9;
    static final int EVENT_UPDATE_SNE_RECORD_DONE = 15;
    /* @} */
    //***** Constructor

    AdnRecordLoaderEx(IccFileHandler fh) {
        // The telephony unit-test cases may create AdnRecords
        // in secondary threads
        super(Looper.getMainLooper());
        mFh = fh;
    }

    private String getEFPath(int efid) {
        if (efid == IccConstantsEx.EF_ADN) {
            return IccConstantsEx.MF_SIM + IccConstantsEx.DF_TELECOM;
        }

        return null;
    }

    /**
     * Resulting AdnRecord is placed in response.obj.result
     * or response.obj.exception is set
     */
    public void
    loadFromEF(int ef, int extensionEF, int recordNumber,
                Message response) {
        mEf = ef;
        mExtensionEF = extensionEF;
        mRecordNumber = recordNumber;
        mUserResponse = response;

        mFh.loadEFLinearFixed(
               ef, getEFPath(ef), recordNumber,
               obtainMessage(EVENT_ADN_LOAD_DONE));
    }


    /**
     * Resulting ArrayList&lt;adnRecord> is placed in response.obj.result
     * or response.obj.exception is set
     */
    public void
    loadAllFromEF(int ef, int extensionEF,
                Message response) {
        mEf = ef;
        mExtensionEF = extensionEF;
        mUserResponse = response;

        /* If we are loading from EF_ADN, specifically
         * specify the path as well, since, on some cards,
         * the fileid is not unique.
         */
        mFh.loadEFLinearFixedAll(
                ef, getEFPath(ef),
                obtainMessage(EVENT_ADN_LOAD_ALL_DONE));
    }

    /**
     * Write adn to a EF SIM record
     * It will get the record size of EF record and compose hex adn array
     * then write the hex array to EF record
     *
     * @param adn is set with alphaTag and phone number
     * @param ef EF fileid
     * @param extensionEF extension EF fileid
     * @param recordNumber 1-based record index
     * @param pin2 for CHV2 operations, must be null if pin2 is not needed
     * @param response will be sent to its handler when completed
     */
    public void
    updateEF(AdnRecordEx adn, int ef, int extensionEF, int recordNumber,
            String pin2, Message response) {
        mEf = ef;
        mExtensionEF = extensionEF;
        mRecordNumber = recordNumber;
        mUserResponse = response;
        mPin2 = pin2;

        mFh.getEFLinearRecordSize( ef, getEFPath(ef),
                obtainMessage(EVENT_EF_LINEAR_RECORD_SIZE_DONE, adn));
     }

    //***** Overridden from Handler

    @Override
    public void
    handleMessage(Message msg) {
        AsyncResult ar;
        byte data[];
        AdnRecordEx adn;

        try {
            switch (msg.what) {
                case EVENT_EF_LINEAR_RECORD_SIZE_DONE:
                    Rlog.d(LOG_TAG,"AdnRecordLoaderEx handle EVENT_EF_LINEAR_RECORD_SIZE_DONE, ef:0x"
                                + Integer.toHexString(mEf));
                    ar = (AsyncResult)(msg.obj);
                    adn = (AdnRecordEx)(ar.userObj);

                    if (ar.exception != null) {
                        throw new IccPhoneBookOperationException(
                                IccPhoneBookOperationException.WRITE_OPREATION_FAILED,
                                "get EF record size failed", ar.exception);
                    }

                    int[] recordSize = (int[])ar.result;
                    if (recordSize.length != 3 || mRecordNumber > recordSize[2]) {
                        throw new IccPhoneBookOperationException(
                               IccPhoneBookOperationException.WRITE_OPREATION_FAILED,
                               "get wrong EF record size format", ar.exception);
                    }
                    Rlog.d(LOG_TAG, "extensionEF = " + mExtensionEF);
                    try {
                       data = adn.buildAdnString(recordSize[0]);
                    } catch (IccPhoneBookOperationException e) {
                       // TODO: handle exception
                       throw e;
                    }
                    Rlog.i("AdnRecordLoaderEx", "recordNumber " + mRecordNumber + "adn.ext"
                           + adn.mExtRecord);
                    if(data == null) {
                        throw new RuntimeException("wrong ADN format", ar.exception);
                    }
                    mFh.updateEFLinearFixed(mEf, mRecordNumber,
                           data, mPin2, obtainMessage(EVENT_UPDATE_RECORD_DONE));
                    mPendingExtLoads = 1;

                    break;
                case EVENT_UPDATE_RECORD_DONE:
                    ar = (AsyncResult)(msg.obj);
                    if (ar.exception != null) {
                        throw new RuntimeException("update EF adn record failed",
                                ar.exception);
                    }
                    mPendingExtLoads = 0;
                    mResult = null;
                    break;
                case EVENT_ADN_LOAD_DONE:
                    Rlog.d(LOG_TAG,"AdnRecordLoaderEx handle EVENT_ADN_LOAD_DONE");
                    ar = (AsyncResult)(msg.obj);
                    data = (byte[])(ar.result);

                    if (ar.exception != null) {
                        throw new IccPhoneBookOperationException(IccPhoneBookOperationException.LOAD_ADN_FAIL,
                                "adn load failed", ar.exception);
                    }

                    if (VDBG) {
                        Rlog.d(LOG_TAG,"ADN EF: 0x"
                            + Integer.toHexString(mEf)
                            + ":" + mRecordNumber
                            + "\n" + IccUtils.bytesToHexString(data));
                    }

                    adn = new AdnRecordEx(mEf, mRecordNumber, data);
                    mResult = adn;

                    if (adn.hasExtendedRecord()) {
                        // If we have a valid value in the ext record field,
                        // we're not done yet: we need to read the corresponding
                        // ext record and append it

                        mPendingExtLoads = 1;

                        mFh.loadEFLinearFixed(
                            mExtensionEF, adn.mExtRecord,
                            obtainMessage(EVENT_EXT_RECORD_LOAD_DONE, adn));
                    }
                break;

                case EVENT_EXT_RECORD_LOAD_DONE:
                    Rlog.d(LOG_TAG,"AdnRecordLoaderEx handle EVENT_EXT_RECORD_LOAD_DONE");
                    ar = (AsyncResult)(msg.obj);
                    data = (byte[])(ar.result);
                    adn = (AdnRecordEx)(ar.userObj);

                    if (ar.exception == null) {
                        Rlog.d(LOG_TAG,"ADN extension EF: 0x"
                                + Integer.toHexString(mExtensionEF)
                                + ":" + adn.mExtRecord
                                + "\n" + IccUtils.bytesToHexString(data));

                        adn.appendExtRecord(data);
                    } else {
                        // If we can't get the rest of the number from EF_EXT1, rather than
                        // providing the partial number, we clear the number since it's not
                        // dialable anyway. Do not throw exception here otherwise the rest
                        // of the good records will be dropped.

                        Rlog.e(LOG_TAG, "Failed to read ext record. Clear the number now.");
                        adn.setNumber("");
                    }

                    mPendingExtLoads--;
                    // result should have been set in
                    // EVENT_ADN_LOAD_DONE or EVENT_ADN_LOAD_ALL_DONE
                break;

                case EVENT_ADN_LOAD_ALL_DONE:
                    Rlog.d(LOG_TAG,"AdnRecordLoaderEx handle EVENT_ADN_LOAD_ALL_DONE");
                    ar = (AsyncResult)(msg.obj);
                    ArrayList<byte[]> datas = (ArrayList<byte[]>)(ar.result);

                    if (ar.exception != null) {
                        throw new RuntimeException("load failed", ar.exception);
                    }

                    mAdns = new ArrayList<AdnRecordEx>(datas.size());
                    mResult = mAdns;
                    mPendingExtLoads = 0;

                    for(int i = 0, s = datas.size() ; i < s ; i++) {
                        adn = new AdnRecordEx(mEf, 1 + i, datas.get(i));
                        mAdns.add(adn);

                        if (adn.hasExtendedRecord()) {
                            // If we have a valid value in the ext record field,
                            // we're not done yet: we need to read the corresponding
                            // ext record and append it

                            mPendingExtLoads++;

                            mFh.loadEFLinearFixed(
                                mExtensionEF, adn.mExtRecord,
                                obtainMessage(EVENT_EXT_RECORD_LOAD_DONE, adn));
                        }
                    }
                break;
                case EVENT_EXT_LOAD_ALL_DONE:
                    Rlog.d(LOG_TAG,"AdnRecordLoaderEx handle EVENT_EXT_LOAD_ALL_DONE");
                    ar = (AsyncResult) (msg.obj);
                    ArrayList<byte[]> extDatas = (ArrayList<byte[]>) (ar.result);
                    Rlog.d(LOG_TAG, "EVENT_EXT_LOAD_ALL_DONE:" + mExtensionEF + "extDatas" + extDatas);
                    if (ar.exception != null) {
                        throw new IccPhoneBookOperationException(
                                IccPhoneBookOperationException.LOAD_ADN_FAIL,
                                "load all ext failed", ar.exception);
                    }
                    mResult = extDatas;
                    break;
                case EVENT_EF_CYCLIC_LINEAR_RECORD_SIZE_DONE:
                    Rlog.d(LOG_TAG,"AdnRecordLoaderEx handle EVENT_EF_CYCLIC_LINEAR_RECORD_SIZE_DONE, ef:0x" + Integer.toHexString(mEf));
                    ar = (AsyncResult)(msg.obj);
                    adn = (AdnRecordEx)(ar.userObj);
                    if (ar.exception != null) {
                        throw new IccPhoneBookOperationException(
                                IccPhoneBookOperationException.WRITE_OPREATION_FAILED,
                               "get EF record size failed",
                                ar.exception);
                    }

                    int[] CyclicrecordSize = (int[])ar.result;
                    // recordSize is int[3] array
                    // int[0]  is the record length
                    // int[1]  is the total length of the EF file
                    // int[2]  is the number of records in the EF file
                    // So int[0] * int[2] = int[1]
                   if (CyclicrecordSize.length != 3 || mRecordNumber > CyclicrecordSize[2]) {
                        throw new IccPhoneBookOperationException(
                               IccPhoneBookOperationException.WRITE_OPREATION_FAILED,
                               "get wrong EF record size format",
                               ar.exception);
                    }
                    try {
                        data = adn.buildAdnString(CyclicrecordSize[0]);
                    } catch (IccPhoneBookOperationException e) {
                        // TODO: handle exception
                        throw e;
                    }
                    Rlog.i("AdnRecordLoaderEx", "recordNumber " + mRecordNumber);
                    mFh.updateEFCYCLICLinearFixed(mEf, mRecordNumber,
                            data, mPin2, obtainMessage(EVENT_UPDATE_RECORD_DONE));
                    mPendingExtLoads = 1;

                    break;
                case EVENT_UPDATE_ANR_RECORD_DONE:
                    Rlog.d(LOG_TAG,"AdnRecordLoaderEx handle EVENT_UPDATE_ANR_RECORD_DONE");
                    ar = (AsyncResult) (msg.obj);
                    if (ar.exception != null) {
                        throw new IccPhoneBookOperationException(
                                IccPhoneBookOperationException.WRITE_OPREATION_FAILED,
                                "update EF Anr record failed",
                                ar.exception);
                    }
                    Rlog.e(LOG_TAG, "EVENT_UPDATE_ANR_RECORD_DONE, message is "
                            + msg.toString() + "fileCount " + mFileCount);
                    mPendingExtLoads = 0;
                    mResult = null;
                    mFileCount--;
                    if (mFileCount == 0)
                        break;
                    else
                        return;
                case EVENT_UPDATE_EXT_RECORD_DONE:
                    Rlog.d(LOG_TAG,"AdnRecordLoaderEx handle EVENT_UPDATE_EXT_RECORD_DONE");
                    ar = (AsyncResult) (msg.obj);
                    mExtensionEF = msg.arg1;
                    int index = msg.arg2;
                    Rlog.d(LOG_TAG, "EVENT_UPDATE_EXT_RECORD_DONE extensionEF =" + mExtensionEF  + " index = " + index);
                    if (ar.exception != null) {
                        throw new IccPhoneBookOperationException(
                                IccPhoneBookOperationException.WRITE_OPREATION_FAILED,
                                "update EF ext record failed",
                                ar.exception);
                    }
                    mResult = ar.userObj;
                    mPendingExtLoads = 0;
                    break;
                case EVENT_GET_SIZE_DONE:
                    Rlog.d(LOG_TAG,"AdnRecordLoaderEx handle EVENT_GET_SIZE_DONE");
                    ar = (AsyncResult) msg.obj;
                    synchronized (mLock) {
                        if (ar.exception == null) {
                            mRecordSize = (int[])ar.result;
                            // recordSize[0]  is the record length
                            // recordSize[1]  is the total length of the EF file
                            // recordSize[2]  is the number of records in the EF file
                            Rlog.d(LOG_TAG, "GET_RECORD_SIZE Size " + mRecordSize[0] +
                                    " total " + mRecordSize[1] +
                                    " #record " + mRecordSize[2]);
                        }
                        notifyPending(ar);
                    }
                    break;
                 /* SPRD: PhoneBook for AAS {@ */
                 case EVENT_UPDATE_AAS_RECORD_DONE:
                     ar = (AsyncResult)(msg.obj);
                     if (ar.exception != null) {
                          throw new IccPhoneBookOperationException(
                                 IccPhoneBookOperationException.WRITE_OPREATION_FAILED,
                                 "update EF adn record failed",
                                 ar.exception);
                     }
                     mPendingExtLoads = 0;
                     break;
                 /*@}*/
                 /* SPRD: PhoneBook for SNE {@ */
                 case EVENT_UPDATE_SNE_RECORD_DONE:
                      ar = (AsyncResult)(msg.obj);
                      if (ar.exception != null) {
                            throw new IccPhoneBookOperationException(
                                    IccPhoneBookOperationException.WRITE_OPREATION_FAILED,
                                    "update EF adn record failed",
                                    ar.exception);
                      }
                      mPendingExtLoads = 0;
                      break;
                 case EVENT_EF_PBR_SNE_LINEAR_RECORD_SIZE_DONE:
                      Rlog.d(LOG_TAG,"EVENT_EF_PBR_SNE_LINEAR_RECORD_SIZE_DONE sneNum :" + sneNums);
                      ar = (AsyncResult) (msg.obj);
                      adn = (AdnRecordEx) (ar.userObj);
                      if (ar.exception != null) {
                             throw new IccPhoneBookOperationException(
                                   IccPhoneBookOperationException.WRITE_OPREATION_FAILED,
                                   "get EF record size failed",ar.exception);
                      }
                      recordSize = (int[]) ar.result;
                      // recordSize is int[3] array
                      // int[0] is the record length
                      // int[1] is the total length of the EF file
                      // int[2] is the number of records in the EF file
                      // So int[0] * int[2] = int[1]
                      Rlog.d(LOG_TAG,"EVENT_EF_PBR_SNE_LINEAR_RECORD_SIZE_DONE (1) :  adnNum " + mAdnNum + " number " + recordSize[2]);
                      if (recordSize.length != 3) {
                            throw new IccPhoneBookOperationException(
                                    IccPhoneBookOperationException.WRITE_OPREATION_FAILED,
                                    "get wrong EF record size format", ar.exception);
                      }
                      Rlog.d(LOG_TAG,"EVENT_EF_PBR_SNE_LINEAR_RECORD_SIZE_DONE (2) :");
                      int fileCount = 0;
                      for (int i = 0, size = sneEfids.size(); i < size; i++) {
                           Rlog.e("GSM", "efids.get(" + i + ") is " + sneEfids.get(i) + " number " + sneNums.get(i) );
                           if (sneEfids.get(i) != 0 &&sneNums.get(i)!=0 ) {
                               fileCount++;
                               data = adn.buildSneString(recordSize[0], sneEfIndex.get(i), mEf, mAdnNum);
                               if (data == null) {
                                   throw new IccPhoneBookOperationException(
                                           IccPhoneBookOperationException.WRITE_OPREATION_FAILED,
                                           "wrong ADN format", ar.exception);
                                }
                                mFh.updateEFLinearFixed(sneEfids.get(i), sneNums.get(i), data,
                                        mPin2, obtainMessage(EVENT_UPDATE_SNE_RECORD_DONE));
                           }
                       }
                       mPendingExtLoads = 1;
                       break;
                    /* @} */
                default:
                    super.handleMessage(msg);
                    return;
            }
        } catch (IccPhoneBookOperationException exc) {
            if (mUserResponse != null) {
                AsyncResult.forMessage(mUserResponse).exception = exc;
                mUserResponse.sendToTarget();
                // Loading is all or nothing--either every load succeeds
                // or we fail the whole thing.
                mUserResponse = null;
            }
            return;
        }catch (Exception e) {
            e.printStackTrace();
            IccPhoneBookOperationException exc = new IccPhoneBookOperationException(
                    IccPhoneBookOperationException.WRITE_OPREATION_FAILED, e.getMessage());
            if (mUserResponse != null) {
                AsyncResult.forMessage(mUserResponse).exception = exc;
                mUserResponse.sendToTarget();
                // Loading is all or nothing--either every load succeeds
                // or we fail the whole thing.
                mUserResponse = null;
            }
            return;
        }

        if (mUserResponse != null && mPendingExtLoads == 0) {
            AsyncResult.forMessage(mUserResponse).result = mResult;

            mUserResponse.sendToTarget();
            mUserResponse = null;
        }
    }
    /**
     * Resulting ArrayList&lt;adnRecord> is placed in response.obj.result
     * or response.obj.exception is set
     */
    public void
    loadAllExtFromEF(int ef, int extensionEF,
                Message response) {
        mEf = ef;
        mExtensionEF = extensionEF;
        mUserResponse = response;
        mFh.loadEFLinearFixedAll(
                extensionEF,
                obtainMessage(EVENT_EXT_LOAD_ALL_DONE,ef,extensionEF));
    }

    public void updateEFGrpToUsim(int grpEfId, int recordNum, byte[] data, String pin2) {

        mFh.updateEFLinearFixed(grpEfId, recordNum, data, pin2,
                obtainMessage(EVENT_UPDATE_RECORD_DONE));
    }

    public void updateEFCyclic(AdnRecordEx adn, int ef, int extensionEF, int recordNumber,
            String pin2, Message response) {
        mEf = ef;
        mExtensionEF = extensionEF;
        mRecordNumber = recordNumber;
        mUserResponse = response;
        mPin2 = pin2;
        mFh.getEFLinearRecordSize(ef, obtainMessage(EVENT_EF_CYCLIC_LINEAR_RECORD_SIZE_DONE, adn));
    }

    public void updateEFEmailToUsim(AdnRecordEx adn, ArrayList<Integer> emailEfids,
       ArrayList<Integer> emailNums,int efid,int adnNum,ArrayList<Integer> emailEfIndex,
       String pin2,Message response,int[] size) {
       Rlog.i("AdnRecordLoaderEx ","updateEFEmailToUsim  emailEfids " +emailEfids
                    + "emailNums "+emailNums + "emailEfIndex " + emailEfIndex);
       mUserResponse = response;

       byte[] data;
       mFileCount = 0;
       if (size == null) {
           throw new IccPhoneBookOperationException(
                   IccPhoneBookOperationException.WRITE_OPREATION_FAILED,
                   "size is null");
       }
       for (int i = 0, count = emailEfids.size(); i < count; i++) {
           Rlog.e("GSM", "efids.get(" + i + ") is " + emailEfids.get(i) + " number " + emailNums.get(i) );
           if (emailEfids.get(i) != 0 &&emailNums.get(i)!=0 ) {
               mFileCount++;
               data = adn.buildEmailString(size[0], emailEfIndex.get(i), mEf, adnNum);
               if (data == null) {
                    throw new IccPhoneBookOperationException(
                            IccPhoneBookOperationException.WRITE_OPREATION_FAILED,
                            "wrong ADN format");
               }
               mFh.updateEFLinearFixed(emailEfids.get(i), emailNums.get(i), data,
                       pin2, obtainMessage(EVENT_UPDATE_RECORD_DONE));
           }
       }
       mPendingExtLoads = 1;
   }

    public void updateEFAnrToUsim(AdnRecordEx adn, ArrayList<Integer> anrefids,
            int efid, int adnNum, ArrayList<Integer> anrNums,  ArrayList<Integer> anrEfIndex,
            String pin2, Message response,int[] size,/* SPRD: PhoneBook for AAS*/int aasIndex) {
        mUserResponse = response;
        Rlog.e("GSM", "anrefids = " + anrefids);
        Rlog.e("GSM", "anrefids.size is " + anrefids.size()
                + "   ADNefid == ef:" + mEf);
        byte[] data;
        mFileCount = 0;
        if (size == null) {
            throw new IccPhoneBookOperationException(
                    IccPhoneBookOperationException.WRITE_OPREATION_FAILED,
                    "size is null");
        }
        for (int i = 0, count = anrefids.size(); i < count; i++) {
            Rlog.e("GSM", "efids.get(" + i + ") is " + anrefids.get(i) + " number " + anrNums.get(i));

            if (anrefids.get(i)!=0 && anrNums.get(i)!=0) {
                mFileCount++;
                try {
                    data = adn.buildAnrString(size[0], anrEfIndex.get(i), mEf,
                                adnNum,/* SPRD: PhoneBook for AAS*/aasIndex);
                } catch (IccPhoneBookOperationException e) {
                    // TODO: handle exception
                    if (mUserResponse != null) {
                        AsyncResult.forMessage(mUserResponse).exception = e;
                        mUserResponse.sendToTarget();
                        mUserResponse = null;
                    }
                    return;
                }
                mFh.updateEFLinearFixed(anrefids.get(i),
                        anrNums.get(i), data, pin2,
                        obtainMessage(EVENT_UPDATE_ANR_RECORD_DONE));
            }
        }
        mPendingExtLoads = 1;
     }

    public void updateEFIapToUsim(AdnRecordEx adn, int iapEF, int adnNum,
            byte[] record, String pin2, Message response, int[] size) {
         Rlog.i(LOG_TAG,"updateEFIapToUsim  iapEF " +iapEF );
         byte[] data;
        mIapEF = iapEF;
        mAdnNum = adnNum;
        mIapRec = record;
        mUserResponse = response;
        mPin2 = pin2;
        if (record != null) {
            data = mIapRec;
        } else {
            if (size == null) {
                throw new IccPhoneBookOperationException(
                        IccPhoneBookOperationException.WRITE_OPREATION_FAILED,
                        "size is null");
            }
            data = adn.buildIapString(size[0], 0xff);
        }
        if (data == null) {
            throw new IccPhoneBookOperationException(
                    IccPhoneBookOperationException.WRITE_OPREATION_FAILED,
                    "wrong Iap format");
        }
        mFh.updateEFLinearFixed(iapEF, adnNum, data, pin2,
                obtainMessage(EVENT_UPDATE_RECORD_DONE));
        mPendingExtLoads = 1;

    }

    public void
    updateExtEF(AdnRecordEx adn, int ef, int extensionEF, int recordNumber,
            String pin2, Message response) {
        mEf = ef;
        mExtensionEF = extensionEF;
        mRecordNumber = recordNumber;
        mUserResponse = response;
        mPin2 = pin2;
        byte[] extData = adn.buildExtString();
        mPendingExtLoads = 1;
        mFh.updateEFLinearFixed(extensionEF,recordNumber ,
                extData, pin2, obtainMessage(EVENT_UPDATE_EXT_RECORD_DONE,
                extensionEF,recordNumber,extData));
    }

    public void updateEFAdnToUsim(AdnRecordEx adn, int ef, int extensionEF,
            int recordNumber, String pin2, Message response,int[] size) {
        mUserResponse = response;

        byte[] data;
        if (size == null) {
            throw new IccPhoneBookOperationException(
                    IccPhoneBookOperationException.WRITE_OPREATION_FAILED,
                    "size is null");
        }
        try {
            data = adn.buildAdnString(size[0]);
        } catch (IccPhoneBookOperationException e) {
            // TODO: handle exception
            if (mUserResponse != null) {
                AsyncResult.forMessage(mUserResponse)
                                .exception = e;
                mUserResponse.sendToTarget();
                mUserResponse = null;
            }
            return;
        }
        Rlog.i("AdnRecordLoaderEx", "recordNumber " + recordNumber + "adn.ext"
                + adn.mExtRecord);
        mFh.updateEFLinearFixed(ef, recordNumber,
                data, pin2, obtainMessage(EVENT_UPDATE_RECORD_DONE));
        mPendingExtLoads = 1;
    }

    public void updateEFGasToUsim(int gasEfId, int recordNum, byte[] data, String pin2) {

        mFh.updateEFLinearFixed(gasEfId, recordNum, data, pin2,
                obtainMessage(EVENT_UPDATE_RECORD_DONE));
    }

    public int[] getRecordsSize(int efid){
        Rlog.d(LOG_TAG, "getRecordsSize: efid=" + Integer.toHexString(efid));
        if(efid <= 0) {
            Rlog.d(LOG_TAG, "the efid is invalid");
            return null;
        }
        synchronized (mLock) {
            mRecordSize = new int[3];

            // Using mBaseHandler, no difference in EVENT_GET_SIZE_DONE handling
            AtomicBoolean status = new AtomicBoolean(false);
            Message response = obtainMessage(EVENT_GET_SIZE_DONE, status);
            mFh.getEFLinearRecordSize(efid, response);
            waitForResult(status);
        }
        return mRecordSize;
    }

    protected void waitForResult(AtomicBoolean status) {
        while (!status.get()) {
            try {
                mLock.wait();
            } catch (InterruptedException e) {
                Rlog.d(LOG_TAG, "interrupted while trying to update by search");
            }
        }
    }

    private void notifyPending(AsyncResult ar) {
        if (ar.userObj == null) {
            return;
        }
        AtomicBoolean status = (AtomicBoolean) ar.userObj;
        status.set(true);
        mLock.notifyAll();
    }

    /* SPRD: PhoneBook for SNE {@ */
    public void updateEFSneToUsim(AdnRecordEx adn, ArrayList<Integer> sneEfids,
            ArrayList<Integer> sneNums,int efid,int adnNum,ArrayList<Integer> sneEfIndex, String pin2,Message response) {
        this.sneEfids = sneEfids;
        this.sneNums = sneNums;
        mEf = efid;
        mAdnNum = adnNum;
        this.sneEfIndex = sneEfIndex;
        mUserResponse = response;
        mPin2 = pin2;
        mFh.getEFLinearRecordSize(sneEfids.get(0), obtainMessage(
                EVENT_EF_PBR_SNE_LINEAR_RECORD_SIZE_DONE, adn));
    }
    /* @} */
}
