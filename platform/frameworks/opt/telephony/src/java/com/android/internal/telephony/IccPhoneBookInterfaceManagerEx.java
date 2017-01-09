package com.android.internal.telephony;

import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.List;
import java.util.ArrayList;

import android.os.AsyncResult;
import android.os.Looper;
import android.os.HandlerThread;
import android.os.Handler;
import android.content.pm.PackageManager;
import android.os.Message;
import android.telephony.Rlog;

import com.android.internal.telephony.uicc.IccRecords;
import com.android.internal.telephony.uicc.IccCardApplicationStatus.AppType;
import com.android.internal.telephony.uicc.IccCardApplicationStatus;
import com.android.internal.telephony.IccPhoneBookOperationException;
import com.android.internal.telephony.uicc.AdnRecordEx;
import com.android.internal.telephony.uicc.IccConstantsEx;
import com.android.internal.telephony.gsm.UsimPhoneBookManager;
/**
 * {@hide}
 */
public class IccPhoneBookInterfaceManagerEx extends IccPhoneBookInterfaceManager {
    private int mSimIndex = -1;
    private List<AdnRecordEx> mRecordsEx;

    private HandlerThread mUpdateThread;
    private UpdateThreadHandler mBaseHandler;
    private Handler mUIHandler = new Handler();

    private boolean mReadAdnRecordSuccess = false;
    private boolean mReadFdnRecordSuccess = false;
    private boolean mReadSdnRecordSuccess = false;
    private boolean mReadLndRecordSuccess = false;

    public IccPhoneBookInterfaceManagerEx(Phone phone) {
        super(phone);
        createUpdateThread();
    }

    protected class UpdateThreadHandler extends Handler {
        public UpdateThreadHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            AsyncResult ar;

            switch (msg.what) {
            case EVENT_GET_SIZE_DONE:
                ar = (AsyncResult) msg.obj;
                synchronized (mLock) {
                    if (ar.exception == null) {
                        mRecordSize = (int[]) ar.result;
                        // recordSize[0] is the record length
                        // recordSize[1] is the total length of the EF file
                        // recordSize[2] is the number of records in the EF file
                        logd("GET_RECORD_SIZE Size " + mRecordSize[0]
                                + " total " + mRecordSize[1] + " #record "
                                + mRecordSize[2]);
                    }
                    notifyPending(ar);
                }
                break;
            case EVENT_UPDATE_DONE:
                ar = (AsyncResult) msg.obj;
                synchronized (mLock) {
                    mSuccess = (ar.exception == null);
                    logd("EVENT_UPDATE_DONE  mSuccess " + mSuccess);
                    if (mSuccess) {
                        mSimIndex = getInsertIndex();
                    } else {
                        loge("[EVENT_UPDATE_DONE] exception = " + ar.exception);
                        if (ar.exception instanceof IccPhoneBookOperationException) {
                            mSimIndex = ((IccPhoneBookOperationException) ar.exception).mErrorCode;
                        } else {
                            mSimIndex = -1;
                        }
                    }
                    loge("EVENT_UPDATE_DONE  mSimIndex " + mSimIndex);
                    notifyPending(ar);
                }
                break;
            case EVENT_LOAD_DONE:
                ar = (AsyncResult) msg.obj;
                synchronized (mLock) {
                    logd("EVENT_LOAD_DONE, ar.exception = " + ar.exception);
                    if (ar.exception == null) {
                        logd("EVENT_LOAD_DONE, msg.arg1 = " + msg.arg1);
                        setReadRecordOfEfid(msg.arg1, true);
                        mRecordsEx = (List<AdnRecordEx>) ar.result;
                    } else {
                        logd("Cannot load ADN records");
                        setReadRecordOfEfid(msg.arg1, false);
                    }
                    notifyPending(ar);
                }
                break;
            }
        }

