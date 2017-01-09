package plugin.sprd.simlock;

import com.sprd.phone.SimLockManager;
import android.app.AddonManager;
import android.content.Context;
import android.os.AsyncResult;
import android.os.Handler;
import android.os.Message;
import android.os.SystemProperties;
import android.util.Log;
import android.telephony.TelephonyManager;
import com.android.phone.PhoneGlobals;
import com.android.internal.telephony.IccCard;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneBase;
import com.android.internal.telephony.IccUtils;
import com.android.internal.telephony.PhoneFactory;
import java.util.ArrayList;
import android.widget.Toast;
import android.provider.Settings;
import android.content.Intent;
import plugin.sprd.simlock.IccSimLockDepersonalizationPanelSprd;

public class SimLockManagerPlugin extends SimLockManager implements AddonManager.InitialCallback {

    private static final String TAG = "SimLockManagerPlugin";
    private Context mAddonContext;
    private TelephonyManager mTelephonyManager;

    public static final String IS_SIMLOCK = "is_sim_locked";
    protected static final int EVENT_SIM_SIM_LOCKED = 2;
    protected static final int EVENT_SIM_NETWORK_LOCKED = 3;
    protected static final int EVENT_SIM_NETWORK_SUBSET_LOCKED = 4;
    protected static final int EVENT_SIM_SERVICE_PROVIDER_LOCKED = 5;
    protected static final int EVENT_SIM_CORPORATE_LOCKED = 6;
    protected static final int EVENT_SIM_SIM_LOCKED_FOREVER = 7;
    protected static final int EVENT_SIM_SIM_LOCKED_PUK = 8;
    protected static final int EVENT_SIM_NETWORK_LOCKED_PUK = 9;
    protected static final int EVENT_SIM_NETWORK_SUBSET_LOCKED_PUK = 10;
    protected static final int EVENT_SIM_SERVICE_PROVIDER_LOCKED_PUK = 11;
    protected static final int EVENT_SIM_CORPORATE_LOCKED_PUK = 12;
    private int sim_network_locked_count = 0;
    private int sim_network_subset_locked_count = 0;
    private int sim_service_provider_locked_count = 0;
    private int sim_corporate_locked_count = 0;
    private int sim_sim_locked_count = 0;
    private int sim_sim_locked_forver_count = 0;
    private int sim_network_locked_puk_count = 0;
    private int sim_network_subset_locked_puk_count = 0;
    private int sim_service_provider_locked_puk_count = 0;
    private int sim_corporate_locked_puk_count = 0;
    private int sim_sim_locked_puk_count = 0;
    private int mTypeForNvUnlock = 0;
    private int[] mUnlockTypes = {TelephonyManager.UNLOCK_SIM,
                                  TelephonyManager.UNLOCK_CORPORATE,
                                  TelephonyManager.UNLOCK_SERVICE_PORIVDER,
                                  TelephonyManager.UNLOCK_NETWORK_SUBSET,
                                  TelephonyManager.UNLOCK_NETWORK
                                  };
    public boolean isPinOrPuk = false;
    public ArrayList<IccPanelSprd> mPanelArr = new ArrayList<IccPanelSprd>();

    private static final String ACTION_EMERGENCY_DIAL_START = "com.android.phone.emergency_dial_start_intent";
    private static final String ACTION_EMERGENCY_DIAL_STOP = "com.android.phone.emergency_dial_stop_intent";

