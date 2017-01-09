package com.sprd.appbackup;

import android.graphics.drawable.Drawable;

public class AppInfo {
    //Fix bug 291712, do not checkAllAppData while first onResume.
    /*for bug 388383,cmcc new req,need default select all*/
    private boolean checked = true;

    private Drawable icon = null;

    private String name = null;

    private String packageName = null;

    private String packagePath = null;

    private int versionCode = -1;

    private String versionName = null;

    private String sourceDir = null;

    private String apkFileName = null;

    private long apkSize = 0;

    public String toString(){
        StringBuffer sb = new StringBuffer();
        sb.append("packageName = "+packageName);
        sb.append("/packagePath = "+packagePath);
        sb.append("/checked = "+checked);
        sb.append("/name = "+name);
        sb.append("/versionName = "+versionName);
        sb.append("/sourceDir = "+sourceDir);
        sb.append("/apkFileName = " + apkFileName);
        return sb.toString();
    }

    public String getSourceDir() {
        return sourceDir;
    }

    public void setSourceDir(String sourceDir) {
        this.sourceDir = sourceDir;
    }

    public boolean isChecked() {
        return checked;
    }

    public void setChecked(boolean checked) {
        this.checked = checked;
    }

    public Drawable getIcon() {
        return icon;
    }

    public void setIcon(Drawable icon) {
        this.icon = icon;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public String getPackageName() {
        return packageName;
    }

    public void setPackageName(String packageName) {
        this.packageName = packageName;
    }

    public String getPackagePath() {
        return packagePath;
    }

    public void setPackagePath(String packagePath) {
        this.packagePath = packagePath;
    }

    public int getVersionCode() {
        return versionCode;
    }

    public void setVersionCode(int versionCode) {
        this.versionCode = versionCode;
    }

    public String getVersionName() {
        return versionName;
    }

    public void setVersionName(String versionName) {
        this.versionName = versionName;
    }

    public void setApkSize(long size) {
        this.apkSize = size;
    }

    public long getApkSize() {
        return apkSize;
    }

    public String getApkFileName() {
        return apkFileName;
    }

    public void setApkFileName(String apkFileName) {
        this.apkFileName = apkFileName;
    }

    @Override
    public int hashCode() {
        // TODO Auto-generated method stub
        final int prime = 31;
        int result = 1;
        result = prime *result + ((null == icon) ? 0 : icon.hashCode());
        result = prime *result + ((null == name) ? 0 : name.hashCode());
        result = prime *result + ((null == packageName) ? 0 : packageName.hashCode());
        result = prime *result + ((null == packagePath) ? 0 : packagePath.hashCode());
        result = prime *result + ((null == versionName) ? 0 : versionName.hashCode());
        result = prime *result + ((null == sourceDir) ? 0 : sourceDir.hashCode());
        result = prime *result + ((null == apkFileName) ? 0 : apkFileName.hashCode());
        result = prime *result + versionCode;
        result = prime *result + (int)apkSize;
        result = prime *result + (checked ? 1 : 0);

        return result;
    }

    @Override
    public boolean equals(Object o) {
        // TODO Auto-generated method stub
        if (this == o) {
            return true;
        }

        if (null == o) {
            return false;
        }

        if (!(o instanceof AppInfo)) {
            return false;
        }

        AppInfo compareObj = (AppInfo)o;

        if (!(this.apkFileName.equals(compareObj.getApkFileName())))
            return false;

        if (!(this.packageName.equals(compareObj.getPackageName())))
            return false;

        if (!(this.versionCode == compareObj.getVersionCode()))
            return false;

        return true;
    }

}
