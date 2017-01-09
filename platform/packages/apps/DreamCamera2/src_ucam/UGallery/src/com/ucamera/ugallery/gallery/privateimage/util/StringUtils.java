package com.ucamera.ugallery.gallery.privateimage.util;
/**
 * Copyright (C) 2010,2013 Thundersoft Corporation
 * All rights Reserved
 */
import android.text.TextUtils;

public class StringUtils {

    public static String trimLeftSlash(String paramString) {
        if (TextUtils.isEmpty(paramString))
            paramString = "";
        else if (paramString.startsWith("/"))
            paramString = paramString.substring(1, paramString.length());
        return paramString;
    }

    public static String trimRightSlash(String paramString) {
        if (TextUtils.isEmpty(paramString))
            paramString = "";
        else if (paramString.endsWith("/"))
            paramString = paramString.substring(0, -1 + paramString.length());
        return paramString;
    }

    public static String trimSlash(String paramString) {
        if (TextUtils.isEmpty(paramString)) {
            paramString = "";
        } else {
            if (paramString.endsWith("/"))
                paramString = paramString.substring(0,
                        -1 + paramString.length());
            if (paramString.startsWith("/"))
                paramString = paramString.substring(1, paramString.length());
        }
        return paramString;
    }
}