package com.android.internal.telephony;

import android.content.Context;
import android.content.res.Resources;

import com.android.internal.telephony.uicc.UsimFileHandler;
import com.android.internal.telephony.uicc.SIMFileHandler;
import com.android.internal.telephony.uicc.IccFileHandler;
import com.android.internal.telephony.uicc.SIMFileHandlerEx;
import com.android.internal.telephony.uicc.SIMRecords;
import com.android.internal.telephony.uicc.SIMRecordsEx;
import com.android.internal.telephony.uicc.UsimFileHandlerEx;
import com.android.internal.telephony.uicc.policy.PrimaryCardController;
import com.android.internal.telephony.uicc.AdnRecordCache;
import com.android.internal.telephony.uicc.AdnRecordCacheEx;
import com.android.internal.telephony.uicc.ExtraIccRecordsController;
import com.android.internal.telephony.uicc.AdnRecordCacheControllerEx;
import com.android.internal.telephony.uicc.UiccCardApplication;
import com.android.internal.telephony.uicc.IccCardProxy;
import com.android.internal.telephony.uicc.IccCardProxyEx;
import com.android.internal.telephony.uicc.IccRecords;
import com.android.internal.telephony.uicc.UiccCard;
import com.android.internal.telephony.cat.CatService;
import com.android.internal.telephony.cat.CatServiceSprd;
import com.android.internal.telephony.imsphone.ImsPhone;
import com.android.internal.telephony.imsphone.ImsPhoneCallTracker;
import com.android.internal.telephony.imsphone.ImsPhoneCallTrackerEx;
import com.android.internal.telephony.imsphone.ImsPhoneConnectionEx;
import com.android.internal.telephony.imsphone.ImsPhoneConnection;
import com.android.internal.telephony.imsphone.ImsPhoneCall;
import com.android.ims.ImsCall;
import com.android.internal.telephony.DataEnableController;
import com.android.internal.telephony.GlobalConfigController;

import android.content.Intent;
import android.os.UserHandle;
import android.app.ActivityManagerNative;
import android.util.Log;
import android.telephony.TelephonyManagerEx;

import com.android.internal.telephony.CommandsInterface;
import com.android.internal.telephony.GsmCdmaPhone;
import com.android.internal.telephony.PhoneNotifier;
import com.android.internal.telephony.TelephonyComponentFactory;
import com.android.internal.telephony.gsm.GsmMmiCode;
import com.android.internal.telephony.gsm.GsmMmiCodeEx;
import com.android.internal.telephony.uicc.UiccCardApplication;
import com.android.internal.telephony.dataconnection.DcTracker;
import com.android.internal.telephony.dataconnection.DcTrackerEx;

public class TeleFrameworkFactoryEx extends TeleFrameworkFactory {
    private static final String TAG = "TeleFrameworkFactoryEx";

    public void make(Context context, CommandsInterface[] ci) {
        PrimaryCardController.init(context);
        DataEnableController.init(context);
        ExtraIccRecordsController.init(context, ci);
        OperatorNameHandler.init(context);
        GlobalConfigController.getInstance().init(context);
    }

    /*SPRD: 474587 Feature for PhoneBook @{ */
    public AbsIccProvider createAbsIccProvider (Context context) {
        return new IccProviderExImpl(context);
    }

    public void initUiccPhoneBookControllerEx (Phone[] phone) {
        new UiccPhoneBookControllerEx(phone);
    }
    
    /*SPRD: 489257 Feature for CellBroadcastReceiver @{ */
    public void initUiccSmsControllerEx(Phone[] phone) {
        new UiccSmsControllerEx(phone);
    }
    /*SPRD: 489257 Feature for CellBroadcastReceiver @} */
    public IccPhoneBookInterfaceManager createIccPhoneBookInterfaceManager (Phone phone) {
        return new IccPhoneBookInterfaceManagerEx(phone);
    }

    public IccSmsInterfaceManager createIccSmsInterfaceManager (Phone phone) {
        return new IccSmsInterfaceManagerEx(phone);
    }

    public SIMFileHandler createSIMFileHandler (UiccCardApplication uicccardapplacation,
            String aid, CommandsInterface ci) {
        return new SIMFileHandlerEx(uicccardapplacation, aid, ci);
    }

    public UsimFileHandler createUsimFileHandler (UiccCardApplication uicccardapplacation,
            String aid, CommandsInterface ci) {
        return new UsimFileHandlerEx(uicccardapplacation, aid, ci);
    }

    public AdnRecordCache createAdnRecordCache (IccFileHandler fh) {
        return AdnRecordCacheControllerEx.createAdnRecordCache(fh);
    }

    public CatService createCatService(CommandsInterface ci,
            Context context, UiccCard ic, int slotId) {
        Log.d(TAG, "createCatService");
        return (CatService) CatServiceSprd.getInstance(ci, context, ic, slotId);
    }

