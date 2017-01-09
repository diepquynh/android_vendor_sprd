package com.android.internal.telephony.uicc;

import android.telephony.Rlog;
import android.os.*;

import com.android.internal.telephony.CommandsInterface;
import java.util.Map;
import java.util.HashMap;
import java.util.ArrayList;

/**
 * {@hide}
 */
public final class SIMFileHandlerEx extends SIMFileHandler implements IccConstantsEx {
    protected static final String TAG = "SIMFileHandlerEx";

    static protected final int READ_RECORD_MODE_CYCLIC = 3;
    static protected final int EVENT_READ_EF_LINEAR_RECORD_DONE = 12;

    static protected final byte TYPE_FCP = 0x62;
    static protected final byte RESPONSE_DATA_FCP_FLAG = 0;
    static protected final byte TYPE_FILE_DES = (byte) 0x82;
    static protected final byte TYPE_FCP_SIZE = (byte) 0x80;
    static protected final byte RESPONSE_DATA_FILE_DES_FLAG = 2;
    static protected final byte RESPONSE_DATA_FILE_DES_LEN_FLAG = 3;
    static protected final byte TYPE_FILE_DES_LEN = 5;
    static protected final byte RESPONSE_DATA_FILE_RECORD_LEN_1 = 6;
    static protected final byte RESPONSE_DATA_FILE_RECORD_LEN_2 = 7;
    static protected final byte RESPONSE_DATA_FILE_RECORD_COUNT_FLAG = 8;
    static private final int USIM_DATA_OFFSET_2 = 2;
    static private final int USIM_DATA_OFFSET_3 = 3;

    // private final int mDualMapFile[] ={EF_ADN,EF_ARR,EF_FDN,EF_SMS,EF_MSISDN,
    // EF_SMSP,EF_SMSS,EF_SMSR,EF_SDN,EF_EXT2,EF_EXT3,EF_EXT4,EF_BDN,EF_TEST};
    private final int mDualMapFile[] = { EF_SMS, EF_PBR };
    private Map<Integer, String> mDualMapFileList;
    private ArrayList<Integer> mFileList;

    public SIMFileHandlerEx(UiccCardApplication app, String aid,
            CommandsInterface ci) {
        super(app, aid, ci);
        initDualMapFileSet();
    }

    @Override
    protected String getEFPath(int efid) {
        String path = null;
        switch (efid) {
        case EF_SMS:
            return MF_SIM + DF_TELECOM;
        case EF_ECC:
        case EF_EXT6:
        case EF_MWIS:
        case EF_MBI:
        case EF_SPN:
        case EF_AD:
        case EF_MBDN:
        case EF_PNN:
        case EF_OPL:
        case EF_SPDI:
        case EF_SST:
        case EF_CFIS:
            return MF_SIM + DF_GSM;
        case EF_FDN:
            break;
        case EF_GID1:
        case EF_GID2:
            return MF_SIM + DF_GSM;

        case EF_MAILBOX_CPHS:
        case EF_VOICE_MAIL_INDICATOR_CPHS:
        case EF_CFF_CPHS:
        case EF_SPN_CPHS:
        case EF_SPN_SHORT_CPHS:
        case EF_INFO_CPHS:
        case EF_CSP_CPHS:
        case EF_PLMN_SEL:
            return MF_SIM + DF_GSM;
        case EF_MSISDN:
            return MF_SIM + DF_TELECOM;
        case EF_PBR:
            return mDualMapFileList.get(EF_PBR);
        case EF_DIR:
            return MF_SIM;
        }
        path = getCommonIccEFPath(efid);
        if (path == null) {
            loge("Error: EF Path being returned in null");
        }
        return path;
    }

    private void initDualMapFileSet() {
        mDualMapFileList = new HashMap<Integer, String>();
        mFileList = new ArrayList<Integer>();
        mDualMapFileList.put(mDualMapFile[0], MF_SIM + DF_ADF);
        mDualMapFileList.put(mDualMapFile[1], MF_SIM + DF_TELECOM
                + DF_PHONEBOOK);
    }

    private void clearDualMapFileSet() {
        if (mFileList != null) {
            mFileList = null;
        }
        if (mDualMapFileList != null) {
            mDualMapFileList.clear();
            mDualMapFileList = null;
        }
    }

    public void dispose() {
        super.dispose();
        clearDualMapFileSet();
    }

    protected void finalize() throws Throwable {
        super.finalize();
        logd("SIMFileHandlerEx finalized");
    }

    protected String getEFPathofUsim(int efid) {
        return null;
    }

