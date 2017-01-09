package com.android.internal.telephony.uicc;

import android.telephony.Rlog;

import com.android.internal.telephony.CommandsInterface;
import java.util.ArrayList;
import java.util.Map;
import java.util.HashMap;

import android.os.*;
import android.util.Log;

/**
 * {@hide}
 */
public final class UsimFileHandlerEx extends UsimFileHandler implements IccConstantsEx {
    protected static final String LOG_TAG = "UsimFileHandlerEx";

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

    static private final byte USIM_RECORD_SIZE_1 = 4;
    static private final byte USIM_RECORD_SIZE_2 = 5;
    static private final byte USIM_RECORD_COUNT = 6;
    static private final int USIM_DATA_OFFSET_2 = 2;
    static private final int USIM_DATA_OFFSET_3 = 3;

    private CommandsInterface mCi;
    // private final int mDualMapFile[] ={EF_ADN,EF_ARR,EF_FDN,EF_SMS,EF_MSISDN,
    // EF_SMSP,EF_SMSS,EF_SMSR,EF_SDN,EF_EXT2,EF_EXT3,EF_EXT4,EF_BDN,EF_TEST};
    private final int mDualMapFile[] = { EF_SMS, EF_PBR, EF_SDN, EF_EXT3 };
    private Map<Integer, String> mDualMapFileList;
    private ArrayList<Integer> mFileList;

    public UsimFileHandlerEx(UiccCardApplication app, String aid,
            CommandsInterface ci) {
        super(app, aid, ci);
        mCi = ci;
        initDualMapFileSet();
    }

    @Override
    protected String getEFPath(int efid) {
        String path = null;

        path = getEfPathFromList(efid);
        if (path != null) {
            return path;
        }

        switch (efid) {
        case EF_SMS:
            return MF_SIM + DF_ADF;
        case EF_ECC:
        case EF_EXT5:
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
            return MF_SIM + DF_ADF;
        case EF_FDN:
            return MF_SIM + DF_ADF;
        case EF_MAILBOX_CPHS:
        case EF_VOICE_MAIL_INDICATOR_CPHS:
        case EF_CFF_CPHS:
        case EF_SPN_CPHS:
        case EF_SPN_SHORT_CPHS:
        case EF_EXT2:
        case EF_INFO_CPHS:
        case EF_CSP_CPHS:
            return MF_SIM + DF_GSM;
        case EF_MSISDN:
        case EF_PLMN_ACT:
        case EF_GID1:
        case EF_GID2:
        case EF_LI:
            return MF_SIM + DF_ADF;

        case EF_DIR:
            return MF_SIM;
        case EF_PBR:
            // we only support global phonebook.
            // return MF_SIM + DF_TELECOM + DF_PHONEBOOK;
            if (mDualMapFileList != null) {
                return mDualMapFileList.get(EF_PBR);
            } else {
                return MF_SIM + DF_TELECOM + DF_PHONEBOOK;
            }
        }
        path = getCommonIccEFPath(efid);
        if (path == null) {
            if (mDualMapFileList != null) {
                return mDualMapFileList.get(EF_PBR);
            } else {
                return MF_SIM + DF_TELECOM + DF_PHONEBOOK;
            }
        }
        return path;
    }

