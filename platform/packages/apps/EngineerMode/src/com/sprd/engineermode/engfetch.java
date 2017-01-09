package com.sprd.engineermode;

//import android.preference.PreferenceActivity;
import android.util.Log;

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;

public class engfetch  {
    
    private static final String TAG = "engfetch";
    
    static {
    	// The runtime will add "lib" on the front and ".o" on the end of
    	// the name supplied to loadLibrary.
        System.loadLibrary("engappjni");
    }
/*    
    public int engopen() throws IOException {
    	int result = engf_open();
    	if (result >=0){
    		return result;
    	} else {
    		throw new IOException("engopen result=0x" + Integer.toHexString(result));
    	}
    }
    public void engclose(int fd){
    	engf_close(fd);
    }
    //get from writeEntityData function
    public int engwrite(int fd, byte[] data, int dataSize) throws IOException {
        int result = engf_write(fd, data, dataSize);
        if (result >= 0) {
            return result;
        } else {
            throw new IOException("engwrite result=0x" + Integer.toHexString(result));
        }
    }
    
    //get from readEntityData function
    public int engread(int fd, byte[] data, int size) throws IOException {
        if (true) {
            int result = engf_read(fd, data, size);
            if (result >= 0) {
                return result;
            } else {
                throw new IOException("result=0x" + Integer.toHexString(result));
            }
        } else {
            throw new IllegalStateException("engread not read");
        }
    }    
    */
    /**
     * write cmd
     * by wangxiaobin
     */
    public void writeCmd(String cmd){
        int sockid;
        
        sockid = engopen();
        
        ByteArrayOutputStream outputBuffer = new ByteArrayOutputStream();
        DataOutputStream outputBufferStream = new DataOutputStream(outputBuffer);

        /*Modify 20130205 sprd of 125480 change the method of creating cmd start*/
        //String str=String.format("CMD:%s",cmd);
        String str = new StringBuilder().append("CMD:").append(cmd).toString();
        /*Modify 20130205 sprd of 125480 change the method of creating cmd end*/
        try {
            outputBufferStream.writeBytes(str);
        } catch (IOException e) {
            Log.e(TAG, "writebytes error");
           return;
        }
        engwrite(sockid,outputBuffer.toByteArray(),outputBuffer.toByteArray().length);
        Log.d(TAG, "write cmd '"+str+"'");
        engclose(sockid);
    }
    
    public int engopen() {
    	return engopen(0);
    }

    public int engopen(int type) {
    	int result = engf_open(type);
    	if (result >=0){
    		return result;
    	} else {
    		return 0;
    	}
    }
    
    public void engclose(int fd){
    	engf_close(fd);
    }
    //get from writeEntityData function
    public int engwrite(int fd, byte[] data, int dataSize) {
        int result = engf_write(fd, data, dataSize);
        if (result >= 0) {
            return result;
        } else {
            return 0;
        }
    }
    
    //get from readEntityData function
    public int engread(int fd, byte[] data, int size) {
        int result = engf_read(fd, data, size);
        if (result >= 0) {
            return result;
        } else {
            return 0;
        }
    }
    public int enggetphasecheck(byte[] data, int size){
        return  engf_getphasecheck(data, size);
    }
      
    //public native int add(int a, int b);
    private native static int engf_open(int type);
    private native static void engf_close(int sfd);
    private native static int engf_write(int sfd, byte[] data, int size);    
    private native static int engf_read(int sfd, byte[] data, int size);
    private native static int engf_getphasecheck(byte[] data, int size);
}