    public boolean loadFileAgain(int fileId, int pathNum, int event, Object obj) {
        return false;
    }

    @Override
    public void loadEFTransparent(int fileid, Message onLoaded) {
        logd("loadEFTransparent fileid " + Integer.toHexString(fileid));
        Message response = obtainMessage(EVENT_GET_BINARY_SIZE_DONE, fileid, 0, onLoaded);
        mCi.iccIO(COMMAND_GET_RESPONSE, fileid, getEFPath(fileid), 0, 0,
                GET_RESPONSE_EF_SIZE_BYTES, null, null, response);
    }

    // ***** Overridden from IccFileHandler
    @Override
    public void handleMessage(Message msg) {
        AsyncResult ar;
        IccIoResult result;
        Message response = null;
        IccFileHandler.LoadLinearFixedContext lc;
        IccException iccException;
        byte data[];
        int size, fcp_size;
        int fileid;
        int recordNum;
        int recordSize[];
        int index = 0;
        int pathNum = msg.arg2;
        String path;
        try {
            switch (msg.what) {
            case EVENT_READ_IMG_DONE:
                ar = (AsyncResult) msg.obj;
                lc = (IccFileHandler.LoadLinearFixedContext) ar.userObj;
                result = (IccIoResult) ar.result;
                response = lc.mOnLoaded;

                if (ar.exception != null) {
                    logd("EVENT_READ_IMG_DONE ar fail");
                    sendResult(response, null, ar.exception);
                    break;
                }
                iccException = result.getException();
                if (iccException != null) {
                    logd("EVENT_READ_IMG_DONE icc fail");
                    sendResult(response, null, iccException);
                    break;
                }
                data = result.payload;
                fileid = lc.mEfid;
                recordNum = lc.mRecordNum;
                logd("data = " + IccUtils.bytesToHexString(data) + " fileid = "
                        + fileid + " recordNum = " + recordNum);
                if (TYPE_EF != data[RESPONSE_DATA_FILE_TYPE]) {
                    logd("EVENT_READ_IMG_DONE TYPE_EF mismatch");
                    throw new IccFileTypeMismatch();
                }
                if (EF_TYPE_LINEAR_FIXED != data[RESPONSE_DATA_STRUCTURE]) {
                    logd("EVENT_READ_IMG_DONE EF_TYPE_LINEAR_FIXED mismatch");
                    throw new IccFileTypeMismatch();
                }
                lc.mRecordSize = data[RESPONSE_DATA_RECORD_LENGTH] & 0xFF;
                size = ((data[RESPONSE_DATA_FILE_SIZE_1] & 0xff) << 8)
                        + (data[RESPONSE_DATA_FILE_SIZE_2] & 0xff);
                lc.mCountRecords = size / lc.mRecordSize;
                if (lc.mLoadAll) {
                    lc.results = new ArrayList<byte[]>(lc.mCountRecords);
                }
                logd("recordsize:" + lc.mRecordSize + "counts:" + lc.mCountRecords);
                mCi.iccIO(COMMAND_READ_RECORD, lc.mEfid, getEFPath(lc.mEfid),
                        lc.mRecordNum, READ_RECORD_MODE_ABSOLUTE,
                        lc.mRecordSize, null, null,
                        obtainMessage(EVENT_READ_RECORD_DONE, lc));
                break;
            case EVENT_READ_ICON_DONE:
                ar = (AsyncResult) msg.obj;
                response = (Message) ar.userObj;
                result = (IccIoResult) ar.result;
                iccException = result.getException();
                if (iccException != null) {
                    sendResult(response, result.payload, ar.exception);
                } else {
                    sendResult(response, result.payload, null);
                }
                break;
            case EVENT_GET_EF_LINEAR_RECORD_SIZE_DONE:
                ar = (AsyncResult) msg.obj;
                lc = (IccFileHandler.LoadLinearFixedContext) ar.userObj;
                result = (IccIoResult) ar.result;
                response = lc.mOnLoaded;
                data = result.payload;

                if (ar.exception != null) {
                   logd("EVENT_GET_EF_LINEAR_RECORD_SIZE_DONE exception ");
                    sendResult(response, null, ar.exception);
                    break;
                }

                if (data != null) {
                    logbyte(data);
                } else {
                    logd("EVENT_GET_EF_LINEAR_RECORD_SIZE_DONE exception result.payload is null");
                    sendResult(response, null, new NullPointerException());
                    break;
                }

                iccException = result.getException();
                logd("EVENT_GET_EF_LINEAR_RECORD_SIZE_DONE (4)");
                if (iccException != null) {
                    sendResult(response, null, iccException);
                    break;
                }

                if (TYPE_EF != data[RESPONSE_DATA_FILE_TYPE]
                        || (EF_TYPE_LINEAR_FIXED != data[RESPONSE_DATA_STRUCTURE] && EF_TYPE_CYCLIC != data[RESPONSE_DATA_STRUCTURE])) {
                    throw new IccFileTypeMismatch();
                }

                recordSize = new int[3];
                recordSize[0] = data[RESPONSE_DATA_RECORD_LENGTH] & 0xFF;
                recordSize[1] = ((data[RESPONSE_DATA_FILE_SIZE_1] & 0xff) << 8)
                        + (data[RESPONSE_DATA_FILE_SIZE_2] & 0xff);
                recordSize[2] = recordSize[1] / recordSize[0];

                sendResult(response, recordSize, null);
                break;
            case EVENT_GET_RECORD_SIZE_IMG_DONE:
            case EVENT_GET_RECORD_SIZE_DONE:
                logd("EVENT_GET_RECORD_SIZE_DONE");
                ar = (AsyncResult) msg.obj;
                lc = (IccFileHandler.LoadLinearFixedContext) ar.userObj;
                result = (IccIoResult) ar.result;
                response = lc.mOnLoaded;
                if (ar.exception != null) {
                    loge("EVENT_GET_RECORD_SIZE_DONE ar.exception");
                    sendResult(response, null, ar.exception);
                    break;
                }

                data = result.payload;
                fileid = lc.mEfid;
                recordNum = lc.mRecordNum;
                iccException = result.getException();

                if (iccException != null) {
                    loge("EVENT_GET_RECORD_SIZE_DONE iccException");
                    sendResult(response, null, iccException);
                    break;
                }

                logd("EVENT_GET_RECORD_SIZE_DONE, fileid:" + fileid + "("
                        + Integer.toHexString(fileid) + ")");

                logbyte(data);
                logd("FCP:"
                        + Integer.toHexString(data[RESPONSE_DATA_FCP_FLAG])
                        + " DES:"
                        + Integer
                                .toHexString(data[RESPONSE_DATA_FILE_DES_FLAG])
                        + " DES_LEN:"
                        + Integer
                                .toHexString(data[RESPONSE_DATA_FILE_DES_LEN_FLAG]));
                // Use FCP flag to indicate GSM or TD simcard
                if (TYPE_FCP == data[RESPONSE_DATA_FCP_FLAG]) {
                    fcp_size = data[RESPONSE_DATA_FILE_SIZE_1] & 0xff;
                    if (TYPE_FILE_DES != data[RESPONSE_DATA_FILE_DES_FLAG]) {
                        loge("TYPE_FILE_DES exception");
                        throw new IccFileTypeMismatch();
                    }
                    if (TYPE_FILE_DES_LEN != data[RESPONSE_DATA_FILE_DES_LEN_FLAG]) {
                        loge("TYPE_FILE_DES_LEN exception");
                        throw new IccFileTypeMismatch();
                    }
                    lc.mRecordSize = ((data[RESPONSE_DATA_FILE_RECORD_LEN_1] & 0xff) << 8)
                            + (data[RESPONSE_DATA_FILE_RECORD_LEN_2] & 0xff);
                    lc.mCountRecords = data[RESPONSE_DATA_FILE_RECORD_COUNT_FLAG] & 0xFF;
                } else {
                    if (TYPE_EF != data[RESPONSE_DATA_FILE_TYPE]) {
                        loge("GSM: TYPE_EF exception");
                        throw new IccFileTypeMismatch();
                    }

                    if ((EF_TYPE_LINEAR_FIXED != data[RESPONSE_DATA_STRUCTURE])
                            && (EF_TYPE_CYCLIC != data[RESPONSE_DATA_STRUCTURE])) {
                        loge("GSM: EF_TYPE_LINEAR_FIXED exception");
                        throw new IccFileTypeMismatch();
                    }

                    lc.mRecordSize = data[RESPONSE_DATA_RECORD_LENGTH] & 0xFF;
                    size = ((data[RESPONSE_DATA_FILE_SIZE_1] & 0xff) << 8)
                            + (data[RESPONSE_DATA_FILE_SIZE_2] & 0xff);

                    lc.mCountRecords = size / lc.mRecordSize;
                }
                loge("recordsize:" + lc.mRecordSize + " counts:"
                        + lc.mCountRecords);
                if (lc.mLoadAll) {
                    lc.results = new ArrayList<byte[]>(lc.mCountRecords);
                }

                path = getEFPath(lc.mEfid);
                logd("EVENT_GET_RECORD_SIZE_DONE, path " + path
                        + ",  lc.mRecordNum:" + lc.mRecordNum);
                mCi.iccIO(COMMAND_READ_RECORD, lc.mEfid, path, lc.mRecordNum,
                        READ_RECORD_MODE_ABSOLUTE, lc.mRecordSize, null, null,
                        obtainMessage(EVENT_READ_RECORD_DONE, 0, pathNum, lc));
                break;
            case EVENT_GET_BINARY_SIZE_DONE:
                ar = (AsyncResult) msg.obj;
                response = (Message) ar.userObj;
                result = (IccIoResult) ar.result;

                if (ar.exception != null) {
                    sendResult(response, null, ar.exception);
                    break;
                }

                iccException = result.getException();
                if (iccException != null) {
                    loge("EVENT_GET_BINARY_SIZE_DONE iccException");
                    sendResult(response, null, iccException);
                    break;
                }

                data = result.payload;
                logbyte(data);
                fileid = msg.arg1;
                if (TYPE_FCP == data[RESPONSE_DATA_FCP_FLAG]) {
                    Rlog.i(TAG,
                            "EVENT_GET_BINARY_SIZE_DONE fileid "
                                    + Integer.toHexString(fileid));

                    for (int i = 0; i < data.length; i++) {
                        if (data[i] == TYPE_FILE_DES) {
                            index = i;
                            break;
                        }
                    }
                    logd("TYPE_FILE_DES index " + index);
                    if ((data[index + RESPONSE_DATA_FILE_DES_FLAG] & 0x01) != 1) {
                        Rlog.i(TAG, "EVENT_GET_BINARY_SIZE_DONE the efid "
                                + Integer.toHexString(fileid)
                                + " is not transparent file");
                        throw new IccFileTypeMismatch();
                    }
                    for (int i = index; i < data.length;) {
                        if (data[i] == TYPE_FCP_SIZE) {
                            index = i;
                            break;
                        } else {
                            i += (data[i + 1] + 2);
                        }
                    }

                    logd("TYPE_FCP_SIZE index " + index);
                    size = ((data[index + USIM_DATA_OFFSET_2] & 0xff) << 8)
                            + (data[index + USIM_DATA_OFFSET_3] & 0xff);
                } else {
                    if (TYPE_EF != data[RESPONSE_DATA_FILE_TYPE]) {
                        throw new IccFileTypeMismatch();
                    }

                    if (EF_TYPE_TRANSPARENT != data[RESPONSE_DATA_STRUCTURE]) {
                        throw new IccFileTypeMismatch();
                    }

                    size = ((data[RESPONSE_DATA_FILE_SIZE_1] & 0xff) << 8)
                            + (data[RESPONSE_DATA_FILE_SIZE_2] & 0xff);
                }

                path = getEFPath(fileid);
                loge("EVENT_GET_BINARY_SIZE_DONE path " + path);
                mCi.iccIO(
                        COMMAND_READ_BINARY,
                        fileid,
                        path,
                        0,
                        0,
                        size,
                        null,
                        null,
                        obtainMessage(EVENT_READ_BINARY_DONE, fileid, pathNum,
                                response));
                break;

            case EVENT_READ_RECORD_DONE:
                ar = (AsyncResult) msg.obj;
                lc = (IccFileHandler.LoadLinearFixedContext) ar.userObj;
                result = (IccIoResult) ar.result;
                response = lc.mOnLoaded;

                if (ar.exception != null) {
                    sendResult(response, null, ar.exception);
                    break;
                }

                iccException = result.getException();
                if (iccException != null) {
                    sendResult(response, null, iccException);
                    break;
                }

                if (!lc.mLoadAll) {
                    sendResult(response, result.payload, null);
                } else {
                    lc.results.add(result.payload);
                    lc.mRecordNum++;
                    if (lc.mRecordNum > lc.mCountRecords) {
                        sendResult(response, lc.results, null);
                    } else {
                        path = getEFPath(lc.mEfid);
                        mCi.iccIO(
                                COMMAND_READ_RECORD,
                                lc.mEfid,
                                path,
                                lc.mRecordNum,
                                READ_RECORD_MODE_ABSOLUTE,
                                lc.mRecordSize,
                                null,
                                null,
                                obtainMessage(EVENT_READ_RECORD_DONE, 0,
                                        pathNum, lc));
                    }
                }
                break;
            case EVENT_READ_BINARY_DONE:
                ar = (AsyncResult) msg.obj;
                response = (Message) ar.userObj;
                result = (IccIoResult) ar.result;

                if (ar.exception != null) {
                    sendResult(response, null, ar.exception);
                    break;
                }

                iccException = result.getException();

                if (iccException != null) {
                    sendResult(response, null, iccException);
                    break;
                }

                sendResult(response, result.payload, null);
                break;
            case EVENT_READ_EF_LINEAR_RECORD_DONE:
                ar = (AsyncResult) msg.obj;
                result = (IccIoResult) ar.result;
                response = (Message) ar.userObj;
                if (processException(response, (AsyncResult) msg.obj)) {
                    break;
                }
                sendResult(response, result.payload, null);
                break;
            default:
                super.handleMessage(msg);

            }
        } catch (Exception exc) {
            exc.printStackTrace();
            if (response != null && response.getTarget() != null) {
                sendResult(response, null, exc);
            } else {
                loge("uncaught exception" + exc);
            }
        }
    }

