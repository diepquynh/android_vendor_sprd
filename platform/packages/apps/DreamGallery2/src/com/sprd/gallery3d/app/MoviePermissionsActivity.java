/** Created by Spreadst */

package com.sprd.gallery3d.app;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;

import com.android.gallery3d.util.GalleryUtils;

public class MoviePermissionsActivity extends PermissionsActivity {

    private static final String TAG = "MoviePermissionsActivity";
    private static Activity sLastActivity;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        Log.d(TAG, "onCreate, MovieActivity start requset permissions");
        super.onCreate(savedInstanceState);
        /* SPRD: bug612471,avoid ANR in monkey test cause by create too many MoviePermissionsActivity @{ */
        if (GalleryUtils.isMonkey()) {
            if (sLastActivity != null) {
                Log.e(TAG, "MoviePermissionsActivity in monkey test -> last activity is not finished! ");
                sLastActivity.finish();
                sLastActivity = null;
            }
            sLastActivity = this;
        }
        /* @} */
    }

    @Override
    protected  int getToastFlag(){
        return GalleryUtils.DONT_SUPPORT_VIEW_VIDEO;
    }

    @Override
    protected void onDestroy() {
        Log.d(TAG, "destroy  MoviePermissionsActivity");
        super.onDestroy();
    }
}
