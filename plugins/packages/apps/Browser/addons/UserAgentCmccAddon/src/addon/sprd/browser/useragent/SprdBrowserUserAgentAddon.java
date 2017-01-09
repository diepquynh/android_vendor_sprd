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

import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;

import com.sprd.useragent.SprdBrowserUserAgentAddonStub;

import android.content.Context;
import android.os.Build;
import android.os.SystemProperties;
import android.util.Log;
import android.app.AddonManager;

public class SprdBrowserUserAgentAddon extends SprdBrowserUserAgentAddonStub implements AddonManager.InitialCallback {

    private Context mContext;
    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mContext = context;
        return clazz;
    }

    // fix bug 139736 on 20130403 begin
    public String getUserAgentString() {
        StringBuilder buffer = new StringBuilder();
        StringBuffer otherInfo = new StringBuffer();
        // Add version
        final String version = Build.VERSION.RELEASE;
        String finalVersion = null;
        if (version.length() > 0) {
            if (Character.isDigit(version.charAt(0))) {
                // Release is a version, eg "3.1"
                // SPRD: add version for 412001
                otherInfo.append(version);
                finalVersion = version;
            } else {
                // Release is a codename, eg "Honeycomb"
                // In this case, use the previous release's version
                otherInfo.append("3.1");
                finalVersion = "3.1";
            }
        } else {
            // default to "1.0"
            otherInfo.append("1.0");
            finalVersion = "1.0";
        }
        // SPRD: add for 412001
        otherInfo.append("; ");

        //Add language.  Preset to "zh-cn",  only for CMCC version.
        otherInfo.append(" zh-cn;");

        // add the model for the release build
        if ("REL".equals(Build.VERSION.CODENAME)) {
            final String model = Build.MODEL;
            if (model.length() > 0) {
                otherInfo.append(" ");
                otherInfo.append(model);
                otherInfo.append(";");
            }
        }

        // add software version
        otherInfo.append(" Android/");
        otherInfo.append(finalVersion);
        otherInfo.append(";");

        // add release date
        otherInfo.append(" Release");
        if(SystemProperties.get("ro.product.board.customer", "none").equalsIgnoreCase("cgmobile")){
            //cg yangyongshan modified B 20140504
            //Modified Browser UA profile
            otherInfo.append("/04.21.2014");
            /*Calendar cl = Calendar.getInstance();
            cl.setTimeInMillis(Build.TIME);
            Date date = cl.getTime();
            SimpleDateFormat sdf = new SimpleDateFormat("MM.dd.yyyy");
            otherInfo.append(sdf.format(date));*/
            //cg yangyongshan modified B 20140504
        }else{
            otherInfo.append("/");
            Calendar cl = Calendar.getInstance();
            cl.setTimeInMillis(Build.TIME);
            Date date = cl.getTime();
            SimpleDateFormat sdf = new SimpleDateFormat("MM.dd.yyyy");
            otherInfo.append(sdf.format(date));
        }

        final String base = mContext.getResources().getText(R.string.web_user_agent).toString();
        String uaStr = String.format(base, otherInfo, buffer);
        Log.i("browser", "getUserAgentString cmcc --> UA -> ua = " + uaStr);
        return uaStr;
    }
}
