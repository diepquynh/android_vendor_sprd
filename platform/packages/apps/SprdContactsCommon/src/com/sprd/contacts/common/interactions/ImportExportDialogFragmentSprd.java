package com.sprd.contacts.common.interactions;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.FragmentManager;
import android.content.Context;
import android.content.DialogInterface;
import android.content.res.Resources;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.TextView;

import com.android.contacts.common.R;
import com.android.contacts.common.model.AccountTypeManager;
import com.android.contacts.common.model.account.AccountWithDataSet;
import com.android.contacts.common.vcard.VCardCommonArguments;
import com.android.contacts.common.interactions.ImportExportDialogFragment;
import android.content.*;
import android.provider.ContactsContract.*;
import com.android.contacts.common.util.Constants;
import com.android.contacts.commonbind.analytics.AnalyticsUtil;
import android.graphics.Paint;


public class ImportExportDialogFragmentSprd extends ImportExportDialogFragment{

    public static final String TAG = "ImportExportDialogFragmentSprd";

    private String mPhoneAccountName = "";
    private static final String KEY_RES_ID = "resourceId";
    private static final String ARG_CONTACTS_ARE_AVAILABLE = "CONTACTS_ARE_AVAILABLE";

    private final String[] LOOKUP_PROJECTION = new String[] {
            Contacts.LOOKUP_KEY
    };

    private static final int MODE_IMPORT_FROM_SIM = 0;
    private static final int MODE_EXPORT_TO_SIM = 1;

    public interface Listener {
        void doCopy();
        void doShareVisible();
        void doExport();
        void doPreImport(int resId, int subscriptionId);

    }

    public ImportExportDialogFragmentSprd() {
    }

    /** Preferred way to show this dialog */
    public static void show(FragmentManager fragmentManager, boolean contactsAreAvailable,
            Class callingActivity) {
        final ImportExportDialogFragmentSprd fragment = new ImportExportDialogFragmentSprd();
        Bundle args = new Bundle();
        args.putBoolean(ARG_CONTACTS_ARE_AVAILABLE, contactsAreAvailable);
        args.putString(VCardCommonArguments.ARG_CALLING_ACTIVITY, callingActivity.getName());
        fragment.setArguments(args);
        fragment.show(fragmentManager, ImportExportDialogFragmentSprd.TAG);
    }

    private static class AdapterEntry {
        public final CharSequence mLabel;
        public final int mChoiceResourceId;
        public final int mSubscriptionId;

        public AdapterEntry(CharSequence label, int resId, int subId) {
            mLabel = label;
            mChoiceResourceId = resId;
            mSubscriptionId = subId;
        }

        public AdapterEntry(String label, int resId) {
            // Store a nonsense value for mSubscriptionId. If this constructor is used,
            // the mSubscriptionId value should not be read later.
            this(label, resId, /* subId = */ -1);
        }
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        // Wrap our context to inflate list items using the correct theme
        final Resources res = getActivity().getResources();
        final LayoutInflater dialogInflater = (LayoutInflater)getActivity()
                .getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        final boolean contactsAreAvailable = getArguments().getBoolean(ARG_CONTACTS_ARE_AVAILABLE);
        final String callingActivity = getArguments().getString(
                VCardCommonArguments.ARG_CALLING_ACTIVITY);

        // Adapter that shows a list of string resources
        final ArrayAdapter<AdapterEntry> adapter = new ArrayAdapter<AdapterEntry>(getActivity(),
                R.layout.select_dialog_item) {
            @Override
            public View getView(int position, View convertView, ViewGroup parent) {
                final TextView result = (TextView)(convertView != null ? convertView :
                        dialogInflater.inflate(R.layout.select_dialog_item, parent, false));

                result.setText(getItem(position).mLabel);
                return result;
            }
        };
        if (AccountTypeManager.getInstance(getActivity()).getAccounts(true).size() > 1) {
            adapter.add(new AdapterEntry(getString(R.string.copy_to),
                    R.string.copy_to));
        }

        if (res.getBoolean(R.bool.config_allow_import_from_vcf_file)) {
            adapter.add(new AdapterEntry(getString(R.string.import_from_vcf_file),
                    R.string.import_from_vcf_file));
        }

        if (res.getBoolean(R.bool.config_allow_export)) {
            if (contactsAreAvailable) {
                adapter.add(new AdapterEntry(getString(R.string.export_to_vcf_file),
                        R.string.export_to_vcf_file));
            }
        }
        if (res.getBoolean(R.bool.config_allow_share_visible_contacts)) {
            if (contactsAreAvailable) {
                adapter.add(new AdapterEntry(getString(R.string.share_visible_contacts),
                        R.string.share_visible_contacts));
            }
        }

        final DialogInterface.OnClickListener clickListener =
                new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                boolean dismissDialog;
                final int resId = adapter.getItem(which).mChoiceResourceId;
                switch (resId) {
                    case R.string.copy_to:
                        dismissDialog = handleCopyRequest();
                        break;

                    case R.string.import_from_vcf_file: {
                        dismissDialog = handleImportRequest(resId,
                                adapter.getItem(which).mSubscriptionId);
                        break;
                    }
                    case R.string.export_to_vcf_file: {
                        dismissDialog = handleExport();
                        break;
                    }
                    case R.string.share_visible_contacts: {
                        dismissDialog = handleShareVisible();
                        break;
                    }
                    default: {
                        dismissDialog = true;
                        Log.e(TAG, "Unexpected resource: "
                                + getActivity().getResources().getResourceEntryName(resId));
                    }
                }
                if (dismissDialog) {
                    dialog.dismiss();
                }
            }
        };
        /**
         * SPRD: androidN porting Bug 592059 The DUT shows unfriendly alertDialog
         * while importing and exporting contacts.
         * original code:
        return new AlertDialog.Builder(getActivity())
                .setTitle(contactsAreAvailable
                        ? R.string.dialog_import_export
                        : R.string.dialog_import)
                .setSingleChoiceItems(adapter, -1, clickListener)
                .create();
         * @{
         */
        View titleView = dialogInflater.inflate(R.layout.select_dialog_title, null);
        TextView titleText = (TextView) titleView.findViewById(R.id.import_and_export_dialog_title);
        titleText.setText(contactsAreAvailable
                ? R.string.dialog_import_export
                : R.string.dialog_import);
        return new AlertDialog.Builder(getActivity())
                .setCustomTitle(titleView)
                .setSingleChoiceItems(adapter, -1, clickListener)
                .create();
        /*
         * @}
         */
    }

    private boolean handleCopyRequest() {
        if (getActivity() != null) {
            ((Listener) getActivity()).doCopy();
            return true;
        } else {
            return false;
        }
    }

    private boolean handleExport() {
        if (getActivity() != null) {
            ((Listener) getActivity()).doExport();
            return true;
        } else {
            return false;
        }
    }

    private boolean handleShareVisible() {
        if (getActivity() != null) {
            ((Listener) getActivity()).doShareVisible();
            return true;
        } else {
            return false;
        }
    }

    private boolean handleImportRequest(int resId, int subscriptionId) {
        if (getActivity() != null) {
            ((Listener) getActivity()).doPreImport(resId, subscriptionId);
            return true;
        } else {
            return false;
        }
    }

    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);
        AnalyticsUtil.sendScreenView(this);
    }




}
