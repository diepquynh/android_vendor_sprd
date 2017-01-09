
package com.android.emailcommon.utility;


import java.io.IOException;
import java.io.InputStream;

import com.android.mail.utils.LogUtils;
import com.ibm.icu.text.CharsetDetector;
import com.ibm.icu.text.CharsetMatch;

public class CharsetUtilities {
    public static String emailCharsetDetect(InputStream in) {
        String properCharset = null;
        // use ICU lib to detect charset.
        CharsetDetector cd = new CharsetDetector();
        try {
            cd.setText(in);
        } catch (IOException e) {
            e.printStackTrace();
        }
         CharsetMatch[] cm = cd.detectAll();
         if (cm != null && cm.length > 0) {
             // print chraset information.
             for (CharsetMatch match:cm) {
                 LogUtils.d(LogUtils.TAG, "Charset Detect Result: " + match.getName());
             }
             // If the most possible charset was "UTF-8", we do nothing.
             if (cm[0].getName().equals("UTF-8")) {
                 return cm[0].getName();
             }

            properCharset = cm[0].getName();
            return properCharset;
         } else {
             return null;
         }
    }

}
