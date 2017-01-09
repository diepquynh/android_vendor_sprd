
package com.sprd.validationtools.itemstest;

import android.os.SystemProperties;

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import com.sprd.validationtools.BaseActivity;
import com.sprd.validationtools.PhaseCheckParse;

import android.app.ListActivity;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;
import com.sprd.validationtools.R;

public class SystemVersionTest extends BaseActivity
{
    private static final String TAG = "SystemVersionTest";
    TextView androidVersion;
    TextView linuxVersion;
    TextView platformVersion;
    TextView platformSn;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.version);
        setTitle(R.string.version_test);
        androidVersion = (TextView) findViewById(R.id.android_version);
        linuxVersion = (TextView) findViewById(R.id.linux_version);
        platformVersion = (TextView) findViewById(R.id.platform_version);
        platformSn = (TextView) findViewById(R.id.platform_sn);
        androidVersion.setText(getString(R.string.android_version) + "\n" + Build.VERSION.RELEASE
                + "\n");
        linuxVersion.setText(getString(R.string.prop_version) + "\n" + getPropVersion() + "\n");
        platformVersion.setText(getString(R.string.build_number) + "\n" +
                SystemProperties.get("ro.build.display.id", "unknown") + "\n");
        platformSn.setText(getString(R.string.device_sn) + "\n" + getSn() + "\n");
    }

    private String getPropVersion() {
        BufferedReader bReader = null;
        StringBuffer sBuffer = new StringBuffer();

        try {
            FileInputStream fi = new FileInputStream("/proc/version");
            bReader = new BufferedReader(new InputStreamReader(fi));
            String str = bReader.readLine();

            while (str != null) {
                sBuffer.append(str + "\n");
                str = bReader.readLine();
            }
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            if (bReader != null) {
                try {
                    bReader.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
        return sBuffer.toString();
    }

    private String getFormattedKernelVersion() {
        String procVersionStr;

        try {
            BufferedReader reader = new BufferedReader(new FileReader("/proc/version"), 256);
            try {
                procVersionStr = reader.readLine();
            } finally {
                reader.close();
            }

            final String PROC_VERSION_REGEX =
                    "\\w+\\s+" + /* ignore: Linux */
                            "\\w+\\s+" + /* ignore: version */
                            "([^\\s]+)\\s+" + /* group 1: 2.6.22-omap1 */
                            "\\(([^\\s@]+(?:@[^\\s.]+)?)[^)]*\\)\\s+" + /*
                                                                         * group
                                                                         * 2:
                                                                         * (xxxxxx
                                                                         * @
                                                                         * xxxxx
                                                                         * .
                                                                         * constant
                                                                         * )
                                                                         */
                            "\\((?:[^(]*\\([^)]*\\))?[^)]*\\)\\s+" + /*
                                                                      * ignore:
                                                                      * (gcc ..)
                                                                      */
                            "([^\\s]+)\\s+" + /* group 3: #26 */
                            "(?:PREEMPT\\s+)?" + /* ignore: PREEMPT (optional) */
                            "(.+)"; /* group 4: date */

            Pattern p = Pattern.compile(PROC_VERSION_REGEX);
            Matcher m = p.matcher(procVersionStr);

            if (!m.matches()) {
                Log.e(TAG, "Regex did not match on /proc/version: " + procVersionStr);
                return "Unavailable";
            } else if (m.groupCount() < 4) {
                Log.e(TAG, "Regex match on /proc/version only returned " + m.groupCount()
                        + " groups");
                return "Unavailable";
            } else {
                return (new StringBuilder(m.group(1)).append("\n").append(
                        m.group(2)).append(" ").append(m.group(3)).append("\n")
                        .append(m.group(4))).toString();
            }
        } catch (IOException e) {
            Log.e(TAG,
                    "IO Exception when getting kernel version for Device Info screen",
                    e);

            return "Unavailable";
        }
    }

    private String getSn() {
        PhaseCheckParse parse = new PhaseCheckParse();
        return parse.getSn();
    }

//    @Override
//    public void onBackPressed() {
//        storeRusult(true);
//        finish();
//    }
}