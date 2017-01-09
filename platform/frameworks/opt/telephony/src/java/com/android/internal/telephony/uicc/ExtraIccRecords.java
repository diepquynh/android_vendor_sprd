
package com.android.internal.telephony.uicc;

import java.util.ArrayList;

import com.android.internal.telephony.CommandsInterface;
import com.android.internal.telephony.IccCard;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneFactory;
import com.android.internal.telephony.TeleUtils;
import com.android.internal.telephony.uicc.IccCardApplicationStatus.AppType;
import com.android.internal.telephony.plugin.TelephonyForTelcelPluginsUtils;

import android.content.Context;
import android.os.AsyncResult;
import android.os.Handler;
import android.os.Message;
import android.os.Registrant;
import android.os.RegistrantList;
import android.telephony.PhoneNumberUtils;
import android.telephony.Rlog;
import android.telephony.TelephonyManager;
import android.util.Log;

public class ExtraIccRecords extends Handler implements IccConstantsEx {

    private static final boolean DBG = true;
    private static final String LOG_TAG = "ExtendedIccRecords";

    private static final int EVENT_GET_CPHS_SPN_DONE = 0;
    private static final int EVENT_GET_SST_DONE = 1;
    private static final int EVENT_GET_ALL_OPL_LOAD_DONE = 2;
    private static final int EVENT_GET_ALL_PNN_LOAD_DONE = 3;

    private static final int EVENT_GET_SIM_ECC_DONE = 10;
    private static final int EVENT_GET_USIM_ECC_DONE = 11;

    private static final int EVENT_SIM_REFRESH = 21;

    // Each emergency call code is coded on three bytes From TS 51.011 EF[ECC] section.
    private static final int ECC_BYTES_COUNT = 3;
    private static final int CATEGORY_BYTES_COUNT = 1;

    private ArrayList<Oplrecord> mOplRecords = null;
    private ArrayList<Pnnrecord> mPnnRecords = null;

    /**
     * States only used by getCPHSSpnFsm FSM
     */
    private GetCPHSSpnFsmState mOnsState;

    private String mOns = null;
    private String mSimEccList = null;
    private String mPnnHomeName = null;

    private int mSstPlmnOplPnnValue = 0;
    private boolean mSimPnnEnabled = false;
    private boolean mSimOplPnnEnabled = false;
    private boolean mSimOplEnabled = false;

    private int mEccRecordsToLoad = 0;
    private int mONSRecordsToLoad = 0;

    private int mPhoneId = -1;

    protected RegistrantList mEccRecordsLoadedRegistrants = new RegistrantList();
    protected RegistrantList mONSRecordsLoadedRegistrants = new RegistrantList();

    private CommandsInterface mCi;
    private Context mContext;

    public ExtraIccRecords(Context c, int phoneId, CommandsInterface ci) {
        mCi = ci;
        mPhoneId = phoneId;
        mContext = c;

        mCi.registerForIccRefresh(this, EVENT_SIM_REFRESH, null);

        if (DBG)
            log("Create ExtendedIccRecords");
    }

    public void fetchExtendedIccRecords() {
        loadONSFiles();
        loadEccFiles();
    }

    private void loadONSFiles() {
        if (DBG)
            log("loadONSFiles");
        IccFileHandler ifh = getIccFileHandler();
        if (ifh != null) {
            ifh.loadEFTransparent(EF_SST, obtainMessage(EVENT_GET_SST_DONE));
            mONSRecordsToLoad++;
            ifh.loadEFLinearFixedAll(EF_OPL,MF_SIM + DF_GSM,obtainMessage(EVENT_GET_ALL_OPL_LOAD_DONE));
            mONSRecordsToLoad++;
            ifh.loadEFLinearFixedAll(EF_PNN, obtainMessage(EVENT_GET_ALL_PNN_LOAD_DONE));
            mONSRecordsToLoad++;
        }
        // Load ONS
        getCPHSSpnFsm(true, null);
    }

    private void loadEccFiles() {
        UiccCardApplication app = getUiccCardApp();
        IccFileHandler ifh = getIccFileHandler();
        if (app != null && ifh != null) {
            if (app.getType() == AppType.APPTYPE_USIM) {
                if (DBG)
                    log("Load USIM eccList");
                ifh.loadEFLinearFixedAll(EF_ECC, MF_SIM + DF_ADF,
                        obtainMessage(EVENT_GET_USIM_ECC_DONE));
            } else {
                if (DBG)
                    log("Load SIM eccList");
                new IccFileHandlerEx(app, app.getAid(), mCi).loadEFTransparent(EF_ECC,
                        MF_SIM + DF_GSM, obtainMessage(EVENT_GET_SIM_ECC_DONE));
            }
        }
        mEccRecordsToLoad++;
    }