        private void notifyPending(AsyncResult ar) {
            if (ar.userObj != null) {
                AtomicBoolean status = (AtomicBoolean) ar.userObj;
                status.set(true);
            }
            mLock.notifyAll();
        }
    };

    private void createUpdateThread() {
        mUpdateThread = new HandlerThread("RunningState:Background");
        mUpdateThread.start();
        mBaseHandler = new UpdateThreadHandler(mUpdateThread.getLooper());
    }

    public void updateIccRecords(IccRecords iccRecords) {
        super.updateIccRecords(iccRecords);
    }

    private int updateEfForIccType(int efid) {
        boolean isPbrFileExisting = true;
        boolean isContainAdnInPbr = true;
        if (mAdnCache != null && mAdnCache.getUsimPhoneBookManager() != null) {
            isPbrFileExisting = mAdnCache.getUsimPhoneBookManager().isPbrFileExisting();
            isContainAdnInPbr = mAdnCache.getUsimPhoneBookManager().isContainAdnInPbr();
        }
        // Check if we are trying to read ADN records
        if (efid == IccConstantsEx.EF_ADN) {
            logd("isPbrFileExisting = " + isPbrFileExisting + "isContainAdnInPbr" + isContainAdnInPbr);
            if (mPhone.getCurrentUiccAppType() == AppType.APPTYPE_USIM && isPbrFileExisting
                    && isContainAdnInPbr) {
                return IccConstantsEx.EF_PBR;
            }
        }
        return efid;
    }

    public List<AdnRecordEx> getAdnRecordsInEfEx(int efid) {

        if (mPhone.getContext().checkCallingOrSelfPermission(
                android.Manifest.permission.READ_CONTACTS) != PackageManager.PERMISSION_GRANTED) {
            throw new SecurityException(
                    "Requires android.permission.READ_CONTACTS permission");
        }

        efid = updateEfForIccType(efid);
        logd("getAdnRecordsInEFEx: efid = 0x" + Integer.toHexString(efid).toUpperCase());

        synchronized (mLock) {
            setReadRecordOfEfid(efid, false);
            AtomicBoolean status = new AtomicBoolean(false);
            Message response = mBaseHandler.obtainMessage(EVENT_LOAD_DONE, efid, 0, status);
            logd("requestLoadAllAdnLike  efid = 0x" + Integer.toHexString(efid).toUpperCase());
            if (mAdnCache != null) {
                mAdnCache.requestLoadAllAdnLike(efid, mAdnCache.extensionEfForEf(efid), response);
                waitForResult(status);
            } else {
                loge("Failure while trying to load from SIM due to uninitialised adncache");
            }
        }
        if (!(getReadRecordOfEfid(efid))) {
            logd("mReadRecordSuccess = false efid = 0x" + Integer.toHexString(efid).toUpperCase());
            return null;
        }
        logd("getAdnRecordsInEFEx success :efid = 0x" + Integer.toHexString(efid).toUpperCase());
        return mRecordsEx;
    }

    @Override
    public int[] getAdnRecordsSize(int efid) {
        logd("getAdnRecordsSize");
        if (mPhone.getCurrentUiccAppType() == AppType.APPTYPE_USIM && (efid == IccConstantsEx.EF_ADN)) {
            int[] size = getUsimAdnRecordsSize();
            if (null == size) {
                size = getRecordsSize(efid);
            }
            return size;
        } else {
            return getRecordsSize(efid);
        }
    }

    public int[] getRecordsSize(int efid) {
        logd("getRecordsSize: efid = 0x" + Integer.toHexString(efid).toUpperCase());

        if (efid <= 0) {
            loge("the efid is invalid");
            return null;
        }
        synchronized (mLock) {
            checkThread();
            mRecordSize = new int[3];
            AtomicBoolean status = new AtomicBoolean(false);
            Message response = mBaseHandler.obtainMessage(EVENT_GET_SIZE_DONE, status);

            if (mPhone.getIccFileHandler() != null) {
                mPhone.getIccFileHandler().getEFLinearRecordSize(efid, response);
            }
            waitForResult(status);
        }

        return mRecordSize;
    }

    @Override
    public boolean updateAdnRecordsInEfBySearch(int efid, String oldTag,
            String oldPhoneNumber, String newTag, String newPhoneNumber,
            String pin2) {

        if (mPhone.getContext().checkCallingOrSelfPermission(
                android.Manifest.permission.WRITE_CONTACTS) != PackageManager.PERMISSION_GRANTED) {
            throw new SecurityException(
                    "Requires android.permission.WRITE_CONTACTS permission");
        }

        logd("updateAdnRecordsInEfBySearch: efid = 0x" + Integer.toHexString(efid).toUpperCase() 
                + " (" + oldTag + "," + oldPhoneNumber + ")" + "==>" + " (" + newTag + ","
                + newPhoneNumber + ")" + " pin2=" + pin2);

        efid = updateEfForIccType(efid);
        synchronized (mLock) {
            checkThread();
            mSuccess = false;
            AtomicBoolean status = new AtomicBoolean(false);
            Message response = mBaseHandler.obtainMessage(EVENT_UPDATE_DONE, status);

            AdnRecordEx oldAdn = new AdnRecordEx(oldTag, oldPhoneNumber);
            AdnRecordEx newAdn = new AdnRecordEx(newTag, newPhoneNumber);
            if (mAdnCache != null) {
                mAdnCache.updateAdnBySearchEx(efid, oldAdn, newAdn, pin2, response);
                waitForResult(status);
            } else {
                loge("Failure while trying to update by search due to uninitialised adncache");
            }
        }
        return mSuccess;
    }

    public int updateAdnRecordsInEfBySearch(int efid, String oldTag,
            String oldPhoneNumber, String[] oldEmailList, String oldAnr,
            String oldSne, String oldGrp, String newTag, String newPhoneNumber,
            String[] newEmailList, String newAnr, String newAas, String newSne,
            String newGrp, String newGas, String pin2) {

        if (mPhone.getContext().checkCallingOrSelfPermission(
                android.Manifest.permission.WRITE_CONTACTS) != PackageManager.PERMISSION_GRANTED) {
            throw new SecurityException(
                    "Requires android.permission.WRITE_CONTACTS permission");
        }
        logd("updateAdnRecordsInEfBySearchEx: efid = 0x" + Integer.toHexString(efid).toUpperCase() 
                + " (" + newTag + "," + newPhoneNumber + ")" + " pin2=" + pin2);

        int newid = updateEfForIccType(efid);

        synchronized (mLock) {
            checkThread();
            mSuccess = false;
            AtomicBoolean status = new AtomicBoolean(false);
            Message response = mBaseHandler.obtainMessage(EVENT_UPDATE_DONE, status);
            AdnRecordEx oldAdn = null;
            AdnRecordEx newAdn = null;
            if (mAdnCache == null) {
                loge("updateAdnRecordsInEfBySearchEx failed because mAdnCache is null");
                return mSimIndex;
            }
            if (newid == IccConstantsEx.EF_LND) {
                logd("insertLNDRecord: efid = 0x" + Integer.toHexString(efid).toUpperCase()
                        + " (" + newTag + "," + newPhoneNumber + ")" + " pin2=" + pin2);
                oldAdn = new AdnRecordEx(oldTag, oldPhoneNumber);
                newAdn = new AdnRecordEx(newTag, newPhoneNumber);
                mAdnCache.insertLndBySearch(newid, oldAdn, newAdn, pin2, response);
            } else if (newid == IccConstantsEx.EF_PBR) {
                oldAdn = new AdnRecordEx(oldTag, oldPhoneNumber, oldEmailList,
                        oldAnr, "", oldSne, oldGrp, "");
                newAdn = new AdnRecordEx(newTag, newPhoneNumber, newEmailList,
                        newAnr, newAas, newSne, newGrp, newGas);

                mAdnCache.updateUSIMAdnBySearch(newid, oldAdn, newAdn, pin2,
                        response);

            } else {
                oldAdn = new AdnRecordEx(oldTag, oldPhoneNumber);
                newAdn = new AdnRecordEx(newTag, newPhoneNumber);
                mAdnCache.updateAdnBySearchEx(newid, oldAdn, newAdn, pin2,
                        response);
            }
            waitForResult(status);
        }
        logd("updateAdnRecordsInEfBySearch end " + mSuccess + ", mSimIndex " + mSimIndex);
        return mSimIndex;
    }

    public int updateAdnRecordsInEfByIndex(int efid, String newTag,
            String newPhoneNumber, String[] newEmailList, String newAnr,
            String newAas, String newSne, String newGrp, String newGas,
            int index, String pin2) {

        if (mPhone.getContext().checkCallingOrSelfPermission(
                android.Manifest.permission.WRITE_CONTACTS) != PackageManager.PERMISSION_GRANTED) {
            throw new SecurityException(
                    "Requires android.permission.WRITE_CONTACTS permission");
        }

        logd("updateAdnRecordsInEfByIndexEx: efid = 0x" + Integer.toHexString(efid).toUpperCase()
                + " (" + newTag + "," + newPhoneNumber + ")" + " index=" + index);

        int newid = updateEfForIccType(efid);

        logd("updateAdnRecordsInEfByIndexEx: newid = 0x" + Integer.toHexString(newid).toUpperCase());

        synchronized (mLock) {
            checkThread();
            mSuccess = false;
            AtomicBoolean status = new AtomicBoolean(false);
            Message response = mBaseHandler.obtainMessage(EVENT_UPDATE_DONE, status);
            AdnRecordEx oldAdn = null;
            AdnRecordEx newAdn = null;
            if (mAdnCache == null) {
                loge("updateAdnRecordsInEfByIndexEx failed because mAdnCache is null");
                return mSimIndex;
            }
            if (newid == IccConstantsEx.EF_PBR) {
                newAdn = new AdnRecordEx(newTag, newPhoneNumber, newEmailList,
                        newAnr, newAas, newSne, newGrp, newGas);
                mAdnCache.updateUSIMAdnByIndex(newid, index, newAdn, pin2,
                        response);
            } else {
                newAdn = new AdnRecordEx(newTag, newPhoneNumber);
                mAdnCache.updateAdnByIndexEx(newid, newAdn, index, pin2,
                        response);
            }
            waitForResult(status);
        }
        logd("updateAdnRecordsInEfByIndexEx end " + mSuccess + " mSimIndex = " + mSimIndex);
        return mSimIndex;
    }

    public List<String> getGasInEf() {

        if (mPhone.getContext().checkCallingOrSelfPermission(
                android.Manifest.permission.READ_CONTACTS) != PackageManager.PERMISSION_GRANTED) {
            throw new SecurityException(
                    "Requires android.permission.READ_CONTACTS permission");
        }
        if (mPhone.getCurrentUiccAppType() != AppType.APPTYPE_USIM) {
            loge("Can not get gas from a sim card");
            return null;
        }
        if (mAdnCache == null) {
            loge("getGasInEf failed because mAdnCache is null");
            return new ArrayList<String>();
        }
        return mAdnCache.loadGasFromUsim();
    }

    public int updateUsimGroupBySearch(String oldName, String newName) {

        if (mPhone.getContext().checkCallingOrSelfPermission(
                android.Manifest.permission.WRITE_CONTACTS) != PackageManager.PERMISSION_GRANTED) {
            throw new SecurityException(
                    "Requires android.permission.WRITE_CONTACTS permission");
        }
        if (mAdnCache == null) {
            loge("updateUsimGroupBySearchEx failed because mAdnCache is null");
            return -1;
        }
        return mAdnCache.updateGasBySearch(oldName, newName);
    }

    public int updateUsimGroupByIndex(String newName, int groupId) {

        if (mPhone.getContext().checkCallingOrSelfPermission(
                android.Manifest.permission.WRITE_CONTACTS) != PackageManager.PERMISSION_GRANTED) {
            throw new SecurityException(
                    "Requires android.permission.WRITE_CONTACTS permission");
        }
        if (mAdnCache == null) {
            loge("updateUsimGroupById failed because mAdnCache is null");
            return -1;
        }
        return mAdnCache.updateGasByIndex(newName, groupId);
    }

    public boolean isApplicationOnIcc(int type) {
        if (mPhone.getCurrentUiccAppType() == IccCardApplicationStatus.AppType.values()[type]) {
            return true;
        }
        return false;
    }

    private UsimPhoneBookManager getUsimPhoneBookManager() {
        if (mPhone.getCurrentUiccAppType() == AppType.APPTYPE_USIM) {
            return mAdnCache.getUsimPhoneBookManager();
        }
        return null;
    }

    private int[] getUsimAdnRecordsSize() {
        logd("getUsimAdnRecordsSize");
        if (mAdnCache == null) {
            return null;
        }
        UsimPhoneBookManager mUsimPhoneBookManager = getUsimPhoneBookManager();
        if (mUsimPhoneBookManager == null) {
            return null;
        }
        return mUsimPhoneBookManager.getAdnRecordsSize();
    }

    public int[] getEmailRecordsSize() {
        logd("getEmailRecordsSize");
        if (mAdnCache == null) {
            return null;
        }
        UsimPhoneBookManager mUsimPhoneBookManager = getUsimPhoneBookManager();
        if (mUsimPhoneBookManager == null) {
            return null;
        }
        int efid;
        Set<Integer> usedEfIds = new HashSet<Integer>();
        int[] recordSizeEmail, recordSizeTotal = new int[3];

        for (int num = 0; num < mUsimPhoneBookManager.getNumRecs(); num++) {
            efid = mUsimPhoneBookManager.findEFEmailInfo(num);

            if (efid <= 0 || usedEfIds.contains(efid)) {
                continue;
            } else {
                usedEfIds.add(efid);
            }

            recordSizeEmail = getRecordsSize(efid);
            recordSizeTotal[0] = recordSizeEmail[0];
            recordSizeTotal[1] += recordSizeEmail[1];
            recordSizeTotal[2] += recordSizeEmail[2];
        }
        return recordSizeTotal;
    }

    public int[] getAnrRecordsSize() {
        logd("getAnrRecordsSize");
        if (mAdnCache == null) {
            return null;
        }
        UsimPhoneBookManager mUsimPhoneBookManager = getUsimPhoneBookManager();
        if (mUsimPhoneBookManager == null) {
            return null;
        }
        int efid;
        int[] recordSizeAnr, recordSizeTotal = new int[3];
        for (int num = 0; num < mUsimPhoneBookManager.getNumRecs(); num++) {
            efid = mUsimPhoneBookManager.findEFAnrInfo(num);
            if (efid <= 0) {
                return null;
            }
            recordSizeAnr = getRecordsSize(efid);
            recordSizeTotal[0] = recordSizeAnr[0];
            recordSizeTotal[1] += recordSizeAnr[1];
            recordSizeTotal[2] += recordSizeAnr[2];
        }
        return recordSizeTotal;
    }

    public int getEmailNum() {
        int[] record = null;
        if (mPhone.getCurrentUiccAppType() == AppType.APPTYPE_USIM) {
            if (mAdnCache == null) {
                return 0;
            }
            UsimPhoneBookManager mUsimPhoneBookManager = getUsimPhoneBookManager();
            if (mUsimPhoneBookManager == null) {
                return 0;
            }
            return mUsimPhoneBookManager.getEmailNum();
        }
        return 0;
    }

    public int getAnrNum() {
        if (mPhone.getCurrentUiccAppType() == AppType.APPTYPE_USIM) {
            if (mAdnCache == null) {
                return 0;
            }
            UsimPhoneBookManager mUsimPhoneBookManager = getUsimPhoneBookManager();
            if (mUsimPhoneBookManager == null) {
                return 0;
            }
            return mUsimPhoneBookManager.getAnrNum();
        }
        return 0;
    }

    public int getEmailMaxLen() {
        logd("getEmailMaxLen");
        if (mAdnCache == null) {
            return 0;
        }
        UsimPhoneBookManager mUsimPhoneBookManager = getUsimPhoneBookManager();
        if (mUsimPhoneBookManager == null) {
            return 0;
        }

        int efid = mUsimPhoneBookManager.findEFEmailInfo(0);
        int[] recordSizeEmail = getRecordsSize(efid);
        if (recordSizeEmail == null) {
            return 0;
        }
        if (mUsimPhoneBookManager.getEmailType() == 1) {
            return recordSizeEmail[0];
        } else {
            return recordSizeEmail[0] - 2;
        }
    }

    // If the telephone number or SSC is longer than 20 digits, the first 20
    // digits are stored in this data item and the remainder is stored in an
    // associated record in the EFEXT1.
    public int getPhoneNumMaxLen() {
        UsimPhoneBookManager mUsimPhoneBookManager = getUsimPhoneBookManager();
        if (mUsimPhoneBookManager == null) {
            return AdnRecordEx.MAX_LENTH_NUMBER;
        } else {
            return mUsimPhoneBookManager.getPhoneNumMaxLen();
        }
    }

    public int getUsimGroupNameMaxLen() {
        logd("getGroupNameMaxLen");
        if (mAdnCache == null) {
            return -1;
        }
        UsimPhoneBookManager mUsimPhoneBookManager = getUsimPhoneBookManager();
        if (mUsimPhoneBookManager == null) {
            return -1;
        }
        int gasEfId = mUsimPhoneBookManager.findEFGasInfo();
        int[] gasSize = getRecordsSize(gasEfId);
        if (gasSize == null)
            return -1;
        return gasSize[0];
    }

    public int getUsimGroupCapacity() {
        logd("getUsimGroupCapacity");
        if (mPhone.getCurrentUiccAppType() != AppType.APPTYPE_USIM) {
            loge("Can not get gas from a sim card");
            return 0;
        }
        if (mAdnCache == null) {
            loge("mAdnCache == null");
            return 0;
        }
        UsimPhoneBookManager mUsimPhoneBookManager = getUsimPhoneBookManager();
        if (mUsimPhoneBookManager == null) {
            loge("mUsimPhoneBookManager == null");
            return 0;
        }
        int gasEfId = mUsimPhoneBookManager.findEFGasInfo();
        int[] gasSize = getRecordsSize(gasEfId);
        if (gasSize == null || gasSize.length < 3) {
            return 0;
        }
        return gasSize[2];
    }

    public int[] getAvalibleEmailCount(String name, String number,
            String[] emails, String anr, int[] emailNums) {
        if (mPhone.getCurrentUiccAppType() == AppType.APPTYPE_USIM) {
            if (mAdnCache == null) {
                return null;
            }
            UsimPhoneBookManager mUsimPhoneBookManager = getUsimPhoneBookManager();
            if (mUsimPhoneBookManager == null) {
                return null;
            }
            return mUsimPhoneBookManager.getAvalibleEmailCount(name, number,
                    emails, anr, emailNums);
        }
        return null;
    }

    public int[] getAvalibleAnrCount(String name, String number,
            String[] emails, String anr, int[] anrNums) {
        int[] record = null;
        if (mPhone.getCurrentUiccAppType() == AppType.APPTYPE_USIM) {
            if (mAdnCache == null) {
                return null;
            }
            UsimPhoneBookManager mUsimPhoneBookManager = getUsimPhoneBookManager();
            if (mUsimPhoneBookManager == null) {
                return null;
            }
            return mUsimPhoneBookManager.getAvalibleAnrCount(name, number,
                    emails, anr, anrNums);
        }
        return null;
    }

    public int getInsertIndex() {
        if (mAdnCache == null) {
            loge("getInsertIndex:adn cache is null");
            return -1;
        }
        return mAdnCache.getInsertId();
    }

    private void setReadRecordOfEfid(int efid , boolean readSuccess){
        switch (efid) {
        case IccConstantsEx.EF_ADN:
        case IccConstantsEx.EF_PBR:
             mReadAdnRecordSuccess = readSuccess;
             break;
        case IccConstantsEx.EF_SDN:
             mReadSdnRecordSuccess = readSuccess;
             break;
        case IccConstantsEx.EF_FDN:
             mReadFdnRecordSuccess = readSuccess;
             break;
        case IccConstantsEx.EF_LND:
             mReadLndRecordSuccess = readSuccess;
             break;
       }
    }

    private boolean getReadRecordOfEfid(int efid){
        switch (efid) {
        case IccConstantsEx.EF_ADN:
        case IccConstantsEx.EF_PBR:
                return mReadAdnRecordSuccess;
        case IccConstantsEx.EF_SDN:
                return mReadSdnRecordSuccess;
        case IccConstantsEx.EF_FDN:
                return mReadFdnRecordSuccess;
        case IccConstantsEx.EF_LND:
                return mReadLndRecordSuccess;
        default: return false;
       }
    }

    public List<String> getAasInEf() {
        if (mPhone.getContext().checkCallingOrSelfPermission(
                android.Manifest.permission.READ_CONTACTS) != PackageManager.PERMISSION_GRANTED) {
            throw new SecurityException(
                    "Requires android.permission.READ_CONTACTS permission");
        }
        if (mPhone.getCurrentUiccAppType() != AppType.APPTYPE_USIM) {
            loge("Can not get aas from a sim card");
            return null;
        }
        if (mAdnCache == null) {
            loge("getAasInEf failed because mAdnCache is null");
            return new ArrayList<String>();
        }
        return mAdnCache.loadAasFromUsim();
    }

    public int updateUsimAasBySearch(String oldName, String newName) {
        if (mPhone.getContext().checkCallingOrSelfPermission(
                android.Manifest.permission.WRITE_CONTACTS) != PackageManager.PERMISSION_GRANTED) {
            throw new SecurityException(
                    "Requires android.permission.WRITE_CONTACTS permission");
        }
        if (mAdnCache == null) {
            loge("updateUsimAasBySearch failed because mAdnCache is null");
            return -1;
        }
        return mAdnCache.updateAasBySearch(oldName, newName);
    }

    public int updateUsimAasByIndex(String newName, int aasIndex) {
        if (mPhone.getContext().checkCallingOrSelfPermission(
                android.Manifest.permission.WRITE_CONTACTS) != PackageManager.PERMISSION_GRANTED) {
            throw new SecurityException(
                    "Requires android.permission.WRITE_CONTACTS permission");
        }
        if (mAdnCache == null) {
            loge("updateUsimAasByIndex failed because mAdnCache is null");
            return -1;
        }
        return mAdnCache.updateAasByIndex(newName, aasIndex);
    }

    public int getSneSize() {
        if (mPhone.getContext().checkCallingOrSelfPermission(
                android.Manifest.permission.READ_CONTACTS) != PackageManager.PERMISSION_GRANTED) {
            throw new SecurityException(
                    "Requires android.permission.READ_CONTACTS permission");
        }
        if (mPhone.getCurrentUiccAppType() != AppType.APPTYPE_USIM) {
            loge("Can not get sne size from a sim card");
            return 0;
        }
        if (mAdnCache == null) {
            loge("getSneSize failed because mAdnCache is null");
            return 0;
        }
        return mAdnCache.getSneSize();
    }

    public int[] getSneLength() {
        if (mPhone.getContext().checkCallingOrSelfPermission(
                android.Manifest.permission.READ_CONTACTS) != PackageManager.PERMISSION_GRANTED) {
            throw new SecurityException(
                    "Requires android.permission.READ_CONTACTS permission");
        }
        if (mPhone.getCurrentUiccAppType() != AppType.APPTYPE_USIM) {
            loge("Can not get sne length from a sim card");
            return null;
        }
        if (mAdnCache == null) {
            loge("getSneLength failed because mAdnCache is null");
            return null;
        }
        return mAdnCache.getSneLength();
    }
}
