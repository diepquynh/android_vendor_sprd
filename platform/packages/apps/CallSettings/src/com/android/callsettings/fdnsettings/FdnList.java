/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.callsettings.fdnsettings;

import android.app.ActionBar;
import android.content.Intent;
import android.content.res.Resources;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.ListView;


import com.android.callsettings.SubscriptionInfoHelper;

/* SPRD: function FDN support. @{ */
import android.app.AlertDialog;
import android.content.Context;
import android.os.SystemProperties;
import android.telecom.TelecomManager;
import android.telecom.VideoProfile;
import android.telephony.PhoneNumberUtils;
import android.telephony.PhoneStateListener;
import android.telephony.ServiceState;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.telephony.VoLteServiceState;
import android.text.TextUtils;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.ViewGroup;
import android.view.View.OnClickListener;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

import com.android.callsettings.CallSettingsActivityContainer;
import com.android.callsettings.R;
import com.android.ims.ImsManager;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.PhoneFactory;
import com.android.internal.telephony.SubscriptionController;
import com.android.internal.telephony.uicc.IccConstants;
import com.android.callsettings.R;
/* }@ */

/**
 * Fixed Dialing Number (FDN) List UI for the Phone app. FDN is a feature of the service provider
 * that allows a user to specify a limited set of phone numbers that the SIM can dial.
 */
public class FdnList extends ADNList {
    private static final int MENU_ADD = 1;
    private static final int MENU_EDIT = 2;
    private static final int MENU_DELETE = 3;

    private static final String INTENT_EXTRA_NAME = "name";
    private static final String INTENT_EXTRA_NUMBER = "number";

    private static final Uri FDN_CONTENT_URI = Uri.parse("content://icc/fdn");
    private static final String FDN_CONTENT_PATH_WITH_SUB_ID = "content://icc/fdn/subId/";

    private SubscriptionInfoHelper mSubscriptionInfoHelper;