    @Override
    public void handleMessage(Message msg) {
        AsyncResult ar;
        byte data[];
        boolean isEccRecordLoadResponse = false;
        boolean isOnsRecordLoadResponse = false;

        try {
            switch (msg.what) {
                case EVENT_SIM_REFRESH:
                    ar = (AsyncResult) msg.obj;
                    if (DBG)
                        log("Sim REFRESH with exception: " + ar.exception);
                    if (ar.exception == null) {
                        handleSimRefresh((IccRefreshResponse) ar.result);
                    }
                    break;
                case EVENT_GET_CPHS_SPN_DONE:
                    isOnsRecordLoadResponse = true;
                    ar = (AsyncResult) msg.obj;
                    Log.d(LOG_TAG, "Load ons false");
                    getCPHSSpnFsm(false, ar);
                    break;

                case EVENT_GET_SST_DONE:
                    isOnsRecordLoadResponse = true;

                    ar = (AsyncResult) msg.obj;
                    data = (byte[]) ar.result;

                    if (ar.exception != null) {
                        break;
                    }

                    handleSstOPLPNNData(data);

                    break;

                case EVENT_GET_ALL_OPL_LOAD_DONE:
                    isOnsRecordLoadResponse = true;
                    ar = (AsyncResult) msg.obj;
                    if (ar.exception != null) {
                        loge("Exception in fetching OPL Records " + ar.exception);
                        mSimOplEnabled = false;
                        return;
                    }

                    handleGetOplResponse(ar);
                    break;

                case EVENT_GET_ALL_PNN_LOAD_DONE:
                    isOnsRecordLoadResponse = true;
                    ar = (AsyncResult) msg.obj;
                    if (ar.exception != null) {
                        loge("Exception in fetching PNN Records " + ar.exception);
                        return;
                    }

                    handleGetPnnResponse(ar);
                    break;
                case EVENT_GET_USIM_ECC_DONE:
                    isEccRecordLoadResponse = true;
                    ar = (AsyncResult) msg.obj;
                    if (ar.exception != null) {
                        return;
                    }

                    handleUsimEccResponse(ar);
                    break;
                case EVENT_GET_SIM_ECC_DONE:
                    isEccRecordLoadResponse = true;
                    ar = (AsyncResult) msg.obj;
                    if (ar.exception != null) {
                        return;
                    }

                    handleSimEccResponse(ar);
                    break;
                default:
                    super.handleMessage(msg);
                    break;
            }
        } catch (RuntimeException exc) {
            // I don't want these exceptions to be fatal
            logw("Exception parsing SIM record ex", exc);
        } finally {
            // Count up record load responses even if they are fails
            if (isOnsRecordLoadResponse) {
                onONSRecordLoaded();
            } else if (isEccRecordLoadResponse) {
                onEccRecordLoaded();
            }
        }
    }

    private void onONSRecordLoaded() {
        // One record loaded successfully or failed, In either case
        // we need to update the recordsToLoad count
        if (DBG)
            log("mONSRecordsToLoad " + mONSRecordsToLoad);

        if (--mONSRecordsToLoad == 0) {
            mONSRecordsLoadedRegistrants.notifyRegistrants(new AsyncResult(null, mPhoneId, null));
        } else if (mONSRecordsToLoad < 0) {
            loge("mONSRecordsToLoad < 0, programmer error suspected");
            mONSRecordsToLoad = 0;
        }
    }

    private void onEccRecordLoaded() {
        if (DBG)
            log("mEccRecordsToLoad " + mEccRecordsToLoad);

        if (--mEccRecordsToLoad == 0) {
            mEccRecordsLoadedRegistrants.notifyRegistrants(new AsyncResult(null, mPhoneId, null));
        } else if (mEccRecordsToLoad < 0) {
            loge("mEccRecordsToLoad < 0, programmer error suspected");
            mEccRecordsToLoad = 0;
        }
    }

