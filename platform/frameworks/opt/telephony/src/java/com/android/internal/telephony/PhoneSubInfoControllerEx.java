package com.android.internal.telephony;

import android.app.AppOpsManager;
import android.content.Context;
import android.os.Binder;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.telephony.Rlog;
import android.telephony.SubscriptionManager;

import com.android.internal.telephony.Phone;
import com.android.internal.telephony.uicc.IsimRecords;

import static android.Manifest.permission.READ_PHONE_STATE;
import static android.Manifest.permission.READ_PRIVILEGED_PHONE_STATE;


public class PhoneSubInfoControllerEx extends IPhoneSubInfoEx.Stub {
    private static final String TAG = "PhoneSubInfoControllerEx";

    private final Phone[] mPhone;
    private final Context mContext;
    private final AppOpsManager mAppOps;

    public PhoneSubInfoControllerEx(Context context, Phone[] phone) {
        mPhone = phone;
        if (ServiceManager.getService("iphonesubinfoEx") == null) {
            ServiceManager.addService("iphonesubinfoEx", this);
        }
        mContext = context;
        mAppOps = (AppOpsManager) mContext.getSystemService(Context.APP_OPS_SERVICE);
    }

    private Phone getPhone(int subId) {
        int phoneId = SubscriptionManager.getPhoneId(subId);
        if (!SubscriptionManager.isValidPhoneId(phoneId)) {
            phoneId = 0;
        }
        return mPhone[phoneId];
    }

    public String getDeviceSvnUsingPhoneId(int phoneId, String callingPackage) {
        Phone phone = mPhone[phoneId];
        if (phone != null) {
            if (!checkReadPhoneState(callingPackage, "getDeviceSvn")) {
                return null;
            }
            return phone.getDeviceSvn();
        } else {
            loge("getDeviceSvn phone is null");
            return null;
        }
    }

    private boolean checkReadPhoneState(String callingPackage, String message) {
        try {
            mContext.enforceCallingOrSelfPermission(READ_PRIVILEGED_PHONE_STATE, message);

            // SKIP checking run-time OP_READ_PHONE_STATE since self or using PRIVILEGED
            return true;
        } catch (SecurityException e) {
            mContext.enforceCallingOrSelfPermission(READ_PHONE_STATE, message);
        }

        return mAppOps.noteOp(AppOpsManager.OP_READ_PHONE_STATE, Binder.getCallingUid(),
                callingPackage) == AppOpsManager.MODE_ALLOWED;
    }

    private void loge(String s) {
        Rlog.e(TAG, s);
    }
}
