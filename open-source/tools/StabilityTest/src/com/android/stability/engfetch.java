package com.android.stability;

import android.util.Log;

public class engfetch {
	private static final String TAG = "StabilityEngfetch";
	
	static{
		try{
			System.loadLibrary("stabilityjni");
		}
		catch(UnsatisfiedLinkError ule)
		{
			Log.d(TAG, "Load stabilityjni failed");
		}
	}

	public int enggetphasecheck(byte[] data, int size){
		return  engf_getphasecheck(data, size);
	}
	
	private native static int engf_getphasecheck(byte[] data, int size);
	public void vibratortest(int size) 
	{  
	    vibrator_test(size);
	}
	private native static void vibrator_test(int size);
	public void vibratorstop(int size) 
	{  
	    vibrator_stop(size);
	}
	private native static void vibrator_stop(int size);
	public void flashoff(int size) 
	{  
	    flash_off(size);
	}
	private native static void flash_off(int size);
	public void flashon(int size) 
	{  
	    flash_on(size);
	}
	private native static void flash_on(int size);
	
    private int mSocketID = -1;
    private int mType = 0;

    public int engopen() {
        return engopen(0);
    }

    public int engopen(int type) {
        int result = -1;
        if (mSocketID >= 0) {
            engclose(mSocketID);
        }
        result = engf_open(type);
        if (result >= 0) {
            mType = type;
            mSocketID = result;
            return result;
        } else {
            return 0;
        }
    }

    public void engclose(int fd) {
        engf_close(mSocketID);
        mSocketID = -1;
    }

    public int engwrite(int fd, byte[] data, int dataSize) {
        int result = 0;
        if (mSocketID < 0) {
            if (!engreopen()) {
                return 0;
            }
        }
        int iCount = 0;
        do {
            result = engf_write(mSocketID, data, dataSize);
            if (result >= 0) {
                break;
            }
            if (iCount < 1) {
                if (!engreopen()) {
                    break;
                }
            } else {
                break;
            }
            iCount++;
        } while (true);
        return result;
    }

    public int engread(int fd, byte[] data, int size) {
        int result = 0;
        if (mSocketID < 0) {
            if (!engreopen()) {
                return 0;
            }
        }
        int iCount = 0;
        do {
            result = engf_read(mSocketID, data, size);
            if (result >= 0) {
                break;
            }
            if (iCount < 1) {
                if (!engreopen()) {
                    break;
                }
            } else {
                break;
            }
            iCount++;
        } while (true);
        return result;
    }

    private boolean engreopen() {
        int result = engf_open(mType);
        if (result >= 0) {
            mSocketID = result;
        }
        Log.e(TAG, "engreopen: " + (result >= 0));
        return (result >= 0);
    }

    // native
    private native static int engf_open(int type);

    private native static void engf_close(int sfd);

    private native static int engf_write(int sfd, byte[] data, int size);

    private native static int engf_read(int sfd, byte[] data, int size);
}