    private void handleUsimEccResponse(AsyncResult ar) {
        String eccList = "";
        // Linear fixed EF: ((AsyncResult)(onLoaded.obj)).result is an ArrayList<byte[]>
        ArrayList<byte[]> results = (ArrayList<byte[]>) ar.result;
        if (results != null) {
            for (int i = 0; i < results.size(); i++) {
                String number = PhoneNumberUtils.calledPartyBCDFragmentToString(
                        results.get(i), 0, ECC_BYTES_COUNT);
                String category = (results.get(i)[results.get(i).length-1] & 0xFF) + "";
                eccList = TeleUtils.concatenateEccList(eccList, TeleUtils.concatenateCategoryList(number,category));
            }
            log("USIM ECC List: " + eccList);
        }

        mSimEccList = eccList;
    }

    private void handleSimEccResponse(AsyncResult ar) {
        String eccList = "";
        // Transparent EF: ((AsyncResult)(onLoaded.obj)).result is the byte[]
        byte[] numbers = (byte[]) ar.result;
        if (numbers != null) {
            for (int offSet = 0; offSet < numbers.length / ECC_BYTES_COUNT; offSet++) {
                String number = PhoneNumberUtils.calledPartyBCDFragmentToString(numbers,
                        offSet * ECC_BYTES_COUNT, ECC_BYTES_COUNT);
                eccList = TeleUtils.concatenateEccList(eccList,number);
            }
            log("ECC list: " + eccList);
        }

        mSimEccList = eccList;
    }

    private void handleSstOPLPNNData(byte[] data) {
        mSstPlmnOplPnnValue = -1;

        UiccCardApplication app = getUiccCardApp();

        if (app != null && app.getType() == AppType.APPTYPE_SIM) {
            // 2GSim,51: PLMN Network Name,52: Operator PLMN List
            if (data.length > 12) {
                mSstPlmnOplPnnValue = ((data[12] >> 4) & 0x0F);
                log("SSTOPLPNN: 2G Sim,sstPlmnOplPnnValue: " + mSstPlmnOplPnnValue);
                if (mSstPlmnOplPnnValue == 0x0F) {
                    mSimOplPnnEnabled = true;
                } else if (mSstPlmnOplPnnValue == 0x03) {
                    mSimPnnEnabled = true;
                }
                log("SSTOPLPNN: 2G Sim,sstPlmnOplPnnValue: " + mSstPlmnOplPnnValue
                        + ", simOplPnnEnabled:" + mSimOplPnnEnabled
                        + ", simPnnEnabled:" +
                        mSimPnnEnabled);
            }

        } else if (app != null && app.getType() == AppType.APPTYPE_USIM) {
            // 3GUSim.45: PLMN Network Name,46: Operator PLMN List
            if (data.length > 5) {
                mSstPlmnOplPnnValue = ((data[5] >> 4) & 0x03);
                if (mSstPlmnOplPnnValue == 0x03) {
                    mSimOplPnnEnabled = true;
                } else if (mSstPlmnOplPnnValue == 0x01) {
                    mSimPnnEnabled = true;
                }
            }
            log("SSTOPLPNN: 3G Sim,sstPlmnOplPnnValue: " + mSstPlmnOplPnnValue
                    + ", simOplPnnEnabled:" + mSimOplPnnEnabled + ", simPnnEnabled:"
                    + mSimPnnEnabled);
        } else {
            log("SSTOPLPNN: sstPlmnOplPnnValue:" + mSstPlmnOplPnnValue);
        }
    }

    private void handleGetOplResponse(AsyncResult ar) {
        Oplrecord opl;

        if (ar.exception != null) {
            loge("Exception in fetching OPL Records " + ar.exception);
            mSimOplEnabled = false;
            return;
        }
        ArrayList<byte[]> dataOpl = (ArrayList<byte[]>) (ar.result);
        mOplRecords = new ArrayList<Oplrecord>(dataOpl.size());
        for (int i = 0, s = dataOpl.size(); i < s; i++) {
            opl = new Oplrecord(dataOpl.get(i));
            mOplRecords.add(opl);
            if (DBG)
                log("OPL" + i + ": " + opl);
        }
        mSimOplEnabled = true;
    }

    private void handleGetPnnResponse(AsyncResult ar) {
        Pnnrecord pnn;

        if (ar.exception != null) {
            loge("Exception in fetching PNN Records " + ar.exception);
            return;
        }
        ArrayList<byte[]> dataPnn = (ArrayList<byte[]>) (ar.result);
        mPnnRecords = new ArrayList<Pnnrecord>(dataPnn.size());
        for (int i = 0, s = dataPnn.size(); i < s; i++) {
            pnn = new Pnnrecord(dataPnn.get(i));
            mPnnRecords.add(pnn);
            if (i == 0) {
                mPnnHomeName = pnn.getLongname();
            }
            if (DBG)
                log("PNN" + i + ": " + pnn);
        }
    }

