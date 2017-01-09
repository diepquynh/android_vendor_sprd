/*
 * Copyright (C) 2010 he Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License
 */

package addon.sprd.browser.useragent;

import java.util.Locale;

import com.sprd.useragent.SprdBrowserUserAgentAddonStub;

import android.content.Context;
import android.os.Build;
import android.util.Log;
import android.app.AddonManager;

public class SprdBrowserUserAgentAddon extends SprdBrowserUserAgentAddonStub implements AddonManager.InitialCallback {
    private static final String DEFAULT_VERSION = "4.1";
    private Context mContext;
    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mContext = context;
        return clazz;
    }

    public String getUserAgentString() {
        /*Calendar cl = Calendar.getInstance();
        cl.setTimeInMillis(Build.TIME);
        Date date = cl.getTime();
        SimpleDateFormat sdf = new SimpleDateFormat("MM.dd.yyyy");

        String usString = Build.MODEL + "/" + "V1.0.0" +
                " Linux/3.4.0" + " Android/" + Build.VERSION.RELEASE +
                " Release/" + "05.29.2013" +
                " Browser/AppleWebkit533.1" + " Profile/MIDP-2.0" +
                " Configuration/CLDC-1.1";
        Log.i(LOGTAG, "getCuUserAgentString(): " + usString);

        return usString;*/
        //new UA standard
        Locale locale = Locale.getDefault();
        StringBuffer buffer = new StringBuffer();
        // Add version
        final String version = Build.VERSION.RELEASE;
        if (version.length() > 0) {
            if (Character.isDigit(version.charAt(0))) {
                // Release is a version, eg "3.1"
                buffer.append(version);
            } else {
                // Release is a codename, eg "Honeycomb"
                // In this case, use the previous release's version
                buffer.append(DEFAULT_VERSION);//4.1
            }
        } else {
            // default to "1.0"
            buffer.append("1.0");
        }
        buffer.append("; ");
        final String language = locale.getLanguage();
        if (language != null) {
            buffer.append(convertObsoleteLanguageCodeToNew(language));
            final String country = locale.getCountry();
            if (country != null) {
                buffer.append("-");
                buffer.append(country.toLowerCase());
            }
        } else {
            // default to "en"
            buffer.append("en");
        }
        buffer.append(";");
        // add the model for the release build
        if ("REL".equals(Build.VERSION.CODENAME)) {
            final String model = Build.MODEL;
            if (model.length() > 0) {
                buffer.append(" ");
                buffer.append(model);
            }
        }
        final String id = Build.ID;
        if (id.length() > 0) {
            buffer.append(" Build/");
            buffer.append(id);
        }
        String mobile = mContext.getResources().getText(R.string.web_user_agent_target_content).toString();
        final String base = mContext.getResources().getText(R.string.web_user_agent).toString();

        String uaStr = String.format(base, buffer, mobile);
        Log.i("browser", "getUserAgentString cucc --> UA -> ua = " + uaStr);
        return uaStr;
    }

    private static String convertObsoleteLanguageCodeToNew(String langCode) {
        if (langCode == null) {
            return null;
        }
        if ("iw".equals(langCode)) {
            // Hebrew
            return "he";
        } else if ("in".equals(langCode)) {
            // Indonesian
            return "id";
        } else if ("ji".equals(langCode)) {
            // Yiddish
            return "yi";
        }
        return langCode;
    }
}
