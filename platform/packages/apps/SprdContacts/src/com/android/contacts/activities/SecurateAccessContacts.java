package com.android.contacts.activities;

import com.android.contacts.R;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.util.Log;
import android.os.Bundle;
import android.content.Intent;

public class SecurateAccessContacts extends Activity {
    private AlertDialog.Builder mContactsDialog;
    private Context mContext;
    private static final String RESULT = "result";
    private static final String RESULT_PERMIT = "permited";
    @Override
    public void onCreate(Bundle savedInstanceState) {
       super.onCreate(savedInstanceState);
       mContext = this;
       createWarnDialog();
    }

    @Override
    protected void onDestroy() {
       super.onDestroy();
       if(PeopleActivity.instance != null){
          PeopleActivity.instance = null;
       }
    }

    public void createWarnDialog(){
        mContactsDialog = new AlertDialog.Builder(SecurateAccessContacts.this);
        mContactsDialog.setTitle(mContext.getResources().getText(R.string.warn_location_title))
                    .setMessage(mContext.getResources().getText(R.string.warn_location_message))
                        .setPositiveButton(android.R.string.ok,
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog,int which) {
                        Intent contactsIntent = new Intent(SecurateAccessContacts.this, PeopleActivity.class);
                        Bundle bundle = new Bundle();
                        bundle.putString(RESULT, RESULT_PERMIT);
                        contactsIntent.putExtras(bundle);
                        SecurateAccessContacts.this.startActivity(contactsIntent);
                        SecurateAccessContacts.this.finish();
                    }
               }).setNegativeButton(android.R.string.cancel,
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog,int which) {
                        SecurateAccessContacts.this.finish();
                        if(PeopleActivity.instance != null){
                           PeopleActivity.instance.finish();
                        }
                    }
               });
          AlertDialog dialog = mContactsDialog.create();
          dialog.setCanceledOnTouchOutside(false);
          dialog.show();
    }
}