    /**
     * States of Get SPN Finite State Machine which only used by getSpnFsm()
     */
    private enum GetCPHSSpnFsmState {
        IDLE, // No initialized
        INIT, // Start FSM
        READ_SPN_CPHS, // Load EF_SPN_CPHS secondly
        READ_SPN_SHORT_CPHS // Load EF_SPN_SHORT_CPHS last
    }

    /**
     * Finite State Machine to load Service Provider Name , which can be stored in either
     * EF_SPN_CPHS, or EF_SPN_SHORT_CPHS (CPHS4.2), EF_SPN (3GPP) After starting, FSM will search
     * SPN EFs in order and stop after finding the first valid SPN
     *
     * @param start set true only for initialize loading
     * @param ar the AsyncResult from loadEFTransparent ar.exception holds exception in error
     *            ar.result is byte[] for data in success
     */
    private void getCPHSSpnFsm(boolean start, AsyncResult ar) {
        byte[] data;

        IccFileHandler ifh = getIccFileHandler();
        if (ifh == null) {
            mOnsState = GetCPHSSpnFsmState.IDLE;
            return;
        }

        if (start) {
            mOnsState = GetCPHSSpnFsmState.INIT;
        }

        switch (mOnsState) {
            case INIT:
                mOns = null;

                ifh.loadEFTransparent(EF_SPN_CPHS,
                        obtainMessage(EVENT_GET_CPHS_SPN_DONE));
                mONSRecordsToLoad++;
                // mojo require firstly read CPHS firstly
                mOnsState = GetCPHSSpnFsmState.READ_SPN_CPHS;
                break;
            case READ_SPN_CPHS:
                if (ar != null && ar.exception == null) {
                    data = (byte[]) ar.result;
                    mOns = IccUtils.adnStringFieldToString(
                            data, 0, data.length - 1);

                    if (DBG)
                        log("Load EF_SPN_CPHS: " + mOns);

                    mOnsState = GetCPHSSpnFsmState.IDLE;
                } else {
                    ifh.loadEFTransparent(
                            EF_SPN_SHORT_CPHS, obtainMessage(EVENT_GET_CPHS_SPN_DONE));
                    mONSRecordsToLoad++;

                    mOnsState = GetCPHSSpnFsmState.READ_SPN_SHORT_CPHS;
                }
                break;
            case READ_SPN_SHORT_CPHS:
                if (ar != null && ar.exception == null) {
                    data = (byte[]) ar.result;
                    mOns = IccUtils.adnStringFieldToString(
                            data, 0, data.length - 1);
                    if (DBG)
                        log("Load EF_SPN_SHORT_CPHS: " + mOns);
                } else {
                    if (DBG)
                        log("Load EF_SPN_SHORT_CPHS Failure");
                }
                mOnsState = GetCPHSSpnFsmState.IDLE;
                break;
            default:
                mOnsState = GetCPHSSpnFsmState.IDLE;
        }
    }

    /**
     * Returns SIM OPL/PNN support
     */
    public boolean isSimOplPnnSupport() {
        return mSimPnnEnabled || mSimOplPnnEnabled;
    }

    /**
     * Returns SIM Operator Name String
     */
    public String getPnn(String regPlmn, int lac) {
        String pnn = null;
        if (mSimOplPnnEnabled && mSimOplEnabled) {
            // TODO dependent on IMSI loaded
            pnn = getOperatorNameFromOplPnn(regPlmn, lac);
        } else if (mSimPnnEnabled || (mSimOplPnnEnabled && !mSimOplEnabled)) {
            pnn = getFirstPnn(regPlmn);
        }

        return pnn;
    }

    public String getSimOns(String plmn) {
        // ONS(CPHS) is allowed to be shown as PLMN only when UE register on HPLMN or SPDI.
        if (isOnMatchingPlmn(plmn)) { // TODO dependent on IMSI and SPDI loaded
            return mOns;
        }
        return null;
    }

