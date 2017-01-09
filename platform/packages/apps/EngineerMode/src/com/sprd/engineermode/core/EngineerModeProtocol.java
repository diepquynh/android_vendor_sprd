package com.sprd.engineermode.core;

import android.util.Log;
import com.sprd.engineermode.core.SlogCore;
import com.sprd.engineermode.utils.IATUtils;
import com.sprd.engineermode.utils.SocketUtils;
import com.sprd.engineermode.utils.CommonUtils;

import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.lang.reflect.Method;

/**
 * Created by SPREADTRUM\zhengxu.zhang on 9/9/15.
 */
public class EngineerModeProtocol {

    private static final int OPEN_CP2 = 0;
    private static final int GET_ARM_LOG = 1;
    public static final int SET_ARM_LOG_OPEN = 2;
    public static final int SET_ARM_LOG_CLOSE = 3;
    public static final int GET_DSP_LOG = 4;
    public static final int SET_DSP_LOG = 5;
    private static final int GET_CAP_LOG = 6;
    public static final int SET_CAP_LOG_OPEN = 7;
    public static final int SET_CAP_LOG_CLOSE = 8;
    private static final int GET_AUDIO_LOG = 9;
    private static final int SET_AUDIO_LOG_OPEN = 10;
    private static final int SET_AUDIO_LOG_CLOSE = 11;
    private static final int GET_CP2_LOG = 12;
    private static final int SET_CP2_LOG_OPEN = 13;
    private static final int SET_CP2_LOG_CLOSE = 14;
    private static final int MEMORY_LEAK = 15;
    private static final int SET_LOG_SCENARIOS_STATUS = 16;
    private static final int SET_LOG_OUTPUT_STYLE = 17;
    private static final int SET_SAVE_SLEEPLOG = 18;
    private static final int SET_SAVE_RINGBUF = 19;
    private static final int GET_ENABLE_DUMP_MARLIN = 20;
    private static final int SET_ENABLE_DUMP_MARLIN_OPEN = 21;
    private static final int SET_ENABLE_DUMP_MARLIN_CLOSE = 22;
    private static final int SET_DUMP_MARLIN_MEM = 23;

    private static final String TAG = "EngineerModeProtocol";
    private static final String PACKAGE_NAME = "com.sprd.engineermode.core";

    public static String sendCommand(String name,String arg){

        try {
        	Log.d(TAG, String.format("cmd name:%s \t arg: %s", name,arg));
        	String className = name.split("_")[0];
        	Log.d(className, String.format("package: %s \t class: %s \t arg: %s",PACKAGE_NAME, name, arg));
            Class classType = Class.forName(String.format("%s.%s", PACKAGE_NAME,className));
            
            Log.d(className,"1"+classType.toString());
            Method method = classType.getMethod(name,String.class);
            Log.d(className,"2"+method.toString());
            Object result = method.invoke(classType.newInstance(),arg);
            Log.d(className,"3"+result.toString());

        return "";
        }catch(Exception e){
            Log.d(TAG,String.format("cmd name: [%s] arg: [%s] has exception[%s]",name,arg,e.toString()));
        }finally {
            return "";
        }
    }

    public static String sendAt(String cmd, String serverName) {
        if(!CommonUtils.isModemATAvaliable()) {
            Log.d(TAG, "modem at not Aviable, return");
            return IATUtils.AT_FAIL;
        }
        String strTmp = IATUtils.sendATCmd(cmd, serverName);
        return strTmp;
    }



    public static String analysisResponse(String response, int type) {
        Log.d(TAG, "analysisResponse response= " + response + "type = " + type);
        if (response != null && response.contains(IATUtils.AT_OK)) {
            if (type == GET_ARM_LOG || type == GET_CAP_LOG
                    || type == GET_DSP_LOG) {
                String[] str = response.split("\n");
                String[] str1 = str[0].split(":");
                Log.d(TAG, type + "  " + str1[1]);
                return str1[1].trim();
            } else if (type == SET_ARM_LOG_CLOSE || type == SET_ARM_LOG_OPEN
                    || type == SET_CAP_LOG_CLOSE || type == SET_CAP_LOG_OPEN
                    || type == SET_AUDIO_LOG_CLOSE
                    || type == SET_AUDIO_LOG_OPEN || type == SET_DSP_LOG) {
                return IATUtils.AT_OK;
            }
        }

        if (type == GET_CP2_LOG || type == SET_CP2_LOG_OPEN
                || type == SET_CP2_LOG_CLOSE) {
            if (response != null && !response.startsWith("Fail")) {
                if (type == GET_CP2_LOG) {
                    if (response.contains("FAIL")) {
                        return IATUtils.AT_FAIL;
                    } else {
                        String[] str1 = response.split(":");
                        Log.d(TAG, type + "  " + str1[1]);
                        return str1[1].trim();
                    }
                } else if (type == SET_CP2_LOG_OPEN
                        || type == SET_CP2_LOG_CLOSE) {
                    return IATUtils.AT_OK;
                }
            }
        }
        return IATUtils.AT_FAIL;

    }

    public static String resultBuilder(String result){
        if(result.equals("1"))
            Log.d(TAG,"success");
        else if(result.equals("0"))
            Log.d(TAG, "fail");
        Log.d(TAG, "finish");
        return result;
    }

    public static String resultBuilder(boolean result){
        Log.d(TAG, "finish");
        if(result)return "1";
        return "0";
    }

    public static String resultBuilder(int result){
        Log.d(TAG, "finish");
        return String.valueOf(result);
    }

    public void writeFileData(String fileName,String message){
        FileOutputStream fout;
        try{
            fout = new FileOutputStream(fileName);//openFileOutput(fileName,MODE_PRIVATE);
            byte[] bytes = message.getBytes();
            fout.write(bytes);
            fout.close();
        }catch(Exception e){
            Log.d(TAG, e.toString());
        }

    }

    /*
    public String readFileData(String fileName){
        String res = "";
        FileInputStream fin;
        try{
            fin = new FileInputStream(fileName);//openFileInput(fileName,MODE_PRIVATE);
            int length = fin.available();
            byte[] buffer = new byte[length];
            fin.read(buffer);
            res = EncodingUtils.getString(buffer, "UTF-8");
            fin.close();

        }catch(Exception e){
            Log.d(TAG, e.toString());

        }finally{

        }

        return res;
    }*/




}
