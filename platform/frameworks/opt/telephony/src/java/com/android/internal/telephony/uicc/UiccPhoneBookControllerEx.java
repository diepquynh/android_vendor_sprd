package com.android.internal.telephony;

import android.os.ServiceManager;
import android.os.RemoteException;
import android.telephony.Rlog;

import com.android.internal.telephony.uicc.AdnRecordEx;
import com.android.internal.telephony.IIccPhoneBookEx;

import java.lang.ArrayIndexOutOfBoundsException;
import java.lang.NullPointerException;
import java.util.List;

/**
 * {@hide}
 */
public class UiccPhoneBookControllerEx extends IIccPhoneBookEx.Stub {
    private static final String TAG = "UiccPhoneBookControllerEx";
    private Phone[] mPhone;

    /* only one UiccPhoneBookController exists */
    public UiccPhoneBookControllerEx(Phone[] phone) {
        if (ServiceManager.getService("simphonebookEx") == null) {
            ServiceManager.addService("simphonebookEx", this);
        }
        mPhone = phone;
    }

    @Override
    public List<AdnRecordEx> getAdnRecordsInEfForSubscriber(int subId, int efid)
            throws android.os.RemoteException {
        IccPhoneBookInterfaceManager iccPbkIntMgr =
                getIccPhoneBookInterfaceManager(subId);
        if (iccPbkIntMgr != null) {
            return iccPbkIntMgr.getAdnRecordsInEfEx(efid);
        } else {
            Rlog.e(TAG, "getAdnRecordsInEfEx iccPbkIntMgr is"
                    + "null for Subscription:" + subId);
            return null;
        }
    }

    @Override
    public int[] getAdnRecordsSizeForSubscriber(int subId, int efid)
            throws android.os.RemoteException {
        IccPhoneBookInterfaceManager iccPbkIntMgr =
                getIccPhoneBookInterfaceManager(subId);
        if (iccPbkIntMgr != null) {
            return iccPbkIntMgr.getAdnRecordsSize(efid);
        } else {
            Rlog.e(TAG, "getAdnRecordsSizeEx iccPbkIntMgr is"
                    + " null for Subscription:" + subId);
            return null;
        }
    }

    @Override
    public boolean updateAdnRecordsInEfBySearchForSubscriber(int subId,
            int efid, String oldTag, String oldPhoneNumber, String newTag,
            String newPhoneNumber, String pin2)
            throws android.os.RemoteException {
        IccPhoneBookInterfaceManager iccPbkIntMgr =
                getIccPhoneBookInterfaceManager(subId);
        if (iccPbkIntMgr != null) {
            return iccPbkIntMgr.updateAdnRecordsInEfBySearch(efid,
                    oldTag, oldPhoneNumber, newTag, newPhoneNumber, pin2);
        } else {
            Rlog.e(TAG, "updateAdnRecordsInEfBySearchEx iccPbkIntMgr is"
                    + " null for Subscription:" + subId);
            return false;
        }
    }

    @Override
    public int updateAdnRecordsInEfBySearchForSubscriberEx(int subId, int efid,
            String oldTag, String oldPhoneNumber, String[] oldEmailList,
            String oldAnr, String oldSne, String oldGrp, String newTag,
            String newPhoneNumber, String[] newEmailList, String newAnr,
            String newAas, String newSne, String newGrp, String newGas,
            String pin2) throws android.os.RemoteException {
        IccPhoneBookInterfaceManager iccPbkIntMgr =
                getIccPhoneBookInterfaceManager(subId);
        if (iccPbkIntMgr != null) {
            return iccPbkIntMgr.updateAdnRecordsInEfBySearch(efid, oldTag,
                    oldPhoneNumber, oldEmailList, oldAnr, oldSne, oldGrp,
                    newTag, newPhoneNumber, newEmailList, newAnr, newAas,
                    newSne, newGrp, newGas, pin2);
        } else {
            Rlog.e(TAG, "updateAdnRecordsInEfBySearch iccPbkIntMgr is"
                    + " null for Subscription:" + subId);
            return -1;
        }
    }

