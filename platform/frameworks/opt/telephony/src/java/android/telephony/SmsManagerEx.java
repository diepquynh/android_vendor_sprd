package android.telephony;

import android.app.ActivityThread;
import android.app.PendingIntent;
import android.content.Context;

import com.android.internal.telephony.ISmsEx;
import com.android.internal.telephony.fdnsvr.FdnServiceManger.FdnInterface;
import com.android.internal.telephony.fdnsvr.FdnServiceManger;
import com.android.internal.telephony.fdnsvr.FdnCommandUtil;
import com.android.internal.telephony.fdnsvr.FdnServiceUtil;

import android.telephony.TelephonyManagerEx;
import android.os.SystemProperties;
import android.os.ServiceManager;
import android.text.TextUtils;
import android.os.RemoteException;
import android.provider.Settings;
import android.util.Log;
import android.text.format.Time;
import com.android.internal.telephony.ISms;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import static android.telephony.TelephonyManager.PHONE_TYPE_CDMA;

public class SmsManagerEx{
    private static final String TAG = "SmsManagerEx";
    private FdnServiceManger mFdnServiceManger;

    private TelephonyManagerEx mTelephonyManagerEx;

     /**
     * A psuedo-subId that represents the default subId at any given time. The actual subId it
     * represents changes as the default subId is changed.
     */
    private static final int DEFAULT_SUBSCRIPTION_ID = -1002;

    /** Singleton object constructed during class initialization. */
    private static final SmsManagerEx sInstance = new SmsManagerEx(DEFAULT_SUBSCRIPTION_ID);

    /** A concrete subscription id, or the pseudo DEFAULT_SUBSCRIPTION_ID */
    private int mSubId;

    /**
     * Get the SmsManager associated with the default subscription id. The instance will always be
     * associated with the default subscription id, even if the default subscription id is changed.
     *
     * @return the SmsManager associated with the default subscription id
     */
    public static SmsManagerEx getDefault() {
        return sInstance;
    }
    //add bug 612674 --begin
    public static SmsManagerEx getSubActivity(int subID){
        return new SmsManagerEx(subID);
    }
    //add bug 612674 --end
    private SmsManagerEx(int subId) {
        mTelephonyManagerEx = TelephonyManagerEx.from(ActivityThread.currentApplication().getApplicationContext());
        mSubId = subId;
    }

    private class FdnFilterImpl implements FdnInterface {

        @Override
        public boolean isNeedFilterMessaging() {
            String tempt = Settings.Global.getString(ActivityThread.currentApplication()
                    .getApplicationContext().getContentResolver(),"ro.messag.fdnfilter");
            if (TextUtils.isEmpty(tempt)) {
                return true;
            } else {
                if(Integer.valueOf(tempt) == 0){
                    return false;
                }
            }
            return false;
        }

        @Override
        public boolean getFdnEnable() {
            try {
                //add bug 612674 --begin
                /*return mTelephonyManagerEx.getIccFdnEnabled(SubscriptionManager
                        .getDefaultSubscriptionId());*/
                return mTelephonyManagerEx.getIccFdnEnabled(mSubId);
                //add bug 612674 --end
            } catch (Exception e) {
                e.printStackTrace();
            }
            return false;
        }
    }

    public boolean checkFdnContact(String destinationAddress) {
        final Context context = ActivityThread.currentApplication()
                .getApplicationContext();
        byte[] bs = new byte[64];
        boolean flag = false;

        if (mFdnServiceManger == null) {
            mFdnServiceManger = new FdnServiceManger(context);
            mFdnServiceManger.setFdnListner(new FdnFilterImpl());
        }

        List<String> listContacts = new ArrayList<String>();
        listContacts.add(destinationAddress);
        //add bug 612674 --begin
        int resault = mFdnServiceManger.process(context,
                FdnCommandUtil.COMPARE_SINGEL_NUMBER,
                //SubscriptionManager.getDefaultSubscriptionId(), listContacts, bs);
                mSubId, listContacts, bs);
        //add bug 612674 --end
        if(resault > 0){
           Log.e(TAG, "mService.process is ok and filtering is well");
           flag = false;
        }else if(resault == 0){
           Log.e(TAG, "mService.process is no need to filter or fdn disenable.");
           flag = false;
        }else{
           Log.e(TAG, "mService.process is end, and filtering is no match");
           flag = true;// can' t sending ok , reture failed for sening action
        }
        Log.e(TAG, "Sms checkFdnContacts is successful");
        return flag;
    }