    /**
     * Checks if plmn is HPLMN or on the spdiNetworks list.
     */
    private boolean isOnMatchingPlmn(String plmn) {
        if (plmn == null)
            return false;

        Phone phone = PhoneFactory.getPhone(mPhoneId);
        IccRecords iccRecords = phone == null ? null : phone.getIccCard().getIccRecords();
        if (iccRecords != null && iccRecords instanceof SIMRecords) {
            ArrayList<String> spdiNetworks = ((SIMRecords) iccRecords).mSpdiNetworks;
            if (spdiNetworks != null) {
                for (String spdiNet : spdiNetworks) {
                    if (plmn.equals(spdiNet)) {
                        return true;
                    }
                }
            }
            // SPRD: MODIFY FOR BUG 599169
            String homePlmn = ((SIMRecords) iccRecords).getOperatorNumeric();
            if (plmn.equals(homePlmn)) {
                return true;
            }
        }
        return false;
    }

    private String getOperatorNameFromOplPnn(String regPlmn, int lac) {

        if (regPlmn == null) {
            log("regplmn is  null,doesn't  getOperatorNameFromOplPnn from sim.");
            return null;
        }

        if (mPnnRecords == null) {
            log("mPnnRecords is null");
            return null;
        }

        int pnnRecordnum = getpnnRecordnum(regPlmn, lac);

        if (pnnRecordnum <= 0 || pnnRecordnum > mPnnRecords.size()) {
            log("invalid pnnRecordnum = " + pnnRecordnum);
            return null;
        } else {
            log("mPnnRecords.get(pnnRecordnum - 1).getLongname(): "
                    + mPnnRecords.get(pnnRecordnum - 1).getLongname());
            return mPnnRecords.get(pnnRecordnum - 1).getLongname();
        }
    }

    private String getFirstPnn(String regPlmn) {

        if (regPlmn == null) {
            log("regplmn is  null, doesn't get pnn name from sim.");
            return null;
        }

        String homePlmn = TelephonyManager.getDefault().getSimOperatorNumericForPhone(mPhoneId);
        if (regPlmn.equals(homePlmn)) {
            if (mPnnRecords != null) {
                log("PNN first record name: " + mPnnRecords.get(0).getLongname());
                return mPnnRecords.get(0).getLongname();
            }
        }

        return null;
    }

    private int getpnnRecordnum(String regplmn, int lac) {
        int[] regplmnarray = {
                0, 0, 0, 0, 0, 0
        };

        if (regplmn == null) {
            log("regplmn is  null,doesn't get pnn name from sim.");
            return -1;
        }

        if (lac == -1) {
            log("invalid lac");
            return -1;
        }

        if (mOplRecords == null) {
            log("oplrecord not exist");
            return -1;
        } else {
            for (int i = 0; i < regplmn.length(); i++) {
                regplmnarray[i] = regplmn.charAt(i) - '0';
            }
        }

        for (Oplrecord record : mOplRecords) {
            if (matchOplplmn(record.mOplplmn, regplmnarray)) {
                log("getpnnRecordnum  lac:" + lac + ", record.mOpllac1:" + record.mOpllac1
                        + ", record.mOpllac2:" + record.mOpllac2);
                if ((record.mOpllac1 <= lac) && (lac <= record.mOpllac2)) {
                    log("record.getPnnRecordNum() = " + record.getPnnRecordNum());
                    return record.getPnnRecordNum();
                }
            }

        }

        log("No invalid pnn record match");
        return -1;

    }

    private boolean matchOplplmn(int oplplmn[], int regplmn[]) {
        boolean match = true;
        int SpecialDigit = 0x0D;

        if (regplmn == null | oplplmn == null) {
            return false;
        }

        if (regplmn.length != oplplmn.length) {
            log("regplmn length is not equal oplmn length");
            return false;
        }

        for (int i = 0; i < regplmn.length; i++) {
            if (oplplmn[i] == SpecialDigit) {
                oplplmn[i] = regplmn[i];
            }
        }

        for (int i = 0; i < regplmn.length; i++) {
            if (oplplmn[i] != regplmn[i]) {
                match = false;
                break;
            }
        }

        log("matchOplplmn match:" + match);
        return match;
    }

    public void registerForEccRecordsChanged(Handler h, int what, Object obj) {

        Registrant r = new Registrant(h, what, obj);
        mEccRecordsLoadedRegistrants.add(r);

        if (mEccRecordsToLoad == 0 && mSimEccList != null) {
            r.notifyRegistrant(new AsyncResult(null, mPhoneId, null));
        }
    }