    @Override
    public int updateAdnRecordsInEfByIndexForSubscriber(int subId, int efid,
            String newTag, String newPhoneNumber, String[] newEmailList,
            String newAnr, String newAas, String newSne, String newGrp,
            String newGas, int index, String pin2) {
        IccPhoneBookInterfaceManager iccPbkIntMgr =
                getIccPhoneBookInterfaceManager(subId);
        if (iccPbkIntMgr != null) {
            return iccPbkIntMgr.updateAdnRecordsInEfByIndex(efid, newTag,
                    newPhoneNumber, newEmailList, newAnr, newAas, newSne,
                    newGrp, newGas, index, pin2);
        } else {
            Rlog.e(TAG, "updateAdnRecordsInEfBySearch iccPbkIntMgr is"
                    + " null for Subscription:" + subId);
            return -1;
        }
    }

    /**
     * get phone book interface manager object based on subscription.
     **/
    private IccPhoneBookInterfaceManager
            getIccPhoneBookInterfaceManager(int subId) {

        int phoneId = SubscriptionController.getInstance().getPhoneId(subId);
        try {
            return mPhone[phoneId].getIccPhoneBookInterfaceManager();
        } catch (NullPointerException e) {
            Rlog.e(TAG, "Exception is :" + e.toString() + " For subscription :" + subId);
            e.printStackTrace(); // To print stack trace
            return null;
        } catch (ArrayIndexOutOfBoundsException e) {
            Rlog.e(TAG, "Exception is :" + e.toString() + " For subscription :" + subId);
            e.printStackTrace();
            return null;
        }
    }

    private int getDefaultSubscription() {
        return PhoneFactory.getDefaultSubscription();
    }

    @Override
    public int updateUsimGroupBySearchForSubscriber(int subId, String oldName,
            String newName) {
        IccPhoneBookInterfaceManager iccPbkIntMgr =
                getIccPhoneBookInterfaceManager(subId);
        if (iccPbkIntMgr != null) {
            return iccPbkIntMgr.updateUsimGroupBySearch(oldName, newName);
        } else {
            Rlog.e(TAG,
                    "updateUsimGroupBySearchForSubscriber iccPbkIntMgr is"
                            + " null for Subscription:" + subId);
            return -1;
        }
    }

    @Override
    public int updateUsimGroupByIndexForSubscriber(int subId, String newName,
            int groupId) {
        IccPhoneBookInterfaceManager iccPbkIntMgr =
                getIccPhoneBookInterfaceManager(subId);
        if (iccPbkIntMgr != null) {
            return iccPbkIntMgr.updateUsimGroupByIndex(newName, groupId);
        } else {
            Rlog.e(TAG, "updateUsimGroupByIdForSubscriber iccPbkIntMgr is"
                    + " null for Subscription:" + subId);
            return -1;
        }
    }

    @Override
    public List<String> getGasInEfForSubscriber(int subId) {
        IccPhoneBookInterfaceManager iccPbkIntMgr =
                getIccPhoneBookInterfaceManager(subId);
        if (iccPbkIntMgr != null) {
            return iccPbkIntMgr.getGasInEf();
        } else {
            Rlog.e(TAG, "getGasInEfForSubscriber iccPbkIntMgr is"
                    + " null for Subscription:" + subId);
            return null;
        }
    }

    @Override
    public boolean isApplicationOnIcc(int type, int subId) {
        IccPhoneBookInterfaceManager iccPbkIntMgr =
                getIccPhoneBookInterfaceManager(subId);
        if (iccPbkIntMgr != null) {
            return iccPbkIntMgr.isApplicationOnIcc(type);
        } else {
            Rlog.e(TAG, "isApplicationOnIcc iccPbkIntMgr is"
                    + " null for Subscription:" + subId);
            return false;
        }
    }

