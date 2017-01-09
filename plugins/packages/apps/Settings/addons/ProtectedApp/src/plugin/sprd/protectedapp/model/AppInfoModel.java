package plugin.sprd.protectedapp.model;

public class AppInfoModel {
    private String packageName;
    private String enable;
    private String userID;
    private long memoryInfo;

    public long getMemoryInfo() {
        return memoryInfo;
    }

    public void setMemoryInfo(long memoryInfo) {
        this.memoryInfo = memoryInfo;
    }

    public String getPackageName() {
        return packageName;
    }

    public void setPackageName(String packageName) {
        this.packageName = packageName;
    }

    public String getEnable() {
        return enable;
    }

    public void setEnable(String enable) {
        this.enable = enable;
    }

    public String getUserID() {
        return userID;
    }

    public void setUserID(String userID) {
        this.userID = userID;
    }
}