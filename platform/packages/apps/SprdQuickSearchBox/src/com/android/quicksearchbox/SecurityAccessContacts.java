
package com.android.quicksearchbox;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.net.Uri;
import android.util.Log;

public class SecurityAccessContacts {
    public static String fileName = "preferences";
    public static String KEY = "toast";
    private static final String CONTACTS_SUGGEST_QUERY_URI = "content://com.android.contacts/search_suggest_query";

    private static SharedPreferences mSharedPreferences;
    private SharedPreferences.Editor mEditor;
    private AlertDialog.Builder mContactsDialog;
    private Context mContext;

    public SecurityAccessContacts(Context c) {
        mContext = c;
        mSharedPreferences = mContext.getSharedPreferences(fileName, Activity.MODE_PRIVATE);
        mEditor = mSharedPreferences.edit();
        mEditor.putBoolean(KEY, false);
        mEditor.commit();
    }

    public static boolean querySuggetsUri(Uri uri) {
        boolean isToast = mSharedPreferences.getBoolean(KEY, false);
        if (uri.toString().equals(CONTACTS_SUGGEST_QUERY_URI) && !isToast) {
            return false;
        } else {
            return true;
        }
    }

    public void updateContactsCorpus() {
        mContactsDialog = new AlertDialog.Builder(mContext);
        mContactsDialog
                .setTitle(mContext.getResources().getText(R.string.check_contacts_dialog_title))
                .setMessage(mContext.getResources().getText(R.string.check_contacts_dialog_message))
                .setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        if (mSharedPreferences != null) {
                            mEditor.putBoolean(KEY, true);
                            mEditor.commit();
                        }
                    }
                })
                .setNegativeButton(android.R.string.cancel, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        mEditor.putBoolean(KEY, false);
                        mEditor.commit();
                    }
                });
        AlertDialog dialog = mContactsDialog.create();
        dialog.setCanceledOnTouchOutside(false);
        dialog.show();
    }
}
