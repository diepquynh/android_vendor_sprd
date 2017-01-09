package com.android.internal.telephony;

import android.content.Context;
import com.android.internal.telephony.dataconnection.DcTracker;

import java.lang.reflect.Constructor;

import com.android.internal.telephony.CommandsInterface;
import com.android.internal.telephony.GsmCdmaPhone;
import com.android.internal.telephony.PhoneNotifier;
import com.android.internal.telephony.TelephonyComponentFactory;
import com.android.internal.telephony.uicc.UiccCardApplication;
import com.android.internal.telephony.gsm.GsmMmiCode;

import android.util.Log;

import com.android.internal.telephony.uicc.IccCardProxy;
import com.android.internal.telephony.uicc.IccFileHandler;
import com.android.internal.telephony.uicc.UsimFileHandler;
import com.android.internal.telephony.uicc.SIMFileHandler;
import com.android.internal.telephony.uicc.SIMRecords;
import com.android.internal.telephony.uicc.AdnRecordCache;
import com.android.internal.telephony.uicc.UiccCardApplication;
import com.android.internal.telephony.uicc.AdnRecordCacheController;
import com.android.internal.telephony.uicc.UiccCard;
import com.android.ims.ImsCall;
import com.android.internal.telephony.imsphone.ImsPhoneCall;
import com.android.internal.telephony.imsphone.ImsPhoneCallTracker;
import com.android.internal.telephony.imsphone.ImsPhone;
import com.android.internal.telephony.imsphone.ImsPhoneConnection;
import com.android.internal.telephony.cat.CatService;
import com.android.internal.telephony.cat.TextMessage;
import com.android.internal.telephony.cat.TextMessageSprd;
import com.android.internal.telephony.uicc.UiccCard;

public class TeleFrameworkFactory {
    private static final String TAG = "TeleFrameworkFactory";
    private static TeleFrameworkFactory sInstance;

    public synchronized static TeleFrameworkFactory getInstance() {
        if (sInstance != null) {
            return sInstance;
        }

        Class clazz = null;
        // Load vendor specific factory
        try {
            clazz = Class.forName("com.android.internal.telephony.TeleFrameworkFactoryEx");
        } catch (Throwable t) {
            Log.d(TAG, "Can't find specific TeleFrameworkFactoryEx");
        }

        if (clazz != null) {
            try {
                Constructor ctor = clazz.getDeclaredConstructor();
                if (ctor != null) {
                    sInstance = (TeleFrameworkFactory) ctor.newInstance();
                }
            } catch (Throwable t) {
                Log.e(TAG, "Can't create specific ObjectFactory");
            }
        }

        if (sInstance == null) {
            // Fallback to default factory
            sInstance = new TeleFrameworkFactory();
        }
        return sInstance;
    }

    public void make(Context context, CommandsInterface[] ci) {
        // do nothing
    }

    /*SPRD: 474587 Feature for PhoneBook @{ */
    public AbsIccProvider createAbsIccProvider (Context context) {
        return null;
    }

    public void initUiccPhoneBookControllerEx (Phone[] phone) {

    }
   
    //BUG489257 Feature for CellBroadcastReceiver start
    public void initUiccSmsControllerEx (Phone[] phone) {

    }
  //BUG489257 Feature for CellBroadcastReceiver end
    
    public IccPhoneBookInterfaceManager createIccPhoneBookInterfaceManager (Phone phone) {
        return new IccPhoneBookInterfaceManager(phone);
    }

    public IccSmsInterfaceManager createIccSmsInterfaceManager (Phone phone) {
        return new IccSmsInterfaceManager(phone);
    }

    public SIMFileHandler createSIMFileHandler (UiccCardApplication uicccardapplacation,
            String aid, CommandsInterface ci) {
        return new SIMFileHandler(uicccardapplacation, aid, ci);
    }

    public UsimFileHandler createUsimFileHandler (UiccCardApplication uicccardapplacation,
            String aid, CommandsInterface ci) {
        return new UsimFileHandler(uicccardapplacation, aid, ci);
    }

