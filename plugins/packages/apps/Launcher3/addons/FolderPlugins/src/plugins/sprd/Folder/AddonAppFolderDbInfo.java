package plugins.sprd.Folder;

import java.util.ArrayList;

import com.android.launcher3.FolderInfo;
import com.android.launcher3.Folderplugins.AppDbInfo;
import com.android.launcher3.Folderplugins.AppFolderDbInfo;
import android.app.AddonManager;
import android.content.Context;
import android.util.Log;

public class AddonAppFolderDbInfo extends AppFolderDbInfo implements AddonManager.InitialCallback {

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        return clazz;
    }

    public void addChild(AppDbInfo item) {
        children.add(item);
    }

    public boolean removeChild(AppDbInfo item) {
        return children.remove(item);
    }

    public AppDbInfo removeChild(int index) {
        AppDbInfo dbInfo = children.remove(index);
        folderInfo.remove(dbInfo.appInfo);
        return dbInfo;
    }

    public AppDbInfo getChild(int index) {
        return children.get(index);
    }
}