    public boolean[] checkFdnContacts(List<String> listContacts) {
        final Context context = ActivityThread.currentApplication()
                .getApplicationContext();
        boolean[] enable = new boolean[listContacts.size()];
        byte[] bs = new byte[64];

        if (mFdnServiceManger == null) {
            mFdnServiceManger = new FdnServiceManger(context);
            mFdnServiceManger.setFdnListner(new FdnFilterImpl());
        }

        List<String> tempList = new ArrayList<String>(1);
        for(int i = 0; i < listContacts.size() ; i++){
            tempList.add(listContacts.get(i));
            Log.i(TAG, "mService.process is begin");
            Log.i(TAG, "destination --->["+listContacts.get(i)+"]");
            //add bug 612674 --begin
            int resault = mFdnServiceManger.process(context, FdnCommandUtil.COMPARE_SINGEL_NUMBER ,
                    //SubscriptionManager.getDefaultSubscriptionId(), tempList, bs);
                    mSubId, tempList, bs);
            //add bug 612674 --begin
            if(resault > 0){
               Log.i(TAG, "mService.process is ok and filtering is well");
               enable[i] = false;
            }else if(resault == 0){
               Log.i(TAG, "mService.process is no need to filter or fdn disenable.");
               enable[i] = false;
            }else{
               Log.i(TAG, "mService.process is end, and filtering is no match");
               enable[i] = true;// can' t sending ok , reture failed for sening action
            }
            tempList.remove(0);
        }
        Log.i(TAG, "Mms checkFdnContacts is successful");
        return enable;
    }

    public int copyMessageToIccEfWithResult(String scAddr, String destAddr, String message, int status, long time, int subId) {
        Time time1 = new Time(Time.TIMEZONE_UTC);
        time1.set(time);
        byte[] data = getPdu(destAddr, message, status, time1);
        if (data == null) {
            Log.d(TAG, "copyMessageToIccEfWithResult ===> data is null ! ");
            return 0;
        }
        return copyMessageToIcc(null, data, status, subId);
    }

    private int copyMessageToIcc(byte[] smsc, byte[] pdu, int status, int subId) {
        int index = 0;
        if (null == pdu) {
            throw new IllegalArgumentException("pdu is NULL");
        }
        try {
            ISmsEx iccISms = getISmsExService();
            if (iccISms != null) {
                index = iccISms.copyMessageToIccEfForSubscriber(subId,
                        ActivityThread.currentPackageName(),
                        status, pdu, smsc);
                Log.d(TAG, "copyMessageToIcc subId:[" + subId + "]success:[" + index + "]");
            }
        } catch (RemoteException ex) {
            // ignore it
        }

        return index;
    }

    private static ISmsEx getISmsExService() {
        return ISmsEx.Stub.asInterface(ServiceManager.getService("ismsEx"));
    }

    //489223 begin
    public static byte[] getPdu(String destAddr, String message, int status, Time time) {
        byte[] data;
        int activePhone = TelephonyManager.getDefault().getPhoneType();
        if (PHONE_TYPE_CDMA == activePhone) {
            return null;
        } else {
            data = com.android.internal.telephony.gsm.SmsMessage.getReceivedPdu(destAddr, message, time);
        }
        return data;
    }
    //489223 end

};
