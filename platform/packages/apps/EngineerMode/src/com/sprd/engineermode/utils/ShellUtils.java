package com.sprd.engineermode.utils;

import java.io.BufferedOutputStream;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import android.util.Log;

public class ShellUtils { 
    static final String TAG = "ShellUtils";

    public static synchronized String run(String[] cmd) { 
        String line = ""; 
        String mResult = "";
        String proNum = "";
        String lstproNum = "";
        int topNum = 5;
        InputStream is = null; 
        try { 
            Runtime runtime = Runtime.getRuntime(); 
            Process proc = runtime.exec(cmd); 
            is = proc.getInputStream(); 
            BufferedReader buf = new BufferedReader(new InputStreamReader(is)); 
            do { 
                line = buf.readLine(); 
                if (line.startsWith("  "+"PID")) { 
                    break;
                } 
            } while (true); 

            do {
                mResult = mResult+"\n"+line;
                line = buf.readLine();
                if (line == null) {
                    break;
                }
                proNum = getTrim(line);
                if (!proNum.equals(lstproNum)){
                    topNum--;
                    if (topNum == -1){
                        break;
                    }
                }
                lstproNum = proNum;
            } while (true);

            if (is != null) { 
                buf.close(); 
                is.close(); 
            } 
        } catch (IOException e) { 
            e.printStackTrace(); 
        } 
        return mResult; 
    } 

    private static String getTrim(String str){
        int startIndex=getStartIndex(str);
        int lastIndex=getLastIndex(str);
        return str.substring(startIndex,lastIndex+1);
    }

    private static int getStartIndex(String str){
        int index=0;
        do {
            if(index >= str.length() || str.charAt(index)!=' '){
                break;
            }
            index++;
        }while(true);
        return index;
    }

    private static int getLastIndex(String str){
        int index=0;

        do {
            if(index >= str.length() || (str.charAt(index)!=' '&&str.charAt(index+1)==' ')){
                break;
            }
            index++;
        }while(true);

        return index;
    }

    public static synchronized String sortThr(String[] cmd) { 
        String line = ""; 
        String mResult = "";
        String proNum = "";
        String lstproNum = "";
        String[] result = new String[3];
        int topNum = -1;    
        InputStream is = null; 
        try { 
            Runtime runtime = Runtime.getRuntime(); 
            Process proc = runtime.exec(cmd); 
            is = proc.getInputStream(); 
            BufferedReader buf = new BufferedReader(new InputStreamReader(is)); 
            do { 
                line = buf.readLine(); 
                if (line.startsWith("  "+"PID")) { 
                    break;
                } 
            } while (true); 

            do {
                line = buf.readLine();
                if (line == null) {
                    break;
                }
                proNum = getTrim(line);
                if (!proNum.equals(lstproNum)){
                    topNum++;     
                    if (topNum == 3){
                        break;
                    }
                    result[topNum] = proNum;
                }
                lstproNum = proNum;                            
            } while (true);
            mResult = psThr(result);
            if (is != null) { 
                buf.close(); 
                is.close(); 
            } 
        } catch (IOException e) { 
            e.printStackTrace(); 
        } 
        return mResult; 
    } 

    public static synchronized String psThr(String[] result) { 
        String[] PSTHR = {"/system/bin/top", "-n", "1","-t"};
        String[] sortResult = new String[3];
        String thrInfo0 = "";
        String thrInfo1 = "";
        String thrInfo2 = "";
        String thrInfo3 = "";
        String line = ""; 
        String mResult = "";
        String proNum = "";   
        InputStream is = null; 

        if (result != null) {
            sortResult = result;
            for(int i=0;i<=2;i++) {
                Log.d(TAG,"sortResult"+"["+i+"] is "+sortResult[i]);  
            }
        }else{
            return null;
        }
        try { 
            Runtime runtime = Runtime.getRuntime(); 
            Process proc = runtime.exec(PSTHR); 
            is = proc.getInputStream(); 
            BufferedReader buf = new BufferedReader(new InputStreamReader(is)); 
            do { 
                line = buf.readLine(); 
                if (line.startsWith("  "+"PID")) { 
                    thrInfo0 = line;
                    break;
                } 
            } while (true); 

            do {
                line = buf.readLine();
                if (line == null) {
                    break;
                }
                proNum = getTrim(line);
                Log.d(TAG,"porNum is "+proNum);
                if(proNum.equals(sortResult[0])) {
                    thrInfo1 = thrInfo1+"\n"+line;
                }else if(proNum.equals(sortResult[1])){
                    thrInfo2 = thrInfo2+"\n"+line;
                }else if(proNum.equals(sortResult[2])){
                    thrInfo3 = thrInfo3+"\n"+line;
                }else{
                    continue;
                }                                           
            } while (true);

            mResult = thrInfo0+thrInfo1+thrInfo2+thrInfo3;

            if (is != null) { 
                buf.close(); 
                is.close(); 
            } 
        } catch (IOException e) { 
            e.printStackTrace(); 
        } 
        return mResult; 
    } 

    /** BEGIN BUG547014 zhijie.yang 2016/05/09 SPRD:add mipi log function **/
    public static boolean writeToFile(String fileName,String str) {
        FileOutputStream fos = null;
        BufferedOutputStream bos = null;
        File file = new File(fileName);
        if(!file.exists()) {
            Log.d(TAG,"the file is not exists");
            return false;
        }
        try {
            fos = new FileOutputStream(file);
            bos = new BufferedOutputStream(fos);
            byte[] bytes = str.getBytes();
            bos.write(bytes);
        } catch (Exception e) {
            e.printStackTrace();
            Log.d(TAG,"Exception: " + e.toString());
            return false;
        } finally {
            if (bos != null) {
                try {
                    bos.close();
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
            if (fos != null) {
                try {
                    fos.close();
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        }
        return true;
    }
    /** END BUG547014 zhijie.yang 2016/05/09 SPRD:add mipi log function **/
}
