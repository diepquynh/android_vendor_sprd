package com.android.internal.telephony.uicc;

import com.android.internal.telephony.CommandsInterface;
import com.android.internal.telephony.OperatorNameHandler;
import com.android.internal.telephony.TelephonyIntentsEx;
import com.android.internal.telephony.uicc.IccCardApplicationStatus.AppType;

import android.content.Context;
import android.content.Intent;
import android.os.AsyncResult;
import android.os.Message;
import android.os.PersistableBundle;
import android.telephony.CarrierConfigManager;
import android.telephony.CarrierConfigManagerEx;
import android.telephony.SubscriptionManager;

public class SIMRecordsEx extends SIMRecords{

    private static final int EVENT_GET_MBDN_DONE = 6;
    private static final int EVENT_GET_CPHS_MAILBOX_DONE = 11;
    private static final int EVENT_GET_CFF_DONE = 24;
    private static final int EVENT_SIM_REFRESH = 31;
    private static final int EVENT_GET_CFIS_DONE = 32;
    private static final int EVENT_GET_CSP_CPHS_DONE = 33;
    private static final int EVENT_CARRIER_CONFIG_CHANGED = 37;
    public SIMRecordsEx(UiccCardApplication app, Context c, CommandsInterface ci) {
        super(app, c, ci);
    }

    @Override
    public void handleMessage(Message msg) {
        AsyncResult ar;

        if (mDestroyed.get()) {
            loge("SIMRecordsEx： Received message " + msg + "[" + msg.what + "] "
                    + " while being destroyed. Ignoring.");
            return;
        }

        switch (msg.what) {
        case EVENT_CARRIER_CONFIG_CHANGED:
            Intent intent = (Intent) msg.obj;
            int CONFIG_CHANGED_SUB = 2;
            if (intent != null) {
                int configChangedType = intent.getIntExtra(CarrierConfigManagerEx.CARRIER_CONFIG_CHANGED_TYPE, -1);
                log("configTag --> " + configChangedType);
                if (configChangedType == CONFIG_CHANGED_SUB) {
                    handleCarrierNameOverride();
                }
            }
            break;
        case EVENT_SIM_REFRESH:
            ar = (AsyncResult)msg.obj;
            log("Sim REFRESH with exception: " + ar.exception);
            if (ar.exception == null) {
                handleSimRefresh((IccRefreshResponse)ar.result);
            }
            break;

        default:
            super.handleMessage(msg);
        }
    }

    private void handleCarrierNameOverride() {
        log("!-----handleCarrierNameOverride----!");
        CarrierConfigManagerEx configLoader = CarrierConfigManagerEx.from(mContext);
        int subId[] = SubscriptionManager.getSubId(mParentApp.getPhoneId());
        PersistableBundle persistableBundle = null;
        if (subId == null) return;
              persistableBundle = configLoader.getConfigForSubId(subId[0]);
        if (persistableBundle != null && persistableBundle.getBoolean(
                     CarrierConfigManager.KEY_CARRIER_NAME_OVERRIDE_BOOL)) {
            String carrierName = persistableBundle.getString(
                    CarrierConfigManager.KEY_CARRIER_NAME_STRING);
            log("[carrierName] =" + carrierName);
            //setServiceProviderName(carrierName);
            mTelephonyManager.setSimOperatorNameForPhone(mParentApp.getPhoneId(),
                    carrierName);
            OperatorNameHandler.getInstance().updateSpnFromCarrierConfig(mParentApp.getPhoneId(),carrierName);
        } else {
            setSpnFromConfig(getOperatorNumeric());
        }
    }

    private void setSpnFromConfig(String carrier) {
        if (mSpnOverride.containsCarrier(carrier)) {
            setServiceProviderName(mSpnOverride.getSpn(carrier));
            mTelephonyManager.setSimOperatorNameForPhone(
                    mParentApp.getPhoneId(), getServiceProviderName());
        }
        OperatorNameHandler.getInstance().updateSpnFromCarrierConfig(mParentApp.getPhoneId(),
                getServiceProviderName());
    }

