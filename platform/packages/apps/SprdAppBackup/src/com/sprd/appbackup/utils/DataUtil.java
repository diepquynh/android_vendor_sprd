package com.sprd.appbackup.utils;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FilenameFilter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.RemoteException;
import android.util.Log;

import com.sprd.appbackup.DataItem;
import com.sprd.appbackup.service.Agent;
import com.sprd.appbackup.service.Category;
import com.sprd.appbackup.service.Config;
import com.sprd.appbackup.service.Account;
import com.sprd.appbackup.service.IAppBackupManager;

public class DataUtil {
    public static final String VMSG = ".vmsg";
    public static final String PDU = ".pdu";
    public static final String DATA_FOLDER = "DataFolder";
    public static final String TAG = "DataUtil";
    //SPRD 599796
    private static final String MESSAGE_PACKAGE_NAME = "com.android.messaging";

    public static List<DataItem> getEnabledArchive(Context context, 
            IAppBackupManager manager, Agent[] allAgents, String archivePath) {
        if(context == null || manager == null || allAgents == null || archivePath == null){
            Log.w(TAG, "Some input parameters are null!!!");
            return null;
        }
        List<DataItem> mListData = new ArrayList<DataItem>();
        final List<String> mFolderName = new ArrayList<String>();
        Agent mmsAgent = null;
        for(Agent a:allAgents){
            if(a != null){
                mFolderName.add(a.getAgentName());
                mFolderName.add(a.getAgentName().toLowerCase());
                if(a.getAgentName().equals("Mms")){
                    mmsAgent = a;
                }
            }
        }
        mFolderName.add("Sms");
        mFolderName.add("sms");
        mFolderName.add("contacts");
        Log.i(TAG, "mFolderName=" + mFolderName);
        File rootFolder = new File(archivePath);
        if(!rootFolder.exists() || !rootFolder.isDirectory()){
            Log.w(TAG, "!rootFolder.exists() || !rootFolder.isDirectory(), rootFolder = " + rootFolder);
            return null;
        }
        File[] agentArray = rootFolder.listFiles(new FilenameFilter(){

            @Override
            public boolean accept(File dir, String filename) {
                if(dir.isDirectory() && mFolderName.contains(filename)){
                    return true;
                }
                return false;
            }
        });
        DataItem data = null;
        if (agentArray != null) {
            for (File arch : agentArray) {
                if (arch != null && arch.isDirectory()) {
                    Log.i(TAG, "fileName=" + arch.getName());
                    Log.d(TAG, "arch.isDirectory() && arch.list()");
                    File[] subFolders = arch.listFiles();
                    if (subFolders != null && subFolders.length > 0) {
                        if (arch.getName().equalsIgnoreCase("Sms")) {
                            Category[] cate = null;
                            try {
                                cate = manager.getCategory(mmsAgent);
                            } catch (RemoteException e) {
                                e.printStackTrace();
                            }
                            if (cate != null && cate.length > 0) {
                                for (File message : subFolders) {
                                    if (message.getName().endsWith(VMSG)
                                            || message.getName().startsWith("sms")) {
                                        data = getDataItem(context, manager,
                                                mmsAgent, cate[0].getmCode(),
                                                cate[0].getmDescription());
                                        data.setEnabled(true);
                                        data.setChecked(false);
                                        mListData.add(data);
                                        break;
                                    }
                                }
                            }
                            continue;
                        }
                        for (Agent agent : allAgents) {
                            if (arch.getName().equalsIgnoreCase(agent.getAgentName())
                                    || ("Contact".equals(agent.getAgentName()) && "contacts".equals(arch.getName()))) {
                                Log.d(TAG, "arch.getName().equals(agent.getAgentName()");
                                Category[] cate = null;
                                try {
                                    cate = manager.getCategory(agent);
                                } catch (RemoteException e) {
                                    e.printStackTrace();
                                }
                                if (cate == null) {
                                    Log.d(TAG, "cate == null");
                                    /* SPRD: Modify for bug569406. @{ */
                                    try {
                                        data = getDataItem(context, manager, agent);
                                        data.setEnabled(true);
                                        data.setChecked(false);
                                        mListData.add(data);
                                    } catch (SecurityException e) {
                                        Log.e(TAG, "some Apps lack of Permission");
                                    }
                                    /* @} */
                                } else {
                                    if (cate.length > 1) {
                                        for (File fi : subFolders) {
                                            if (fi.getName().toString()
                                                    .endsWith(PDU)) {
                                                data = getDataItem(
                                                        context,
                                                        manager,
                                                        agent,
                                                        cate[1].getmCode(),
                                                        cate[1].getmDescription());
                                                data.setEnabled(true);
                                                data.setChecked(false);
                                                mListData.add(data);
                                                break;
                                            }
                                        }
                                    }
                                }
                                break;
                            }
                        }
                    }
                }
            }
        }
        return mListData;
    }
    public static DataItem getDataItem(Context context, IAppBackupManager manager, Agent agent, int categoryCode, String cateDescription) {
        DataItem tmpData = new DataItem();
        if(context == null || manager == null || agent == null){
            Log.w(TAG, "Some input parameters are null!!!");
            return tmpData;
        }
        String packageName;
        PackageManager pm = context.getPackageManager();
        PackageInfo info = null;
        boolean isEnable = false;
        packageName = agent.getPackageName();
        isEnable = false;
        List<Account> account = null;
        try {
            isEnable = manager.isEnabled(agent, categoryCode);
            account = manager.getAccounts(agent, categoryCode);
            Log.d(TAG, "getDataItem account = "+account);
        } catch (RemoteException e1) {
            e1.printStackTrace();
        } catch (Exception e){
            e.printStackTrace();
        }
        try {
            info = pm.getPackageInfo(packageName, 0);
        } catch (NameNotFoundException e) {
            e.printStackTrace();
        }
        /* SPRD 599796
         * This method of getDataItem is only used in MMS condition,so set the package name as MMS derectly,
         * after testing, there isn't any problem */
        try {
            if (info != null) {
                tmpData.setAgentName(agent.getAgentName());
                tmpData.setCategoryName(cateDescription);
                tmpData.setCategoryCode(categoryCode);
                // Fix bug 291712, do not checkAllAppData while first onResume.
                /* for bug 388383,cmcc new req,need default select all */
                tmpData.setChecked(isEnable);
                tmpData.setEnabled(isEnable);
                tmpData.setIcon(pm.getApplicationIcon(MESSAGE_PACKAGE_NAME));
            }
        } catch (NameNotFoundException e) {
            e.printStackTrace();
        }
        /* @} */
        if(account != null){
            for(Account a:account){
                a.setChecked(true);
            }
        }
        tmpData.setAccounts(account);
        tmpData.setAgent(agent);
        return tmpData;
    }
    public static DataItem getDataItem(Context context, IAppBackupManager manager, Agent agent) {
        DataItem tmpData = new DataItem();
        if(context == null || manager == null || agent == null){
            Log.w(TAG, "Some input parameters are null!!!");
            return tmpData;
        }
        String packageName;
        PackageManager pm = context.getPackageManager();
        PackageInfo info = null;
        boolean isEnable = false;
        List<Account> account = null;
        try {
            isEnable = manager.isEnabled(agent, 0);
            account = manager.getAccounts(agent, 0);
            Log.d(TAG, "getDataItem account = "+account);
        } catch (RemoteException e1) {
            e1.printStackTrace();
        }
        packageName = agent.getPackageName();
        try {
            info = pm.getPackageInfo(packageName, 0);
        } catch (NameNotFoundException e) {
            e.printStackTrace();
        }
        if (info != null) {
            tmpData.setAgentName(agent.getAgentName());
            tmpData.setCategoryName(pm.getApplicationLabel(info.applicationInfo).toString());
            //Fix bug 291712, do not checkAllAppData while first onResume.
            tmpData.setChecked(isEnable);
            tmpData.setEnabled(isEnable);
            tmpData.setIcon(pm.getApplicationIcon(info.applicationInfo));
        }
        if(account != null){
            for(Account a:account){
                a.setChecked(true);
            }
        }
        tmpData.setAgent(agent);
        tmpData.setAccounts(account);
        return tmpData;
    }