    public void unRegisterForEccRecordsChanged(Handler h) {
        mEccRecordsLoadedRegistrants.remove(h);
    }

    private void handleSimRefresh(IccRefreshResponse refreshResponse) {
        if (refreshResponse == null) {
            if (DBG)
                log("handleSimRefresh received without input");
            return;
        }

        UiccCardApplication app = getUiccCardApp();
        if (refreshResponse.aid != null && app != null
                && !refreshResponse.aid.equals(app.getAid())) {
            // This is for different app. Ignore.
            return;
        }

        switch (refreshResponse.refreshResult) {
            case IccRefreshResponse.REFRESH_RESULT_FILE_UPDATE:
                if (DBG)
                    log("handleSimRefresh with SIM_FILE_UPDATED");
                handleFileUpdate(refreshResponse.efId);
            /* SPRD: add for Telcel feature @{ */
            case IccRefreshResponse.REFRESH_RESULT_RESET:
                if (DBG)
                    log("Telcel feature handleSimRefresh with SIM_REFRESH_RESET");
                TelephonyForTelcelPluginsUtils.getInstance().needRebootPhone();
            /* @} */
            default:
                // UiccController and SIMRecords will handler the other refresh operations.
                if (DBG)
                    log("handleSimRefresh other operation");
                break;
        }
    }

    private void handleFileUpdate(int efId) {
        switch (efId) {
            case EF_ECC:
                loadEccFiles();
                break;
            case EF_SST:
            case EF_OPL:
            case EF_PNN:
            case EF_SPN_CPHS:
            case EF_SPN_SHORT_CPHS:
                loadONSFiles();
            default:
                // Do not care other files.
                break;
        }
    }

    public String getSimEccList() {
        if (mEccRecordsToLoad == 0) {
            return mSimEccList;
        }
        // Ecc files loading, just return null.
        return null;
    }

    public String getPnnHomeName() {
        return mPnnHomeName;
    }

    void resetRecords() {
        log("reset records");

        mOplRecords = null;
        mPnnRecords = null;
        mSstPlmnOplPnnValue = 0;
        mSimPnnEnabled = false;
        mSimOplPnnEnabled = false;
        mSimOplEnabled = false;
        mOns = null;

        if (mSimEccList != null) {
            mSimEccList = null;
            mEccRecordsLoadedRegistrants.notifyRegistrants(new AsyncResult(null, mPhoneId, null));
        }
    }

    private UiccCardApplication getUiccCardApp() {
        Phone phone = PhoneFactory.getPhone(mPhoneId);
        UiccCard uiccCard = phone == null ? null : phone.getUiccCard();
        return uiccCard == null ? null : uiccCard.getApplication(UiccController.APP_FAM_3GPP);
    }

    private IccFileHandler getIccFileHandler() {
        Phone phone = PhoneFactory.getPhone(mPhoneId);
        IccCard iccCard = phone == null ? null : phone.getIccCard();
        return iccCard == null ? null : iccCard.getIccFileHandler();
    }

    private void log(String s) {
        Rlog.d(LOG_TAG, "[SIMRecordsEx" + mPhoneId + "] " + s);
    }

    private void loge(String s) {
        Rlog.e(LOG_TAG, "[SIMRecordsEx" + mPhoneId + "] " + s);
    }

    private void logw(String s, Throwable tr) {
        Rlog.w(LOG_TAG, "[SIMRecordsEx" + mPhoneId + "] " + s, tr);
    }

    private void logv(String s) {
        Rlog.v(LOG_TAG, "[SIMRecordsEx" + mPhoneId + "] " + s);
    }

    class IccFileHandlerEx extends IccFileHandler {

        public IccFileHandlerEx(UiccCardApplication app, String aid, CommandsInterface ci) {
            super(app, aid, ci);
        }

        /**
         * Load a SIM Transparent EF under the given path
         */
        public void loadEFTransparent(int fileid, String path, Message onLoaded) {
            Message response = obtainMessage(EVENT_GET_BINARY_SIZE_DONE, fileid, 0,
                    onLoaded);

            mCi.iccIOForApp(COMMAND_GET_RESPONSE, fileid, path, 0, 0, GET_RESPONSE_EF_SIZE_BYTES,
                    null, null, mAid, response);
        }

        public String getEFPath(int efid) {
            return null;
        }

        public void logd(String s) {
        }

        public void loge(String s) {
        }
    }
}
