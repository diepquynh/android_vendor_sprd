/** Created by Spreadst */

package com.sprd.gallery3d.app;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;

import com.android.gallery3d.util.GalleryUtils;

public class PickPhotosPermissionsActivity extends PermissionsActivity {

    private static final String TAG = "PickPhotosPermissionsActivity";
    private static Activity sLastActivity;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        Log.d(TAG, "onCreate, GalleryActivity start requset permissions");
        super.onCreate(savedInstanceState);
        /* SPRD: bug 636918 Gallery no permisson, pick photo exception @{*/
        if (GalleryUtils.isMonkey()) {
            if (sLastActivity != null) {
                Log.e(TAG, "PickPhotosPermissionsActivity in monkey test -> last activity is not finished! ");
                sLastActivity.finish();
                sLastActivity = null;
            }
            sLastActivity = this;
        }
        /* @} */
    }

    @Override
    protected void onDestroy() {
        Log.d(TAG, "destroy  PickPhotosPermissionsActivity");
        super.onDestroy();
    }
}
