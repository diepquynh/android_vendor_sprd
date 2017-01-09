/* Created by Spreadst */

package com.android.phone;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.AsyncResult;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.PowerManager;
import android.telephony.RadioAccessFamily;
import android.telephony.ServiceState;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.util.Log;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneFactory;
import com.android.internal.telephony.ProxyController;
import com.android.sprd.telephony.RadioInteractor;

class NetworkQueryServiceEx extends INetworkQueryService.Stub {
    private static final String LOG_TAG = "NetworkQueryEx";
    private static final boolean DBG = true;

    private static final String RI_SERVICE_NAME =
            "com.android.sprd.telephony.server.RADIOINTERACTOR_SERVICE";
    private static final String RI_SERVICE_PACKAGE =
            "com.android.sprd.telephony.server";

    private static final int EVENT_NETWORK_SCAN_COMPLETED = 100;
    private static final int EVENT_ALL_DATA_DISCONNECTED  = 101;
    private static final int EVENT_ALL_DATA_DETACHED      = 102;
    private static final int EVENT_ABORT_QUERY_NETWORK    = 103;
    private static final int EVENT_RI_CONNECTED           = 104;

    private NetworkQueryService mService;
    private TelephonyManager mTelephonyManager;
    private ProxyController mProxyController;
    private int mPhoneId = SubscriptionManager.INVALID_PHONE_INDEX;
    private int mDisconnectPendingCount;
    private int mConnectedPhoneId; //current data activate  phone id
    private PowerManager.WakeLock mWakeLock = null;
    protected RadioInteractor mRi;

    private Handler mMyHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case EVENT_NETWORK_SCAN_COMPLETED:
                    if (DBG) log("scan completed, re-enable data");
                    enableData();
                    releaseWakeLock();
                    mService.notifyScanCompleted((AsyncResult) msg.obj);
                    break;

                case EVENT_RI_CONNECTED:
                    if (DBG) log("EVENT_RI_CONNECTED");
                    synchronized (mService.mCallbacks) {
                        if (hasClients()) {
                            deactivateData();
                        } else {
                            log("Query has been aborted");
                        }
                    }
                    break;

                case EVENT_ALL_DATA_DISCONNECTED:
                    if (DBG) log("EVENT_ALL_DATA_DISCONNECTED id=" + msg.arg1);
                    if (mDisconnectPendingCount > 0) {
                        mDisconnectPendingCount--;
                    }
                    if (mDisconnectPendingCount == 0) {
                        if (DBG) log("Data has been disabled on all phones");
                        synchronized (mService.mCallbacks) {
                            if (hasClients()) {
                                if (needForceDetach(mConnectedPhoneId)) {
                                    //do data detach on data card when query network
                                    Phone dataPhone = PhoneFactory.getPhone(mConnectedPhoneId);
                                    if (dataPhone != null) {
                                        forceDetachDataConn(dataPhone,
                                                mMyHandler.obtainMessage(EVENT_ALL_DATA_DETACHED));
                                    }
                                } else {
                                    if (DBG) log("not need detach, do query network");
                                    getAvailableNetworks();
                                }
                            } else {
                                log("Query has been aborted");
                            }
                        }
                    }
                    break;

                case EVENT_ABORT_QUERY_NETWORK:
                    if (DBG) log("EVENT_ABORT_QUERY_NETWORK");
                    enableData();
                    releaseWakeLock();
                    mPhoneId = SubscriptionManager.INVALID_PHONE_INDEX;
                    mService.setState(NetworkQueryService.QUERY_READY);
                    break;

