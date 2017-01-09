
package com.sprd.engineermode.connectivity;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class Utils {

    public static int parseInt(String str) {
        if (str != null && !"".equals(str)) {
            return Integer.parseInt(str);
        }
        return 0;
    }

    public static boolean isInt(String str) {
        Pattern p = Pattern.compile("^-?\\d+");
        Matcher m = p.matcher(str);
        if (m.matches()) {
            return true;
        }
        return false;
    }
}
