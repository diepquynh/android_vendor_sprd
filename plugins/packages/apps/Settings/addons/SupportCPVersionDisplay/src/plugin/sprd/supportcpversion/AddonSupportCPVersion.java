package plugin.sprd.supportcpversion;

import android.app.AddonManager;
import android.content.Context;
import android.content.SharedPreferences;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import android.net.LocalSocketAddress.Namespace;
import android.text.TextUtils;
import android.util.Log;
import android.support.v7.preference.Preference;
import android.preference.PreferenceManager;
import android.os.Handler;
import android.os.Message;
import android.os.SystemProperties;

import com.android.settings.SupportCPVersion;
import com.android.settings.R;

import java.io.IOException;
import java.io.OutputStream;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class AddonSupportCPVersion extends SupportCPVersion implements AddonManager.InitialCallback {
    private Runnable mBasedVerRunnable;
    private static final int MSG_UPDATE_BASED_VERSION_SUMMARY = 1;
    private static final String LOG_TAG = "AddonSupportCPVersion";
    private static Preference mBasePreference;
    private Context mContext;


    private Handler mHandler = new Handler() {
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_UPDATE_BASED_VERSION_SUMMARY:
                    mBasePreference.setSummary((CharSequence) msg.obj);
                    break;
            }
        }
    };

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        return clazz;
    }

    public boolean isSupport(){
        return true;
    }

    public void initPreference(Context context,Preference mPreference) {
        mContext = context;
        mBasePreference = mPreference;
    }

    private void getBasedSummary(String property) {
        try {
            String pro = SystemProperties.get(property,
                    mContext.getResources().getString(R.string.device_info_default));
            String cp2 = "";
            String temp;
            String summary = pro;
            temp = getCp2Version();
            Log.d(LOG_TAG, "getCp2Version():" + temp);
            if (temp != null) {
                Log.d(LOG_TAG, " temp = " + temp);
                temp = temp.replaceAll("\\s+", "");
                if (temp.startsWith("Platform")) {
                    final String PROC_VERSION_REGEX =
                            "PlatformVersion:(\\S+)" + "ProjectVersion:(\\S+)" + "HWVersion:(\\S+)";
                    Matcher m = Pattern.compile(PROC_VERSION_REGEX).matcher(temp);
                    if (!m.matches()) {
                        Log.e(LOG_TAG, "Regex did not match on cp2 version: ");
                    } else {
                        String dateTime = m.group(3);
                        String modem = "modem";
                        int endIndex = dateTime.indexOf(modem) + modem.length();
                        String subString1 = dateTime.substring(0, endIndex);
                        String subString2 = dateTime.substring(endIndex);
                        String time = subString2.substring(10);
                        String date = subString2.substring(0, 10);
                        cp2 = m.group(1) + "|" + m.group(2) + "|" + subString1 + "|" + date + " "
                                + time;
                    }
                } else {
                    Log.e(LOG_TAG, "cp2 version is error");
                }
            }
            if (!TextUtils.isEmpty(cp2)) {
                summary = pro + "\n" + cp2;
            }
            Log.d(LOG_TAG, "pro = " + pro + " cp2 = " + cp2);
            Message msg = mHandler.obtainMessage();
            msg.what = MSG_UPDATE_BASED_VERSION_SUMMARY;
            msg.obj = summary;
            mHandler.sendMessage(msg);

        } catch (Exception e) {
            Log.d(LOG_TAG, "Exceptipon:" + e.toString());
        }
    }

    public static String getCp2Version() {
        LocalSocket socket = null;
        final String socketName = "wcnd";
        String result = null;
        byte[] buf = new byte[255];
        OutputStream outputStream = null;
        InputStream inputStream = null;

        try {
            socket = new LocalSocket();
            LocalSocketAddress address = new LocalSocketAddress(socketName,
                    LocalSocketAddress.Namespace.ABSTRACT);
            socket.connect(address);
            outputStream = socket.getOutputStream();
            if (outputStream != null) {
                String strcmd = "wcn at+spatgetcp2info";
                StringBuilder cmdBuilder = new StringBuilder(strcmd).append('\0');
                String cmd = cmdBuilder.toString(); /* Cmd + \0 */
                try {
                    outputStream.write(cmd.getBytes(StandardCharsets.UTF_8));
                    outputStream.flush();
                } catch (IOException e) {
                    Log.e(LOG_TAG, "Failed wrirting output stream: " + e);
                }
            }
            inputStream = socket.getInputStream();
            int count = inputStream.read(buf, 0, 255);
            result = new String(buf, "utf-8");
            Log.d(LOG_TAG,"count = "+count);
            if (result.startsWith("Fail")) {
                Log.d(LOG_TAG,"cp2 no data available");
                return null;
            }
        } catch (Exception e) {
            Log.i(LOG_TAG, " get socket info fail about:" + e.toString());
        } finally {
            try {
                buf = null;
                if (outputStream != null) {
                    outputStream.close();
                }
                if (inputStream != null) {
                    inputStream.close();
                }
                socket.close();
            } catch (Exception e) {
                Log.i(LOG_TAG, "socket fail about:" + e.toString());
            }
        }
        return result;
    }

    public void startRunnable() {
        mBasedVerRunnable = new Runnable() {
            public void run() {
                getBasedSummary("gsm.version.baseband");
            }
        };
        new Thread(mBasedVerRunnable).start();
    }
}