                case EVENT_ALL_DATA_DETACHED:
                    if (DBG) log("EVENT_ALL_DATA_DETACHED");
                    synchronized (mService.mCallbacks) {
                        if (hasClients()) {
                            getAvailableNetworks();
                        } else {
                            log("Query has been aborted");
                        }
                    }
                    break;
            }
        }
    };

    private ServiceConnection mConnection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            Log.d(LOG_TAG, "RadioInteractor service connected: service=" + service);
            mRi = new RadioInteractor(mService);
            mMyHandler.sendEmptyMessage(EVENT_RI_CONNECTED);
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            mRi = null;
        }
    };

    public NetworkQueryServiceEx(NetworkQueryService service) {
        mService = service;
        mTelephonyManager = (TelephonyManager) mService.getSystemService(Context.TELEPHONY_SERVICE);
        mProxyController = ProxyController.getInstance();
    }

    @Override
    public void startNetworkQuery(INetworkQueryServiceCallback cb, int phoneId) {
        mPhoneId = phoneId;
        if (cb != null) {
            synchronized (mService.mCallbacks) {
                mService.mCallbacks.register(cb);
                int state = mService.getState();
                if (DBG) {
                    log("registering callback " + cb.getClass().toString() + " state=" + state);
                }

                switch (state) {
                    case NetworkQueryService.QUERY_READY:
                        // ensure RI is ready
                        connectRadioInteractor();
                        break;

                    // do nothing if we're currently busy.
                    case NetworkQueryService.QUERY_IS_RUNNING:
                        if (DBG) log("query already in progress");
                        break;
                    default:
                }
            }
        }
    }

    @Override
    public void stopNetworkQuery(INetworkQueryServiceCallback cb) {
        synchronized (mService.mCallbacks) {
            int state = mService.getState();
            log("stopNetworkQuery state=" + state);
            if (state == NetworkQueryService.QUERY_IS_RUNNING) {
                Phone phone = PhoneFactory.getPhone(mPhoneId);
                if (phone != null) {
                    abortSearchNetwork(phone,
                            mMyHandler.obtainMessage(EVENT_ABORT_QUERY_NETWORK));
                }
            } else {
                // not running, just notify aborted
                mMyHandler.sendEmptyMessage(EVENT_ABORT_QUERY_NETWORK);
            }
        }
        unregisterCallback(cb);
    }

    @Override
    public void unregisterCallback(INetworkQueryServiceCallback cb) {
        if (cb != null) {
            synchronized (mService.mCallbacks) {
                if (DBG) log("unregistering callback " + cb.getClass().toString());
                mService.mCallbacks.unregister(cb);
            }
        }
    }

    void getAvailableNetworks() {
        if (mService.getState() == NetworkQueryService.QUERY_IS_RUNNING) {
            if (DBG) log("query already in progress");
        } else {
            Phone phone = PhoneFactory.getPhone(mPhoneId);
            if (phone != null) {
                // SPRD: set screen on when query network
                acquireWakeLock();
                phone.getAvailableNetworks(
                        mMyHandler.obtainMessage(EVENT_NETWORK_SCAN_COMPLETED));
                mService.setState(NetworkQueryService.QUERY_IS_RUNNING);
                if (DBG) log("starting new query");
            } else {
                if (DBG) {
                    log("phone is null");
                }
            }
        }
    }

    private void connectRadioInteractor() {
        if (mRi == null) {
            // bind to radio interactor service
            Intent serviceIntent = new Intent(RI_SERVICE_NAME);
            serviceIntent.setPackage(RI_SERVICE_PACKAGE);
            if (DBG) log("bind RI service");
            mService.bindService(serviceIntent, mConnection, Context.BIND_AUTO_CREATE);
        } else {
            mMyHandler.sendEmptyMessage(EVENT_RI_CONNECTED);
        }
    }

    private void deactivateData() {
        mDisconnectPendingCount = mTelephonyManager.getPhoneCount();
        mConnectedPhoneId = SubscriptionManager.INVALID_PHONE_INDEX;
        // Disable data connection on all phones
        for (int id = 0; id < mTelephonyManager.getPhoneCount(); id++) {
            if (mConnectedPhoneId == SubscriptionManager.INVALID_PHONE_INDEX) {
                int[] subId = SubscriptionManager.getSubId(id);
                if (subId != null && subId.length > 0) {
                    if (!mProxyController.isDataDisconnected(subId[0])) {
                        mConnectedPhoneId = id; //current data phone id
                    }
                }
            }

            mProxyController.disableDataConnectivity(id,
                    mMyHandler.obtainMessage(EVENT_ALL_DATA_DISCONNECTED, id, 0));
        }
        log("Exit deactivateData(): mConnectedPhoneId=" + mConnectedPhoneId);
    }

    private boolean hasClients() {
        return mService.mCallbacks.getRegisteredCallbackCount() != 0;
    }

    private void acquireWakeLock() {
        if (mWakeLock == null) {
            PowerManager pm = (PowerManager) mService.getSystemService(Context.POWER_SERVICE);
            mWakeLock = pm.newWakeLock(PowerManager.SCREEN_DIM_WAKE_LOCK, LOG_TAG);
            mWakeLock.acquire();
        }
    }

    private void releaseWakeLock() {
        if (mWakeLock != null && mWakeLock.isHeld()) {
            mWakeLock.release();
            mWakeLock = null;
        }
    }

    private boolean needForceDetach(int phoneId) {
        if (SubscriptionManager.isValidPhoneId(phoneId)) {
            Phone phone = PhoneFactory.getPhone(phoneId);
            int rt = phone.getServiceState().getRilVoiceRadioTechnology();
            log("needForceDetach() rt=" + rt);
            // Modem will handle data detach in 4G, so we don't send close data in 4G
            return rt != ServiceState.RIL_RADIO_TECHNOLOGY_LTE
                       && rt != ServiceState.RIL_RADIO_TECHNOLOGY_LTE_CA;
        }

        return false;
    }

    private void enableData() {
        for (int id = 0; id < mTelephonyManager.getPhoneCount(); id++) {
            mProxyController.enableDataConnectivity(id);
        }
    }

    private void forceDetachDataConn(Phone phone, Message onCompleted) {
        if (mRi != null) {
            mRi.forceDetachDataConn(onCompleted, phone.getPhoneId());
        } else {
            loge("forceDetachDataConn(): RI is disconnected");
        }
    }

    private void abortSearchNetwork(Phone phone, Message onCompleted) {
        if (mRi != null) {
            mRi.abortSearchNetwork(onCompleted, phone.getPhoneId());
        } else {
            loge("abortSearchNetwork(): RI is disconnected");
        }
    }

    private static void log(String msg) {
        Log.d(LOG_TAG, msg);
    }

    private static void loge(String msg) {
        Log.e(LOG_TAG, msg);
    }
}
