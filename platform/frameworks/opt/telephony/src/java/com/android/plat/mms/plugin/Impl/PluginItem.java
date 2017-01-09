
package com.android.plat.mms.plugin.Impl;

import java.io.File;

public class PluginItem {
    public PluginItem(String priority, String jarPath, String className) {
        setPriority(priority);
        setJarPath(jarPath);
        setClassName(className);
    }

    private boolean isEmpty(String szValue) {
        return (szValue == null || szValue.isEmpty());

    }

    private boolean checkFileExist(String szValue) {
        if (isEmpty(szValue)) {
            return false;
        }
        File f = new File(szValue);
        return (f.exists() && f.isFile());
    }

    public int getPriority() {
        return mPriority;
    }

    public void setPriority(String szPriority) {
        if (isEmpty(szPriority)) {
            throw new RuntimeException("Plus Init Priority Error ");
        } else {
            mPriority = Integer.valueOf(szPriority.trim());
        }
    }

    public String getJarPath() {
        return mJarPath;
    }

    public void setJarPath(String jarPath) {
        if (checkFileExist(jarPath)) {
            mJarPath = new String(jarPath.trim());
        } else {
            throw new RuntimeException("Plus Init Jar Path Error ");
        }
    }

    public String getClassName() {
        return mClassName;
    }

    public void setClassName(String szClassName) {
        if (isEmpty(szClassName)) {
            throw new RuntimeException("Plus  Init Class Name  Error ");
        } else {
            mClassName = new String(szClassName.trim());
        }
    }

    public void Debug() {
        System.out.println("<<<-----debug PluginItem begin------------");
        System.out.println("priority =[" + getPriority() + "] " + " jarPath = [" + getJarPath()
                + "] " + " className = [" + getClassName() + "]");
        System.out.println("--------debug PluginItem end----------->>>");
    }

    private int mPriority;
    private String mJarPath = null;
    private String mClassName = null;

}
