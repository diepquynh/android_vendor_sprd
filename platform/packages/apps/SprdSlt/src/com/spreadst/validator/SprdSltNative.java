package com.spreadst.validator;

import android.util.Log;


public class SprdSltNative {
	
	static{
		try{
			System.loadLibrary("sprdslt_jni");
		}catch(UnsatisfiedLinkError e) {
			Log.e("SprdSltNative","SprdSlt loadjni fail");
			e.printStackTrace();
		}
	}
	
    /**
     * send AT cmd to modem
     *
     * @return (String: the return value of send cmd, "OK":sucess, "ERROR":fail)
     */
    public static native String native_sendATCmd(int phoneId, String cmd);
	

}
