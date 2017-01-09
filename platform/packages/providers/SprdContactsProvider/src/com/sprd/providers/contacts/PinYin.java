package com.sprd.providers.contacts;

import java.util.ArrayList;

import com.android.providers.contacts.HanziToPinyin;
import com.android.providers.contacts.HanziToPinyin.Token;

import android.content.Context;
import android.text.TextUtils;
import android.util.Log;

/**
 * This class is used to get the spelling string of a Chinese char or string.
 *
 */
public class PinYin {
    private static final String TAG = "PinYin";

    public static final int BUFFER_SIZE = 13;

    private static PinYin instance = new PinYin();
    private HanziToPinyin han2pin = HanziToPinyin.getInstance();

    public synchronized static PinYin getInstance(Context context) {
        return instance;
    }

    /**
     * //cct01:get pinyin of one string
     *
     * @param code
     * @return
     */
    private String getPinyin(String code) {
        ArrayList<Token> tokens = han2pin.getTokens(code);
        StringBuffer number_full = new StringBuffer();
        for (int i = 0; i < tokens.size(); ++i) {
            Token token = tokens.get(i);
            if (token.type == Token.LATIN || token.type == Token.PINYIN) {
                number_full.append(token.target.toLowerCase());
            } else {
                Log.w(TAG, "unknow type of character found!" + token.source);
            }
        }
        return number_full.toString();
    }

    /**
     * Check the indicated char is Chinese char or not.
     *
     * @param c : the indicated char.
     * @return
     */
    public static boolean isChinese(char c) {
        if (c == '\u3007' || (c >= '\u4E00' && c <= '\u9FA5')
                || (c >= '\uF900' && c <= '\uFA2D')) {
            return true;
        }
        return false;
    }

    /**
     * Test if the specified string has a Chinese char.
     *
     * @param s
     * @return
     */
    public static boolean hasChinese(String s) {
        if (null == s) {
            return false;
        }
        char[] sChar = s.toCharArray();
        for (char c : sChar) {
            if (isChinese(c)) {
                return true;
            }
        }
        return false;
    }

    private String appendPinYin(StringBuffer sb, char ch) {
        if (isChinese(ch)) {
            String tmp = String.valueOf(ch);
            String str = getPinyin(tmp);
            if (!TextUtils.isEmpty(str)) {
                sb.append(str);
            }
        } else {
            sb.append(ch);
        }
        return sb.toString();
    }

    private void appendPinYinAndShorPinYin(StringBuffer sbPinYin, StringBuffer sbShortPinYin, char ch) {
        if (isChinese(ch)) {
            String tmp = String.valueOf(ch);
            String str = getPinyin(tmp);
            if (!TextUtils.isEmpty(str)) {
                sbPinYin.append(str);
                sbShortPinYin.append(str.toCharArray()[0]);
            }
        } else {
            sbPinYin.append(ch);
            sbShortPinYin.append(ch);
        }
        // return sb.toString();
    }

    /**
     * Get the pinyin string array of the name string.
     *
     * @param nameStr the name string to be converted to pinyin
     * @return the pinyin string array
     */
    public String[] getPinYinStringArray(String nameStr) {
        if (nameStr == null) {
            return null;
        }
        char[] nameChar = nameStr.toCharArray();
        String[] strs = new String[nameChar.length];
        for (int i = 0; i < nameChar.length; i++) {
            strs[i] = appendPinYin(new StringBuffer(), nameChar[i]);
        }
        return strs;
    }

    /**
     *
     * Get the pinyin string of the name string.
     *
     * @param nameStr the name string to be converted to pinyin
     * @return the pinyin string
     */
    public String getPinyinString(String nameStr) {
        if (nameStr == null) {
            return null;
        }
        StringBuffer sb = new StringBuffer("");
        char[] nameChar = nameStr.toCharArray();
        for (int i = 0; i < nameChar.length; i++) {
            // cct01:
            appendPinYin(sb, nameChar[i]);
        }
        return sb.toString();
    }

    public String[] getPinyinAndShortPinyinString(String nameStr) {
        if (nameStr == null) {
            return null;
        }
        String[] pinyin = new String[2];
        StringBuffer sbPinyin = new StringBuffer("");
        StringBuffer sbShortPinyin = new StringBuffer("");
        char[] nameChar = nameStr.toCharArray();
        for (int i = 0; i < nameChar.length; i++) {
            // cct01:
            // appendPinYin(sb, nameChar[i]);
            appendPinYinAndShorPinYin(sbPinyin, sbShortPinyin, nameChar[i]);
        }
        pinyin[0] = sbPinyin.toString();
        pinyin[1] = sbShortPinyin.toString();
        return pinyin;
    }

    public String getPinyinString(char nameChar) {
        StringBuffer sb = new StringBuffer("");
        //cct01:
        appendPinYin(sb, nameChar);
        return sb.toString();
    }

    public void free() {

    }

    /**
     * judge that the phone number is valid or not.
     *
     * @param text
     * @return
     */
    public static boolean isValidPhoneNumber(String text) {
        char[] sChar = text.toCharArray();
        for (char c : sChar) { // “,;*#+”
            if ((c >= '0' && c <= '9') || c == ',' || c == ';' || c == '*' || c == '#' || c == '+' || c == '-') {
                continue;
            } else {
                return false;
            }
        }

        return true;
    }

    /**
     * filter invalid characters from a number.
     *
     * @param originalText
     * @return
     */
    public static String filterInvalidNumbers(String originalText) {
        StringBuffer buffer = new StringBuffer(originalText);
        for (int i = 0; i < buffer.length(); i++) {
            char c = buffer.charAt(i);
            if ((c >= '0' && c <= '9') || c == ',' || c == ';' || c == '*' || c == '#' || c == '+' || c == '-') {
                continue;
            } else {
                buffer.deleteCharAt(i);
            }
        }
        return buffer.toString();
    }
}

