package com.android.insertdata.smstest.entity;

public class SingleParam {
	private static SingleParam instance = null;
	
	public static synchronized SingleParam getInstance() {
		if(instance == null)
		{
			instance = new SingleParam();
		}
		return instance;
	}
	public static boolean threadSwitch = true;
	public static long time;
	public static int status;
	
}