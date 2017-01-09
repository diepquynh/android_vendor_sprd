package com.android.insertdata.addmms;

public class SingleMmsParam {
	private static SingleMmsParam instance = null;
	
	public static synchronized SingleMmsParam getInstance() {
		if(instance == null)
		{
			instance = new SingleMmsParam();
		}
		return instance;
	}
	public static boolean threadSwitch = true;
	public static long time;
	public static long finishCount;
	
}