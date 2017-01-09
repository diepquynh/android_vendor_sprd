package com.sprd.hz.selfportrait.util;

import android.content.Context;
public class ContextUtil{
	private static ContextUtil instance;
	public Context appContext = null;
	public byte[] data = null;

    public static ContextUtil getInstance() {
        if (instance == null) {
            instance = new ContextUtil();
        }
        return instance;
    }

    private ContextUtil() {
    }
    public Context getContext(){
        return appContext;
    }
}
