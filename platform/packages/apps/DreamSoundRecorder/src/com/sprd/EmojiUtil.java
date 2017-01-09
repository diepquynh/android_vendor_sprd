package com.sprd.soundrecorder;


import android.text.TextUtils;
import android.util.Log;

/**
*
* @author byl
*
*/
public class EmojiUtil {

    /**
    * 判断是否有emoji
    * @param str
    * @return
    */
    public static boolean containsEmoji(String str) {
        if (TextUtils.isEmpty(str)) {
            return false;
    }

    for (int i = 0; i < str.length(); i++) {
    char codePoint = str.charAt(i);
        if (isEmojiCharacter(codePoint)) {
            return true;
        }
    }
        return false;
    }

    private static boolean isEmojiCharacter(char codePoint) {
        return !((codePoint == 0x0) ||
            (codePoint == 0x9) ||
            (codePoint == 0xA) ||
            (codePoint == 0xD) ||
            ((codePoint >= 0x20) && (codePoint <= 0xD7FF)) ||
            ((codePoint >= 0xE000) && (codePoint <= 0xFFFD)) ||
            ((codePoint >= 0x10000) && (codePoint <= 0x10FFFF)));
    }

    /**
    * 过滤emoji 或者 其他非文字类型的字符
    * @param source
    * @return
    */
    public static String filterEmoji(String str) {

        if (!containsEmoji(str)) {
            return str;//如果不包含，直接返回
        }else{
            Log.e("jj", "字符串中含有emoji表情");
        }
        //到这里铁定包含
        StringBuilder buf = null;
        int len = str.length();
        for (int i = 0; i < len; i++) {
            char codePoint = str.charAt(i);
            if (isEmojiCharacter(codePoint)) {
                if (buf == null) {
                    buf = new StringBuilder(str.length());
                }
                buf.append(codePoint);
            } else {

            }
    }

        if (buf == null) {
            return "";//如果全部为 emoji表情，则返回空字符串
        } else {
            return buf.toString();
        }

    }
}