    @Override
    public int[] getEmailRecordsSize(int subId)
            throws android.os.RemoteException {
        IccPhoneBookInterfaceManager iccPbkIntMgr =
                getIccPhoneBookInterfaceManager(subId);
        if (iccPbkIntMgr != null) {
            return iccPbkIntMgr.getEmailRecordsSize();
        } else {
            Rlog.e(TAG, "getEmailRecordsSize iccPbkIntMgr is"
                    + " null for Subscription:" + subId);
            return null;
        }
    }

    @Override
    public int[] getAnrRecordsSize(int subId) throws android.os.RemoteException {
        IccPhoneBookInterfaceManager iccPbkIntMgr =
                getIccPhoneBookInterfaceManager(subId);
        if (iccPbkIntMgr != null) {
            return iccPbkIntMgr.getAnrRecordsSize();
        } else {
            Rlog.e(TAG, "getAnrRecordsSize iccPbkIntMgr is"
                    + " null for Subscription:" + subId);
            return null;
        }
    }

    @Override
    public int getAnrNum(int subId) throws android.os.RemoteException {
        IccPhoneBookInterfaceManager iccPbkIntMgr =
                getIccPhoneBookInterfaceManager(subId);
        if (iccPbkIntMgr != null) {
            return iccPbkIntMgr.getAnrNum();
        } else {
            Rlog.e(TAG, "getAnrNum iccPbkIntMgr is"
                    + " null for Subscription:" + subId);
            return 0;
        }
    }

    @Override
    public int getEmailNum(int subId) throws android.os.RemoteException {
        IccPhoneBookInterfaceManager iccPbkIntMgr =
                getIccPhoneBookInterfaceManager(subId);
        if (iccPbkIntMgr != null) {
            return iccPbkIntMgr.getEmailNum();
        } else {
            Rlog.e(TAG, "getEmailNum iccPbkIntMgr is"
                    + " null for Subscription:" + subId);
            return 0;
        }
    }

    @Override
    public int getInsertIndex(int subId) {
        IccPhoneBookInterfaceManager iccPbkIntMgr =
                getIccPhoneBookInterfaceManager(subId);
        if (iccPbkIntMgr != null) {
            return iccPbkIntMgr.getInsertIndex();
        } else {
            Rlog.e(TAG, "getInsertIndex iccPbkIntMgr is"
                    + " null for Subscription:" + subId);
            return -1;
        }
    }

    @Override
    public int[] getAvalibleEmailCount(String name, String number,
            String[] emails, String anr, int[] emailNums, int subId)
            throws android.os.RemoteException {
        IccPhoneBookInterfaceManager iccPbkIntMgr =
                getIccPhoneBookInterfaceManager(subId);
        if (iccPbkIntMgr != null) {
            return iccPbkIntMgr.getAvalibleEmailCount(name, number,
                    emails, anr, emailNums);
        } else {
            Rlog.e(TAG, "getAvalibleEmailCount iccPbkIntMgr is"
                    + " null for Subscription:" + subId);
            return null;
        }
    }

    @Override
    public int[] getAvalibleAnrCount(String name, String number,
            String[] emails, String anr, int[] anrNums, int subId)
            throws android.os.RemoteException {
        IccPhoneBookInterfaceManager iccPbkIntMgr =
                getIccPhoneBookInterfaceManager(subId);
        if (iccPbkIntMgr != null) {
            return iccPbkIntMgr.getAvalibleAnrCount(name, number, emails,
                    anr, anrNums);
        } else {
            Rlog.e(TAG, "getAvalibleAnrCount iccPbkIntMgr is"
                    + " null for Subscription:" + subId);
            return null;
        }
    }

    @Override
    public int getEmailMaxLen(int subId) throws android.os.RemoteException {
        IccPhoneBookInterfaceManager iccPbkIntMgr =
                getIccPhoneBookInterfaceManager(subId);
        if (iccPbkIntMgr != null) {
            return iccPbkIntMgr.getEmailMaxLen();
        } else {
            Rlog.e(TAG, "getEmailMaxLen iccPbkIntMgr is"
                    + " null for Subscription:" + subId);
            return 0;
        }
    }

