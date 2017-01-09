package com.android.callsettings.callforward;

import android.app.ActionBar;
import android.content.Intent;
import android.database.Cursor;
import android.os.Bundle;
import android.preference.PreferenceActivity;
import android.provider.ContactsContract.CommonDataKinds;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.EditText;

import com.android.internal.telephony.PhoneConstants;
import com.android.callsettings.EditPhoneNumberPreference;
import com.android.callsettings.R;

public class CallForwardTimeEditPreference extends PreferenceActivity {

    private static final String LOG_TAG = CallForwardTimeEditPreference.class.getSimpleName();
    protected static final int CFT_PREF_ID = 1;
    private static final String NUM_PROJECTION[] = { CommonDataKinds.Phone.NUMBER };
    private static final int SET_OK = (1 << 0) | (1 << 1) | (1 << 2);
    protected static final int MENU_CANCLE = Menu.FIRST;
    protected static final int MENU_OK = Menu.FIRST + 2;
    protected static final int MENU_DISABLE = Menu.FIRST + 1;
    CallForwardTimeEditPreFragement mFragment;
    Menu mOptionMenu;
    private int mPhoneId = 0;
    /* SPRD: add for bug560757 @{ */
    private static final String IS_TIME_EDIT_DIALOG_SHOWING = "is_time_edit_dialog_showing";
    private static final String TIME_EDIT_PHONE_NUMBER = "time_edit_phone_number";
    /* @} */

    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        ActionBar actionBar = getActionBar();
        if (actionBar != null) {
            // android.R.id.home will be triggered in onOptionsItemSelected()
            actionBar.setDisplayHomeAsUpEnabled(true);
        }
        if (getIntent() != null) {
            mPhoneId = Integer.valueOf(getIntent().getStringExtra("phone_id"));
        }
        mFragment = new CallForwardTimeEditPreFragement(this.getApplicationContext(), mPhoneId);
        // SPRD: add for bug560757
        mFragment.setArguments(icicle);
        getFragmentManager().beginTransaction().replace(android.R.id.content, mFragment).commit();
    }

    @Override
    protected void onResume() {
        super.onResume();
        mFragment.setParentActivity(CFT_PREF_ID);
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        Log.i("CallForwardTimeEditPreference", "onActivityResult: requestCode: " + requestCode
                + ", resultCode: " + resultCode
                + ", data: " + data);
        if (requestCode == CFT_PREF_ID) {
            if (resultCode != RESULT_OK) {
                log("onActivityResult: contact picker result not OK.");
                return;
            }

            Cursor cursor = null;
            try {
                cursor = getContentResolver().query(data.getData(),
                        NUM_PROJECTION, null, null, null);
                if ((cursor == null) || (!cursor.moveToFirst())) {
                    log("onActivityResult: bad contact data, no results found.");
                    return;
                }
                mFragment.onPickActivityResult(cursor.getString(0));
                return;
            } finally {
                if (cursor != null) {
                    cursor.close();
                }
            }
        }
    }

    /* SPRD: add for bug560757 @{ */
    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        EditPhoneNumberPreference phoneNumberPreference = mFragment.getEditPhoneNumberPreference();
        if (phoneNumberPreference != null && phoneNumberPreference.getDialog() != null) {
            outState.putBoolean(IS_TIME_EDIT_DIALOG_SHOWING, phoneNumberPreference.getDialog()
                    .isShowing());
            EditText phoneNumberEditText = phoneNumberPreference.getEditText();
            if (phoneNumberEditText != null && phoneNumberEditText.getText() != null)
                outState.putString(TIME_EDIT_PHONE_NUMBER, phoneNumberEditText.getText().toString());
        }
    }
    /* @} */

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        super.onCreateOptionsMenu(menu);
        mOptionMenu = menu;
        menu.add(0, MENU_CANCLE, 0, R.string.cancel).setShowAsAction(
                MenuItem.SHOW_AS_ACTION_ALWAYS);
        menu.add(1, MENU_DISABLE, 0, R.string.disable).setShowAsAction(
                MenuItem.SHOW_AS_ACTION_ALWAYS);
        menu.add(2, MENU_OK, 0, R.string.ok).setShowAsAction(
                MenuItem.SHOW_AS_ACTION_ALWAYS);
        return true;
    }

    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        super.onPrepareOptionsMenu(menu);
        notifySetNumberChange();
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {

        switch (item.getItemId()) {
            case MENU_OK:
                mFragment.onMenuOKClicked("MENU_OK");
                finish();
                return true;
            case MENU_DISABLE:
                mFragment.onMenuOKClicked("MENU_DISABLE");
                finish();
                return true;
            case MENU_CANCLE:
                finish();
                return true;
            case android.R.id.home:
                finish();
                return true;
        }
        return false;
    }

    public void notifySetNumberChange() {
        int setNumber = mFragment.getSetNumber();
        log("notifySetNumberChange  setNumber = " + setNumber);
        /* SPRD: add for bug 526019 @{ */
        if (setNumber == SET_OK) {
            mOptionMenu.setGroupVisible(1, true);
            mOptionMenu.setGroupVisible(2, true);
        } else {
            mOptionMenu.setGroupVisible(1, false);
            mOptionMenu.setGroupVisible(2, false);
        }
        /* @} */
    }

    private static void log(String msg) {
        Log.d(LOG_TAG, msg);
    }

}