    public AdnRecordCache createAdnRecordCache (IccFileHandler fh) {
        return AdnRecordCacheController.createAdnRecordCache(fh);
    }

    public CatService createCatService(CommandsInterface ci,
            Context context, UiccCard ic, int slotId) {
        Log.d(TAG, "createCatService");
        return CatService.getInstance(ci, context, ic, slotId);
    }

    public TextMessage createTextMessage() {
        Log.d(TAG, "createCatTextMessage");
        return (TextMessage)new TextMessageSprd();
    }

    public boolean isSupportOrange() {
        return false;
    }

    public byte[] stringToGsmAlphaSS(String s)
            throws EncodeException {
        return null;
    }

    public byte[] isAsciiStringToGsm8BitUnpackedField(String s)
            throws EncodeException {
        return null;
    }

    public void broadcastFdnChangedDone(boolean iccFdnEnabled, boolean desiredFdnEnabled,
            int phoneId){

    }
    /* @} */

    /*SPRD: 510942 Feature for SIM Languages @{ */
    public void setSystemLocaleLock(String prefLang, String imsi , Context context){
    }
    /* @} */

    public ImsPhoneCallTracker createImsPhoneCallTracker(ImsPhone phone) {
        return new ImsPhoneCallTracker(phone,true);
    }

    public GsmCdmaPhone createGsmCdmaPhone(Context context, CommandsInterface ci,
            PhoneNotifier notifier,
            int phoneId,
            int precisePhoneType, TelephonyComponentFactory telephonyComponentFactory) {
        return new GsmCdmaPhone(context, ci, notifier, phoneId, precisePhoneType,
                telephonyComponentFactory);
    }

    public GsmMmiCode createGsmMmiCode(GsmCdmaPhone phone, UiccCardApplication app) {
        return new GsmMmiCode(phone, app);
    }

    public GsmCdmaConnection createGsmCdmaConnection(GsmCdmaPhone phone, DriverCall dc,
            GsmCdmaCallTracker ct, int index) {
        return new GsmCdmaConnection(phone, dc, ct, index);
    }

    /* SPRD: FEATURE_SUSPEND_DATA_WHEN_IN_CALL @{ */
    public DcTracker makeDcTracker(Phone phone) {
        return new DcTracker(phone);
    }
    /* @} */

    public void createOmaApnReceiver(Context context) {

    }

    /* SPRD: Feature for Uplmn @{*/
    public boolean isUsimCard(Context context, int phoneId) {
        return false;
    }
    /* @} */

    /*Bug #475223 Add global Global parameter adjust for carrier config**/
    public SIMRecords createExtraSIMRecords(UiccCardApplication app,Context c,CommandsInterface ci) {
        return new SIMRecords(app, c, ci);
    }

    public IccCardProxy createIccCardProxy(Context context, CommandsInterface ci, int phoneId) {
        return new IccCardProxy(context, ci, phoneId);
    }

    public GsmCdmaCallTracker createGsmCdmaCallTracker(GsmCdmaPhone phone) {
        return new GsmCdmaCallTracker(phone);
    }

    /** This is probably an MT call */
    public ImsPhoneConnection createImsPhoneConnection(Phone phone, ImsCall imsCall, ImsPhoneCallTracker ct,
            ImsPhoneCall parent, boolean isUnknown){
        return new ImsPhoneConnection(phone,imsCall,ct,parent,isUnknown);
    }

    /** This is an MO call, created when dialing */
    public ImsPhoneConnection createImsPhoneConnection(Phone phone, String dialString, ImsPhoneCallTracker ct,
            ImsPhoneCall parent, boolean isEmergency) {
        return new ImsPhoneConnection(phone,dialString,ct,parent,isEmergency);
    }

    //SPRD for CMCC new case:Mobile card priority strategy
    public void setDefaultDataSubId(Context context,int subId) {
        TelephonyPluginUtils.getInstance().setDefaultDataSubId(context, subId);
    }

    public void initPhoneSubInfoControllerEx (Context context, Phone[] phone) {

    }
}
