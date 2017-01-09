
package com.sprd.engineermode.debuglog;

import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.UserHandle;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceGroup;
import android.provider.Settings;
import android.os.ServiceManager;
import android.os.RemoteException;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.Toast;
import android.telephony.PhoneStateListener;
import android.telephony.ServiceState;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.util.Log;
import com.android.internal.telephony.ITelephony;

import com.sprd.engineermode.R;
import com.sprd.engineermode.telephony.TelephonyFragment;

public class BandModeSetActivity extends PreferenceActivity implements
        Preference.OnPreferenceClickListener {

    private static final String TAG = "BandModeSetActivity";
    private static final int KEY_SAVE_BAND = 1;
    private PreferenceGroup mPreGroup = null;
    private ProgressDialog mProgressDlg;
    private BandSelector mBandSelector;
    private FBHandler mFBHandler;
    private int mPhoneID = -1;
    private Handler mUiThread = new Handler();
    private TelephonyManager mTelephonyManager;
    public static BandModeSetActivity BandModeSetActivityInstance = null;

    class FBHandler extends Handler {
        public FBHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            Log.d(TAG, " handleMessage:" + msg.what);
            switch (msg.what) {
                case KEY_SAVE_BAND:
                    showProgressDialog("Saving band");
                    mBandSelector.saveBand();
                    dismissProgressDialog();
                    AlertDialog alertDialog = new AlertDialog.Builder(
                            BandModeSetActivity.this)
                            .setTitle("Band Select")
                            .setMessage(mBandSelector.getSetInfo())
                            .setPositiveButton(R.string.alertdialog_ok,
                                    new DialogInterface.OnClickListener() {
                                        @Override
                                        public void onClick(DialogInterface dialog,
                                                int which) {
                                        }
                                    }).create();
                    alertDialog.show();
                    break;
            }
        }
    }

    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        mFBHandler = new FBHandler(ht.getLooper());

        setPreferenceScreen(getPreferenceManager().createPreferenceScreen(this));
        mPreGroup = getPreferenceScreen();

        mPhoneID = getIntent().getIntExtra(TelephonyFragment.KEY_PHONEID, 0);
        mTelephonyManager = TelephonyManager.from(this);
        Log.d(TAG, "onCreate mPhoneID:" + mPhoneID);
        mBandSelector = new BandSelector(mPhoneID, this, mUiThread);
        BandModeSetActivityInstance = this;
    }

    @Override
    protected void onStart() {
        mBandSelector.initModes(mPreGroup);
        mBandSelector.loadBands();
        super.onStart();
    }

    @Override
    protected void onDestroy() {
        if (mFBHandler != null) {
            mFBHandler.getLooper().quit();
        }
        BandModeSetActivityInstance = null;
        super.onDestroy();
    }

    @Override
    public boolean onPreferenceClick(Preference preference) {
        // TODO Auto-generated method stub
        return false;
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.frequency_set, menu);
        MenuItem item = menu.findItem(R.id.frequency_set);
        if (item != null) {
            item.setVisible(true);
        }
        return super.onCreateOptionsMenu(menu);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.frequency_set: {
                if (!mBandSelector.isCheckOneOrMore()) {
                    Toast.makeText(getApplicationContext(),
                            "Please check at least one every mode!",
                            Toast.LENGTH_SHORT).show();
                } else {
                    mFBHandler.sendEmptyMessage(KEY_SAVE_BAND);
                }
            }
                break;
            default:
                Log.i(TAG, "default");
                break;
        }
        return super.onOptionsItemSelected(item);
    }

    private void showProgressDialog(final String msg) {
        mUiThread.post(new Runnable() {
            public void run() {
                mProgressDlg = ProgressDialog.show(BandModeSetActivity.this,
                        msg, "Please wait...", true, false);
            }
        });
    }

    private void dismissProgressDialog() {
        mUiThread.post(new Runnable() {
            @Override
            public void run() {
                if (mProgressDlg != null) {
                    mProgressDlg.dismiss();
                }
            }
        });
    }

}
