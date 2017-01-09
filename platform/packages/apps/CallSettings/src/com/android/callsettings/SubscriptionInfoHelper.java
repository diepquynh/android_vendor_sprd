package com.android.callsettings;

import android.app.ActionBar;
import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.text.TextUtils;

import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneFactory;

/**
 * Helper for manipulating intents or components with subscription-related information.
 *
 * In settings, subscription ids and labels are passed along to indicate that settings
 * are being changed for particular subscriptions. This helper provides functions for
 * helping extract this info and perform common operations using this info.
 */
public class SubscriptionInfoHelper {
    public static final int NO_SUB_ID = -1;

    // Extra on intent containing the id of a subscription.
    public static final String SUB_ID_EXTRA =
            "com.android.phone.settings.SubscriptionInfoHelper.SubscriptionId";
    // Extra on intent containing the label of a subscription.
    public static final String SUB_LABEL_EXTRA =
            "com.android.phone.settings.SubscriptionInfoHelper.SubscriptionLabel";

    private static Context mContext;

    private static int mSubId = NO_SUB_ID;
    private static String mSubLabel;

    /**
     * Instantiates the helper, by extracting the subscription id and label from the intent.
     */
    public SubscriptionInfoHelper(Context context, Intent intent) {
        mContext = context;
        mSubId = intent.getIntExtra(SUB_ID_EXTRA, NO_SUB_ID);
        mSubLabel = intent.getStringExtra(SUB_LABEL_EXTRA);
    }

    /**
     * @param newActivityClass The class of the activity for the intent to start.
     * @return Intent containing extras for the subscription id and label if they exist.
     */
    public Intent getIntent(Class newActivityClass) {
        Intent intent = new Intent(mContext, newActivityClass);

        if (hasSubId()) {
            intent.putExtra(SUB_ID_EXTRA, mSubId);
        }

        if (!TextUtils.isEmpty(mSubLabel)) {
            intent.putExtra(SUB_LABEL_EXTRA, mSubLabel);
        }

        return intent;
    }

    public Intent getIntent(String packageName, String className) {
        Intent intent = new Intent();

        if (hasSubId()) {
            intent.putExtra(SUB_ID_EXTRA, mSubId);
        }

        if (!TextUtils.isEmpty(mSubLabel)) {
            intent.putExtra(SUB_LABEL_EXTRA, mSubLabel);
        }
        intent.setClassName(packageName, className);

        return intent;
    }

    public static void addExtrasToIntent(Intent intent, SubscriptionInfo subscription) {
        if (subscription == null) {
            return;
        }

        intent.putExtra(SubscriptionInfoHelper.SUB_ID_EXTRA, subscription.getSubscriptionId());
        intent.putExtra(
                SubscriptionInfoHelper.SUB_LABEL_EXTRA, subscription.getDisplayName().toString());
    }

    /**
     * @return Phone object. If a subscription id exists, it returns the phone for the id.
     */
    public Phone getPhone() {
        return hasSubId()
                ? PhoneFactory.getPhone(SubscriptionManager.getPhoneId(mSubId))
                : PhoneFactory.getDefaultPhone();
    }

    /**
     * Sets the action bar title to the string specified by the given resource id, formatting
     * it with the subscription label. This assumes the resource string is formattable with a
     * string-type specifier.
     *
     * If the subscription label does not exists, leave the existing title.
     */
    public void setActionBarTitle(ActionBar actionBar, Resources res, int resId) {
        if (actionBar == null || TextUtils.isEmpty(mSubLabel)) {
            return;
        }

        if (!TelephonyManager.from(mContext).isMultiSimEnabled()) {
            return;
        }

        String title = String.format(res.getString(resId), mSubLabel);
        actionBar.setTitle(title);
    }

    public boolean hasSubId() {
        return mSubId != NO_SUB_ID;
    }

    public int getSubId() {
        return mSubId;
    }
}
