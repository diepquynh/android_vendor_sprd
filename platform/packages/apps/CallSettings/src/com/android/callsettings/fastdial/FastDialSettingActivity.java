/**
 * Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *   SelectPhoneAccountDialogFragment.java
 *   Created at 1:48:02 PM, Sep 1, 2015
 *
 */
package com.android.callsettings.fastdial;

import android.app.ActionBar;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.ActivityNotFoundException;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.Configuration;
import android.os.Bundle;
import android.provider.ContactsContract.CommonDataKinds.Phone;
import android.provider.Settings;
import android.telecom.PhoneAccount;
import android.telecom.PhoneAccountHandle;
import android.telecom.TelecomManager;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.widget.EditText;
import android.widget.ImageView;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.GridView;
import android.widget.Toast;
import android.widget.AdapterView.OnItemClickListener;

import com.android.callsettings.CallSettingsActivityContainer;
import com.android.callsettings.fastdial.SelectPhoneAccountDialogFragment.SelectPhoneAccountListener;
import com.android.callsettings.plugins.CallSettingsRelianceHelper;
import com.android.callsettings.R;
import com.android.callsettings.SubscriptionInfoHelper;
import com.android.internal.telephony.IccCardConstants;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.TelephonyIntents;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

public class FastDialSettingActivity extends Activity  implements View.OnClickListener,
    DialogInterface.OnClickListener, DialogInterface.OnShowListener {

    private static String TAG = FastDialManager.TAG;
    private static final int REQUESET_CODE_SELECT_CONTACTS = 4;
    private static final String PHONE_PACKAGE = "com.android.phone";
    private static final String CALL_SETTINGS_CLASS_NAME = "com.android.phone.CallFeaturesSetting";

    private GridView gridView;
    private int fastDialIndex;
    private Dialog mInputDialog;

    private SubscriptionManager mSubscriptionManager;
    private TelephonyManager mTelephonyManager;
    // SPRD: add for bug604693
    private static final String KEY_IS_FAST_INDEX = "is_fast_index";

    /* SPRD: add for bug645817 @{ */
    private CallSettingsActivityContainer mActivityContainer;
    private static final int NO_DECIDE_BY_PHONEID = -1;
    /* @} */

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        /* SPRD: for bug494223.  @{ */
        if(FastDialManager.getInstance() == null) {
            FastDialManager.init(this);
        }
        /* }@ */
        mSubscriptionManager = SubscriptionManager.from(this);
        mTelephonyManager = (TelephonyManager) TelephonyManager.from(this);
        if (this.getResources().getConfiguration().orientation
                == Configuration.ORIENTATION_LANDSCAPE) {
            setContentView(R.layout.fast_dial_setting_land_ex);
        } else if (this.getResources().getConfiguration().orientation
                == Configuration.ORIENTATION_PORTRAIT) {
            setContentView(R.layout.fast_dial_setting_ex);
        }

        ActionBar actionBar = getActionBar();
        if (actionBar != null) {
            actionBar.setDisplayOptions(ActionBar.DISPLAY_HOME_AS_UP
                    | ActionBar.DISPLAY_SHOW_TITLE,
                    ActionBar.DISPLAY_HOME_AS_UP
                    | ActionBar.DISPLAY_SHOW_TITLE
                    | ActionBar.DISPLAY_SHOW_HOME);
            actionBar.setTitle(R.string.fast_dial_settings);
        }
        gridView = (GridView) findViewById(R.id.fast_dial_grid);
        gridView.setOnItemClickListener(new OnItemClickListenerImpl(this));
        FastDialManager.getInstance().setGridView(gridView);
        /* SPRD: add for bug604693 @{ */
        if (savedInstanceState != null) {
            fastDialIndex = savedInstanceState.getInt(KEY_IS_FAST_INDEX);
        }
        /* @} */
        /* SPRD: add to receive sim state to fix bug 506773 @{ */
        final IntentFilter intentFilter = new IntentFilter(
                TelephonyIntents.ACTION_SIM_STATE_CHANGED);
        registerReceiver(mReceiver, intentFilter);
        /* @} */

        /* SPRD: add for bug645817 @{ */
        mActivityContainer = CallSettingsActivityContainer.getInstance();
        mActivityContainer.setApplication(getApplication());
        mActivityContainer.addActivity(this, NO_DECIDE_BY_PHONEID);
        /* @} */
    }

    private class OnItemClickListenerImpl implements OnItemClickListener {

        private Context mContext = null;

        public OnItemClickListenerImpl(Context context) {
            // TODO Auto-generated constructor stub
            mContext = context;
        }

        public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
            if (position == 0) {
                // voice mail setting
                int[] subInfos = mSubscriptionManager.getActiveSubscriptionIdList();
                int subCount = (subInfos == null) ? 0 : subInfos.length;
                int[] standbySims = new int[subCount];
                int standbyCount = 0;

                for (int i = 0; i < subCount; i++) {
                    int phoneId = SubscriptionManager.getPhoneId(subInfos[i]);
                    boolean isStandby = true;
                    /* SPRD: bug#494066, delete useless code @{ */
                    // Judge the state of SIMCard, if it's not ready, making the card not standby.
                    int simState = mTelephonyManager.getSimState(phoneId);
                    if (simState != TelephonyManager.SIM_STATE_READY
                            // TODO: the interface isSimStandby is not exists
                            /*|| !mTelephonyManager.isSimStandby(phoneId)*/) {
                        isStandby = false;
                    }
                    /* @} */
                    if (isStandby) {
                        standbySims[i] = 1;
                        standbyCount++;
                    }
                }

                if (0 == standbyCount) {
                    Toast.makeText(mContext, R.string.no_sim_text, Toast.LENGTH_SHORT).show();
                } else if (1 == standbyCount) {
                    for (int i = 0; i < subCount; i++) {
                        if (1 == standbySims[i]) {
                            int subId = subInfos[i];
                            final Intent intent = new Intent(Intent.ACTION_MAIN);
                            intent.setClassName(PHONE_PACKAGE, CALL_SETTINGS_CLASS_NAME);
                            intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
                            // SPRD: fix bug  426079
                            intent.putExtra(SubscriptionInfoHelper.SUB_ID_EXTRA, subId);
                            startActivity(intent);
                        }
                    }
                } else {
                    // BUG424690 add voicemail funciton on dual card mode.
                    final TelecomManager mTelecomManager
                            = (TelecomManager) mContext.getSystemService(Context.TELECOM_SERVICE);
                    List<PhoneAccountHandle> subscriptionAccountHandles
                                                  = new ArrayList<PhoneAccountHandle>();
                    List<PhoneAccountHandle> accountHandles
                                      = mTelecomManager.getCallCapablePhoneAccounts();
                    final List<SubscriptionInfo> sil
                                   = mSubscriptionManager.getActiveSubscriptionInfoList();
                    for (PhoneAccountHandle accountHandle : accountHandles) {
                        PhoneAccount account = mTelecomManager.getPhoneAccount(accountHandle);
                        if (account.hasCapabilities(PhoneAccount.CAPABILITY_SIM_SUBSCRIPTION)) {
                            subscriptionAccountHandles.add(accountHandle);
                            Log.d(TAG, "accountHandle  id = " + accountHandle.getId());
                        }
                    }

                    SelectPhoneAccountListener listener = new SelectPhoneAccountListener() {
                        @Override
                        public void onPhoneAccountSelected(
                                PhoneAccountHandle selectedAccountHandle,
                                boolean setDefault) {
                            SubscriptionInfo subscription = null;
                            for(int i = 0; i < sil.size(); i++) {
                                /* SPRD: bug#494066, change getId() to getIccId() @{ */
                                if (selectedAccountHandle.getId()
                                        .equals(String.valueOf(sil.get(i).getIccId()))) {
                                    subscription = sil.get(i);
                                }
                                /* @} */
                            }
                            Intent intent = new Intent(TelecomManager.ACTION_SHOW_CALL_SETTINGS);
                            intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
                            SubscriptionInfoHelper.addExtrasToIntent(intent, subscription);
                            Log.d(TAG, "selectedAccountHandle  id = "
                                                + selectedAccountHandle.getId());
                            startActivity(intent);
                        }
                        @Override
                        public void onDialogDismissed() {}
                    };

                    SelectPhoneAccountDialogFragment
                                    .showAccountDialog(((Activity) mContext).getFragmentManager(),
                            subscriptionAccountHandles, listener);
                }
            } else {
                fastDialIndex = position + 1;
                Log.i(TAG, "fastDialIndex= " + fastDialIndex);

                if (!CallSettingsRelianceHelper.getInstance().canEditFastDialNumber(fastDialIndex)) {
                    return;
                }
                HashMap<String, Object> map
                            = (HashMap<String, Object>) parent.getAdapter().getItem(position);
                String mPhoneNameString = (String) map.get("contacts_cell_name");

                if (getString(R.string.menu_add).equals(map.get("contacts_cell_name"))) {
                    // add new number
                    selectFastDial();
                } else {
                    // popup delete or update dialog
                    showDeleteDialog(mPhoneNameString);
                }
            }
        }

        public void selectFastDial() {
            if (mInputDialog != null) {
                    mInputDialog.dismiss();
            }
            AlertDialog.Builder builder = new AlertDialog.Builder(FastDialSettingActivity.this);
            builder.setIcon(R.mipmap.ic_launcher_phone);
            builder.setTitle(R.string.fast_dial_settings);
            builder.setPositiveButton(android.R.string.ok, FastDialSettingActivity.this);
            builder.setNegativeButton(android.R.string.cancel, FastDialSettingActivity.this);
            builder.setView(View.inflate(FastDialSettingActivity.this,
                               R.layout.fast_dial_input_dialog_ex, null));
            mInputDialog = builder.create();
            mInputDialog.setOnShowListener(FastDialSettingActivity.this);
            mInputDialog.show();
        }

        public void showDeleteDialog(final String phoneName) {
            new AlertDialog.Builder(FastDialSettingActivity.this)
                    .setTitle(phoneName)
                    .setItems(R.array.items_fastdial_dialog,
                            new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which) {
                                    if (which == 0) {// update
                                        selectFastDial();
                                    } else {// delete
                                        showDeleteConfirmDialog(phoneName);
                                        dialog.dismiss();
                                    }
                                }
                            })
                    .setNegativeButton(getString(R.string.cancel),
                            new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int which) {
                                    dialog.dismiss();
                                }
                            }).show();
        }

        public void showDeleteConfirmDialog(String phoneName) {
            new AlertDialog.Builder(FastDialSettingActivity.this)
                    .setTitle(R.string.delete_fastdial)
                    .setMessage(phoneName)
                    .setPositiveButton(getString(R.string.alert_dialog_yes),
                            new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog,
                                        int which) {
                                    FastDialManager.getInstance().deleteFastDial(fastDialIndex);
                                }

                            })
                    .setNegativeButton(getString(R.string.alert_dialog_no),
                            new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int which) {
                                    dialog.dismiss();
                                }
                            }).show();
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        // SPRD: modify for bug601333
        FastDialManager.getInstance().flushFdMem();
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (mInputDialog != null) {
            mInputDialog.dismiss();
            mInputDialog = null;
        }
        SelectPhoneAccountDialogFragment.dissmissDialog();
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        if (mSubscriptionManager.getActiveSubscriptionIdList().length
                                     < TelephonyManager.getDefault().getPhoneCount()) {
            SelectPhoneAccountDialogFragment.dissmissDialog();
        }
        if (this.getResources().getConfiguration().orientation
                                  == Configuration.ORIENTATION_LANDSCAPE) {
            setContentView(R.layout.fast_dial_setting_land_ex);
            Log.d(TAG, "onConfigurationChanged: landscape");
            gridView = (GridView) findViewById(R.id.fast_dial_grid);
            gridView.setOnItemClickListener(new OnItemClickListenerImpl(this));
            FastDialManager.getInstance().setGridView(gridView);

        } else if (this.getResources().getConfiguration().orientation
                                            == Configuration.ORIENTATION_PORTRAIT) {
            Log.d(TAG, "onConfigurationChanged: portrait");
            setContentView(R.layout.fast_dial_setting_ex);
            gridView = (GridView) findViewById(R.id.fast_dial_grid);
            gridView.setOnItemClickListener(new OnItemClickListenerImpl(this));
            FastDialManager.getInstance().setGridView(gridView);
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        return false;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home: {
                finish();
                return true;
            }
        }
        return false;
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        // SPRD: add to fix bug 506773
        unregisterReceiver(mReceiver);
        /* SPRD: add for bug645817 @{ */
        if (mActivityContainer != null) {
            mActivityContainer.removeActivity(this);
        }
        /* @} */
        Log.d(TAG, "FastDialSettingActivity onDestroy");
    }

    /* SPRD: add for bug604693 @{ */
    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putInt(KEY_IS_FAST_INDEX, fastDialIndex);
    }
    /* @} */

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        Log.d(TAG, "requestCode=" + requestCode + ", resultCode=" + resultCode);

        if (resultCode != RESULT_OK) {
            Log.d(TAG, "fail due to resultCode=" + resultCode);
            return;
        }
        int myRequestCode = requestCode;
        switch (myRequestCode) {
            case REQUESET_CODE_SELECT_CONTACTS:
                FastDialManager.getInstance().addFastDial(data, fastDialIndex);
                break;
            default:
        }
    }

    @Override
    public void onClick(DialogInterface dialog, int which) {
        if (which == DialogInterface.BUTTON_POSITIVE) {
            EditText editText = (EditText) ((AlertDialog) dialog).findViewById(R.id.number);
            final String number = editText.getText().toString();
            if (TextUtils.isEmpty(number)) {
                return;
            }
            FastDialManager.getInstance().addFastDial(null, fastDialIndex, number);
        }
    }

    @Override
    public void onShow(DialogInterface dialog) {
             ImageView imageView = (ImageView) ((AlertDialog) dialog).findViewById(R.id.contacts);
             imageView.setOnClickListener(this);
    }

    @Override
    public void onClick(View v) {
        if (v.getId() == R.id.contacts) {
            Intent mContactListIntent = new Intent(Intent.ACTION_PICK);
            mContactListIntent.setType(Phone.CONTENT_TYPE);
            try {
                FastDialSettingActivity.this
                       .startActivityForResult(mContactListIntent, REQUESET_CODE_SELECT_CONTACTS);
            } catch (ActivityNotFoundException e) {
                String toast = this.getResources()
                        .getString(com.android.internal.R.string.noApplications);
                Toast.makeText(this, toast,Toast.LENGTH_LONG).show();
                Log.e(TAG, "No Activity found to handle Intent: "+ mContactListIntent);
            }
         }
     }

    /* SPRD: add to receive sim state to fix bug 506773 @{ */
    private BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            final String action = intent.getAction();
            if (TelephonyIntents.ACTION_SIM_STATE_CHANGED.equals(action)) {
                String stateExtra = intent.getStringExtra(IccCardConstants.INTENT_KEY_ICC_STATE);
                if (stateExtra != null
                        && IccCardConstants.INTENT_VALUE_ICC_ABSENT.equals(stateExtra)) {
                    SelectPhoneAccountDialogFragment.dissmissDialog();
                }
            }
        }
    };
    /* @} */
}

