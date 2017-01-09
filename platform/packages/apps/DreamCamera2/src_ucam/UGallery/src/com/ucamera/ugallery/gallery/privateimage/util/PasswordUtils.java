package com.ucamera.ugallery.gallery.privateimage.util;
/**
 * Copyright (C) 2010,2013 Thundersoft Corporation
 * All rights Reserved
 */
import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.Serializable;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Properties;
import android.text.TextUtils;
import android.util.Log;

public class PasswordUtils
{
//  private static final String EMAIL_PATTERN = "\\w+([-+.]\\w+)*@\\w+([-.]\\w+)*\\.\\w+([-.]\\w+)*";
  public static final String KEY_PASSWORD = "key";

  public static final String PASSWORD_PATH  = Constants.SD_PATH + "/Android/.gallery.properties";
//  private static final String PASSWORD_PATTERN = "[\\w\\d]{4,}";
  private static final String TAG = PasswordUtils.class.getName();
  public String mPassowrd;
  static {
      File file = new File(PASSWORD_PATH);
      if(!file.exists()) {
          try {
            file.createNewFile();
        } catch (IOException e) {
            e.printStackTrace();
        }
      }
  }
  public static boolean check(String paramString)
  {
    return (TextUtils.isEmpty(paramString)) || (paramString.length() < 4) || (paramString.length() > 32);
  }

    public static boolean isPasswordFileExist() {
        if (new File(PASSWORD_PATH).exists()) {
            if (readValue(PASSWORD_PATH, KEY_PASSWORD) != null) {
                return true;
            }
        }
        return false;
    }

  public static boolean isValidPassword(String paramString)
  {
    return (TextUtils.isEmpty(paramString)) || (!paramString.matches("[\\w\\d]{4,}"));
  }

    public class PasswordModel implements Serializable {
        private static final long serialVersionUID = 7028230093972511280L;
        public PasswordModel(String password) {
            mPassowrd = password;
        }
    }
    public static String readValue( String key) {
       return readValue(PASSWORD_PATH, key);
    }
    public static String readValue(String filePath, String key) {

        /* SPRD: CID 108995 : Resource leak (RESOURCE_LEAK) @{ */
        Properties props = new Properties();
        InputStream ips = null;
        String value = null;

        try {
            ips = new BufferedInputStream(new FileInputStream(filePath));
            props.load(ips);
            value = props.getProperty(key);
        } catch (Exception e) {
            Log.d(TAG, "readValue failed :" + e);
            value = null;
        }

        try {
            if(ips != null) {
                ips.close();
            }
        } catch (Exception e){
            Log.d(TAG, "close failed :" + e);
        }

        return value;
        /**
        Properties props = new Properties();
        try {
            InputStream ips = new BufferedInputStream(new FileInputStream(filePath));
            props.load(ips);
            String value = props.getProperty(key);
            return value;
        } catch (Exception e) {
            Log.d(TAG, "readValue failed :" + e);
            return null;
        }
        */
        /* @} */
    }
    public static void saveFile(String parameterName, String parameterValue) {
        writeProperties(PASSWORD_PATH, parameterName, parameterValue);
    }
    public static void writeProperties(String filePath, String parameterName, String parameterValue) {
        /* SPRD: CID 109006 : Resource leak (RESOURCE_LEAK)
         * CID 109008 : Resource leak (RESOURCE_LEAK) @{ */
        Properties prop = new Properties();
        InputStream fis = null;
        OutputStream fos = null;
        try {
            fis = new FileInputStream(filePath);
            prop.load(fis);
            fos = new FileOutputStream(filePath);
            prop.setProperty(parameterName, parameterValue);
            prop.store(fos, "Update '" + parameterName + "' value");
        } catch (IOException e) {
            Log.d(TAG, "Visit " + filePath + " for updating " + parameterName + " value error");
        }

        try {
            if(fis != null) {
                fis.close();
            }
            if(fos != null) {
                fos.close();
            }
        } catch (Exception e){
            Log.d(TAG, "close failed :" + e);
        }
        /* @} */

        /**
        Properties prop = new Properties();
        try {
            InputStream fis = new FileInputStream(filePath);
            prop.load(fis);
            OutputStream fos = new FileOutputStream(filePath);
            prop.setProperty(parameterName, parameterValue);
            prop.store(fos, "Update '" + parameterName + "' value");
        } catch (IOException e) {
            Log.d(TAG, "Visit " + filePath + " for updating " + parameterName + " value error");
        }
        */

    }

    public static String MD5Encode(String pwd) {
        MessageDigest digest;
        try {
            digest = MessageDigest.getInstance("MD5");
            byte[] result = digest.digest(pwd.getBytes());
            StringBuilder sb = new StringBuilder();
            for (byte b : result) {
                int data = (b & 0xff);
                String str = Integer.toHexString(data);
                if (str.length() == 1) {
                    sb.append("0");
                }
                sb.append(str);
            }
            return sb.toString();
        } catch (NoSuchAlgorithmException e) {
            e.printStackTrace();
            return "";
        }
    }
/*    public static void writePropertie(String filePath, String paraKey, String paraValue) {

        Properties props = new Properties();
        try {
            OutputStream ops = new FileOutputStream(filePath);
            props.setProperty(paraKey, paraValue);
            props.store(ops, "set");
        } catch (IOException e) {
            Log.d(TAG, "writePropertie failed :" + e);
        }

    }*/
}