    private void initDualMapFileSet() {
        mDualMapFileList = new HashMap<Integer, String>();
        mFileList = new ArrayList<Integer>();

        mDualMapFileList.put(mDualMapFile[0], MF_SIM + DF_ADF);
        mDualMapFileList.put(mDualMapFile[1], MF_SIM + DF_TELECOM
                + DF_PHONEBOOK);
        mDualMapFileList.put(mDualMapFile[2], MF_SIM + DF_ADF);
        mDualMapFileList.put(mDualMapFile[3], MF_SIM + DF_ADF);
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

    private void UpdatePathOfDualMapFile(int efid, String path) {
        loge("UpdatePathOfDualMapFile  efid " + efid + " path " + path);
        if (mDualMapFileList != null) {
            mDualMapFileList.put(efid, path);
        }

    }

    public void dispose() {
        super.dispose();
        clearDualMapFileSet();
    }

    protected void finalize() throws Throwable {
        super.finalize();
        logd("UsimFileHandler finalized");
    }

    private boolean isDualMapFile(int fileId) {
       logd("isDualMapFile  fileId " + fileId
                + " mDualMapFileList " + mDualMapFileList);
        if (mDualMapFileList == null) {
            return false;
        }

        if (mDualMapFileList.containsKey(fileId)) {
            return true;
        }
        return false;
    }

    private boolean isFinishLoadFile(int fileId, int pathNum) {
        logd("isFinishLoadFile  fileId " + fileId + ", pathNum "
                + pathNum);
        if (isDualMapFile(fileId)) {
            if (pathNum == 1) {
                return true;
            }
            if (pathNum == 0) {
                return false;
            }
        }
        return true;
    }

    protected String getEFPathofUsim(int efid) {
        String oldPath = getEFPath(efid);
        String pathFirst = "";
        String pathSecond = "";
        String pathLast = "";

        if (oldPath.length() < 8) {
            return null;
        } else {

            pathFirst = oldPath.substring(0, 4);
            pathSecond = oldPath.substring(4, 8);

            if (oldPath.length() > 8) {
                pathLast = oldPath.substring(8, oldPath.length());
            }
        }

        logd("getEFPathofUsim false , try again pathFirst "
                + pathFirst + ", pathSecond " + pathSecond + ", pathLast "
                + pathLast);
        if (pathSecond.equals(DF_ADF)) {
            pathSecond = DF_TELECOM;
        } else if (pathSecond.equals(DF_TELECOM)) {
            pathSecond = DF_ADF;
        } else {
            return null;
        }

        String newPath = pathFirst + pathSecond + pathLast;
        UpdatePathOfDualMapFile(efid, newPath);
        return newPath;

    }

    public boolean loadFileAgain(int fileId, int pathNum, int event, Object obj) {
        if (isFinishLoadFile(fileId, pathNum)) {
            return false;
        } else {
            String newPath = getEFPathofUsim(fileId);
            if (newPath == null) {
                return false;
            }

            ((LoadLinearFixedContext) obj).mPath = newPath;
            Message response = obtainMessage(event, fileId, 1, obj);
            logd("isFinishLoadFile  try again newPath   " + newPath);
            mCi.iccIO(COMMAND_GET_RESPONSE, fileId, newPath, 0, 0,
                    GET_RESPONSE_EF_SIZE_BYTES, null, null, response);
        }
        return true;
    }

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
        String path = null;
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
                    Rlog.d(LOG_TAG,
                            "EVENT_READ_IMG_DONE EF_TYPE_LINEAR_FIXED mismatch");
                    throw new IccFileTypeMismatch();
                }
                lc.mRecordSize = data[RESPONSE_DATA_RECORD_LENGTH] & 0xFF;
                size = ((data[RESPONSE_DATA_FILE_SIZE_1] & 0xff) << 8)
                        + (data[RESPONSE_DATA_FILE_SIZE_2] & 0xff);
                lc.mCountRecords = size / lc.mRecordSize;
                if (lc.mLoadAll) {
                    lc.results = new ArrayList<byte[]>(lc.mCountRecords);
                }
                logd("recordsize:" + lc.mRecordSize + "counts:"
                        + lc.mCountRecords);
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
                logd("msg.what = " + msg.what);
                ar = (AsyncResult) msg.obj;
                lc = (IccFileHandler.LoadLinearFixedContext) ar.userObj;
                result = (IccIoResult) ar.result;
                response = lc.mOnLoaded;
                if (ar.exception != null) {
                    loge("ar.exception = " + ar.exception);
                    sendResult(response, null, ar.exception);
                    break;
                }
                data = result.payload;
                fileid = lc.mEfid;
                iccException = result.getException();
                if ((iccException != null || (fileid == EF_PBR && !isDataValid(data)))
                        && loadFileAgain(fileid, pathNum, msg.what, lc)) {
                    logd("load again return");
                    return;
                }

