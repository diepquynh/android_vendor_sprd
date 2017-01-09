
package android.telephony;

import com.android.internal.telephony.ITelephony;

import android.app.ActivityThread;
import android.os.PersistableBundle;
import android.telephony.TelephonyManager;
import android.content.Context;
import android.net.Uri;
import android.os.ServiceManager;
import com.android.internal.telephony.ITelephonyEx;
import com.android.internal.telephony.IccCardConstantsEx.State;
import com.android.internal.telephony.TelephonyProperties;

import android.os.Bundle;
import android.os.RemoteException;
import android.util.Log;

import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;

import android.os.SystemProperties;

import com.android.internal.telephony.IPhoneSubInfo;
import com.android.internal.telephony.TelephonyProperties;

import android.content.Context;
import android.content.Intent;
import android.telecom.VideoProfile;

import android.os.SystemProperties;
import android.os.UserHandle;

public class TelephonyManagerEx extends AbsTelephonyManager {

    private static final String TAG = "TelephonyManagerEx";

    private final Context mContext;
    private final int mSubId;

    /** @hide */
    public TelephonyManagerEx(Context context) {
      this(context, SubscriptionManager.DEFAULT_SUBSCRIPTION_ID);
    }

    /** @hide */
    public TelephonyManagerEx(Context context, int subId) {
        mSubId = subId;
        Context appContext = context.getApplicationContext();
        if (appContext != null) {
            mContext = appContext;
        } else {
            mContext = context;
        }
    }

    /** {@hide} */
    public static TelephonyManagerEx from(Context context) {
        return (TelephonyManagerEx) context.getSystemService("phone_ex");
    }

    /**
     * Create a new TelephonyManagerEx object pinned to the given subscription ID.
     *
     * @return a TelephonyManagerEx that uses the given subId for all calls.
     */
    public TelephonyManagerEx createForSubscriptionId(int subId) {
        // Don't reuse any TelephonyManager objects.
        return new TelephonyManagerEx(mContext, subId);
    }

    /**
     * Return an appropriate subscription ID for any situation.
     *
     * If this object has been created with {@link #createForSubscriptionId}, then the provided
     * subId is returned. Otherwise, the default subId will be returned.
     */
    private int getSubId() {
      if (mSubId == SubscriptionManager.DEFAULT_SUBSCRIPTION_ID) {
        return getDefaultSubscription();
      }
      return mSubId;
    }

    /**
     * Returns Default subscription.
     */
    private static int getDefaultSubscription() {
        return SubscriptionManager.getDefaultSubscriptionId();
    }

    private String getOpPackageName() {
        // For legacy reasons the TelephonyManager has API for getting
        // a static instance with no context set preventing us from
        // getting the op package name. As a workaround we do a best
        // effort and get the context from the current activity thread.
        if (mContext != null) {
            return mContext.getOpPackageName();
        }
        return ActivityThread.currentOpPackageName();
    }

    /**
     * @hide
     */
     private ITelephonyEx getITelephonyEx() {
         return ITelephonyEx.Stub.asInterface(ServiceManager.getService("phone_ex"));
     }

     /**
      * @hide
      */
     private ITelephony getITelephony() {
         return ITelephony.Stub.asInterface(ServiceManager.getService(Context.TELEPHONY_SERVICE));
     }

    /**
     * @hide
     */
    private static State getSimStateFromProperty(int phoneId) {
        State state = State.UNKNOWN;
        String simState = TelephonyManager.getTelephonyProperty(phoneId,
                TelephonyProperties.PROPERTY_SIM_STATE, "");
        if (simState != null && simState.length() > 0 ) {
            try {
                state = State.valueOf(simState);
            } catch (IllegalArgumentException e) {
                e.printStackTrace();
            }
        }
        return state;
    }

     /*SPRD: 474587 Feature for PhoneBook @{ */
     public boolean getIccFdnEnabled(int subId) {
       try {
             return getITelephonyEx().getIccFdnEnabledForSubscriber(subId);
         } catch (RemoteException ex) {
             // Assume no ICC card if remote exception which shouldn't happen
         } catch (NullPointerException ex) {
             // This could happen before phone restarts due to crashing
         }
         return false;
     }
     /* @} */

     /**
      * Uicc carrier privilege rules can't work fine now.
      * This interface will ignore carrier privilege for caller.
      * @hide
      */
     public boolean setLine1NumberForDisplayForSubscriberEx(int subId, String alphaTag, String number) {
         try {
             return getITelephonyEx().setLine1NumberForDisplayForSubscriberEx(subId, alphaTag, number);
         } catch (RemoteException ex) {
             Log.e(TAG, "RemoteException calling ITelephony#setLine1NumberForDisplayForSubscriberEx", ex);
         } catch (NullPointerException ex) {
             Log.e(TAG, "NullPointerException calling ITelephony#setLine1NumberForDisplayForSubscriberEx", ex);
         }
         return false;
     }

