package com.sprd.fileexplorer;

import com.sprd.fileexplorer.drmplugin.FileExplorerAppUtils;
import com.sprd.fileexplorer.util.FileSort;
import com.sprd.fileexplorer.util.FileType;
import com.sprd.fileexplorer.util.NotifyManager;

import android.app.Application;
import android.content.Context;
import android.util.Log;

public class FileExplorerApp extends Application {

    @Override
    public void onCreate() {
        super.onCreate();
        FileExplorerAppUtils.getInstance().getContext();
        FileType.init(this);
        NotifyManager.init(this);
        FileSort.getFileListSort().setSortType(FileSort.SORT_BY_NAME);
    }

}
