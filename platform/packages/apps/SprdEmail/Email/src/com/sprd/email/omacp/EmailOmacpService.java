package com.sprd.email.omacp;

import android.app.Service;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.RemoteException;

import com.android.email.activity.setup.AccountSettingsUtils;
import com.android.email.EmailAddressValidator;
import com.android.email.Preferences;
import com.android.email.mail.Sender;
import com.android.email.mail.Store;
import com.android.email.service.EmailServiceUtils;
import com.android.email2.ui.MailActivityEmail;
import com.android.emailcommon.Configuration;
import com.android.emailcommon.provider.Account;
import com.android.emailcommon.provider.EmailContent.AccountColumns;
import com.android.emailcommon.provider.EmailContent.HostAuthColumns;
import com.android.emailcommon.provider.HostAuth;
import com.android.emailcommon.service.EmailServiceProxy;
import com.android.mail.utils.LogUtils;

import com.android.email.provider.EmailProvider;

import java.net.URI;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import android.util.Log;
import android.os.Bundle;

public class EmailOmacpService extends Service {
    private static final String TAG = "EmailOmacpService";
    public static final boolean NEED_CHECK_NET = false;

    // The default port for pop3/imap/smtp/exchange
    public static final int IMAP_DEFAULT_PORT = 143;
    public static final int POP3_DEFAULT_PORT = 110;
    public static final int SMTP_DEFAULT_PORT = 25;
    public static final int EAS_DEFAULT_PORT = 80;

    public static final int IMAP_DEFAULT_SSL_PORT = 993;
    public static final int POP3_DEFAULT_SSL_PORT = 995;
    public static final int SMTP_DEFAULT_SSL_PORT = 465;
    public static final int EAS_DEFAULT_SSL_PORT = 443;

    private static final String SMTP_APPID = String.valueOf(SMTP_DEFAULT_PORT);
    private static final String SMTP_DEAULT_PORT_NUM = String
            .valueOf(SMTP_DEFAULT_PORT);
    private static final String SMTP_DEFAULT_SERVICE = String
            .valueOf(SMTP_DEFAULT_PORT);
    private static final String SMTP_SSL_SERVICE = String
            .valueOf(SMTP_DEFAULT_SSL_PORT);
    private static final String POP_DEFAULT_PORT_NUM = String
            .valueOf(POP3_DEFAULT_PORT);
    private static final String IMAP_DEFAULT_PORT_NUM = String
            .valueOf(IMAP_DEFAULT_PORT);
    private static final String POP_DEFAULT_SERVICE = String
            .valueOf(POP3_DEFAULT_PORT);
    private static final String IMAP_DEFAULT_SERVICE = String
            .valueOf(IMAP_DEFAULT_PORT);
    private static final String POP_SSL_SERVICE = String
            .valueOf(POP3_DEFAULT_SSL_PORT);
    private static final String IMAP_SSL_SERVICE = String
            .valueOf(IMAP_DEFAULT_SSL_PORT);
    private static final String POP_APPID = String.valueOf(POP3_DEFAULT_PORT);
    private static final String IMAP_APPID = String.valueOf(IMAP_DEFAULT_PORT);

    private static final String STR_SSL = "ssl";
    private static final String STR_TLS = "tls";

    private static final int SYNC_INTERVAL = 15;
    private static final int SMTP_SERVER_TYPE = 1;
    private static final int POP_SERVER_TYPE = 2;
    private static final int IMAP_SERVER_TYPE = 3;
    private static final int CONNECT_SUCCESS = 1;
    private static final int CONNECT_FAIL = -1;

    private static final String HOSTAUTH_WHERE_CREDENTIALS = HostAuthColumns.ADDRESS + " like ?"
            + " and " + HostAuthColumns.LOGIN + " like ?" + " and " + HostAuthColumns.PROTOCOL
            + " not like \"smtp\"";

    private static final String ACCOUNT_WHERE_HOSTAUTH = AccountColumns.HOST_AUTH_KEY_RECV + "=?";

    public static final String OTA_EMAIL_ACCOUNT_SIZE="email_size";
    public static final String OTA_EAS_ACCOUNT_SIZE="eas_size";

