/*
 *Created by spreadst
 */

package android.location;

import android.content.Context;
import android.content.res.Resources;
import android.content.res.Resources.NotFoundException;
import android.util.Log;

public class LocationManagerEx {

    private static final String TAG = LocationManagerEx.class.getSimpleName();

    private Context mContext;
    private String mOperatorName = null;

    public LocationManagerEx(Context context) {
        mContext = context;
        mOperatorName = getSupportOperator();
        Log.d(TAG, "mOperatorName : " + mOperatorName);
    }

    /** @hide */
    public boolean isSupportCmcc() {
        return "cmcc".equals(mOperatorName);
    }

    private String getSupportOperator() {
        String operator = null;
        try {
            operator = mContext.getResources().getString(com.android.internal.R.string.config_location_support_operator);
        } catch (NotFoundException e) {
            Log.d(TAG, "config_location_support_operator cannot be found.");
        }
        return operator;
    }
}