    public boolean isSupportOrange() {
        Log.e(TAG,"isSupportOrange = " + Resources.getSystem().getBoolean(com.android.internal.R.bool.config_supportorangeCapable));
        return Resources.getSystem().getBoolean(com.android.internal.R.bool.config_supportorangeCapable);
    }

    public byte[] stringToGsmAlphaSS(String s)
            throws EncodeException {
        return GsmAlphabetEx.stringToGsmAlphaSS(s);
    }

    public byte[] isAsciiStringToGsm8BitUnpackedField(String s)
            throws EncodeException {
        return GsmAlphabetEx.isAsciiStringToGsm8BitUnpackedField(s);
    }

    public void broadcastFdnChangedDone(boolean iccFdnEnabled, boolean desiredFdnEnabled,
            int phoneId){
        if(iccFdnEnabled != desiredFdnEnabled){
            Log.e(TAG,"EVENT_CHANGE_FACILITY_FDN_DONE: " + desiredFdnEnabled + " phoneId " + phoneId);
            Intent intent = new Intent("android.intent.action.FDN_STATE_CHANGED" + phoneId);
            intent.putExtra(PhoneConstants.PHONE_KEY, phoneId);
            intent.putExtra(TelephonyIntentsEx.INTENT_KEY_FDN_STATUS, desiredFdnEnabled);
            ActivityManagerNative.broadcastStickyIntent(intent, null, UserHandle.USER_ALL);
         }
    }
    /* @} */

    /*SPRD: 510942 Feature for SIM Languages @{ */
    public void setSystemLocaleLock(String prefLang, String imsi , Context context){
        if(isSupportOrange()){
            SystemLocaleUtils.setSystemLocaleLock(prefLang,  imsi, context);
        }
    }
    /* @} */

    @Override
    public ImsPhoneCallTracker createImsPhoneCallTracker(ImsPhone phone) {
        return new ImsPhoneCallTrackerEx(phone);
    }

    @Override
    public GsmCdmaCallTracker createGsmCdmaCallTracker(GsmCdmaPhone phone) {
        return new GsmCdmaCallTrackerEx(phone);
    }

    @Override
    public GsmCdmaPhone createGsmCdmaPhone(Context context, CommandsInterface ci,
            PhoneNotifier notifier,
            int phoneId,
            int precisePhoneType, TelephonyComponentFactory telephonyComponentFactory) {
        return new GsmCdmaPhoneEx(context, ci, notifier, phoneId, precisePhoneType,
                telephonyComponentFactory);
    }

    @Override
    public GsmMmiCode createGsmMmiCode(GsmCdmaPhone phone, UiccCardApplication app) {
        return new GsmMmiCodeEx(phone, app);
    }

    @Override
    public GsmCdmaConnection createGsmCdmaConnection(GsmCdmaPhone phone, DriverCall dc,
            GsmCdmaCallTracker ct, int index) {
        return new GsmCdmaConnectionEx(phone, dc, ct, index);
    }

    /* SPRD: FEATURE_SUSPEND_DATA_WHEN_IN_CALL @{ */
    @Override
    public DcTracker makeDcTracker(Phone p) {
        return new DcTrackerEx(p);
    }
    /* SPRD: 474686 Feature for Uplmn @{ */
    public boolean isUsimCard(Context context, int phoneId) {
        return TelephonyManagerEx.from(context).isUsimCard(phoneId);
    }
    /* @} */

    @Override
    public void createOmaApnReceiver(Context context) {
        new OmaApnReceiver(context);
    }

    /*Bug #475223 Add global Global parameter adjust for carrier config*/
    public SIMRecords createExtraSIMRecords(UiccCardApplication app,Context c,CommandsInterface ci) {
        return new SIMRecordsEx(app, c, ci);
    }
    /* @} */

    @Override
    public IccCardProxy createIccCardProxy(Context context, CommandsInterface ci, int phoneId) {
        return new IccCardProxyEx(context, ci, phoneId);
    }

    /** This is probably an MT call */
    public ImsPhoneConnection createImsPhoneConnection(Phone phone, ImsCall imsCall, ImsPhoneCallTracker ct,
            ImsPhoneCall parent, boolean isUnknown){
        return new ImsPhoneConnectionEx(phone,imsCall,ct,parent,isUnknown);
    }

    /** This is an MO call, created when dialing */
    public ImsPhoneConnection createImsPhoneConnection(Phone phone, String dialString, ImsPhoneCallTracker ct,
            ImsPhoneCall parent, boolean isEmergency) {
        return new ImsPhoneConnectionEx(phone,dialString,ct,parent,isEmergency);
    }

    public void initPhoneSubInfoControllerEx (Context context, Phone[] phone) {
        new PhoneSubInfoControllerEx(context, phone);
    }
}