    @Override
    public int getPhoneNumMaxLen(int subId) throws android.os.RemoteException {
        IccPhoneBookInterfaceManager iccPbkIntMgr =
                getIccPhoneBookInterfaceManager(subId);
        if (iccPbkIntMgr != null) {
            return iccPbkIntMgr.getPhoneNumMaxLen();
        } else {
            Rlog.e(TAG, "getPhoneNumMaxLen iccPbkIntMgr is"
                    + " null for Subscription:" + subId);
            return 0;
        }
    }

    @Override
    public int getUsimGroupNameMaxLen(int subId) {
        IccPhoneBookInterfaceManager iccPbkIntMgr =
                getIccPhoneBookInterfaceManager(subId);
        if (iccPbkIntMgr != null) {
            return iccPbkIntMgr.getUsimGroupNameMaxLen();
        } else {
            Rlog.e(TAG, "getUsimGroupNameMaxLen iccPbkIntMgr is"
                    + " null for Subscription:" + subId);
            return 0;
        }
    }

    @Override
    public int getUsimGroupCapacity(int subId) {
        IccPhoneBookInterfaceManager iccPbkIntMgr =
                getIccPhoneBookInterfaceManager(subId);
        int usimGroupCapacity = 0;
        if (iccPbkIntMgr != null) {
            usimGroupCapacity = iccPbkIntMgr.getUsimGroupCapacity();
        } else {
            Rlog.e(TAG, "getUsimGroupCapacity iccPbkIntMgr is"
                    + " null for Subscription:" + subId);
        }
        Rlog.i(TAG, "usimGroupCapacity = " + usimGroupCapacity);
        return usimGroupCapacity;
    }

    @Override
    public List<String> getAasInEfForSubscriber(int subId) {
        IccPhoneBookInterfaceManager iccPbkIntMgr =
                getIccPhoneBookInterfaceManager(subId);
        if (iccPbkIntMgr != null) {
            return iccPbkIntMgr.getAasInEf();
        }
        return null;
    }

    @Override
    public int updateUsimAasBySearchForSubscriber(String oldName,
            String newName, int subId) {
        IccPhoneBookInterfaceManager iccPbkIntMgr =
                getIccPhoneBookInterfaceManager(subId);
        int usimAas = 0;
        if (iccPbkIntMgr != null) {
            usimAas = iccPbkIntMgr.updateUsimAasBySearch(oldName, newName);
        }
        Rlog.d(TAG, "updateUsimAasBySearch, usimAas = " + usimAas);
        return usimAas;
    }

    @Override
    public int updateUsimAasByIndexForSubscriber(String newName, int aasIndex,
            int subId) {
        IccPhoneBookInterfaceManager iccPbkIntMgr =
                getIccPhoneBookInterfaceManager(subId);
        int usimAas = 0;
        if (iccPbkIntMgr != null) {
            usimAas = iccPbkIntMgr.updateUsimAasByIndex(newName, aasIndex);
        }
        Rlog.d(TAG, "updateUsimAasByIndex, usimAas = " + usimAas);
        return usimAas;
    }

    @Override
    public int getSneSize(int subId) {
        IccPhoneBookInterfaceManager iccPbkIntMgr =
                getIccPhoneBookInterfaceManager(subId);
        int sneSize = 0;
        if (iccPbkIntMgr != null) {
            sneSize = iccPbkIntMgr.getSneSize();
        }
        return sneSize;
    }

    @Override
    public int[] getSneLength(int subId) {
        IccPhoneBookInterfaceManager iccPbkIntMgr =
                getIccPhoneBookInterfaceManager(subId);
        if (iccPbkIntMgr != null) {
            return iccPbkIntMgr.getSneLength();
        }
        return null;
    }
}