    protected String getCommonIccEFPath(int efid) {
        switch (efid) {
        case EF_ADN:
        case EF_FDN:
        case EF_MSISDN:
        case EF_SDN:
        case EF_EXT1:
        case EF_EXT2:
        case EF_EXT3:
        case EF_PSI:
        case EF_LND:
            return MF_SIM + DF_TELECOM;
        case EF_ICCID:
        case EF_PL:
            return MF_SIM;
        case EF_PBR:
            // we only support global phonebook.
            return MF_SIM + DF_TELECOM + DF_PHONEBOOK;
        case EF_IMG:
            return MF_SIM + DF_TELECOM + DF_GRAPHICS;
        }
        return null;
    }

    private String getCommonIccEFPathOfUsim(int efid) {
        switch (efid) {
        case EF_ADN:
        case EF_FDN:
        case EF_MSISDN:
        case EF_SDN:
        case EF_LND:
        case EF_EXT1:
        case EF_EXT2:
        case EF_EXT3:
            return MF_SIM + DF_TELECOM;
        case EF_ICCID:
            return MF_SIM;
        case EF_IMG:
            return MF_SIM + DF_TELECOM + DF_GRAPHICS;
        }
        return null;
    }

    protected void logbyte(byte data[]) {
        String test = new String();
        logd("logbyte, data length:" + data.length);
        for (int i = 0; i < data.length; i++) {
            test = Integer.toHexString(data[i] & 0xFF);
            if (test.length() == 1) {
                test = '0' + test;
            }
            logd("payload:" + test);
        }
    }

