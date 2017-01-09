package com.android.internal.telephony;

import android.content.res.Resources;
import android.content.res.Resources.NotFoundException;
import android.text.TextUtils;
import android.util.Log;

public class TeleUtils {
    private static final String LOG_TAG = "TeleUtils";

    /**
     * Supply method to change operators
     * @param value This value can either be an old operator name or an operator numeric
     * @param arrayName ArrayName is the name of an specified array which has been defined in custom xmls
     * @return expected operator name
     */
    public static String updateOperator(String value, String arrayName) {
        Resources r = Resources.getSystem();
        String newName = value;
        Log.d(LOG_TAG, " changeOperator: old value= " + value);
        try {
            int identify = r.getIdentifier(arrayName, "array", "android");
            String itemList[] = r.getStringArray(identify);
            Log.d(LOG_TAG, " changeOperator: itemList length is " + itemList.length);
            for (String item : itemList) {
                String parts[] = item.split("=");
                if (parts[0].equalsIgnoreCase(value)) {
                    newName = parts[1];
                    Log.d(LOG_TAG, "itemList found: parts[0]= " + parts[0] +
                            " parts[1]= " + parts[1] + "  newName= " + newName);
                    return newName;
                }
            }
        } catch (NotFoundException e) {
            Log.e(LOG_TAG, "Error, string array resource ID not found: " + arrayName);
        }
        Log.d(LOG_TAG, "changeOperator not found: original value= " + value + " newName= " + newName);
        return newName;
    }

    public static String concatenateEccList (String eccList, String number) {
        if (!TextUtils.isEmpty(number)) {
            if (!TextUtils.isEmpty(eccList)) {
                eccList += "," + number;
            } else {
                eccList = number;
            }
        }
        return eccList;
    }

    public static String concatenateCategoryList (String eccList, String category) {
        if (!TextUtils.isEmpty(category)) {
            if (!TextUtils.isEmpty(eccList)) {
                eccList += "@" + category;
            }
        }
        return eccList;
    }
    /**
     * Converts a byte array into a integer value.
     *
     * @param bytes an array of bytes
     *
     * @return integer value of bytes array
     */
    public static int bytesToInt(byte[] data) {
        if (data == null) {
            return -1;
        }
        int value = 0;
        for (int i = 0; i < data.length; i++) {
            value |= (data[i] & 0xFF) << ((data.length - i - 1) * 8);
        }
        return value;
    }

    /**
     * Converts  a integer value into a byte array.
     *
     * @param integer value ,the length of bytes
     *
     * @return bytes array
     */
    public static byte[] intToBytes(int value, int len) {
        byte[] data = new byte[len];

        for (int i = 0; i < len; i++) {
            data[i] = (byte) ((value >> ((len - i - 1) * 8)) & 0xFF);
        }
        return data;
    }

    /**
     * @param str1 Format is ecc@category
     * @param str2 Format is ecc@category
     * @return remove duplicate number of oneList
     */
    public static String removeDupNumber(String str1,String str2) {
        String eccList = "";
        if (str1 != null && str2 != null) {
            String[] str1WithCategory = str1.split(",");
            String[] str2WithCategory = str2.split(",");
            boolean noSame = true;
            for (int i = 0; i < str1WithCategory.length; i++) {
                for (int j = 0; j < str2WithCategory.length; j++) {
                    String[] numberFromStr1 = str1WithCategory[i].split("@");
                    String[] numberFromStr2 = str2WithCategory[j].split("@");
                    if ((numberFromStr1[0]).equals(numberFromStr2[0])) {
                        noSame = false;
                        break;
                    }
                }
                if (noSame) {
                    eccList = TeleUtils.concatenateEccList(eccList, str1WithCategory[i]);
                }
                noSame = true;
            }
        }else{
            eccList = str1;
        }
        return eccList;

    }

    /**
     * @param eccListWithCategory Format is ecc@category
     * @return remove "@category"
     */
    public static String removeCategory(String eccListWithCategory) {
        String eccList = "";
        if(eccListWithCategory != null && eccListWithCategory !=""){
            String[] eccWithCategory = eccListWithCategory.split(",");
            for (int i = 0; i < eccWithCategory.length; i++) {
                String[] numberFromEcc = eccWithCategory[i].split("@");
                eccList = TeleUtils.concatenateEccList(eccList, numberFromEcc[0]);
            }
        }
        Log.d(LOG_TAG,"ril.eccList:"+eccList);
        return eccList;
    }
}
