
package com.sprd.voicetrigger;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.content.DialogInterface;
import android.content.DialogInterface.OnCancelListener;
import android.content.DialogInterface.OnClickListener;
import android.os.Bundle;
import android.view.Window;

public class HeadsetActivity extends Activity {
    private static final String TAG = "HeadsetActivity";

    @Override
    public void onCreate(Bundle bundle) {
        super.onCreate(bundle);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        dialog();
    }


    protected void dialog() {
        AlertDialog.Builder builder = new Builder(this);
        builder.setMessage(getString(R.string.voicetrigger_connect_headset_content));
        builder.setTitle(R.string.audio_voicetrigger_connect_headset);
        builder.setPositiveButton(android.R.string.ok, new OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                dialog.dismiss();
                finish();
            }
        });
        // other cancel way ,such as back button ,click other area of dialog;
        builder.setOnCancelListener(new OnCancelListener() {
            @Override
            public void onCancel(DialogInterface dialog) {
                finish();
            }
        });
        builder.create().show();
    }
}
