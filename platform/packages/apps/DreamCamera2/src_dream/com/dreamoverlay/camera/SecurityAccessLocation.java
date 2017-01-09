package com.android.camera;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.util.Log;
import android.os.Bundle;
import com.android.camera.CameraActivity;
import android.content.Intent;
import com.android.camera2.R;

public class SecurityAccessLocation extends Activity {
    private AlertDialog.Builder mLocationDialog;
    private Context mContext;
    private static int DAFAULT_LOCATION_OK = 1;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mContext = this;

        mLocationDialog = new AlertDialog.Builder(SecurityAccessLocation.this);
        mLocationDialog.setTitle(mContext.getResources().getText(R.string.warn_location_title)).setMessage(mContext.getResources().getText(R.string.warn_location_message))
                .setPositiveButton(android.R.string.ok,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog,
                                    int which) {
                                Intent intent = new Intent(SecurityAccessLocation.this, CameraActivity.class);
                                Bundle bundle = new Bundle();
                                bundle.putString("result", CameraActivity.DAFAULT_LOCATION_OK);
                                intent.putExtras(bundle);
                                SecurityAccessLocation.this.startActivity(intent);
                                SecurityAccessLocation.this.finish();
                            }
                        })
                .setNegativeButton(android.R.string.cancel,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog,
                                    int which) {
                                SecurityAccessLocation.this.finish();
                            }
                        });
        AlertDialog dialog = mLocationDialog.create();
        dialog.setCanceledOnTouchOutside(false);
        dialog.show();
    }
}
