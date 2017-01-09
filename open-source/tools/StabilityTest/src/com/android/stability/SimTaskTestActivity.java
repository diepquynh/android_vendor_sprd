
package com.android.stability;

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.nio.charset.Charset;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.database.Cursor;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.AsyncTask.Status;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.provider.Contacts.People;
import android.telephony.PhoneStateListener;
import android.telephony.TelephonyManager;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import com.android.internal.telephony.ITelephony;
import com.android.internal.telephony.TelephonyIntents;

public class SimTaskTestActivity extends Activity implements OnClickListener,
        OnCheckedChangeListener {

    private static final String TAG = "SimTaskTestActivity";

    private static final int DEFAULT_TEST_DURATION = 72;

    private static final int DEFAULT_CALL_DURATION = 3;

    private static final int DEFAULT_R_W_FREQUENC = 30;

    private static final String DEFAULT_NUMBER = "10086";

    private static final int MSSAGE_PLACE_CALL = 1;

    private static final int MSSAGE_HANGUP = 2;

    private static final int MSSAGE_END_TEST = 3;

    private static final int MSSAGE_SIM_UPDATE = 4;

    private EditText edit_duration;

    private EditText edit_number;

    private EditText edit_call_duration;

    private EditText edit_r_w_frequency;

    private CheckBox check_task_reset;

    private TextView text_resualt;

    private Button button_start;

    private SimTestTask mSimTestTask;

    private Intent mIntent;

    private Handler mHandler;

    private Boolean mTestRunning = false;

    private PhoneStateListener mPhoneStateListener;

    private ITelephony mITelephony;

    private Context mContext;

    private int[] simOpraterCount = {
            0, 0
    };

    private int callOpraterCount = 0;

    private int missSimCount = 0;

    private int simResetCount = 0;

    private int simMaxResetCount = 0;

    private String mNumber;

    private int mTest_duration = 0;

    private int mR_w_frequency = 0;

    private int mCall_duration = 0;

    private engfetch mEf;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // TODO Auto-generated method stub
        super.onCreate(savedInstanceState);
        // Log.d(TAG, "oncreate set GSM900: " + setSelectedBandGSM900());
        mContext = this;
        setContentView(R.layout.sim_task_test);
        edit_duration = (EditText) findViewById(R.id.edit_duration);
        edit_number = (EditText) findViewById(R.id.edit_number);
        edit_call_duration = (EditText) findViewById(R.id.edit_call_duration);
        edit_r_w_frequency = (EditText) findViewById(R.id.edit_r_w_frequency);
        check_task_reset = (CheckBox) findViewById(R.id.check_task_reset);
        check_task_reset.setOnCheckedChangeListener(this);
        button_start = (Button) findViewById(R.id.button_start);
        text_resualt = (TextView) findViewById(R.id.testresualt);
        init();
    }

    private void init() {

        mSimTestTask = new SimTestTask();
        mHandler = new SimTestHandler();
        mPhoneStateListener = getPhoneStateListener();
        ((TelephonyManager) getSystemService(Context.TELEPHONY_SERVICE)).listen(
                mPhoneStateListener, PhoneStateListener.LISTEN_CALL_STATE);
        mITelephony = ITelephony.Stub.asInterface(ServiceManager
                .getService(Context.TELEPHONY_SERVICE));
        IntentFilter filter = new IntentFilter();
        filter.addAction(TelephonyIntents.ACTION_SIM_STATE_CHANGED);
        registerReceiver(mIntentReceiver, filter);

        edit_duration.setText(String.valueOf(DEFAULT_TEST_DURATION));
        edit_number.setText(DEFAULT_NUMBER);
        edit_call_duration.setText(String.valueOf(DEFAULT_CALL_DURATION));
        edit_r_w_frequency.setText(String.valueOf(DEFAULT_R_W_FREQUENC));
        mEf = new engfetch();
        Toast.makeText(this,
                "set Band to GSM900 is " + (setSelectedBandGSM900() ? "success." : "failure."), 500)
                .show();
        if (check_task_reset.isChecked()) {
            setSpsimrstCt(10);
        } else {
            setSpsimrstCt(0);
        }

    }

    private BroadcastReceiver mIntentReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            // TODO Auto-generated method stub
            String action = intent.getAction();
            // Log.d(TAG, "onReceive: " + action);
            if (action.startsWith(TelephonyIntents.ACTION_SIM_STATE_CHANGED)) {
                Log.d(TAG, "SimState: " + TelephonyManager.getDefault().getSimState()
                        + " TestRunning: " + mTestRunning);
                if (TelephonyManager.getDefault().getSimState() != TelephonyManager.SIM_STATE_READY
                        && mTestRunning) {
                    missSimCount++;
                    mHandler.sendEmptyMessage(MSSAGE_END_TEST);
                }
            }
        }
    };

    @Override
    protected void onResume() {
        // TODO Auto-generated method stub
        super.onResume();
        boolean enabled = false;
        for (int i = 0; i < TelephonyManager.getPhoneCount(); i++) {

            if (TelephonyManager.getDefault(i).getSimState() == TelephonyManager.SIM_STATE_READY) {
                enabled = true;
            }
        }
        enabled &= !mTestRunning;
        button_start.setEnabled(enabled);
        button_start.setOnClickListener(this);

    }

    @Override
    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
        if (buttonView == check_task_reset) {
            if (isChecked) {
                setSpsimrstCt(10);
            } else {
                setSpsimrstCt(0);
            }

        }
    }

    @Override
    public void onClick(View v) {
        if (v == button_start) {
            try {
                Log.d(TAG, "iTelephony.getCallState()=" +
                        mITelephony.getCallState());
                if (TelephonyManager.CALL_STATE_IDLE == mITelephony.getCallState()) {
                    if (!setDataSuccess()) {
                        Toast.makeText(this, "Input is not validated.", 1000).show();
                        return;
                    }
                    button_start.setEnabled(false);
                    mTestRunning = true;
                    mIntent = new Intent();
                    mIntent.setAction(Intent.ACTION_CALL);
                    mIntent.setData(Uri.parse("tel:" + mNumber));
                    mHandler.sendEmptyMessage(MSSAGE_PLACE_CALL);
                    if (mTest_duration == 100000) { // add for me to test
                        mHandler.sendEmptyMessageDelayed(MSSAGE_END_TEST, 60000
                                );
                    } else {
                        mHandler.sendEmptyMessageDelayed(MSSAGE_END_TEST,
                                mTest_duration * 1000 * 3600
                                );
                    }
                    if (mSimTestTask == null) {
                        mSimTestTask = new SimTestTask();
                    }
                    Status nowTaskStatus = mSimTestTask.getStatus();
                    if (nowTaskStatus.equals(Status.RUNNING)) {
                        return;
                    }
                    mSimTestTask.execute();

                }
            } catch (RemoteException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
        }

    }

    private void endTest() {
        getSpsimrstCt();
        mTestRunning = false;
        if (mHandler != null) {
            mHandler.removeMessages(MSSAGE_PLACE_CALL);
            mHandler.removeMessages(MSSAGE_HANGUP);
            mHandler.removeMessages(MSSAGE_END_TEST);
            mHandler.removeMessages(MSSAGE_SIM_UPDATE);
            mHandler.sendEmptyMessage(MSSAGE_HANGUP);
        }
        if (mSimTestTask != null) {
            Status nowTaskStatus = mSimTestTask.getStatus();
            if (nowTaskStatus.equals(Status.RUNNING)) {
                mSimTestTask.cancel(true);
            }
            mSimTestTask = null;
        }

        String result_str = "test end. | call count:" + callOpraterCount
                + ". | sim oprater count: | success" + simOpraterCount[0] + "  failure"
                + simOpraterCount[1] + ". | sim miss count:" + missSimCount
                + ". | sim reset count:"
                + simResetCount + ". | sim max reset count:" + simMaxResetCount;
        text_resualt.setText(result_str.replace("|", "\n"));
        Log.d(TAG, result_str);
    }

    private void resetResultCount() {
        simOpraterCount[0] = 0;
        simOpraterCount[1] = 0;
        callOpraterCount = 0;
        missSimCount = 0;
        simResetCount = 0;
        simMaxResetCount = 0;
    }

    private boolean setDataSuccess() {
        String number = edit_number.getText().toString();
        String test_duration = edit_duration.getText().toString();
        String r_w_frequency = edit_r_w_frequency.getText().toString();
        String call_duration = edit_call_duration.getText().toString();
        if (inputVerifiy(number) && inputVerifiy(test_duration) && inputVerifiy(r_w_frequency)
                && inputVerifiy(call_duration)) {
            mNumber = number;
            mTest_duration = Integer.parseInt(test_duration);
            mR_w_frequency = Integer.parseInt(r_w_frequency);
            mCall_duration = Integer.parseInt(call_duration);
            resetResultCount();
            return true;
        }

        return false;
    }

    private boolean inputVerifiy(String input) {
        if (input != null && !input.isEmpty() && !input.equals("0")) {
            return true;
        }
        return false;
    }

    private class SimTestHandler extends Handler {
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch (msg.what) {

                case MSSAGE_PLACE_CALL:
                    Log.d(TAG, "MSSAGE_PLACE_CALL");
                    callOpraterCount++;
                    startActivity(mIntent);
                    Log.d(TAG, "call_duration: " + mCall_duration + "m");
                    mHandler.sendEmptyMessageDelayed(MSSAGE_HANGUP, mCall_duration * 1000 * 60);
                    break;

                case MSSAGE_HANGUP:
                    try {
                        mITelephony.endCall();
                    } catch (RemoteException e) {
                        // TODO Auto-generated catch block
                        e.printStackTrace();
                    }
                    break;

                case MSSAGE_END_TEST:
                    Log.d(TAG, "MSSAGE_END_TEST");
                    endTest();
                    break;

                case MSSAGE_SIM_UPDATE:
                    Log.d(TAG, "MSSAGE_SIM_UPDATE");
                    Bundle data = msg.getData();
                    String tag = data.getString("tag");
                    String number = data.getString("number");
                    String newTag = getNewTag(tag);
                    String newNumber = getNewNumber(number);
                    String index = data.getString("index");
                    SimUpdate(tag, number, newTag, index, newNumber);
                    Log.d(TAG, "r_w_frequency: " + mR_w_frequency + "s");
                    sendToUpdateSim(newTag, newNumber, index, mR_w_frequency *
                            1000);
                    break;

                default:
                    break;
            }

        }
    }

    private PhoneStateListener getPhoneStateListener() {

        return new PhoneStateListener() {
            @Override
            public void onCallStateChanged(int state, String incomingNumber) {
                try {
                    Log.d(TAG,
                            "mTestRunning: " + mTestRunning + " state: "
                                    + mITelephony.getCallState() + " incomingNumber: "
                                    + incomingNumber);
                    if (TelephonyManager.CALL_STATE_IDLE == mITelephony.getCallState()
                            && mTestRunning) {
                        mHandler.removeMessages(MSSAGE_PLACE_CALL);
                        mHandler.removeMessages(MSSAGE_HANGUP);
                        mHandler.sendEmptyMessage(MSSAGE_PLACE_CALL);
                    }
                } catch (RemoteException e) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }

            }
        };

    }

    private class SimTestTask extends AsyncTask<Void, Void, Void> {
        Cursor cursor;
        String tag;
        String number;
        String index;

        @Override
        protected void onPreExecute() {
            Log.d(TAG, "onPreExecute");
        }

        @Override
        protected Void doInBackground(Void... params) {
            Log.d(TAG, "doInBackground");
            Uri uri = Uri.parse("content://icc/adn");
            cursor = getContentResolver().query(uri, null, null, null, null);
            if (cursor == null) {
                Log.d(TAG, "query failure,cursor is null");
                simOpraterCount[1]++;
                return null;
            }
            simOpraterCount[0]++;
            Log.d(TAG, ">>>>>>" + cursor.getCount());
            while (cursor.moveToNext()) {
                String id = cursor.getString(cursor.getColumnIndex(People._ID));
                String name = cursor.getString(cursor.getColumnIndex(People.NAME));
                String phoneNumber = cursor.getString(cursor.getColumnIndex(People.NUMBER));
                String simIndex = cursor.getString(cursor.getColumnIndex("index"));
                Log.d(TAG, ">>>>>>" + "_id, " + id);
                Log.d(TAG, ">>>>>>" + "name, " + name);
                Log.d(TAG, ">>>>>>" + "phone number, " + phoneNumber);
                Log.d(TAG, ">>>>>>" + "index, " + simIndex);
                tag = name;
                number = phoneNumber;
                index = simIndex;
                break;
            }
            return null;
        }

        @Override
        protected void onPostExecute(Void result) {
            Log.d(TAG, "onPostExecute");
            if (cursor != null) {
                if (cursor.getCount() > 0) {
                    sendToUpdateSim(tag, number, index, 0);
                    cursor.close();
                    cursor = null;
                } else {
                    SimInsert(getNewTag(tag), getNewNumber(number));
                }
            }
        }
    }

    private void sendToUpdateSim(String tag, String number, String index, long delay) {
        Message msg = Message.obtain();
        Bundle data = new Bundle();
        data.putString("tag", tag);
        data.putString("number", number);
        data.putString("index", index);
        msg.setData(data);
        msg.what = MSSAGE_SIM_UPDATE;
        mHandler.sendMessageDelayed(msg, delay);
    }

    private void SimInsert(String tag, String number) {
        Uri uri = Uri.parse("content://icc/adn");
        ContentValues values = new ContentValues();
        values.put("tag", tag);
        values.put("number", number);
        Uri newSimContactUri = getContentResolver().insert(uri, values);
        Log.d(TAG, ">>>>>>" + "new sim contact uri, " + newSimContactUri);
        if (newSimContactUri != null) {
            simOpraterCount[0]++;
            sendToUpdateSim(tag, number, "1", mR_w_frequency * 1000);
        } else {
            simOpraterCount[1]++;
        }
    }

    private void SimUpdate(String tag, String number, String newTag, String index, String newNumber) {
        Log.d(TAG, ">>>>>>" + tag + " " + number + " " + newTag + " " + newNumber);
        Uri uri = Uri.parse("content://icc/adn");
        ContentValues values = new ContentValues();
        // values.put("tag", tag);
        // values.put("number", number);
        // values.put("newTag", newTag);
        // values.put("newNumber", newNumber);
        values.put("tag", newTag);
        values.put("number", newNumber);
        values.put("index", Integer.valueOf(index));
        int result = getContentResolver().update(uri, values, null, null);
        Log.d(TAG, ">>>>>>" + "update result: " + result);
        if (result > 0) {
            simOpraterCount[0]++;
        } else {
            simOpraterCount[1]++;
        }
    }

    private String getNewTag(String tag) {
        if (tag == null || tag.length() < 2) {
            tag = "abc";
        } else {
            tag = new StringBuilder(tag).reverse().toString();
        }

        return tag;
    }

    private String getNewNumber(String number) {
        if (number == null || number.length() < 2) {
            number = "10086";
        } else {
            number = new StringBuilder(number).reverse().toString();
        }

        return number;
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        Log.d(TAG, "onDestroy");
        ((TelephonyManager) getSystemService(Context.TELEPHONY_SERVICE)).listen(
                mPhoneStateListener, PhoneStateListener.LISTEN_NONE);
        unregisterReceiver(mIntentReceiver);
        endTest();
    }

    private boolean setSelectedBandGSM900() {
        int mSocketID = mEf.engopen();
        ByteArrayOutputStream outputBuffer = new ByteArrayOutputStream();
        DataOutputStream outputBufferStream = new DataOutputStream(outputBuffer);

        String mATline = String.format("%d,%d,%d", 2, 1, 0);

        try {
            outputBufferStream.writeBytes(mATline);
        } catch (IOException e) {
            Log.e(TAG, "writeBytes() error!");
            return true;
        }

        mEf.engwrite(mSocketID, outputBuffer.toByteArray(),
                outputBuffer.toByteArray().length);

        int dataSize = 128;
        byte[] inputBytes = new byte[dataSize];

        int showlen = mEf.engread(mSocketID, inputBytes, dataSize);
        String mATResponse = new String(inputBytes, 0, showlen);
        Log.d(TAG, "setSelectedBandGSM900 mATResponse: " + mATResponse);
        if (mATResponse.indexOf("ERROR") != -1) {
            return false;
        }
        return true;
    }

    private boolean setSpsimrstCt(int resetCount) {
        int mSocketID = mEf.engopen();
        ByteArrayOutputStream outputBuffer = new ByteArrayOutputStream();
        DataOutputStream outputBufferStream = new DataOutputStream(outputBuffer);

        String mATline = String.format("%d,%d,%d", 300, 1, resetCount);

        try {
            outputBufferStream.writeBytes(mATline);
        } catch (IOException e) {
            Log.e(TAG, "writeBytes() error!");
            return true;
        }

        mEf.engwrite(mSocketID, outputBuffer.toByteArray(),
                outputBuffer.toByteArray().length);

        int dataSize = 128;
        byte[] inputBytes = new byte[dataSize];

        int showlen = mEf.engread(mSocketID, inputBytes, dataSize);
        String mATResponse = new String(inputBytes, 0, showlen);
        Log.d(TAG, "setSpsimrstCt mATResponse: " + mATResponse);
        if (mATResponse.indexOf("ERROR") != -1) {
            return false;
        }
        return true;

    }

    private int[] getSpsimrstCt() {
        int[] count = new int[2];
        int mSocketID = mEf.engopen();
        ByteArrayOutputStream outputBuffer = new ByteArrayOutputStream();
        DataOutputStream outputBufferStream = new DataOutputStream(outputBuffer);
        String mATline = new StringBuilder().append("301").append(",")
                .append(0).toString();
        try {
            outputBufferStream.writeBytes(mATline);
        } catch (IOException e) {
            Log.e(TAG, "writeBytes() error!");
            return null;
        }

        mEf.engwrite(mSocketID, outputBuffer.toByteArray(), outputBuffer.toByteArray().length);

        int dataSize = 128;
        byte[] inputBytes = new byte[dataSize];

        int showlen = mEf.engread(mSocketID, inputBytes, dataSize);
        String mATResponse = new String(inputBytes, 0, showlen, Charset.defaultCharset());
        Log.d(TAG, "getSpsimrstCt result : " + mATResponse);
        try {
            if (mATResponse != null) {
                String countString = mATResponse.substring(mATResponse.indexOf(':') + 1);
                Log.d(TAG, "countString=" + countString);
                String[] countStringArry = countString.trim().split(",");
                for (int i = 0; i < countStringArry.length; i++) {
                    count[i] = Integer.parseInt(countStringArry[i]);
                    Log.d(TAG, "count[" + i + "]=" + count[i]);
                }
                simMaxResetCount = count[0];
                simResetCount = count[1];
            }
        } catch (Exception e) {
            Log.e(TAG, "Format String " + mATResponse + " to Integer Error!");
            return null;
        }
        return count;
    }
}
