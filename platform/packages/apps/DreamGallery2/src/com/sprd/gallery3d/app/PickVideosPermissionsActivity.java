/** Created by Spreadst */

package com.sprd.gallery3d.app;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;

import com.android.gallery3d.util.GalleryUtils;

public class PickVideosPermissionsActivity extends PermissionsActivity {

    private static final String TAG = "PickVideosPermissionsActivity";
    private static Activity sLastActivity;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        Log.d(TAG, "onCreate, VideoActivity start requset permissions");
        super.onCreate(savedInstanceState);
        /* SPRD: bug 641014 Gallery no permisson, pick video exception @{*/
        if (GalleryUtils.isMonkey()) {
            if (sLastActivity != null) {
                Log.e(TAG, "PickVideosPermissionsActivity in monkey test -> last activity is not finished! ");
                sLastActivity.finish();
                sLastActivity = null;
            }
            sLastActivity = this;
        }
        /* @} */
    }

    @Override
    protected void onDestroy() {
        Log.d(TAG, "destroy  PickVideosPermissionsActivity");
        super.onDestroy();
    }
}
