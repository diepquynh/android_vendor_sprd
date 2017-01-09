package com.sprd.incallui.inCallUITelcelPlugin;

import java.io.IOException;
import java.util.HashMap;
import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;
import android.app.AddonManager;
import android.app.AlertDialog;
import android.content.Context;
import android.content.res.Resources;
import android.content.res.Resources.NotFoundException;
import android.database.Cursor;

import android.net.Uri;
import android.os.AsyncTask;
import android.telephony.TelephonyManagerEx;
import android.text.TextUtils;
import android.util.Log;
import android.widget.Toast;

import com.android.incallui.CallCardFragment;
import android.text.TextUtils;
import android.util.Log;
import android.widget.Toast;
import com.android.internal.util.XmlUtils;
import com.android.sprd.incallui.InCallUITelcelHelper;
import com.android.incallui.CallList;
import com.android.sprd.telephony.RadioInteractor;
import com.android.sprd.incallui.InCallUiUtils;


public class InCallUITelcelPlugin extends InCallUITelcelHelper implements
        AddonManager.InitialCallback {

    private static final String TAG = "InCallUITelcelPlugin";
    private Context mContext;

    /* SPRD: Voice Clear Code @{ */
    private static final String FIRST_ELEMELT_NAME = "resources";
    private static final String CAUSE_NUMBER = "number";
    private static final String TEXT_REQUIRED = "text_required";
    private final String SPECIAL_VOICE_CLEAR_CODE = "*00015,*00008";
    private HashMap<Integer, String> mEsTextRequiredHashmap = new HashMap<Integer, String>();
    private HashMap<Integer, String> mEnTextRequiredHashmap = new HashMap<Integer, String>();
    /* @} */

    /* SPRD: FDN in dialer feature. @{ */
    private static final int INVALID_SUBSCRIPTION_ID = -1;
    private static final String FDN_CONTENT_URI = "content://icc/fdn/subId/";
    private static final String[] FDN_SELECT_PROJECTION = new String[]{
            "name", "number"
    };
    private static final int FDN_NAME_COLUMN = 0;
    private static final int FDN_NUMBER_COLUMN = 1;
    private int mSubId = -1;
    private String mFdnNumber;
    /* @} */

    /* SPRD: Incoming Third Call @{ */
    private AlertDialog mHangupcallDialog = null;
    /**hang up foreground call and accept ringing/waiting call.
    */
    public static final int MPC_MODE_HF = 1;
    RadioInteractor mRadioInteractor;
    /* @} */

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mContext = context;
        mRadioInteractor = new RadioInteractor(context);
        return clazz;
    }

    public void InCallUITelcelPlugin() {

    }

    /* SPRD: Voice Clear Code @{ */
    @Override
    public void showToastMessage(Context context, String reason) {
        Log.i(TAG, "showToastMessage: reason (" + reason +")");
        if (!TextUtils.isEmpty(reason) && reason.contains(",")) {
            reason = reason.substring(0, reason.indexOf(","));
        }

        parserAttributeValue();
        try {
            String callFailCause = getAttributeValue(Integer.parseInt(reason));
            if (!TextUtils.isEmpty(callFailCause)) {
                Toast.makeText(context, callFailCause, Toast.LENGTH_LONG).show();
            }
        } catch (NumberFormatException e) {
            Log.e(TAG, "NumberFormatException when parse vendorCause.");
        }
    }

    /**
     * Support English and Spanish for CC code.
     */
    public void parserAttributeValue() {
        Log.i(TAG, "start  parserAttributeValue");

        Resources r = mContext.getResources();
        XmlPullParser parser = r.getXml(R.xml.networkvalue_config);
        XmlPullParser enParser = r.getXml(R.xml.networkvalue_config_en);
        parseXml(parser, mEsTextRequiredHashmap);
        parseXml(enParser, mEnTextRequiredHashmap);
    }

    public  String getAttributeValue(Integer causeNumber) {
        Log.i(TAG, "getAttributeValue  causeNumber: " + causeNumber);
        String strRequired = null;
        String language = mContext.getResources().getConfiguration().locale
                .getLanguage();
        if ("es".equals(language)) {
            if (mEsTextRequiredHashmap.containsKey(causeNumber)) {
                strRequired = mEsTextRequiredHashmap.get(causeNumber);
            }
        } else {
            if (mEnTextRequiredHashmap.containsKey(causeNumber)) {
                strRequired = mEnTextRequiredHashmap.get(causeNumber);
            }
        }
        return strRequired;
    }

    private void parseXml(XmlPullParser parser,
                          HashMap<Integer, String> hashmap) {
        if (parser != null && hashmap.isEmpty() ) {
            try {
                XmlUtils.beginDocument(parser, FIRST_ELEMELT_NAME);
                XmlUtils.nextElement(parser);
                while (parser.getEventType() != XmlPullParser.END_DOCUMENT) {
                    int causeNumber = Integer.parseInt(parser.getAttributeValue(null, CAUSE_NUMBER));
                    String textRequired = parser.getAttributeValue(null, TEXT_REQUIRED);
                    hashmap .put(causeNumber, textRequired);
                    XmlUtils.nextElement(parser);
                }
            } catch (XmlPullParserException e) {
                Log.e(TAG, "XmlPullParserException : " + e);
            } catch (IOException e) {
                Log.e(TAG, "IOException: " + e);
            }
        }
    }

    @Override
    public boolean isVoiceClearCodeLabel(String callStateLabel) {
        String[] clearCodeStrings = super.getClearCodeStrings();
        if (clearCodeStrings == null || clearCodeStrings.length == 0) {
            return false;
        }
        for (String s : clearCodeStrings) {
            if (s.equals(callStateLabel)) {
                return true;
            }
            return false;
        }
        return false;
    }

    @Override
    public boolean isSpecialVoiceClearCode(String number) {
        if (!TextUtils.isEmpty(SPECIAL_VOICE_CLEAR_CODE)) {
            for (String code : SPECIAL_VOICE_CLEAR_CODE.split(",")) {
                if (code != null && code.equals(number)) {
                    return true;
                }
            }
            return false;
        }
        return false;
    }
    /* @} */

    /* SPRD: FDN in dialer feature. @{ */
    @Override
    public boolean isSupportFdnListName(int subId) {
        TelephonyManagerEx telephonyManager = TelephonyManagerEx.from(mContext);
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
    /* @} */

    /* SPRD: Incoming Third Call @{ */
    @Override
    public void dealIncomingThirdCall(Context context, boolean show) {
        if (show) {
            if ((CallList.getInstance().getActiveCall() != null)
                    && (CallList.getInstance().getBackgroundCall() != null)
                    && mHangupcallDialog == null) {
                showHangupCallDialog(context);
            }
        } else {
            dismissHangupCallDialog();
        }
    }

    private void showHangupCallDialog(Context context) {

        String note_title = mContext.getString(R.string.hangupcall_note_title);
        String note_message = mContext
                .getString(R.string.hangupcall_note_message);
        mHangupcallDialog = new AlertDialog.Builder(context)
                .setTitle(note_title).setMessage(note_message)
                .setPositiveButton(com.android.internal.R.string.ok, null)
                .setCancelable(false).create();
        mHangupcallDialog.show();
    }

    @Override
    public void dismissHangupCallDialog() {
        if (mHangupcallDialog != null) {
            mHangupcallDialog.dismiss();
            mHangupcallDialog = null;
        }
    }

    @Override
    public boolean isSupportIncomingThirdCall() {
        return true;
    }

    @Override
    public void answerMultiCalls(Context context) {
        mRadioInteractor.switchMultiCalls(InCallUiUtils.getCurrentPhoneId(context),
                            MPC_MODE_HF);
    }
    /* @} */

    @Override
    public boolean isShowTelcelIndicator() {
        return true;
    }
}
