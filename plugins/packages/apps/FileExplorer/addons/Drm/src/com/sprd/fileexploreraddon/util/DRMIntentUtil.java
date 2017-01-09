package com.sprd.fileexploreraddon.util;

import java.io.File;

import com.sprd.fileexplorer.util.IntentUtil;

import android.content.Context;
import android.content.Intent;
import android.util.Log;
public class DRMIntentUtil {

    public static Intent getIntentByFileType(Context context, int fileType, File file) {
        if (!file.exists() || !file.canRead()) {
            return null;
        }
        Intent intent = null;
        if (fileType == DRMFileType.FILE_TYPE_DRM) {
            intent = IntentUtil.getIntent(file, "image/*");
        }
        return intent;
    }

}