    /* SPRD modify for bug653607 {@ */
    private static final String EXCHANGE_PACKAGE_NAME = "com.android.exchange";
    /* @} */

    private EmailAddressValidator mEmailValidator = new EmailAddressValidator();
    private String mFrom;
    private String mProviderId;
    private String mName;
    private boolean mContactEnable = false;
    private boolean mCalendarEnable = false;

    private Handler mHandler = new Handler() {
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            Log.d(TAG, "handleMessage msg.what:" + msg.what);
            if (msg.what == CONNECT_SUCCESS) {
                Account account = (Account) msg.obj;
                addAccount(account);
                sendResultToOmacp(true);
            } else if (msg.what == CONNECT_FAIL) {
                sendResultToOmacp(false);
            }
        }
    };

    public IBinder onBind(Intent arg0) {
        // TODO Auto-generated method stub
        return null;
    }

    public int onStartCommand(Intent intent, int flags, int startId) {
        super.onStartCommand(intent, flags, startId);
        Intent omacpIntent = intent.getParcelableExtra(Intent.EXTRA_INTENT);
        if (omacpIntent != null) {
            Account account = parserOmacpRequest(omacpIntent);
            Log.d(TAG, "account is " + account);
            if (account != null) {
                if(NEED_CHECK_NET){
                    checkEmailServerConnect(this, account);
                }else{
                    sendMessage(CONNECT_SUCCESS, account);
                }
            } else {
                sendResultToOmacp(false);
            }
        }
        return START_NOT_STICKY;
    }

    /**
     * Obtaining Account Object by parsering OMACP request
     * @param intent OMACP intent
     * @return Account
     */
    @SuppressWarnings("unchecked")
    private Account parserOmacpRequest(Intent intent) {
        Log.d(TAG, "parser Omacp Request is begin");
        //A email account has two application node with incoming and outgoing in script
        //so,the otaNum must be divisible by two
        int emailNum = intent.getIntExtra(OTA_EMAIL_ACCOUNT_SIZE, 0);
        int easNum = intent.getIntExtra(OTA_EAS_ACCOUNT_SIZE,0);
        boolean isEas = false;
        Log.d(TAG, "OMACP email data num is " + emailNum);
        Log.d(TAG, "OMACP eas data num is " + easNum);

        if (emailNum <= 1 && easNum == 0) {
            return null;
        }
        if(emailNum <= 1 && easNum > 0){
            emailNum = 1;
            isEas = true;
        }
        //Now only support one account
        Account account = new Account();
        String addr = null;
        String portNbr = null;
        String service = null;
        String scheme = null;
        URI uri = null;
        URI smtpUri = null;
        String appId = null;
        String authType = null;

        try {
            for (int i = 1; i <= 2 && i <= emailNum; i++) {
                Bundle otaData = null;

                if (isEas) {
                    otaData = intent.getBundleExtra("easData" + i);
                } else {
                    otaData = intent.getBundleExtra("emailData" + i);
                }

                if (otaData == null) {
                    Log.d(TAG, "otaData is null return !!! ");
                    break;
                }
                // appId
                appId = otaData.getString("appid");
                // addr
                addr = otaData.getString("addr");
                portNbr = otaData.getString("portnbr");
                service = otaData.getString("service");
                Log.d(TAG, "appId " + appId);
                Log.d(TAG, "addr " + addr);
                Log.d(TAG, "portNbr " + portNbr);
                Log.d(TAG, "service " + service);

                if (isEmpty(addr)) {
                    Log.d(TAG, "addr is empty !!!");
                    return null;
                }

                String aAuthName = null;
                String aAuthSecret = null;
                boolean needAuth = true;

                authType = otaData.getString("aauthtype");
                aAuthName = otaData.getString("aauthname");
                aAuthSecret = otaData.getString("aauthsecret");

                if (isEmpty(authType) && isEmpty(aAuthName) && isEmpty(aAuthSecret)) {
                    needAuth = false;
                }

                if (SMTP_APPID.equals(appId)) {
                    mProviderId = otaData.getString("provider-id");
                    mFrom = otaData.getString("from");
                    mName = otaData.getString("application_name");// oma cp origanal: name

                    Log.d(TAG, "[OMACP] smtp param: PROVIDER-ID=" + mProviderId + ";ADDR=" + addr
                            + ";FROM=" + mFrom + ";NAME=" + mName + ";PORT=" + portNbr
                            + ";SERVICE=" + service + ";AAUTHTYPE=" + authType + ";AAUTHNAME="
                            + aAuthName + ";AAUTHSECRET=" + aAuthSecret);

                    if (isInvalidEmailAddress(mFrom)) {
                        Log.d(TAG, "SMTP from is invalid !!!");
                        return null;
                    }

                    // delete "< >"
                    if (mFrom.contains("<")) {
                        mFrom = mFrom.split("<")[1].replace(">", "");
                    }

                    String userInfo = null;

                    // Just check if contains APPAUTH tag, only failed for APPAUTH tag but without
                    // AAUTHNAME or AAUTHSECRET.
                    if (needAuth) {
                        if (isEmpty(aAuthName) || isEmpty(aAuthSecret)) {
                            Log.d(TAG, "SMTP failed for without aAuthName or aAuthSecret");
                            return null;
                        }
                        userInfo = aAuthName + ":" + aAuthSecret;
                    }

                    portNbr = portNbr == null ? SMTP_DEAULT_PORT_NUM : portNbr;
                    service = service == null ? SMTP_DEFAULT_SERVICE : service;
                    scheme = getScheme(service, SMTP_SERVER_TYPE);
                    if (null == scheme) {
                        Log.d(TAG, "SMTP scheme is null !!!");
                        return null;
                    }

                    smtpUri = new URI(scheme, userInfo, addr, Integer.parseInt(portNbr), null,
                            null, null);
                    HostAuth sendAuth = account.getOrCreateHostAuthSend(this);
                    sendAuth.setHostAuthFromString(smtpUri.toString());
                } else if (POP_APPID.equals(appId)) {
                    Log.d(TAG, "[OMACP] pop param: ADDR=" + addr + ";PORT=" + portNbr + ";SERVICE="
                            + service + ";AAUTHNAME=" + aAuthName + ";AAUTHSECRET=" + aAuthSecret);
                    if (isEmpty(aAuthName) || isEmpty(aAuthSecret)) {
                        Log.d(TAG, "POP aAuthName or aAuthSecret is null !!!");
                        return null;
                    }

                    Account tempAccount = findDuplicateAccount(this, account.mId, addr, aAuthName);
                    if (null != tempAccount) {
                        Log.d(TAG, "POP3 findDuplicateAccount return null");
                        return null;
                    }

                    portNbr = portNbr == null ? POP_DEFAULT_PORT_NUM : portNbr;
                    service = service == null ? POP_DEFAULT_SERVICE : service;
                    scheme = getScheme(service, POP_SERVER_TYPE);
                    if (null == scheme) {
                        Log.d(TAG, "POP scheme is null !!!");
                        return null;
                    }
                    uri = new URI(scheme, aAuthName + ":" + aAuthSecret, addr, Integer
                            .parseInt(portNbr), null, null, null);
                    HostAuth recAuth = account.getOrCreateHostAuthRecv(this);
                    recAuth.setHostAuthFromString(uri.toString());
                } else if (IMAP_APPID.equals(appId)) {
                    Log.d(TAG, "[OMACP] imap param: ADDR=" + addr + ";PORT=" + portNbr
                            + ";SERVICE=" + service + ";AAUTHNAME=" + aAuthName + ";AAUTHSECRET="
                            + aAuthSecret);
                    if (isEmpty(aAuthName) || isEmpty(aAuthSecret)) {
                        Log.d(TAG, "IMAP aAuthName or aAuthSecret is null !!!");
                        return null;
                    }

                    Account tempAccount = findDuplicateAccount(this, account.mId, addr, aAuthName);
                    if (null != tempAccount) {
                        Log.d(TAG, "IMAP findDuplicateAccount return null");
                        return null;
                    }

                    portNbr = portNbr == null ? IMAP_DEFAULT_PORT_NUM : portNbr;
                    service = service == null ? IMAP_DEFAULT_SERVICE : service;
                    scheme = getScheme(service, IMAP_SERVER_TYPE);
                    if (null == scheme) {
                        Log.d(TAG, "IMAP scheme is null !!!");
                        return null;
                    }
                    uri = new URI(scheme, aAuthName + ":" + aAuthSecret, addr, Integer
                            .parseInt(portNbr), null, null, null);
                    account.setDeletePolicy(Account.DELETE_POLICY_ON_DELETE);
                    HostAuth recAuth = account.getOrCreateHostAuthRecv(this);
                    recAuth.setHostAuthFromString(uri.toString());
                /* SPRD modify for bug653607 {@ */
                } else if (isExchangeEnabled(this)) {
                    Log.d(TAG, "appid is : " + appId);
                    String eas_ssl = otaData.getString("ssl");
                    String eas_host = otaData.getString("addr");
                    if (!isEmpty(eas_host) && eas_host.startsWith("http://")) {
                        eas_host = eas_host.substring(7);
                    }
                    Log.d(TAG, " eas_ssl: " + eas_ssl + " eas_host: " + eas_host);
                    if (!isEmpty(eas_ssl) && !isEmpty(eas_host)) {
                        mFrom = otaData.getString("provider-id");
                        mName = otaData.getString("emailaddr");
                        mProviderId = otaData.getString("provider-id");
                        int port = EAS_DEFAULT_PORT;
                        if (eas_ssl.equals("1")) {
                            eas_ssl = "+ssl";
                            port = EAS_DEFAULT_SSL_PORT;
                        } else {
                            eas_ssl = "";
                        }
                        try {
                            uri = new URI("eas" + eas_ssl + "+",
                                    aAuthName + ":" + aAuthSecret,
                                    eas_host, port, null, null, null);
                            Log.d(TAG, " uri " + uri.toString());
                            HostAuth hostAuth = account.getOrCreateHostAuthRecv(this);
                            hostAuth.setHostAuthFromString(uri.toString());
                        } catch (Exception use) {
                            Log.d(TAG, "eas exception " + use);
                            return null;
                        }
                        String contact = otaData.getString("eas_contact_enable");
                        Log.d(TAG, "contact: " + contact);
                        if (!isEmpty(contact) && "1".equals(contact)) {
                            mContactEnable = true;
                        }
                        String calendar = otaData.getString("eas_calendar_enable");
                        Log.d(TAG, "calendar: " + calendar);
                        if (!isEmpty(calendar) && "1".equals(calendar)) {
                            mCalendarEnable = true;
                        }
                    } else {
                        return null;
                    }
                } else {
                    return null;
                }
                /* @} */
            }
        } catch (Exception e) {
            Log.e(TAG, "parse exception:" , e);
            return null;
        }

        return account;
    }

    /**
     * get scheme of email uri
     * @param service service from omacp
     * @param serverType Email server type
     * @return
     */
    private String getScheme(String service, int serverType) {
        String scheme = null;
        if (SMTP_SERVER_TYPE == serverType) {
            if (SMTP_DEFAULT_SERVICE.equalsIgnoreCase(service)) {
                scheme = "smtp";
            } else if ("STARTTLS".equalsIgnoreCase(service) || STR_TLS.equalsIgnoreCase(service)) {
                scheme = "smtp+tls+";
            } else if (SMTP_SSL_SERVICE.equalsIgnoreCase(service)
                    || STR_SSL.equalsIgnoreCase(service)) {
                scheme = "smtp+ssl+";
            }
        } else if (POP_SERVER_TYPE == serverType) {
            if (POP_DEFAULT_SERVICE.equalsIgnoreCase(service)) {
                scheme = "pop3";
            } else if ("STARTTLS".equalsIgnoreCase(service) || STR_TLS.equalsIgnoreCase(service)) {
                scheme = "pop3+tls+";
            } else if (POP_SSL_SERVICE.equalsIgnoreCase(service)
                    || STR_SSL.equalsIgnoreCase(service)) {
                scheme = "pop3+ssl+";
            }
        } else if (IMAP_SERVER_TYPE == serverType) {
            if (IMAP_DEFAULT_SERVICE.equalsIgnoreCase(service)) {
                scheme = "imap";
            } else if ("STARTTLS".equalsIgnoreCase(service) || STR_TLS.equalsIgnoreCase(service)) {
                scheme = "imap+tls+";
            } else if (IMAP_SSL_SERVICE.equalsIgnoreCase(service)
                    || STR_SSL.equalsIgnoreCase(service)) {
                scheme = "imap+ssl+";
            }
        }

        return scheme;
    }

    /**
     * check the email address is invalid or not
     * @param email email address
     * @return
     */
    private boolean isInvalidEmailAddress(String email) {
        return isEmpty(email) || !mEmailValidator.isValid(email);
    }

    /**
     * check the string is empty or not
     * @param str
     * @return
     */
    private boolean isEmpty(String str) {
        return str == null || str.trim().length() == 0;
    }

    /**
     * Look for an existing account with the same username & server
     * @param context a system context
     * @param allowAccountId this account Id will not trigger (when editing an
     *            existing account)
     * @param hostName the server
     * @param userLoggingin the user Loggingin string
     * @result null = no dupes found. non-null = dupe account's display name
     */
    public Account findDuplicateAccount(Context context, long allowAccountId, String hostName,
            String userLoggingin) {
        Account account = null;
        ContentResolver resolver = context.getContentResolver();
        Cursor c = resolver.query(HostAuth.CONTENT_URI, HostAuth.ID_PROJECTION,
                HOSTAUTH_WHERE_CREDENTIALS, new String[] {
                        hostName, userLoggingin
                }, null);

        if (c != null) {
            try {
                while (c.moveToNext()) {
                    long hostAuthId = c.getLong(HostAuth.ID_PROJECTION_COLUMN);

                    // Find account with matching hostauthrecv key, and return its
                    // display name
                    Cursor c2 = resolver.query(Account.CONTENT_URI, Account.ID_PROJECTION,
                            ACCOUNT_WHERE_HOSTAUTH, new String[] {
                                Long.toString(hostAuthId)
                            }, null);
                    if (c2 != null) {
                        try {
                            while (c2.moveToNext()) {
                                long accountId = c2.getLong(Account.ID_PROJECTION_COLUMN);
                                if (accountId != allowAccountId) {
                                    account = Account.restoreAccountWithId(context, accountId);
                                }
                            }
                        } finally {
                            c2.close();
                        }
                    }
                }
            } finally {
                c.close();
            }
        }
        return account;
    }

    /**
     * This only actually matches against the email address. It's technically kosher to allow the
     * same address across different account types, but that's a pretty rare use case and isn't well
     * handled in the UI.
     *
     * @param context context
     * @param address email address to match against
     * @return true or false
     */
    public static boolean findExistingAccount(final Context context,final String address) {
       final ContentResolver resolver = context.getContentResolver();
       final Cursor c = resolver.query(Account.CONTENT_URI, Account.CONTENT_PROJECTION,
               AccountColumns.EMAIL_ADDRESS + "=?", new String[] {address}, null);
       try {
           if (c != null && c.moveToFirst()) {
               return true;
           }
           return false;
       } finally {
           if(c != null){
               c.close();
           }
       }
   }

    /**
     * check email server is connect or not,including sender server and receiver
     * server
     * @param context
     * @param account
     */
    private void checkEmailServerConnect(final Context context, final Account account) {
        Runnable runnable = new Runnable() {

            public void run() {
                try {
                    Log.d(TAG, ">>> checkEmailServerConnect start.");

                    Store store = Store.getInstance(account, context);
                    if (store == null) {
                        Log.d(TAG, ">>> store is null.");
                        sendMessage(CONNECT_FAIL, null);
                        return;
                    }
                    Log.d(TAG, ">>> check incoming start.");
                    store.checkSettings();
                    Log.d(TAG, ">>> check incoming pass.");

                    Sender sender = Sender.getInstance(getApplication(), account);
                    if (sender == null) {
                        Log.d(TAG, ">>> sender is null.");
                        sendMessage(CONNECT_FAIL, null);
                        return;
                    }
                    sender.close();
                    sender.open();
                    sender.close();
                    Log.d(TAG, "email server check finish.");
                    sendMessage(CONNECT_SUCCESS, account);
                } catch (Exception e) {
                    Log.d(TAG, "checkEmailServerConnect exception: " , e);
                    sendMessage(CONNECT_FAIL, null);
                }
            }
        };
        new Thread(runnable).start();
    }

    private void sendMessage(int isConnect, Account account) {
        if (CONNECT_FAIL == isConnect) {
            mHandler.sendEmptyMessage(isConnect);
        } else if (CONNECT_SUCCESS == isConnect) {
            Message message = mHandler.obtainMessage();
            message.what = isConnect;
            message.obj = account;
            mHandler.sendMessage(message);
        }
    }

    /**
     * add account to email
     * @param account
     */
    private void addAccount(Account account) {
        Log.d(TAG, "add Account is beginning");
        int newFlags = account.getFlags() & ~(Account.FLAGS_NOTIFY_NEW_MAIL);
        newFlags |= Account.FLAGS_NOTIFY_NEW_MAIL;
        account.setFlags(newFlags);
        account.setEmailAddress(mFrom);
        account.setSenderName(mName);
        account.setDisplayName(mProviderId);
        account.setSyncInterval(SYNC_INTERVAL);
        account.mFlags &= ~Account.FLAGS_INCOMPLETE;

        //update account flag.
        String protocal = account.mHostAuthRecv == null ? null : account.mHostAuthRecv.mProtocol;
        if (null != protocal) {
            setFlagsForProtocol(account, protocal);
        }
        Log.d(TAG, "add Account with flag : " + account.mFlags);
        if (!findExistingAccount(this,mFrom)) {
            //Save email account.
            AccountSettingsUtils.commitSettings(this, account);
            Log.d(TAG, "AccountSettingsUtils.commitSettings save email Account ");
            //Save system account.
            EmailServiceUtils.setupAccountManagerAccount(this, account, true, mContactEnable, mContactEnable, null);
            Log.d(TAG, "EmailServiceUtils save system Account ");
            EmailProvider.setServicesEnabledSync(this);
            EmailServiceUtils.startService(this, account.mHostAuthRecv.mProtocol);

            // Start fist sync, Update the folder list (to get our starting folders, e.g. Inbox)
            final EmailServiceProxy proxy = EmailServiceUtils.getServiceForAccount(this, account.mId);
            try {
                proxy.updateFolderList(account.mId);
                Log.d(TAG, "*** Setup omcap account " + account.getEmailAddress() + " success, start update folder list " );
            } catch (RemoteException e) {
                // It's ok, it will be started by internal sync or menu sync again.
            }
        } else {
            Log.d(TAG, "have the same email Account !!!");
            return;
        }
    }

    private void sendResultToOmacp(boolean isSucceed) {
        Log.d(TAG, "::: setup result ::: " + isSucceed);
    }

    /**
     * Sets the account sync, delete, and other misc flags not captured in {@code HostAuth}
     * information for the specified account based on the protocol type.
     */
    static void setFlagsForProtocol(Account account, String protocol) {
        if ("imap".equals(protocol)) {
            // Delete policy must be set explicitly, because IMAP does not provide a UI selection
            // for it.
            account.setDeletePolicy(Account.DELETE_POLICY_ON_DELETE);
            account.mFlags |= Account.FLAGS_SUPPORTS_SEARCH;
        }
    }

    /* SPRD modify for bug653607 {@ */
    private static boolean isExchangeEnabled(Context context) {
        PackageManager pm = context.getPackageManager();

        try {
            int status = pm.getApplicationEnabledSetting(EXCHANGE_PACKAGE_NAME);
            Log.e(TAG, "isExchangeEnabled status:" + status);
            return (status == PackageManager.COMPONENT_ENABLED_STATE_ENABLED)
                    || (status == PackageManager.COMPONENT_ENABLED_STATE_DEFAULT);
        } catch (IllegalArgumentException e) {
            Log.e(TAG, "isExchangeEnabled error" + e.getMessage());
            return false;
        }
    }
    /* @} */
}