    public SimLockManagerPlugin() {

    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    public void registerForSimLocked(Context context, Handler handler) {
        Log.d(TAG, " SimLockManagerPlugin registerForSimLocked");
        mTelephonyManager = (TelephonyManager) context.getSystemService(Context.TELEPHONY_SERVICE);
        int phoneCount = mTelephonyManager.getPhoneCount();
        Log.d(TAG, " SimLockManagerPlugin phoneCount = " + phoneCount);
        for (int i = 0; i < phoneCount; i++) {
            Phone phone = PhoneFactory.getPhone(i);//PhoneGlobals/*Sprd kenny*/.getInstance().getPhone(i);
            IccCard sim = phone.getIccCard();
            if (sim != null) {
                Log.v(TAG, "register for ICC status");
                sim.registerForNetworkLocked(handler, EVENT_SIM_NETWORK_LOCKED, Integer.valueOf(i));
                sim.registerForNetworkSubsetLocked(handler, EVENT_SIM_NETWORK_SUBSET_LOCKED, Integer.valueOf(i));
                sim.registerForServiceProviderLocked(handler, EVENT_SIM_SERVICE_PROVIDER_LOCKED, Integer.valueOf(i));
                sim.registerForCorporateLocked(handler, EVENT_SIM_CORPORATE_LOCKED, Integer.valueOf(i));
                sim.registerForSimLocked(handler, EVENT_SIM_SIM_LOCKED, Integer.valueOf(i));
                sim.registerForSimLockedForever(handler, EVENT_SIM_SIM_LOCKED_FOREVER, Integer.valueOf(i));
                sim.registerForNetworkLockedPuk(handler, EVENT_SIM_NETWORK_LOCKED_PUK, Integer.valueOf(i));
                sim.registerForNetworkSubsetLockedPuk(handler, EVENT_SIM_NETWORK_SUBSET_LOCKED_PUK, Integer.valueOf(i));
                sim.registerForServiceProviderLockedPuk(handler, EVENT_SIM_SERVICE_PROVIDER_LOCKED_PUK, Integer.valueOf(i));
                sim.registerForCorporateLockedPuk(handler, EVENT_SIM_CORPORATE_LOCKED_PUK, Integer.valueOf(i));
                sim.registerForSimLockedPuk(handler, EVENT_SIM_SIM_LOCKED_PUK, Integer.valueOf(i));
            }
        }
    }

    public void processSimStateChanged(Context context) {
        TelephonyManager tm = (TelephonyManager) context.getSystemService(Context.TELEPHONY_SERVICE);
        int[] simLockstate;
        boolean[] isSimLocked;
        int phoneCount = tm.getPhoneCount();
        simLockstate = new int[phoneCount];
        isSimLocked = new boolean[phoneCount];

        for (int i = 0; i < phoneCount; i++) {
            simLockstate[i] = tm.getSimState(i);
            switch (simLockstate[i]) {
                case TelephonyManager.SIM_STATE_PIN_REQUIRED :
                case TelephonyManager.SIM_STATE_PUK_REQUIRED :
                    isSimLocked[i] = true;
                    break;
                default:
                    isSimLocked[i] = false;
                    break;
            }
            if(isSimLocked[i]){
                isPinOrPuk = true;
            }
        }
        if (!isPinOrPuk) {
            return;
        } else {
            isPinOrPuk = false;
            if (mPanelArr != null) {
                for (IccPanelSprd panel : mPanelArr) {
                    panel.show();
                    mPanelArr.remove(panel);
                }
            }
        }
    }

    public void showPanel(Context context, Message msg) {
        int event_type = msg.what;
        if((event_type != EVENT_SIM_NETWORK_LOCKED)
                && (event_type != EVENT_SIM_NETWORK_SUBSET_LOCKED)
                && (event_type != EVENT_SIM_SIM_LOCKED)
                && (event_type != EVENT_SIM_SERVICE_PROVIDER_LOCKED)
                && (event_type != EVENT_SIM_CORPORATE_LOCKED)
                && (event_type != EVENT_SIM_SIM_LOCKED_FOREVER)
                && (event_type != EVENT_SIM_NETWORK_LOCKED_PUK)
                && (event_type != EVENT_SIM_NETWORK_SUBSET_LOCKED_PUK)
                && (event_type != EVENT_SIM_SIM_LOCKED_PUK)
                && (event_type != EVENT_SIM_SERVICE_PROVIDER_LOCKED_PUK)
                && (event_type != EVENT_SIM_CORPORATE_LOCKED_PUK)) {
            Log.d(TAG, "event_type -> "+ event_type + " is not sim lock event");
            return;
        }

        int type = 0;
        boolean isAutoShow = SystemProperties.getBoolean("ro.simlock.unlock.autoshow", true);

        switch (event_type) {
        case EVENT_SIM_NETWORK_LOCKED:
            type = TelephonyManager.UNLOCK_NETWORK;
            if(isAutoShow && sim_network_locked_count++ > 0) {
                return ;
            }
            sim_network_locked_puk_count = 0;
            break;
        case EVENT_SIM_NETWORK_SUBSET_LOCKED:
            type = TelephonyManager.UNLOCK_NETWORK_SUBSET;
            if(isAutoShow && sim_network_subset_locked_count++ > 0) {
                return ;
            }
            sim_network_subset_locked_puk_count = 0;
            break;
        case EVENT_SIM_SERVICE_PROVIDER_LOCKED:
            type = TelephonyManager.UNLOCK_SERVICE_PORIVDER;
            if(isAutoShow && sim_service_provider_locked_count++ > 0) {
                return;
            }
            sim_service_provider_locked_puk_count = 0;
            break;
        case EVENT_SIM_CORPORATE_LOCKED:
            type = TelephonyManager.UNLOCK_CORPORATE;
            if(isAutoShow && sim_corporate_locked_count++ > 0) {
                return ;
            }
            sim_corporate_locked_puk_count = 0;
            break;
        case EVENT_SIM_SIM_LOCKED:
            type = TelephonyManager.UNLOCK_SIM;
            if(isAutoShow && sim_sim_locked_count++ > 0) {
                return ;
            }
            sim_sim_locked_puk_count = 0;
            break;
        case EVENT_SIM_SIM_LOCKED_FOREVER:
            if(isAutoShow && sim_sim_locked_forver_count++ == 0) {
                //showNotification();
            }
            return;
        case EVENT_SIM_NETWORK_LOCKED_PUK:
            if (isAutoShow && sim_network_locked_puk_count++ > 0) {
                return;
            }
            sim_network_locked_count = 0;
            type = TelephonyManager.UNLOCK_NETWORK_PUK;
            break;
        case EVENT_SIM_NETWORK_SUBSET_LOCKED_PUK:
            if (isAutoShow && sim_network_subset_locked_puk_count++ > 0) {
                return;
            }
            sim_network_subset_locked_count = 0;
            type = TelephonyManager.UNLOCK_NETWORK_SUBSET_PUK;
            break;
        case EVENT_SIM_SERVICE_PROVIDER_LOCKED_PUK:
            if (isAutoShow && sim_service_provider_locked_puk_count++ > 0) {
                return;
            }
            sim_service_provider_locked_count = 0;
            type = TelephonyManager.UNLOCK_SERVICE_PORIVDER_PUK;
            break;
        case EVENT_SIM_CORPORATE_LOCKED_PUK:
            if (isAutoShow && sim_corporate_locked_puk_count++ > 0) {
                return;
            }
            sim_corporate_locked_count = 0;
            type = TelephonyManager.UNLOCK_CORPORATE_PUK;
            break;
        case EVENT_SIM_SIM_LOCKED_PUK:
            if (isAutoShow && sim_sim_locked_puk_count++ > 0) {
                return;
            }
            sim_sim_locked_count = 0;
            type = TelephonyManager.UNLOCK_SIM_PUK;
            break;
        }

        if (mAddonContext.getResources().getBoolean(R.bool.ignore_sim_network_locked_events)) {
            // Some products don't have the concept of a "SIM network lock"
            Log.i(TAG, "Ignoring type: " + type);
        } else {
            AsyncResult r = (AsyncResult) msg.obj;
            int phoneId = 0;
            if(r.userObj instanceof Integer){
                Integer integer = (Integer)r.userObj;
                phoneId = integer.intValue();
            }
            Log.i(TAG, "show sim depersonal panel type: " + type);
            Log.i(TAG, "phoneId: " + phoneId);

            Phone phone = PhoneFactory.getPhone(phoneId);
            // Normal case: show the "SIM network unlock" PIN entry screen.
            // The user won't be able to do anything else until
            // they enter a valid SIM network PIN.
            int remainCount = 0;
            if(type == TelephonyManager.UNLOCK_NETWORK
                    || type == TelephonyManager.UNLOCK_NETWORK_SUBSET
                    || type == TelephonyManager.UNLOCK_SERVICE_PORIVDER
                    || type == TelephonyManager.UNLOCK_CORPORATE
                    || type == TelephonyManager.UNLOCK_SIM) {
                remainCount = phone.getSimLockRemainTimes(type);
                boolean isLocked = Settings.System.getInt(context.getContentResolver(), IS_SIMLOCK, 0) == 0 ? false : true;
                if (remainCount <= 0 || isLocked) {
                    Toast.makeText(context, mAddonContext.getString(R.string.phone_locked), Toast.LENGTH_LONG).show();
                    Log.i(TAG, "telefk remainCount = 0");
                    return;
                }
                Log.i(TAG, "remainCount = " + remainCount);
            }
            IccPanelSprd ndpPanel = null;
            ndpPanel = new IccSimLockDepersonalizationPanelSprd(mAddonContext, phone, remainCount, type);

            if (isPinOrPuk) {
                mPanelArr.add(ndpPanel);
                return;
            }
            if (ndpPanel != null) {
                ndpPanel.show();
            }
        }
    }

    public void sendEMDialStart(Context context) {
        context.sendBroadcast(new Intent(ACTION_EMERGENCY_DIAL_START));
    }
    
    public void sendEMDialStop(Context context) {
        //TelephonyManager tm = (TelephonyManager) context.getSystemService(Context.TELEPHONY_SERVICE);
        TelephonyManager tm = (TelephonyManager) TelephonyManager.from(context);
        if (tm.getCallState() == TelephonyManager.CALL_STATE_IDLE) {
            context.sendBroadcast(new Intent(ACTION_EMERGENCY_DIAL_STOP));
        }
    }

    public Message decodeMessage(int simState, int phoneId) {
        Message msg = new Message();
        switch(simState) {
            case TelephonyManager.SIM_STATE_NETWORK_LOCKED:
                msg.what = EVENT_SIM_NETWORK_LOCKED;
                break;
            case TelephonyManager.SIM_STATE_NETWORKSUBSET_LOCKED:
                msg.what = EVENT_SIM_NETWORK_SUBSET_LOCKED;
                break;
            case TelephonyManager.SIM_STATE_SERVICEPROVIDER_LOCKED:
                msg.what = EVENT_SIM_SERVICE_PROVIDER_LOCKED;
                break;
            case TelephonyManager.SIM_STATE_CORPORATE_LOCKED:
                msg.what = EVENT_SIM_CORPORATE_LOCKED;
                break;
            case TelephonyManager.SIM_STATE_SIM_LOCKED:
                msg.what = EVENT_SIM_SIM_LOCKED;
                break;
            case TelephonyManager.SIM_STATE_NETWORK_LOCKED_PUK:
                msg.what = EVENT_SIM_NETWORK_LOCKED_PUK;
                break;
            case TelephonyManager.SIM_STATE_NETWORK_SUBSET_LOCKED_PUK:
                msg.what = EVENT_SIM_NETWORK_SUBSET_LOCKED_PUK;
                break;
            case TelephonyManager.SIM_STATE_SERVICE_PROVIDER_LOCKED_PUK:
                msg.what = EVENT_SIM_SERVICE_PROVIDER_LOCKED_PUK;
                break;
            case TelephonyManager.SIM_STATE_CORPORATE_LOCKED_PUK:
                msg.what = EVENT_SIM_CORPORATE_LOCKED_PUK;
                break;
            case TelephonyManager.SIM_STATE_SIM_LOCKED_PUK:
                msg.what = EVENT_SIM_SIM_LOCKED_PUK;
                break;
            default:
                msg.what = 0;
                break;
        }
        msg.obj = phoneId;
        AsyncResult.forMessage(msg);
        return msg;
    }

    public void showPanelForUnlockByNv(Context context) {
        ArrayList<Integer> types = new ArrayList<Integer>();
        Phone phone = PhoneFactory.getDefaultPhone();

        TelephonyManager tm = (TelephonyManager)TelephonyManager.from(context);
        if (tm == null) {
            Toast.makeText(context,
                    mAddonContext.getString(R.string.sim_lock_try_later),
                    Toast.LENGTH_LONG).show();
            Log.d(TAG, "showPanelForUnlockByNv return for tm is null.");
            return;
        }

        //for lock forever
        if (tm.getSimState(0) == TelephonyManager.SIM_STATE_SIM_LOCKED_FOREVER){
            Toast.makeText(context,
                    mAddonContext.getString(R.string.phone_locked),
                    Toast.LENGTH_LONG).show();
            return;
        }

        boolean isNetworkLock = tm.getSimLockStatus(TelephonyManager.UNLOCK_NETWORK);
        boolean isNetworkSubsetLock = tm.getSimLockStatus(TelephonyManager.UNLOCK_NETWORK_SUBSET);
        boolean isServiceProviderLock = tm.getSimLockStatus(TelephonyManager.UNLOCK_SERVICE_PORIVDER);
        boolean isCorporateLock = tm.getSimLockStatus(TelephonyManager.UNLOCK_CORPORATE);
        boolean isSimLock = tm.getSimLockStatus(TelephonyManager.UNLOCK_SIM);

        //for puk
        for (int type : mUnlockTypes) {
            int remainCount = phone.getSimLockRemainTimes(type);
            Log.d(TAG, "remainCount[" + type + "]: "+remainCount);
            if (remainCount <= 0) {
                types.add(type + 5);
                break;
            }
        }

        //for pin
        if(isSimLock){
            mTypeForNvUnlock = TelephonyManager.UNLOCK_SIM;
            types.add(mTypeForNvUnlock);
        }
        if(isCorporateLock){
            mTypeForNvUnlock = TelephonyManager.UNLOCK_CORPORATE;
            types.add(mTypeForNvUnlock);
        }
        if(isServiceProviderLock){
            mTypeForNvUnlock = TelephonyManager.UNLOCK_SERVICE_PORIVDER;
            types.add(mTypeForNvUnlock);
        }
        if(isNetworkSubsetLock){
            mTypeForNvUnlock = TelephonyManager.UNLOCK_NETWORK_SUBSET;
            types.add(mTypeForNvUnlock);
        }
        if(isNetworkLock){
            mTypeForNvUnlock = TelephonyManager.UNLOCK_NETWORK;
            types.add(mTypeForNvUnlock);
        }
        if(types.size() > 0){
            mTypeForNvUnlock = types.get(0);
        }

        int remainCount = 0;
        if(mTypeForNvUnlock == TelephonyManager.UNLOCK_NETWORK
                || mTypeForNvUnlock == TelephonyManager.UNLOCK_NETWORK_SUBSET
                || mTypeForNvUnlock == TelephonyManager.UNLOCK_SERVICE_PORIVDER
                || mTypeForNvUnlock == TelephonyManager.UNLOCK_CORPORATE
                || mTypeForNvUnlock == TelephonyManager.UNLOCK_SIM) {
            remainCount = phone.getSimLockRemainTimes(mTypeForNvUnlock);
            Log.i(TAG, "current remainCount = " + remainCount);
        }

        IccPanelSprd ndpPanel = null;
        ndpPanel = new IccSimLockDepersonalizationPanelSprd(mAddonContext, phone,
                remainCount, mTypeForNvUnlock);
        ((IccSimLockDepersonalizationPanelSprd)ndpPanel).setCurrentTypeArray(types);
        Log.d(TAG, "[simlock]---mTypeForNvUnlock = " + mTypeForNvUnlock);

        ndpPanel.show();
    }
}
