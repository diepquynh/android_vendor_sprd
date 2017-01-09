package plugin.sprd.simlock;

import com.android.phone.SimLockManager;
import android.app.AddonManager;
import android.content.Context;
import android.content.ComponentName;
import android.content.ServiceConnection;
import android.os.AsyncResult;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.SystemProperties;
import android.util.Log;
import android.telephony.TelephonyManager;
import android.telephony.TelephonyManagerEx;
import com.android.phone.PhoneGlobals;
import com.android.internal.telephony.IccCard;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneFactory;
import java.util.ArrayList;
import android.widget.Toast;
import android.provider.Settings;
import android.content.Intent;
import plugin.sprd.simlock.IccSimLockDepersonalizationPanelSprd;
import com.android.sprd.telephony.RadioInteractor;
import com.android.sprd.telephony.RadioInteractorCallbackListener;
import com.android.sprd.telephony.uicc.IccCardStatusEx;

public class SimLockManagerPlugin extends SimLockManager implements AddonManager.InitialCallback {

    private static final String TAG = "SimLockManagerPlugin";
    private Context mAddonContext;
    private TelephonyManager mTelephonyManager;

    public static final String IS_SIMLOCK = "is_sim_locked";
    protected static final int EVENT_SIM_NETWORK_LOCKED = 1000;
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
    private int[] mUnlockTypes = {IccCardStatusEx.UNLOCK_SIM,
                                  IccCardStatusEx.UNLOCK_CORPORATE,
                                  IccCardStatusEx.UNLOCK_SERVICE_PORIVDER,
                                  IccCardStatusEx.UNLOCK_NETWORK_SUBSET,
                                  IccCardStatusEx.UNLOCK_NETWORK
                                  };
    public boolean isPinOrPuk = false;
    public ArrayList<IccPanelSprd> mPanelArr = new ArrayList<IccPanelSprd>();

    private static final String ACTION_EMERGENCY_DIAL_START = "com.android.phone.emergency_dial_start_intent";
    private static final String ACTION_EMERGENCY_DIAL_STOP = "com.android.phone.emergency_dial_stop_intent";

    RadioInteractor mRadioInteractor;
    RadioInteractorCallbackListener[] mCallbackListener;

