/*
 * Copyright (C) 2010 he Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License
 */

package addon.sprd.browser.storage;


import com.sprd.common.StorageAddonStub;

import android.content.Context;
import android.app.AddonManager;
import android.os.EnvironmentEx;
import java.io.File;
import android.util.Log;

public class AddOnStoragePlugIn extends StorageAddonStub implements AddonManager.InitialCallback {

    private Context mContext;
    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mContext = context;
        return clazz;
    }

    public boolean useEnvironmentEx(){
        return true;
    }

    public File getInternalStoragePath() {
        return EnvironmentEx.getInternalStoragePath();
    }

    public String getInternalStoragePathState(){
        return EnvironmentEx.getInternalStoragePathState();
    }

    public File getExternalStoragePath(){
        return EnvironmentEx.getExternalStoragePath();
    }

    public String getExternalStoragePathState(){
        return EnvironmentEx.getExternalStoragePathState();
    }

    public File[] getUsbdiskVolumePaths(){
        return EnvironmentEx.getUsbdiskVolumePaths();
    }

    public String getUsbdiskVolumeState(File path){
        return EnvironmentEx.getUsbdiskVolumeState(path);
    }
}