    private void handleSimRefresh(IccRefreshResponse refreshResponse){
        if (refreshResponse == null) {
            log("handleSimRefresh received without input");
            return;
        }

        if (refreshResponse.aid != null &&
                !refreshResponse.aid.equals(mParentApp.getAid())) {
            // This is for different app. Ignore.
            return;
        }

        switch (refreshResponse.refreshResult) {
            case IccRefreshResponse.REFRESH_RESULT_FILE_UPDATE:
                log("handleSimRefresh with SIM_FILE_UPDATED");
                handleFileUpdate(refreshResponse.efId);
                break;
            case IccRefreshResponse.REFRESH_RESULT_INIT:
                log("handleSimRefresh with SIM_REFRESH_INIT");
                // need to reload all files (that we care about)
                onIccRefreshInit();
                break;
            case IccRefreshResponse.REFRESH_RESULT_RESET:
                // Refresh reset is handled by the UiccCard object.
                log("handleSimRefresh with SIM_REFRESH_RESET");
                break;
            default:
                // unknown refresh operation
                log("handleSimRefresh with unknown operation");
                break;
        }
    }

    private void handleFileUpdate(int efid) {
        switch(efid) {
            case EF_MBDN:
                mRecordsToLoad++;
                new AdnRecordLoaderEx(mFh).loadFromEF(EF_MBDN, EF_EXT6,
                        mMailboxIndex, obtainMessage(EVENT_GET_MBDN_DONE));
                break;
            case EF_MAILBOX_CPHS:
                mRecordsToLoad++;
                new AdnRecordLoaderEx(mFh).loadFromEF(EF_MAILBOX_CPHS, EF_EXT1,
                        1, obtainMessage(EVENT_GET_CPHS_MAILBOX_DONE));
                break;
            case EF_CSP_CPHS:
                mRecordsToLoad++;
                log("[CSP] SIM Refresh for EF_CSP_CPHS");
                mFh.loadEFTransparent(EF_CSP_CPHS,
                        obtainMessage(EVENT_GET_CSP_CPHS_DONE));
                break;
            case EF_FDN:
                log("SIM Refresh called for EF_FDN");
                mParentApp.queryFdn();
                break;
            case EF_MSISDN:
                mRecordsToLoad++;
                log("SIM Refresh called for EF_MSISDN");
                new AdnRecordLoaderEx(mFh).loadFromEF(EF_MSISDN, getExtFromEf(EF_MSISDN), 1,
                        obtainMessage(EVENT_GET_MSISDN_DONE));
                break;
            case EF_CFIS:
            case EF_CFF_CPHS:
                log("SIM Refresh called for EF_CFIS or EF_CFF_CPHS");
                loadCallForwardingRecords();
                break;
            default:
                // For now, fetch all records if this is not a
                // voicemail number.
                // TODO: Handle other cases, instead of fetching all.
                mAdnCache.reset();
                fetchSimRecords();
                log("onRefresh handleSimRefresh with SIM_FILE_UPDATED");
                Intent intent = new Intent(TelephonyIntentsEx.ACTION_STK_REFRESH_SIM_CONTACTS);
                intent.putExtra("phone_id", mParentApp.getPhoneId());
                mContext.sendBroadcast(intent);
                break;
        }
    }

    private int getExtFromEf(int ef) {
        int ext;
        switch (ef) {
            case EF_MSISDN:
                /* For USIM apps use EXT5. (TS 31.102 Section 4.2.37) */
                if (mParentApp.getType() == AppType.APPTYPE_USIM) {
                    ext = EF_EXT5;
                } else {
                    ext = EF_EXT1;
                }
                break;
            default:
                ext = EF_EXT1;
        }
        return ext;
    }

    private void loadCallForwardingRecords() {
        mRecordsRequested = true;
        mFh.loadEFLinearFixed(EF_CFIS, 1, obtainMessage(EVENT_GET_CFIS_DONE));
        mRecordsToLoad++;
        mFh.loadEFTransparent(EF_CFF_CPHS, obtainMessage(EVENT_GET_CFF_DONE));
        mRecordsToLoad++;
    }

    protected void onIccRefreshInit(){
        super.onIccRefreshInit();
        log("ExtraIccRecords  onRefresh init");
        Intent intent = new Intent(TelephonyIntentsEx.ACTION_STK_REFRESH_SIM_CONTACTS);
        intent.putExtra("phone_id", mParentApp.getPhoneId());
        mContext.sendBroadcast(intent);
     }
}