    public SimLockManagerPlugin() {

    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    public void registerForSimLocked(final Context context) {
        Log.d(TAG, " SimLockManagerPlugin registerForSimLocked");
        mTelephonyManager = (TelephonyManager) context.getSystemService(Context.TELEPHONY_SERVICE);

        mAddonContext.bindService(new Intent("com.android.sprd.telephony.server.RADIOINTERACTOR_SERVICE")
            .setPackage("com.android.sprd.telephony.server"),
            new ServiceConnection() {
                @Override
                public void onServiceConnected(ComponentName name, IBinder service) {
                    int phoneCount = mTelephonyManager.getPhoneCount();
                    mRadioInteractor = new RadioInteractor(context);
                    mCallbackListener = new RadioInteractorCallbackListener[phoneCount];
                    Log.d(TAG, " SimLockManagerPlugin phoneCount = " + phoneCount);
                    for (int i = 0; i < phoneCount; i++) {
                        mCallbackListener[i] = new RadioInteractorCallbackListener(i) {
                            @Override
                            public void onSimLockNotifyEvent(Message msg) {
                                if (SystemProperties.getBoolean("ro.simlock.unlock.autoshow", true)
                                        && !SystemProperties.getBoolean("ro.simlock.onekey.lock", false)
                                        && !SystemProperties.getBoolean("ro.simlock.unlock.bynv", false)) {
                                    showPanel(mAddonContext, msg);
                                }
                            }
                        };
                        mRadioInteractor.listen(mCallbackListener[i],
                                RadioInteractorCallbackListener.LISTEN_SIMLOCK_NOTIFY_EVENT,
                                false);
                        mRadioInteractor.getSimStatus(i);
                    }
                }

                @Override
                public void onServiceDisconnected(ComponentName name) {
                    int phoneCount = mTelephonyManager.getPhoneCount();
                    for (int i = 0; i < phoneCount; i++) {
                         mRadioInteractor.listen(mCallbackListener[i],
                                RadioInteractorCallbackListener.LISTEN_NONE);
                    }
                }
            }, Context.BIND_AUTO_CREATE);
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
        int return_type = 0;
        int event_type = msg.what;
        AsyncResult ar = (AsyncResult)msg.obj;
        if(ar != null && ar.result != null){
            return_type = (int)ar.result;
        }

        if((event_type != RadioInteractorCallbackListener.LISTEN_SIMLOCK_NOTIFY_EVENT)
              &&(event_type != EVENT_SIM_NETWORK_LOCKED)) {
            Log.d(TAG, "event_type -> "+ event_type + " is not sim lock event");
            return;
        }

        int type = 0;
        boolean isAutoShow = SystemProperties.getBoolean("ro.simlock.unlock.autoshow", true);

        switch (event_type) {
        case EVENT_SIM_NETWORK_LOCKED:
            type = IccCardStatusEx.UNLOCK_NETWORK;
            if(isAutoShow && sim_network_locked_count++ > 0) {
                return ;
            }
            sim_network_locked_puk_count = 0;
            break;
        case RadioInteractorCallbackListener.LISTEN_SIMLOCK_NOTIFY_EVENT:
            switch (return_type) {
            case TelephonyManager.SIM_STATE_NETWORK_LOCKED:
                type = IccCardStatusEx.UNLOCK_NETWORK;
                if(isAutoShow && sim_network_locked_count++ > 0) {
                    return ;
                }
                sim_network_locked_puk_count = 0;
                break;

            case IccCardStatusEx.SIM_STATE_NETWORKSUBSET_LOCKED:
                type = IccCardStatusEx.UNLOCK_NETWORK_SUBSET;
                if(isAutoShow && sim_network_subset_locked_count++ > 0) {
                    return ;
                }
                sim_network_subset_locked_puk_count = 0;
                break;
            case IccCardStatusEx.SIM_STATE_SERVICEPROVIDER_LOCKED:
                type = IccCardStatusEx.UNLOCK_SERVICE_PORIVDER;
                if(isAutoShow && sim_service_provider_locked_count++ > 0) {
                    return;
                }
                sim_service_provider_locked_puk_count = 0;
                break;
            case IccCardStatusEx.SIM_STATE_CORPORATE_LOCKED:
                type = IccCardStatusEx.UNLOCK_CORPORATE;
                if(isAutoShow && sim_corporate_locked_count++ > 0) {
                    return ;
                }
                sim_corporate_locked_puk_count = 0;
                break;
            case IccCardStatusEx.SIM_STATE_SIM_LOCKED:
                type = IccCardStatusEx.UNLOCK_SIM;
                if(isAutoShow && sim_sim_locked_count++ > 0) {
                    return ;
                }
                sim_sim_locked_puk_count = 0;
                break;
            case IccCardStatusEx.SIM_STATE_SIM_LOCKED_FOREVER:
                if(isAutoShow && sim_sim_locked_forver_count++ == 0) {
                    //showNotification();
                }
                return;
            case IccCardStatusEx.SIM_STATE_NETWORK_LOCKED_PUK:
                if (isAutoShow && sim_network_locked_puk_count++ > 0) {
                    return;
                }
                sim_network_locked_count = 0;
                type = IccCardStatusEx.UNLOCK_NETWORK_PUK;
                break;
            case IccCardStatusEx.SIM_STATE_NETWORK_SUBSET_LOCKED_PUK:
                if (isAutoShow && sim_network_subset_locked_puk_count++ > 0) {
                    return;
                }
                sim_network_subset_locked_count = 0;
                type = IccCardStatusEx.UNLOCK_NETWORK_SUBSET_PUK;
                break;
            case IccCardStatusEx.SIM_STATE_SERVICE_PROVIDER_LOCKED_PUK:
                if (isAutoShow && sim_service_provider_locked_puk_count++ > 0) {
                    return;
                }
                sim_service_provider_locked_count = 0;
                type = IccCardStatusEx.UNLOCK_SERVICE_PORIVDER_PUK;
                break;
            case IccCardStatusEx.SIM_STATE_CORPORATE_LOCKED_PUK:
                if (isAutoShow && sim_corporate_locked_puk_count++ > 0) {
                    return;
                }
                sim_corporate_locked_count = 0;
                type = IccCardStatusEx.UNLOCK_CORPORATE_PUK;
                break;
            case IccCardStatusEx.SIM_STATE_SIM_LOCKED_PUK:
                if (isAutoShow && sim_sim_locked_puk_count++ > 0) {
                    return;
                }
                sim_sim_locked_count = 0;
                type = IccCardStatusEx.UNLOCK_SIM_PUK;
                break;
            }
            break;
        }

        if (mAddonContext.getResources().getBoolean(R.bool.ignore_sim_network_locked_events)) {
            // Some products don't have the concept of a "SIM network lock"
            Log.i(TAG, "Ignoring type: " + type);
        } else {
            int phoneId = 0;
            if(ar.userObj instanceof Integer){
                Integer integer = (Integer)ar.userObj;
                phoneId = integer.intValue();
            }
            Log.i(TAG, "show sim depersonal panel type: " + type);
            Log.i(TAG, "phoneId: " + phoneId);

            Phone phone = PhoneFactory.getPhone(phoneId);
            // Normal case: show the "SIM network unlock" PIN entry screen.
            // The user won't be able to do anything else until
            // they enter a valid SIM network PIN.
            int remainCount = 0;
            if(type == IccCardStatusEx.UNLOCK_NETWORK
                    || type == IccCardStatusEx.UNLOCK_NETWORK_SUBSET
                    || type == IccCardStatusEx.UNLOCK_SERVICE_PORIVDER
                    || type == IccCardStatusEx.UNLOCK_CORPORATE
                    || type == IccCardStatusEx.UNLOCK_SIM) {
                if (mRadioInteractor == null) {
                    mRadioInteractor = new RadioInteractor(context);
                }
                remainCount = mRadioInteractor.getSimLockRemainTimes(type, phoneId);
                //boolean isLocked = Settings.System.getInt(context.getContentResolver(), IS_SIMLOCK, 0) == 0 ? false : true;
                //if (remainCount <= 0 || isLocked) {
                //SPRD: Add for Reliance simlock
                if(TelephonyManagerEx.isRelianceSimlock()){
                    Log.i(TAG, "For Reliance simlock, just return the unlock times:" + remainCount);
                } else {
                    if (remainCount <= 0) {
                        Toast.makeText(context, mAddonContext.getString(R.string.phone_locked), Toast.LENGTH_LONG).show();
                        Log.i(TAG, "telefk remainCount = 0");
                        return;
                    }
                    Log.i(TAG, "remainCount = " + remainCount);
                }
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
            case IccCardStatusEx.SIM_STATE_NETWORKSUBSET_LOCKED:
            case IccCardStatusEx.SIM_STATE_SERVICEPROVIDER_LOCKED:
            case IccCardStatusEx.SIM_STATE_CORPORATE_LOCKED:
            case IccCardStatusEx.SIM_STATE_SIM_LOCKED:
            case IccCardStatusEx.SIM_STATE_NETWORK_LOCKED_PUK:
            case IccCardStatusEx.SIM_STATE_NETWORK_SUBSET_LOCKED_PUK:
            case IccCardStatusEx.SIM_STATE_SERVICE_PROVIDER_LOCKED_PUK:
            case IccCardStatusEx.SIM_STATE_CORPORATE_LOCKED_PUK:
            case IccCardStatusEx.SIM_STATE_SIM_LOCKED_PUK:
                msg.what = RadioInteractorCallbackListener.LISTEN_SIMLOCK_NOTIFY_EVENT;
                break;
            default:
                msg.what = 0;
                break;
        }
        msg.obj = phoneId;
        AsyncResult.forMessage(msg, simState, null);
        return msg;
    }

    public void showPanelForUnlockByNv(Context context) {
        ArrayList<Integer> types = new ArrayList<Integer>();
        Phone phone = PhoneFactory.getDefaultPhone();

        //for lock forever
        if (TelephonyManagerEx.getSimStateEx(0) == IccCardStatusEx.SIM_STATE_SIM_LOCKED_FOREVER){
            Toast.makeText(context,
                    mAddonContext.getString(R.string.phone_locked),
                    Toast.LENGTH_LONG).show();
            return;
        }

        //TODO
        if (mRadioInteractor == null) {
            mRadioInteractor = new RadioInteractor(context);
        }
        int isNetworkLock = mRadioInteractor.getSimLockStatus(IccCardStatusEx.UNLOCK_NETWORK, 0);
        int isNetworkSubsetLock = mRadioInteractor.getSimLockStatus(IccCardStatusEx.UNLOCK_NETWORK_SUBSET, 0);
        int isServiceProviderLock = mRadioInteractor.getSimLockStatus(IccCardStatusEx.UNLOCK_SERVICE_PORIVDER, 0);
        int isCorporateLock = mRadioInteractor.getSimLockStatus(IccCardStatusEx.UNLOCK_CORPORATE, 0);
        int isSimLock = mRadioInteractor.getSimLockStatus(IccCardStatusEx.UNLOCK_SIM, 0);

        if (isNetworkLock < 0 || isNetworkSubsetLock < 0 || isServiceProviderLock < 0
                || isCorporateLock < 0 || isSimLock < 0) {
            Toast.makeText(context,
                    mAddonContext.getString(R.string.sim_lock_try_later),
                    Toast.LENGTH_LONG).show();
            Log.d(TAG, "showPanelForUnlockByNv return for tm is null.");
            return;
        }

        //for puk
        for (int type : mUnlockTypes) {
            int remainCount = mRadioInteractor.getSimLockRemainTimes(type, 0);
            Log.d(TAG, "remainCount[" + type + "]: "+remainCount);
            if (remainCount <= 0) {
                types.add(type + 5);
                break;
            }
        }

        //for pin
        if(isSimLock > 0){
            mTypeForNvUnlock = IccCardStatusEx.UNLOCK_SIM;
            types.add(mTypeForNvUnlock);
        }
        if(isCorporateLock > 0){
            mTypeForNvUnlock = IccCardStatusEx.UNLOCK_CORPORATE;
            types.add(mTypeForNvUnlock);
        }
        if(isServiceProviderLock > 0){
            mTypeForNvUnlock = IccCardStatusEx.UNLOCK_SERVICE_PORIVDER;
            types.add(mTypeForNvUnlock);
        }
        if(isNetworkSubsetLock > 0){
            mTypeForNvUnlock = IccCardStatusEx.UNLOCK_NETWORK_SUBSET;
            types.add(mTypeForNvUnlock);
        }
        if(isNetworkLock > 0){
            mTypeForNvUnlock = IccCardStatusEx.UNLOCK_NETWORK;
            types.add(mTypeForNvUnlock);
        }
        if(types.size() > 0){
            mTypeForNvUnlock = types.get(0);
        }

        int remainCount = 0;
        if(mTypeForNvUnlock == IccCardStatusEx.UNLOCK_NETWORK
                || mTypeForNvUnlock == IccCardStatusEx.UNLOCK_NETWORK_SUBSET
                || mTypeForNvUnlock == IccCardStatusEx.UNLOCK_SERVICE_PORIVDER
                || mTypeForNvUnlock == IccCardStatusEx.UNLOCK_CORPORATE
                || mTypeForNvUnlock == IccCardStatusEx.UNLOCK_SIM) {
            remainCount = mRadioInteractor.getSimLockRemainTimes(mTypeForNvUnlock, 0);
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