                if (iccException != null) {
                    loge("iccException = " + iccException);
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
                logd("msg.what = " + msg.what);
                ar = (AsyncResult) msg.obj;
                lc = (IccFileHandler.LoadLinearFixedContext) ar.userObj;
                result = (IccIoResult) ar.result;
                response = lc.mOnLoaded;
                if (ar.exception != null) {
                    loge("ar.exception = " + ar.exception);
                    sendResult(response, null, ar.exception);
                    break;
                }
                data = result.payload;
                path = lc.mPath;
                fileid = lc.mEfid;
                iccException = result.getException();
                if ((iccException != null || (fileid == EF_PBR && !isDataValid(data)))
                        && loadFileAgain(fileid, pathNum, msg.what, lc)) {
                    logd("load again return");
                    return;
                }

                if (iccException != null) {
                    loge("iccException = " + iccException);
                    sendResult(response, null, iccException);
                    break;
                }

                if (TYPE_EF != data[RESPONSE_DATA_FILE_TYPE]) {
                    throw new IccFileTypeMismatch();
                }

                if (EF_TYPE_LINEAR_FIXED != data[RESPONSE_DATA_STRUCTURE]
                        && EF_TYPE_CYCLIC != data[RESPONSE_DATA_STRUCTURE]) {
                    throw new IccFileTypeMismatch();
                }

                lc.mRecordSize = data[RESPONSE_DATA_RECORD_LENGTH] & 0xFF;

                size = ((data[RESPONSE_DATA_FILE_SIZE_1] & 0xff) << 8)
                        + (data[RESPONSE_DATA_FILE_SIZE_2] & 0xff);

                lc.mCountRecords = size / lc.mRecordSize;

                if (lc.mLoadAll) {
                    lc.results = new ArrayList<byte[]>(lc.mCountRecords);
                }

                if (path == null) {
                    path = getEFPath(lc.mEfid);
                }
                mCi.iccIOForApp(COMMAND_READ_RECORD, lc.mEfid, path,
                        lc.mRecordNum, READ_RECORD_MODE_ABSOLUTE,
                        lc.mRecordSize, null, null, mAid,
                        obtainMessage(EVENT_READ_RECORD_DONE, lc));
                break;
            case EVENT_GET_BINARY_SIZE_DONE:
                logd("EVENT_GET_BINARY_SIZE_DONE");
                ar = (AsyncResult) msg.obj;
                response = (Message) ar.userObj;
                result = (IccIoResult) ar.result;
                if (ar.exception != null) {
                    loge("ar.exception = " + ar.exception);
                    sendResult(response, null, ar.exception);
                    break;
                }
                iccException = result.getException();
                fileid = msg.arg1;
                if (iccException != null && loadFileAgain(fileid, pathNum, msg.what, response)) {
                    logd("load again return");
                    return;
                }
                super.handleMessage(msg);
                break;
            case EVENT_READ_RECORD_DONE:
                logd("EVENT_READ_RECORD_DONE");
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

    private String getEfPathFromList(int efid) {

        String path = null;
        if (mDualMapFileList == null) {
            return null;
        }

        if (mDualMapFileList.containsKey(efid)) {
            path = mDualMapFileList.get(efid);
            if (path != null) {
                return path;
            }
        }

        if (mFileList == null) {
            return null;
        }

        for (int i = 0; i < mFileList.size(); i++) {
            if (mFileList.get(i) == efid) {
                path = mDualMapFileList.get(EF_PBR);
                if (path != null) {
                    return path;
                } else {
                    break;
                }
            }
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
            if (data[i] != (byte) 0xFF) {
                isValid = true;
                break;
            }
        }
        logd("isDataValid:" + isValid);
        return isValid;
    }

    @Override
    protected void logd(String msg) {
        Rlog.d(LOG_TAG, msg);
    }

    @Override
    protected void loge(String msg) {
        Rlog.e(LOG_TAG, msg);
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
            return MF_SIM + DF_TELECOM + DF_PHONEBOOK;
        case EF_IMG:
            return MF_SIM + DF_TELECOM + DF_GRAPHICS;
        }
        return null;
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