    /**
     * Returns the Service Provider Name (SPN).
     * <p>
     * Availability: SIM state must be {@link #SIM_STATE_READY}
     *
     * @see #getSimState
     * @param subId for which SimOperatorName is returned
     * @hide
     */
    public String getSimOperatorNameForSubscription(int subId) {
        int phoneId = SubscriptionManager.getPhoneId(subId);
        return getSimOperatorNameForPhone(phoneId);
    }

    /**
     * Returns the Service Provider Name (SPN).
     *
     * @hide
     */
    public String getSimOperatorNameForPhone(int phoneId) {
        return getTelephonyProperty(phoneId,
                TelephonyProperties.PROPERTY_ICC_OPERATOR_ALPHA, "");
    }

    /**
     * Gets the telephony property.
     *
     * @hide
     */
    public static String getTelephonyProperty(int phoneId, String property, String defaultVal) {
        String propVal = null;
        String prop = SystemProperties.get(property);
        if ((prop != null) && (prop.length() > 0)) {
            String values[] = prop.split(",");
            if ((phoneId >= 0) && (phoneId < values.length) && (values[phoneId] != null)) {
                propVal = values[phoneId];
            }
        }
        return propVal == null ? defaultVal : propVal;
    }

     /**
      * SPRD Add For SmartCardService.
      * Remove hide from TelephonyManager#iccExchangeSimIO
      */
     public byte[] iccExchangeSimIO(int subId, int fileID, int command, int p1, int p2,
             int p3, String filePath) {
         try {
             ITelephony telephony = getITelephony();
             if (telephony != null)
                 return telephony.iccExchangeSimIO(subId, fileID, command, p1, p2, p3, filePath);
         } catch (RemoteException ex) {
         } catch (NullPointerException ex) {
         }
         return null;
     }

     /**
      * SPRD Add For Data/Primary card PhoneId/SlotId.
      * Remove hide from SubscriptionManager#getDefaultDataPhoneId
      */
     public int getDefaultDataPhoneId() {
         return SubscriptionManager.getPhoneId(SubscriptionManager.getDefaultSubscriptionId());
     }

     private static final String SIM_ENABLED_PROPERTY_NAME = "persist.radio.sim_enabled";

     /**
      * Requires the MODIFY_PHONE_STATE permission.
      */
     public void setSimEnabled(int phoneId, boolean turnOn) {
         TelephonyManager.setTelephonyProperty(phoneId, SIM_ENABLED_PROPERTY_NAME,
                     turnOn ? "1" : "0");
         try {
             ITelephonyEx telephony = getITelephonyEx();
             if (telephony != null) {
                 telephony.setSimEnabled(phoneId, turnOn);
             }
         } catch (RemoteException e) {
             Log.e(TAG, "Error calling ITelephony#setSimEnabled", e);
         }
     }

     public boolean isSimEnabled(int phoneId) {
         return !"0".equals(
                 TelephonyManager.getTelephonyProperty(phoneId, SIM_ENABLED_PROPERTY_NAME, "1"));
     }

     /* SPRD: 474686 Feature For Uplmn @{*/
     public boolean isUsimCard(int phoneId) {
         try {
             return getITelephonyEx().isUsimCard(phoneId);
         } catch (RemoteException ex) {
             Log.e(TAG, "RemoteException calling isUsimCard", ex);
         } catch (NullPointerException ex) {
             Log.e(TAG, "NullPointerException  calling isUsimCard", ex);
         }
         return false;
     }
     /* @} */

    public static boolean checkSimlockLocked(int phoneId) {
        return getSimStateFromProperty(phoneId).isSimlockLocked();
    }

    public static int getSimStateEx(int slotIdx) {
        return getSimStateFromProperty(slotIdx).ordinal();
    }

    /**
     * sprd add for smsc @{
     *
     * @hide
     */
    public String getSmsc() {
        try {
            ITelephonyEx telephony = getITelephonyEx();
            if (telephony != null) {
                return getITelephonyEx().getSmsc();
            }
        } catch (RemoteException ex) {
            Rlog.e(TAG, "getSmsc RemoteException", ex);
        } catch (NullPointerException ex) {
            Rlog.e(TAG, "getSmsc NPE", ex);
        }
        return null;
    }

    public boolean setSmsc(String smscAddr) {
        try {
            ITelephonyEx telephony = getITelephonyEx();
            if (telephony != null) {
                return getITelephonyEx().setSmsc(smscAddr);
            }
        } catch (RemoteException ex) {
            Rlog.e(TAG, "setSmsc RemoteException", ex);
        } catch (NullPointerException ex) {
            Rlog.e(TAG, "setSmsc NPE", ex);
        }
        return false;
    }

    public String getSmscForSubscriber(int subId) {
        try {
            ITelephonyEx telephony = getITelephonyEx();
            if (telephony != null) {
                return getITelephonyEx().getSmscForSubscriber(subId);
            }
        } catch (RemoteException ex) {
            Rlog.e(TAG, "getSmscForSubscriber RemoteException", ex);
        } catch (NullPointerException ex) {
            Rlog.e(TAG, "getSmscForSubscriber NPE", ex);
        }
        return null;
    }

