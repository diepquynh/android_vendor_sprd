package com.sprd.engineermode.utils;

import java.io.*;  

import android.util.Log;

public class EMFileUtils {
    private static final String TAG = "FileUtils";

    public static void write(String path, String content ,boolean flag) {
        String readS = new String();
        String writeS = new String();

        try {
            File f = new File(path);
            if (f.exists()) {
                Log.d(TAG,"File is exists");
                FileWriter writer = new FileWriter(path, flag);
                writer.write(content);
                writer.close();
            } else {
                Log.d(TAG,"file is not exist, creating...");
                if (f.createNewFile()) {
                    Log.d(TAG,"creat success！");
                    FileWriter writer = new FileWriter(path, flag);
                    writer.write(content);
                    writer.close();
                } else {
                    Log.d(TAG,"creat fail！");
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public static String read(String file) {  
        String s = null;  
        String result = new String();  
        File f = new File(file);  
        if (f.exists()) {  
            try {  
                BufferedReader br =  
                        new BufferedReader(new InputStreamReader(new FileInputStream(f)));  
                while ((s = br.readLine()) != null) {  
                    result=result+s;  
                }  
                return result;
            } catch (Exception e) {  
                e.printStackTrace();
                return null;
            }  
        } else{ 
            Log.d(TAG,"the file is not exists");
            return null;
        }  
    }  

    public static void newFolder(String folderPath){
        String filePath = folderPath.toString();   
        java.io.File myFilePath = new java.io.File(filePath);
        try{
            if(myFilePath.isDirectory()){
                Log.d(TAG,"the directory is exists!");
            }else{
                myFilePath.mkdir();
                Log.d(TAG,"create directory success");
            }
        }catch(Exception e)
        {
            Log.d(TAG,"create directory fail");
            e.printStackTrace();
        }
    }

    public static boolean deleteFolder(String _filePath){ 
        java.io.File folder = new java.io.File(_filePath);
        boolean  result  =   false ;
        try {
            String childs[] = folder.list(); 
            if(childs == null || childs.length <= 0){ 
                if (folder.delete()){
                    result  =   true ;
                }
            } else {
                for( int i = 0 ; i < childs.length; i ++ ){ 
                    String  childName = childs[i]; 
                    String  childPath = folder.getPath() + File.separator + childName; 
                    File  filePath = new File(childPath); 
                    if(filePath.exists() && filePath.isFile())    {
                        if (filePath.delete()) {
                            result  =   true ;
                        }else {
                            result  =   false ;
                            break ;
                        }
                    }else if(filePath.exists() && filePath.isDirectory()){ 
                        if (deleteFolder(filePath.toString())) {
                            result  =   true ;
                        }else {
                            result  =   false ;
                            break ;
                        }
                    }  
                }
            }
            folder.delete(); 
        } catch (Exception e) {
            e.printStackTrace();
            result  =   false ;
        }
        return  result;
    }

    public static boolean isFileDirExits(String folderPath){
        String filePath = folderPath.toString();   
        java.io.File myFilePath = new java.io.File(filePath);
        if (myFilePath.isDirectory()){
            return true;
        } else {
            return false;
        }    
    }

    public static void chmodeFile(int mode,String filePath){
        String cmd = "chmode mode filePath";
        try{
            Runtime runtime = Runtime.getRuntime(); 
            Process proc = runtime.exec(cmd);  
        }catch (IOException e) { 
            e.printStackTrace(); 
        }

    }
}