    public static boolean unPackageZip(File sourceFile,String targetPath) {
        Log.d(TAG,"-----------sourceFile = "+sourceFile);
        String Temp = sourceFile.toString();
        String dirName = Temp.substring(Temp.lastIndexOf("/")+1, Temp.lastIndexOf("."));
        targetPath = targetPath + dirName + "/";
        Log.d(TAG,"-----------file.targetPath() = "+targetPath);
        File targetFile;
        File destPath;
        ZipInputStream zis = null;
        ZipEntry zipent = null;
        BufferedOutputStream bos = null;
        byte[] buf = null;
        int bufLen = 2048;
        int count = -1;
        try {
            zis = new ZipInputStream(new BufferedInputStream(new FileInputStream(sourceFile)));
            buf = new byte[bufLen];
            while ((zipent = zis.getNextEntry()) != null) {
                String entName = zipent.getName();
                if (entName == null || entName.trim().equalsIgnoreCase("")) {
                    continue;
                }
                if(entName.startsWith(dirName)){
                    entName = entName.substring(entName.indexOf("/") + 1);
                }
                String outFile = targetPath + entName;
                String outPath = outFile.substring(0, outFile.lastIndexOf(File.separator));
                Log.i(TAG,"entName: " + entName + ",\toutPath: " + outPath);
                destPath = new File(outPath);
                if (!destPath.exists()) {
                    if (!destPath.mkdirs()) {
                        continue;
                    }
                }
                targetFile = new File(outFile);
                if (targetFile.exists()) {
                    continue;
                }
                if (!targetFile.createNewFile()) {
                    continue;
                }

                bos = new BufferedOutputStream(new FileOutputStream(targetFile), bufLen);
                while ((count = zis.read(buf, 0, bufLen)) != -1) {
                    bos.write(buf, 0, count);
                }
                bos.flush();
                try {
                    bos.close();
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
            zis.closeEntry();
        } catch (IOException e) {
            e.printStackTrace();
            return false;
        }finally{
            if(bos!=null){
                try{
                  bos.close();
                }catch(Exception e){
                     e.printStackTrace();
                 }
              }
            if(zis!=null){
                try{
                 zis.close();
                }catch(Exception e){
                     e.printStackTrace();
                 }
              }
           }
        return true;
    }

}
