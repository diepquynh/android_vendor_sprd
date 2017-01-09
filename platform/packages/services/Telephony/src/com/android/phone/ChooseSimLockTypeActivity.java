package com.android.phone;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.AsyncResult;
import android.os.PowerManager;
import android.view.View;
import android.widget.Toast;
import android.widget.ListView;
import android.widget.ArrayAdapter;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.util.Log;
import com.android.phone.R;

import android.telephony.TelephonyManager;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.GsmCdmaPhone;
import com.android.internal.telephony.PhoneFactory;
import com.android.sprd.telephony.RadioInteractor;

import java.util.ArrayList;
import java.util.List;
import java.util.Arrays;

public class ChooseSimLockTypeActivity extends Activity {

    private static final String TAG = "ChooseSimLockTypeActivity";
    private static final int SIM_NETWORK_LOCKED = 0;
    private static final int SIM_NETWORK_SUBSET_LOCKED = 1;
    private static final int SIM_SERVICE_PROVIDER_LOCKED = 2;
    private static final int SIM_CORPORATE_LOCKED = 3;
    private static final int SIM_SIM_LOCKED = 4;
    private static final int DEFAULT_SERVICE_CLASS = 7;
    private String[] mSimLockType;
    private List list;
    private ListView mylistview;
    private Phone mPhone;
    private int mCurrentSimLockType = SIM_SIM_LOCKED;
    private String mFacility = "PN";
    private MyHandler mHandler = new MyHandler();
    RadioInteractor mRadioInteractor;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.one_key_simlock_ex);
        mylistview = (ListView)findViewById(R.id.listview);
        mSimLockType = getResources().getStringArray(R.array.one_key_sim_lock_type);
        list = Arrays.asList(mSimLockType);
        ArrayAdapter<String> myArrayAdapter = new ArrayAdapter<String>
                            (this,android.R.layout.simple_list_item_1, list);
        mylistview.setAdapter(myArrayAdapter);
        mylistview.setOnItemClickListener(new OnItemClickListenerImpl());
        mPhone = PhoneFactory.getPhone(0);
        mRadioInteractor = new RadioInteractor(this);
    }

    @Override
    public void onResume() {
        super.onResume();
    }

    private void setFacilityLockByUser(int type, boolean enable) {
        switchSimLockType(type);
        Log.d(TAG, "setFacilityLockByUser enable = " + enable);
        mRadioInteractor.setFacilityLockByUser(
                mFacility, enable, mHandler.obtainMessage(MyHandler.MESSAGE_SET_LOCK), 0);

    }

    private void querySimLockState(int type) {
        mCurrentSimLockType = type;
        switchSimLockType(type);
        // when query simlock type,password is not necessary correct, but need to set a value.
        ((GsmCdmaPhone)mPhone).mCi.queryFacilityLock(mFacility,"00000000", DEFAULT_SERVICE_CLASS,
                mHandler.obtainMessage(MyHandler.MESSAGE_GET_LOCK));
    }

    private void switchSimLockType(int type) {
        switch (type) {
            case SIM_NETWORK_LOCKED:
                mFacility = "PN";
                break;
            case SIM_NETWORK_SUBSET_LOCKED:
                mFacility = "PU";
                break;
            case SIM_SERVICE_PROVIDER_LOCKED:
                mFacility = "PP";
                break;
            case SIM_CORPORATE_LOCKED:
                mFacility = "PC";
                break;
            case SIM_SIM_LOCKED:
                mFacility = "PS";
                break;
            default:
                mFacility = "PN";
        }
    }

    private void showSimLockDialog(final int type,final boolean enable) {
        int notificationMsgID, lockedButtonID;
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle(mSimLockType[type]);
        if (enable) {
            notificationMsgID = R.string.one_key_locked_notification;
            lockedButtonID = R.string.one_key_locked;
        } else {
            notificationMsgID = R.string.one_key_unlocked_notification;
            lockedButtonID = R.string.one_key_unlocked;
        }
        builder.setMessage(notificationMsgID);
        builder.setPositiveButton(lockedButtonID, new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int which) {
                    mylistview.setEnabled(false);
                    setFacilityLockByUser(type, enable);
                }
        });
        builder.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int which) {
                    mylistview.setEnabled(true);
                }
        });

        builder.create();
        if (this != null
                && (this instanceof Activity)
                && (!((Activity)this).isFinishing())) {
            builder.show();
        }
    }

    private class MyHandler extends Handler {
        private static final int MESSAGE_GET_LOCK = 0;
        private static final int MESSAGE_SET_LOCK = 1;
        private static final int EVENT_REBOOT = 500;

        public void handleMessage(Message msg) {
            AsyncResult ar;
            switch (msg.what) {
                case MESSAGE_GET_LOCK:
                    ar = (AsyncResult) msg.obj;
                    if (ar.exception != null) {
                        mylistview.setEnabled(true);
                        Toast.makeText(mPhone.getContext(), R.string.one_key_query_unsuccess, Toast.LENGTH_LONG).show();
                        Log.d(TAG, "MESSAGE_GET_LOCK: ar.exception=" + ar.exception);
                    } else {
                        int infoArray[] = (int[]) ar.result;
                        showSimLockDialog(mCurrentSimLockType, (infoArray[0] != 1));
                    }
                    break;
                case MESSAGE_SET_LOCK:
                    ar = (AsyncResult) msg.obj;
                    if (ar.exception != null) {
                        mylistview.setEnabled(true);
                        Log.d(TAG, "MESSAGE_SET_LOCK: ar.exception=" + ar.exception);
                        Toast.makeText(mPhone.getContext(), R.string.one_key_operation_unsuccess, Toast.LENGTH_LONG).show();
                    } else {
                        Toast.makeText(mPhone.getContext(), R.string.one_key_operation_success, Toast.LENGTH_LONG).show();
                        // set lock else need reboot before device will work,so here lock/unlock all reboot.
                        sendMessageDelayed(obtainMessage(EVENT_REBOOT), 3000);
                    }
                    break;
                case EVENT_REBOOT:
                    PowerManager pm = (PowerManager)mPhone.getContext().getSystemService(
                            Context.POWER_SERVICE);
                    pm.reboot("SIMLOCK Reboot");
                    break;
            }
        }
    }

    private class OnItemClickListenerImpl implements OnItemClickListener {
        @Override
        public void onItemClick(AdapterView<?> parent, View view, int simlockType, long id) {
            querySimLockState(simlockType);
        }
    }
}
