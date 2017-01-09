
package com.sprd.note.utils;

import java.io.UnsupportedEncodingException;

import android.text.Editable;

public class StringUtils {

    /**
     * if use the GBK encode, a chinese character is two bytes , a English
     * character is a bytes.
     */
    public static int getBytesLen(String str, String encodingName) {
        if (str == null) {
            return 0;
        }
        try {
            int len = str.getBytes(encodingName).length;
            return len;
        } catch (UnsupportedEncodingException e) {
            e.printStackTrace();
            return str.length();
        }
    }

    public static int getBytesLenByGBK(String str) {
        return getBytesLen(str, "GBK");
    }

    /**
     * adjust a char is a chinese?
     * 
     * @param c
     * @return true if chinses，false other else.
     * @throws UnsupportedEncodingException
     */
    public static boolean isChineseChar(char c)
            throws UnsupportedEncodingException {
        return String.valueOf(c).getBytes("GBK").length > 1;
    }

    public static void deleteByBytesSize(Editable s, int size) throws UnsupportedEncodingException {
        String orignal = s.toString();
        char c;
        int count = 0;
        for (int i = 0; i < s.length(); i++) {
            c = (char) orignal.codePointAt(i);
            if (isChineseChar(c)) {
                count += 2;
            } else {
                count++;
            }
            if (count >= size) {
                s.delete(i, s.length());
                return;
            }
        }
    }

    /*
     * replace \s, \r, \n
     */
    public static String replaceBlank(String str) {
        if (str == null) {
            return "";
        }
        str = str.trim();
        char[] strArray = str.toCharArray();
        int start = 0;
        int end = strArray.length - 1;
        for (int i = 0; i < strArray.length; i++) {
            if (strArray[i] != '\n' && strArray[i] != '\r' && strArray[i] != ' ') {
                start = i;
                break;
            }
        }

        for (int j = strArray.length - 1; j >= 0; j--) {
            if (strArray[j] != '\n' && strArray[j] != '\r' && strArray[j] != ' ') {
                end = j;
                break;
            }
        }

        return str.substring(start, end + 1);
    }

}