    /* SPRD: function FDN support. @{ */
    private int mSelectIndex = -1;
    private int mRecordLength = 0;
    private boolean mSupportVt = false;
    /* @} */
    /* SPRD: add for bug637586 @{ */
    private Context mContext;
    private static final String TAG = "FdnList";
    /* @} */
    // SPRD: add for bug645817
    private CallSettingsActivityContainer mActivityContainer;

    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);

        ActionBar actionBar = getActionBar();
        if (actionBar != null) {
            // android.R.id.home will be triggered in onOptionsItemSelected()
            actionBar.setDisplayHomeAsUpEnabled(true);
        }
        // SPRD: add for bug637586
        mContext = this;
        mSubscriptionInfoHelper = new SubscriptionInfoHelper(this, getIntent());
        mSubscriptionInfoHelper.setActionBarTitle(
                getActionBar(), getResources(), R.string.fdn_list_with_label);
        /* SPRD: function FDN support. @{ */
        querySingleRecord();
        /* }@ */
        /* SPRD: add for bug645817 @{ */
        int phoneId = mSubscriptionInfoHelper.getPhone().getPhoneId();
        mActivityContainer = CallSettingsActivityContainer.getInstance();
        mActivityContainer.setApplication(getApplication());
        mActivityContainer.addActivity(this, phoneId);
        /* @} */
    }

    @Override
    protected Uri resolveIntent() {
        Intent intent = getIntent();
        intent.setData(getContentUri(mSubscriptionInfoHelper));
        return intent.getData();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        super.onCreateOptionsMenu(menu);

        Resources r = getResources();

        // Added the icons to the context menu
        menu.add(0, MENU_ADD, 0, r.getString(R.string.menu_add))
                .setIcon(android.R.drawable.ic_menu_add);
        menu.add(0, MENU_EDIT, 0, r.getString(R.string.menu_edit))
                .setIcon(android.R.drawable.ic_menu_edit);
        menu.add(0, MENU_DELETE, 0, r.getString(R.string.menu_delete))
                .setIcon(android.R.drawable.ic_menu_delete);
        return true;
    }

    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        super.onPrepareOptionsMenu(menu);
        boolean hasSelection = (getSelectedItemPosition() >= 0);

        menu.findItem(MENU_ADD).setVisible(true);
        menu.findItem(MENU_EDIT).setVisible(hasSelection);
        menu.findItem(MENU_DELETE).setVisible(hasSelection);

        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home:  // See ActionBar#setDisplayHomeAsUpEnabled()
                Intent intent = mSubscriptionInfoHelper.getIntent(FdnSetting.class);
                intent.setAction(Intent.ACTION_MAIN);
                intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
                startActivity(intent);
                finish();
                return true;

            case MENU_ADD:
                addContact();
                return true;

            case MENU_EDIT:
                editSelected();
                return true;

            case MENU_DELETE:
                deleteSelected();
                return true;
        }

        return super.onOptionsItemSelected(item);
    }

    @Override
    public void onListItemClick(ListView l, View v, int position, long id) {
        // TODO: is this what we really want?
        /* SPRD: function FDN support. @{
        ** ORIGIN
        editSelected(position);
        **/
        if (DBG) log("fdnlist get position :" + position);

        mSelectIndex = position;
        final String fdnName;
        final String fdnNumber;
        if (mCursor.moveToPosition(position)) {
            fdnName = mCursor.getString(NAME_COLUMN);
            fdnNumber = mCursor.getString(NUMBER_COLUMN);

            Resources r = getResources();

            AlertDialog.Builder builder = new AlertDialog.Builder(this);
            LayoutInflater inflater = (LayoutInflater)this.getSystemService(
                    Context.LAYOUT_INFLATER_SERVICE);
            View view = null;
            view = inflater.inflate(R.xml.fdn_list_click_dialog_ex,
                    (ViewGroup)v.findViewById(R.id.fdn_list_click_dialog));

            builder.setView(view);

            TextView callBySim = (TextView) view.findViewById(R.id.mo_call_by_sim);
            TextView editToContact = (TextView) view.findViewById(R.id.edit_fdn_to_save);
            TextView deleteContact = (TextView) view.findViewById(R.id.delete_fdn_to_save);

            /* SPRD: add for bug637586 @{ */
            TextView addToContact = (TextView) view.findViewById(R.id.add_fdn_to_save);
            ImageView addToContactImg = (ImageView) view.findViewById(R.id.add_fdn_img);
            boolean isTecelSupport = mContext.getResources().getBoolean(R.bool.add_fdn_to_save);
            Log.d(TAG, "isTecelSupport : " + isTecelSupport);
            if (isTecelSupport) {
                addToContact.setVisibility(View.VISIBLE);
                addToContactImg.setVisibility(View.VISIBLE);
            } else {
                addToContact.setVisibility(View.GONE);
                addToContactImg.setVisibility(View.GONE);
            }
            /* @} */
            boolean titleFlag = false;
            if (fdnNumber == null) {
                callBySim.setVisibility(View.GONE);
                titleFlag = true;
            }

            if (DBG) log("fdnlist get number :" + fdnNumber + ",  name: " + fdnName);

            if (!TextUtils.isEmpty(fdnName)) {
                builder.setTitle(fdnName);
            } else {
                if (titleFlag) {
                    // builder.setTitle(R.string.unknown);
                } else {
                    builder.setTitle(fdnNumber);
                }
            }

            final AlertDialog alertDialog = builder.create();

            String simStr;
            simStr = "SIM" + SubscriptionController.getInstance().getPhoneId(
                    mSubscriptionInfoHelper.getSubId());
            callBySim.setText(r.getString(R.string.call_fdn_number_by_sim, fdnNumber, simStr));
            /* SPRD: video call menu for FDN on volte. See bug #525564 @{*/
            callBySim.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View v) {
                    alertDialog.dismiss();
                    if (DBG) {
                        log("fdnlist get CallBySim: fdn Num= "
                                    + fdnNumber + ", fdn Name=" + fdnName);
                    }
                    startActivityForResult(createCallIntent(fdnNumber), 0);
                }
            });

            /* SPRD: add for bug637586 @{ */
            addToContact.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View v) {
                    alertDialog.dismiss();
                    if (DBG) {
                        log("fdnlist add to list");
                    }
                    addContact();
                }
            });
            /* @} */

            editToContact.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View v) {
                    alertDialog.dismiss();
                    if (DBG) {
                        log("fdnlist get addToContact : fdn Num= "
                                    + fdnNumber + ", fdn Name=" + fdnName);
                    }
                    editSelected(mSelectIndex);
                }
            });

            deleteContact.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View v) {
                    alertDialog.dismiss();
                    if (DBG) {
                        log("fdnlist get deleteContact :fdn Num= "
                                     + fdnNumber + ", fdn Name=" + fdnName);
                    }
                    deleteSelected(mSelectIndex);
                }
            });
            alertDialog.show();
        }
            /* }@ */
    }

    private void addContact() {
        //If there is no INTENT_EXTRA_NAME provided, EditFdnContactScreen treats it as an "add".
        /* SPRD: function FDN support & bug561614. @{ */
        if (mCursorCount == 0 && mRecordLength == 0) {
            String message = String.valueOf(getResources().getText(R.string.wait_message));
            Toast.makeText(this, message, Toast.LENGTH_LONG).show();
        } else if (mCursorCount >= mRecordLength) {
            String message = String.valueOf(getResources().getText(R.string.adn_full));
            Toast.makeText(this, message, Toast.LENGTH_LONG).show();
        } else {
        /* }@ */
        Intent intent = mSubscriptionInfoHelper.getIntent(EditFdnContactScreen.class);
        startActivity(intent);
        }
    }

    /**
     * Overloaded to call editSelected with the current selection
     * by default.  This method may have a problem with touch UI
     * since touch UI does not really have a concept of "selected"
     * items.
     */
    private void editSelected() {
        editSelected(getSelectedItemPosition());
    }

    /**
     * Edit the item at the selected position in the list.
     */
    private void editSelected(int position) {
        if (mCursor.moveToPosition(position)) {
            String name = mCursor.getString(NAME_COLUMN);
            String number = mCursor.getString(NUMBER_COLUMN);

            Intent intent = mSubscriptionInfoHelper.getIntent(EditFdnContactScreen.class);
            intent.putExtra(INTENT_EXTRA_NAME, name);
            intent.putExtra(INTENT_EXTRA_NUMBER, number);
            startActivity(intent);
        }
    }

    /* SPRD: function FDN support. @{ */
    private void deleteSelected() {
        deleteSelected(getSelectedItemPosition());
    }
    /* }@ */

    /* SPRD: function FDN support. @{
    ** ORIGIN
    private void deleteSelected() {
        if (mCursor.moveToPosition(getSelectedItemPosition())) {
    **/
    private void deleteSelected(int position) {
        if (mCursor.moveToPosition(position)) {
        /* }@ */
            String name = mCursor.getString(NAME_COLUMN);
            String number = mCursor.getString(NUMBER_COLUMN);

            Intent intent = mSubscriptionInfoHelper.getIntent(DeleteFdnContactScreen.class);
            intent.putExtra(INTENT_EXTRA_NAME, name);
            intent.putExtra(INTENT_EXTRA_NUMBER, number);
            startActivity(intent);
        }
    }

    /**
     * Returns the uri for updating the ICC FDN entry, taking into account the subscription id.
     */
    public static Uri getContentUri(SubscriptionInfoHelper subscriptionInfoHelper) {
        return subscriptionInfoHelper.hasSubId()
                ? Uri.parse(FDN_CONTENT_PATH_WITH_SUB_ID + subscriptionInfoHelper.getSubId())
                : FDN_CONTENT_URI;
    }

    /* SPRD: function FDN support. @{ */
    private Intent createCallIntent(String mNumber){
        Uri uri = null;
        if (PhoneNumberUtils.isUriNumber(mNumber)) {
            uri = Uri.fromParts("sip", mNumber, null);
        } else {
            uri = Uri.fromParts("tel", mNumber, null);
        }
        // SPRD: add for bug474411
        Intent intent = new Intent(Intent.ACTION_DIAL, uri);
        intent.putExtra(PhoneConstants.SUBSCRIPTION_KEY, mSubscriptionInfoHelper.getSubId());
        return intent;
    }

    public void querySingleRecord() {
        new Thread() {
            @Override
            public void run() {
                int phoneId = SubscriptionController.getInstance().getPhoneId(mSubscriptionInfoHelper.getSubId());
                Phone mPhone = PhoneFactory.getPhone(phoneId);
                try {
                    com.android.internal.telephony.IccPhoneBookInterfaceManager ipm = mPhone.getIccPhoneBookInterfaceManager();
                    int recordSizes[] = ipm.getAdnRecordsSize(IccConstants.EF_FDN);
                    mRecordLength = recordSizes[2];
                } catch (Exception e) {
                    Log.e("FdnList", "query record size error!" + e);
                }
            }
        }.start();
    }
    /* }@ */
    /* SPRD: add for bug645817 @{ */
    protected void onDestroy() {
        super.onDestroy();
        if (mActivityContainer != null) {
            mActivityContainer.removeActivity(this);
        }
    }
    /* @} */
}
