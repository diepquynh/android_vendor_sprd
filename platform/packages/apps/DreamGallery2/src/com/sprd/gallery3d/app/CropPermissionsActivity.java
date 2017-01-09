/** Created by Spreadst */

package com.sprd.gallery3d.app;

import android.os.Bundle;
import android.util.Log;

public class CropPermissionsActivity extends PermissionsActivity {

    private static final String TAG = "CropPermissionsActivity";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        Log.d(TAG, "onCreate, CropActivity start requset permissions");
        super.onCreate(savedInstanceState);
    }

    @Override
    protected void onDestroy() {
        Log.d(TAG, "destroy  CropPermissionsActivity");
        super.onDestroy();
    }
}