    public boolean setSmscForSubscriber(String smscAddr, int subId) {
        try {
            ITelephonyEx telephony = getITelephonyEx();
            if (telephony != null) {
                return getITelephonyEx().setSmscForSubscriber(smscAddr, subId);
            }
        } catch (RemoteException ex) {
            Rlog.e(TAG, "setSmscForSubscriber RemoteException", ex);
        } catch (NullPointerException ex) {
            Rlog.e(TAG, "setSmscForSubscriber NPE", ex);
        }
        return false;
    }
    /** @} */

    /* SPRD: Add feature of low battery for Reliance @{ */
    public static boolean isBatteryLow() {
        return LowBatteryCallUtils.getInstance().isBatteryLow();
    }

    public static void showLowBatteryDialDialog(Context context,Intent intent,boolean isDialingByDialer) {
        LowBatteryCallUtils.getInstance().showLowBatteryDialDialog(context,intent,isDialingByDialer);
    }

    public static void showLowBatteryInCallDialog(Context context,android.telecom.Call call) {
        LowBatteryCallUtils.getInstance().showLowBatteryInCallDialog(context,call);
    }

    public static void showLowBatteryChangeToVideoDialog(android.telecom.Call call,VideoProfile videoProfile) {
        LowBatteryCallUtils.getInstance().showLowBatteryChangeToVideoDialog(call,videoProfile);
    }
    /* @} */

    /**
     * SPRD:add for Bug588409
     */
    public CellLocation getCellLocationForPhone(int phoneId) {
        try {
            Bundle bundle = getITelephonyEx().getCellLocationForPhone(phoneId);
            if (bundle.isEmpty())
                return null;
            CellLocation cl = CellLocation.newFromBundle(bundle);
            if (cl.isEmpty())
                return null;
            return cl;
        } catch (RemoteException ex) {
            return null;
        } catch (NullPointerException ex) {
            return null;
        }
    }

    /* SPRD: add for bug589362 @{ */
    public boolean isRingtongUriAvailable(Uri uri) {
        try {
            return getITelephonyEx().isRingtongUriAvailable(uri);
        } catch (RemoteException ex) {
            Log.e(TAG, "RemoteException calling getRingtoneUri", ex);
        } catch (NullPointerException ex) {
            Log.e(TAG, "NullPointerException  calling getRingtoneUri", ex);
        }
        return false;
    }
    /* @} */

    /*SPRD: Add for bug598376{@*/
    public static boolean isSupportVT() {
        return SystemProperties.getBoolean("persist.sys.support.vt", false)
                && (UserHandle.myUserId() == 0);

    }
    /* @} */

    /* SPRD: Add interface to obtain device capability @{*/
    public boolean isDeviceSupportLte() {
        try {
            return getITelephonyEx().isDeviceSupportLte();
        } catch (RemoteException ex) {
            Log.e(TAG, "RemoteException calling isDeviceSupportLte", ex);
        } catch (NullPointerException ex) {
            Log.e(TAG, "NullPointerException  calling isDeviceSupportLte", ex);
        }
        return false;
    }
    /* @} */

    /**
     * SPRD: [Bug602746]
     * Returns the PNN home name.
     */
    public String getPnnHomeName(int subId) {
        try {
            return getITelephonyEx().getPnnHomeName(subId);
        } catch (RemoteException ex) {
            Log.e(TAG, "RemoteException calling getPnnHomeName", ex);
        } catch (NullPointerException ex) {
            Log.e(TAG, "NullPointerException calling getPnnHomeName", ex);
        }
        return null;
    }

    /**
     * SPRD: Bug 616187 Add for Reliance simlock
     *
     **/
    public static boolean isRelianceSimlock(){
        return "true".equals(SystemProperties.get("persist.sys.reliance.simlock"));
    }

    /* SPRD: Add interface to obtain sim slot capability @{*/
   public boolean isSimSlotSupportLte(int phoneId) {
       try {
           return getITelephonyEx().isSimSlotSupportLte(phoneId);
       } catch (RemoteException ex) {
           Log.e(TAG, "RemoteException calling isDeviceSupportLte", ex);
       } catch (NullPointerException ex) {
           Log.e(TAG, "NullPointerException  calling isDeviceSupportLte", ex);
       }
       return false;
   }
    /* @} */
    /* SPRD: Add interface set default data subId for CMCC case SK_0013 @{*/
    public void setDefaultDataSubId(int subId) {
        try {
            getITelephonyEx().setDefaultDataSubId(subId);
        } catch (RemoteException ex) {
            Log.e(TAG, "RemoteException calling setDefaultDataSubId", ex);
        } catch (NullPointerException ex) {
            Log.e(TAG, "NullPointerException calling setDefaultDataSubId", ex);
        }
    }
    /* @} */
}
