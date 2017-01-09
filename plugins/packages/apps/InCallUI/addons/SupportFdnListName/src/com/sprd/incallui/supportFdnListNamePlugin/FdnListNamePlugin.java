package com.sprd.incallui.supportFdnListNamePlugin;

import android.app.AddonManager;
import android.content.Context;
import android.telephony.TelephonyManager;

import android.database.Cursor;
import android.net.Uri;
import android.os.AsyncTask;

import android.text.TextUtils;
import android.util.Log;

import com.android.incallui.R;
import com.android.incallui.CallCardFragment;
import com.sprd.incallui.FdnListNameHelper;


public class FdnListNamePlugin extends FdnListNameHelper implements AddonManager.InitialCallback {

    private static final String TAG = "FdnListNamePlugin";
    private Context mContext;

    private static final int INVALID_SUBSCRIPTION_ID = -1;
    private static final String FDN_CONTENT_URI = "content://icc/fdn/subId/";
    private static final String[] FDN_SELECT_PROJECTION = new String[]{
            "name", "number"
    };
    private static final int FDN_NAME_COLUMN = 0;
    private static final int FDN_NUMBER_COLUMN = 1;

    private int mSubId = -1;
    private String mFdnNumber;

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mContext = context;
        return clazz;
    }

    public void FdnListNamePlugin() {

    }

    @Override
    public boolean isSupportFdnListName(int subId) {
        TelephonyManager telephonyManager = (TelephonyManager) mContext.
                getSystemService(Context.TELEPHONY_SERVICE);
        return telephonyManager.getIccFdnEnabled(subId);
    }

    @Override
    public void setFDNListName(String number, boolean nameIsNumber, String name,
                               String label, CallCardFragment cardFragment, int subId) {
        if (nameIsNumber) {
            mFdnNumber = name;
        } else {
            mFdnNumber = number;
        }

        if (!TextUtils.isEmpty(mFdnNumber) && subId > INVALID_SUBSCRIPTION_ID) {
            GetFDNListNameAsyncTask getFdnListNameTask = new GetFDNListNameAsyncTask(number,
                    nameIsNumber, name, label, cardFragment, subId);
            getFdnListNameTask.execute();
        } else if (cardFragment != null) {
            cardFragment.setPrimaryName(name, nameIsNumber);
            cardFragment.setCallNumberAndLabel(number, label);
        }
    }

    public String getFDNListName(String number) {
        String fdnListName = null;
        String compareNumber;
        String formatNumber = number.replace(" ", "");

        Cursor cursor = mContext.getContentResolver().query(Uri.parse(FDN_CONTENT_URI + mSubId),
                FDN_SELECT_PROJECTION, null, null, null);
        try {
            while (cursor != null && cursor.moveToNext()) {
                compareNumber = cursor.getString(FDN_NUMBER_COLUMN);

                if (compareNumber != null && compareNumber.equals(formatNumber)) {
                    fdnListName = cursor.getString(FDN_NAME_COLUMN);
                    break;
                }
            }
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
        return fdnListName;
    }

    private class GetFDNListNameAsyncTask extends AsyncTask<String, Void, String> {
        private String number;
        private boolean nameIsNumber;
        private String name;
        private String label;
        private CallCardFragment cardFragment;

        public GetFDNListNameAsyncTask(String number, boolean nameIsNumber, String name,
                                       String label, CallCardFragment cardFragment, int subId) {
            this.number = number;
            this.nameIsNumber = nameIsNumber;
            this.name = name;
            this.label = label;
            this.cardFragment = cardFragment;
            mSubId = subId;
        }

        protected String doInBackground(String[] params) {
            return getFDNListName(mFdnNumber);
        };

        protected void onPostExecute(String result) {
            Log.d(TAG, "onPostExecute.");
            if (cardFragment != null) {
                if (!TextUtils.isEmpty(result)) {
                    cardFragment.setPrimaryName(result, false);
                    cardFragment.setCallNumberAndLabel(result, label);
                } else {
                    cardFragment.setPrimaryName(name, nameIsNumber);
                    cardFragment.setCallNumberAndLabel(number, label);
                }
            } else {
                Log.d(TAG, "cardFragment is null when onPostExecute. ");
            }
        };
    }

}