    protected boolean isDataValid(byte data[]) {
        boolean isValid = false;
        for (int i = 0; i < data.length; i++) {
            if (data[i] != 0xFF) {
                isValid = true;
                break;
            }
        }
        logd("isDataValid:" + isValid);
        return isValid;
    }

    @Override
    protected void logd(String msg) {
        Rlog.d(TAG, msg);
    }

    @Override
    protected void loge(String msg) {
        Rlog.e(TAG, msg);
    }

    public void loadEFLinearFixed(int fileid, int recordNum, int recordSize,
            Message onLoaded) {
        Message response = obtainMessage(EVENT_READ_EF_LINEAR_RECORD_DONE,
                fileid, recordNum, onLoaded);

        mCi.iccIOForApp(COMMAND_READ_RECORD, fileid, getEFPath(fileid),
                recordNum, READ_RECORD_MODE_ABSOLUTE, recordSize, null, null,
                mAid, response);
    }

    public void updateEFCYCLICLinearFixed(int fileid, int recordNum,
            byte[] data, String pin2, Message onComplete) {
        logd("updateEFCYCLICLinearFixed, fileid:" + Integer.toHexString(fileid)
                + ", ef path:" + getEFPath(fileid) + ",recordNum:" + recordNum);
        mCi.iccIO(COMMAND_UPDATE_RECORD, fileid, getEFPath(fileid), recordNum,
                READ_RECORD_MODE_CYCLIC, data.length,
                IccUtils.bytesToHexString(data), pin2, onComplete);
    }

    private void sendResult(Message response, Object result, Throwable ex) {
        if (response == null) {
            return;
        }
        AsyncResult.forMessage(response, result, ex);
        response.sendToTarget();
    }

    private boolean processException(Message response, AsyncResult ar) {
        IccException iccException;
        boolean flag = false;
        IccIoResult result = (IccIoResult) ar.result;
        if (ar.exception != null) {
            sendResult(response, null, ar.exception);
            flag = true;
        } else {
            iccException = result.getException();
            if (iccException != null) {
                sendResult(response, null, iccException);
                flag = true;
            }
        }
        return flag;
    }
}
