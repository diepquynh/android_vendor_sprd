package plugin.sprd.telephonyOrangePlugin;

import android.app.AddonManager;
import android.app.AlertDialog;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.graphics.drawable.Drawable;
import android.location.Country;
import android.location.CountryDetector;
import android.location.CountryListener;
import android.net.Uri;
import android.os.Message;
import android.provider.Browser;
import android.provider.BrowserContract;
import android.provider.ContactsContract;
import android.provider.ContactsContract.Contacts;
import android.provider.Telephony.Mms;
import android.telephony.PhoneNumberUtils;
import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.style.ClickableSpan;
import android.text.style.URLSpan;
import android.text.util.Linkify;
import android.text.TextUtils;
import android.util.Log;
import android.widget.ArrayAdapter;
import android.widget.TextView;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.graphics.Color;

import com.android.internal.telephony.MmiCode;
import com.android.internal.telephony.Phone;
import com.android.phone.PhoneGlobalsEx;
import com.android.phone.TelephonyOrangeHelper;

import java.util.ArrayList;

public class TelephonyOrangePlugin extends TelephonyOrangeHelper implements
        AddonManager.InitialCallback {

    private static final String LOGTAG = "TelephonyOrangePlugin";
    private Context mAddonContext;

    public TelephonyOrangePlugin() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    public void setSpan(Context context, TextView tv) {
        Log.d(LOGTAG, "setSpan");
        if (tv == null || tv.getText() == null || tv.getText().length() <= 1) {
            return;
        }
        tv.setLinksClickable(true);
        Linkify.addLinks(tv, Linkify.EMAIL_ADDRESSES | Linkify.MAP_ADDRESSES
                | Linkify.WEB_URLS | Linkify.PHONE_NUMBERS);
        CharSequence text = tv.getText();
        Log.d(LOGTAG, "setSpan text = " + text);
        if (text instanceof Spannable) {
            Log.d(LOGTAG, "text instanceof Spannable");
            SpannableStringBuilder temp = new SpannableStringBuilder(text);
            Spannable sp = (Spannable) text;
            URLSpan[] oldSpans = sp.getSpans(0, sp.length(), URLSpan.class);

            for (int i = oldSpans.length - 1; i >= 0; i--) {
                Log.d(LOGTAG, "removeSpan[" + i + "]");
                sp.removeSpan(oldSpans[i]);
            }

            for (URLSpan oldSpan : oldSpans) {
                Log.d(LOGTAG, "setSpan oldSpan = " + oldSpan);
                MyURLSpan myspan = new MyURLSpan(context, oldSpan.getURL());
                sp.setSpan(myspan, temp.getSpanStart(oldSpan),
                        temp.getSpanEnd(oldSpan),
                        Spannable.SPAN_EXCLUSIVE_INCLUSIVE);
            }
        }
    }

    public boolean isSupportUSSDCall() {
        return true;
    }

    private static class MyURLSpan extends ClickableSpan {
        private Context mContext;
        private String mUrl;
        private ArrayList<String> mUrlList;
        private String addContact = "";
        private static final String TAG_TEL = "tel:";
        private static final String TAG_VT_TEL = "vt_tel:";
        private static final String TAG_SMS = "smsto:";
        private static final String TAG_BROWSER = "http://";
        private static final String TAG_BROWSER_HTTPS = "https://";
        private static final String TAG_BOOKMARK = "bookmark:";
        private static final String TAG_MAIL = "mailto:";
        private static final String TAG_STREAMING = "rtsp://";
        private static final String TAG = "UssdMyURLSpan";
        private CountryDetector mCountryDetector;
        private CountryListener mCountryListener;
        private String mCountryIso;

        public MyURLSpan(Context context, String url) {
            mContext = context;
            mUrl = url;
            mCountryDetector = (CountryDetector) mContext
                    .getSystemService(Context.COUNTRY_DETECTOR);
            mCountryListener = new CountryListener() {
                @Override
                public synchronized void onCountryDetected(Country country) {
                    mCountryIso = country.getCountryIso();
                }
            };
        }

        // This function CAN return null.
        public String getCurrentCountryIso() {
            if (mCountryIso == null) {
                Country country = mCountryDetector.detectCountry();
                if (country != null) {
                    mCountryIso = country.getCountryIso();
                }
            }
            return mCountryIso;
        }

        @Override
        public void onClick(View widget) {
            Log.d(LOGTAG, "onClick");
            mUrlList = new ArrayList<String>();
            mUrlList.add(mUrl);
            addContact = mContext.getString(R.string.menu_add_to_contacts);
            if (mUrl.indexOf(TAG_TEL) >= 0) {
                mUrlList.add(TAG_SMS + mUrl.substring(TAG_TEL.length()));
                mUrlList.add(addContact);
            } else if (mUrl.indexOf(TAG_BROWSER) >= 0
                    || mUrl.indexOf(TAG_BROWSER_HTTPS) >= 0) {
                mUrlList.add(TAG_BOOKMARK + mUrl);
                try {
                    mContext.getPackageManager().getActivityIcon(
                            new Intent(Intent.ACTION_VIEW, Uri.parse(mUrl)));
                } catch (android.content.pm.PackageManager.NameNotFoundException ex) {
                    mUrlList.clear();
                }
                mUrlList.add(addContact);
            } else if (mUrl.indexOf(TAG_MAIL) >= 0) {
                try {
                    mContext.getPackageManager().getActivityIcon(
                            new Intent(Intent.ACTION_VIEW, Uri.parse(mUrl)));
                } catch (android.content.pm.PackageManager.NameNotFoundException ex) {
                    mUrlList.clear();
                }
                mUrlList.add(addContact);
            }
            ArrayAdapter<String> adapter = new ArrayAdapter<String>(mContext,
                    android.R.layout.select_dialog_item, mUrlList) {
                @Override
                public View getView(int position, View convertView,
                        ViewGroup parent) {
                    View v = super.getView(position, convertView, parent);
                    try {
                        String url = getItem(position).toString();
                        TextView tv = (TextView) v;
                        Drawable d;
                        if (url.startsWith(TAG_BOOKMARK)) {
                            d = mContext
                                    .getResources()
                                    .getDrawable(
                                            R.drawable.ic_launcher_shortcut_browser_bookmark);
                        } else if (getItem(position).toString().equals(
                                addContact)) {
                            d = mContext.getPackageManager()
                                    .getApplicationIcon("com.android.contacts");
                        } else if (url.startsWith(TAG_SMS)) {
                            d = mContext.getPackageManager()
                                    .getApplicationIcon("com.android.mms");
                        } else if (url.startsWith(TAG_TEL)) {
                            d = mContext.getPackageManager()
                                    .getApplicationIcon("com.android.dialer");
                        } else if (url.startsWith(TAG_TEL)) {
                            d = mContext.getPackageManager()
                                    .getApplicationIcon("com.android.dialer");
                        } else {
                            d = mContext.getPackageManager().getActivityIcon(
                                    new Intent(Intent.ACTION_VIEW, Uri
                                            .parse(url)));
                        }
                        if (d != null) {
                            d.setBounds(0, 0, d.getIntrinsicHeight(),
                                    d.getIntrinsicHeight());
                            tv.setCompoundDrawablePadding(10);
                            tv.setCompoundDrawables(d, null, null, null);
                        }

                        if (url.startsWith(TAG_TEL)) {
                            String number = PhoneNumberUtils.formatNumber(
                                    url.substring(TAG_TEL.length()),
                                    getCurrentCountryIso());
                            if (!TextUtils.isEmpty(number)) {
                                url = number;
                            } else {
                                url = url.substring(TAG_TEL.length());
                            }
                        } else if (url.startsWith(TAG_SMS)) {
                            String number = PhoneNumberUtils.formatNumber(
                                    url.substring(TAG_SMS.length()),
                                    getCurrentCountryIso());
                            if (!TextUtils.isEmpty(number)) {
                                url = number;
                            } else {
                                url = url.substring(TAG_SMS.length());
                            }
                        } else if (url.startsWith(TAG_BOOKMARK)) {
                            url = url.substring(TAG_BOOKMARK.length());
                        } else if (url.startsWith(TAG_MAIL)) {
                            url = url.substring(TAG_MAIL.length());
                        } else if (url.startsWith(TAG_STREAMING)) {
                            url = url.substring(TAG_STREAMING.length());
                        }

                        tv.setText(url);
                    } catch (android.content.pm.PackageManager.NameNotFoundException ex) {
                        Log.i("TAG", "NameNotFoundException");
                        TextView tv = (TextView) v;
                        tv.setCompoundDrawables(null, null, null, null);
                    }
                    return v;
                }
            };

            DialogInterface.OnClickListener click = new DialogInterface.OnClickListener() {
                @Override
                public final void onClick(DialogInterface dialog, int which) {
                    if (which >= 0) {
                        Uri uri = Uri.parse(mUrlList.get(which));
                        if (mUrlList.get(which).startsWith(TAG_BROWSER)
                                || mUrlList.get(which).startsWith(
                                        TAG_BROWSER_HTTPS)) {
                            confirmOpenBrowserDialog(uri);
                        } else if (mUrlList.get(which).startsWith(TAG_BOOKMARK)) {
                            String url = mUrlList.get(which).substring(
                                    TAG_BOOKMARK.length());
                            Intent intent = new Intent(Intent.ACTION_INSERT);
                            intent.setType("vnd.android.cursor.dir/bookmark");
                            intent.putExtra(BrowserContract.Bookmarks.URL, url);
                            intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_WHEN_TASK_RESET);
                            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                            try {
                                mContext.startActivity(intent);
                            } catch (ActivityNotFoundException e) {
                                Log.e(TAG,
                                        "No Activity found to handle Intent: "
                                                + intent);
                            }
                        } else if (mUrlList.get(which).equals(addContact)) {
                            if (mUrl == null) {
                                Log.e(TAG,
                                        "fatal:the url is null, we can do nothing!");
                                return;
                            }
                            boolean isWebsite = false;
                            boolean isMailAddr = false;
                            boolean isTel = false;
                            if (mUrl.startsWith(TAG_BROWSER)
                                    || mUrl.startsWith(TAG_BROWSER_HTTPS)) {
                                isWebsite = true;
                            } else if (mUrl.startsWith(TAG_MAIL)) {
                                isMailAddr = true;
                            } else if (mUrl.startsWith(TAG_TEL)) {
                                isTel = true;
                            }
                            Intent intent = new Intent(
                                    Intent.ACTION_INSERT_OR_EDIT);
                            if (isWebsite) {
                                intent.setType(Contacts.CONTENT_ITEM_TYPE);
                                intent.putExtra("website", mUrl);
                                intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_WHEN_TASK_RESET);
                            } else {
                                String tempUrl = mUrl;
                                if (isTel) {
                                    tempUrl = mUrl.substring(TAG_TEL.length());
                                } else if (isMailAddr) {
                                    tempUrl = mUrl.substring(TAG_MAIL.length());
                                }
                                intent = createAddContactIntent(tempUrl);
                            }
                            try {
                                intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                                mContext.startActivity(intent);
                            } catch (ActivityNotFoundException e) {
                                Log.e(TAG,
                                        "No Activity found to handle Intent: "
                                                + intent);
                            }
                        } else if (mUrlList.get(which).startsWith(TAG_SMS)) {
                            Intent intent = new Intent(Intent.ACTION_VIEW, uri);
                            intent.putExtra(Browser.EXTRA_APPLICATION_ID,
                                    mContext.getPackageName());
                            intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_WHEN_TASK_RESET);
                            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                            intent.setClassName("com.android.mms",
                                    "com.android.mms.ui.ComposeMessageActivity");
                            try {
                                mContext.startActivity(intent);
                            } catch (Exception e) {
                                Log.d(TAG, "Exception e : " + e);
                            }
                        } else if (mUrlList.get(which).startsWith(TAG_TEL)) {
                            // SPRD: Modify for bug#261022 @{
                            Intent intent = new Intent(Intent.ACTION_CALL, uri);
                            // @}
                            if (mUrlList.get(which).startsWith(TAG_TEL + "*")
                                    || mUrlList.get(which).startsWith(
                                            TAG_TEL + "#")) {
                                intent = new Intent(
                                        Intent.ACTION_CALL,
                                        Uri.fromParts(
                                                "tel",
                                                mUrlList.get(which).substring(
                                                        TAG_TEL.length()), null));
                            }
                            intent.putExtra(Browser.EXTRA_APPLICATION_ID,
                                    mContext.getPackageName());
                            intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_WHEN_TASK_RESET);
                            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                            mContext.startActivity(intent);
                        } else if (mUrlList.get(which).startsWith(TAG_MAIL)) {
                            Intent intent = new Intent(Intent.ACTION_VIEW, uri);
                            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                            try {
                                mContext.startActivity(intent);
                            } catch (ActivityNotFoundException e) {
                                Log.e(TAG,
                                        "No Activity found to handle Intent: "
                                                + intent);
                            }
                        } else if (mUrlList.get(which)
                                .startsWith(TAG_STREAMING)) {
                            Intent intent = new Intent(Intent.ACTION_VIEW, uri);
                            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                            try {
                                mContext.startActivity(intent);
                            } catch (ActivityNotFoundException e) {
                                Log.e(TAG,
                                        "No Activity found to handle Intent: "
                                                + intent);
                            }
                        }
                    }
                    dialog.dismiss();
                    /*
                     * final PhoneGlobals app = PhoneGlobals.getInstance(); if
                     * ((app != null) && (app.mMmiPreviousAlertAnother != null))
                     * { app.mMmiPreviousAlertAnother.dismiss(); }
                     */
                }
            };
            final AlertDialog b = new AlertDialog.Builder(mContext)
                    .setTitle(R.string.select_link_title)
                    .setCancelable(true)
                    .setAdapter(adapter, click)
                    .setNegativeButton(android.R.string.cancel,
                            new DialogInterface.OnClickListener() {
                                @Override
                                public final void onClick(
                                        DialogInterface dialog, int which) {
                                    dialog.dismiss();
                                }
                            }).create();
            b.getWindow()
                    .setType(WindowManager.LayoutParams.TYPE_SYSTEM_DIALOG);
            b.getWindow().addFlags(WindowManager.LayoutParams.FLAG_BLUR_BEHIND);
            b.show();
        }

        private void confirmOpenBrowserDialog(final Uri uri) {
            final AlertDialog builder = new AlertDialog.Builder(mContext)
                    .setTitle(R.string.confirm_open_browser_dialog_title)
                    .setMessage(R.string.confirm_open_browser_dialog_message)
                    .setPositiveButton(android.R.string.ok,
                            new AlertDialog.OnClickListener() {
                                public void onClick(DialogInterface dialog,
                                        int which) {
                                    Intent intent = new Intent(
                                            Intent.ACTION_VIEW, uri);
                                    intent.putExtra(
                                            Browser.EXTRA_APPLICATION_ID,
                                            mContext.getPackageName());
                                    intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_WHEN_TASK_RESET);
                                    intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                                    try {
                                        mContext.startActivity(intent);
                                    } catch (ActivityNotFoundException e) {
                                        Log.e(TAG,
                                                "No Activity found to handle Intent: "
                                                        + intent);
                                    }
                                }
                            })
                    .setNegativeButton(android.R.string.cancel,
                            new AlertDialog.OnClickListener() {
                                public void onClick(DialogInterface dialog,
                                        int which) {
                                    dialog.dismiss();
                                }
                            }).create();
            builder.getWindow().setType(
                    WindowManager.LayoutParams.TYPE_SYSTEM_DIALOG);
            builder.getWindow().addFlags(
                    WindowManager.LayoutParams.FLAG_BLUR_BEHIND);
            builder.show();
        }

        public static Intent createAddContactIntent(String address) {
            // address must be a single recipient
            Intent intent = new Intent(Intent.ACTION_INSERT_OR_EDIT);
            intent.setType(Contacts.CONTENT_ITEM_TYPE);
            if (Mms.isEmailAddress(address)) {
                intent.putExtra(ContactsContract.Intents.Insert.EMAIL, address);
            } else {
                intent.putExtra(ContactsContract.Intents.Insert.PHONE, address);
                intent.putExtra(ContactsContract.Intents.Insert.PHONE_TYPE,
                        ContactsContract.CommonDataKinds.Phone.TYPE_MOBILE);
            }
            intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_WHEN_TASK_RESET);
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            return intent;
        }
    }

    @Override
    public TextView getTextView(Context context, CharSequence text) {
        TextView tv = new TextView(context);
        tv.setTextColor(Color.BLACK);
        return tv;
    }

    @Override
    public void handleMMIDialogDismiss(final Phone phone, Context context,
            final MmiCode mmiCode, Message dismissCallbackMessage, AlertDialog previousAlert) {
        PhoneGlobalsEx.getInstance().handleMMIDialogDismiss(phone, context,
                mmiCode, dismissCallbackMessage, previousAlert);
    }
    /* SPRD: add for bug620380 @{ */
    public boolean isIncomingCallDialogHide() {
        return true;
    }
    /* @} */
}
