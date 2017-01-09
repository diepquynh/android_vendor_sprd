/*
 * Copyright (C) 2006 The Android Open Source Project
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
 * limitations under the License.
 */

package android.app;

import android.annotation.Nullable;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.IPackageManager;
import android.content.pm.PackageManager;
import android.content.pm.PackageInfo;
import android.content.pm.ParceledListSlice;
import android.os.RemoteException;
import android.os.storage.StorageManager;
import android.os.storage.VolumeInfo;

import java.util.Collections;
import java.util.List;
import com.android.internal.util.Preconditions;

/**
 * Class for retrieving various kinds of information related to the application
 * packages that are currently installed on the device.
 *
 * You can find this class through {@link Context#getPackageManager}.
 */
/** {@hide} */
public abstract class PackageManagerEx extends PackageManager {
    /** {@hide} */
    protected final ContextImpl mContext;
    /** {@hide} */
    protected final IPackageManager mPM;
    /** {@hide} */
    protected PackageManagerEx(ContextImpl context,IPackageManager pm){
        mContext = context;
        mPM = pm;
    }
    /** {@hide} */
    public void setComponentEnabledSettingForSetupMenu(ComponentName componentName,
        int flags, Intent attr) {
        try {
            mPM.setComponentEnabledSettingForSetupMenu(componentName,flags,mContext.getUserId(), attr);
        } catch (RemoteException e) {
            // Should never happen!
        }
    }
    /** {@hide} */
    public void setComponentEnabledSettingForSpecific(ComponentName componentName,
            int newState, int flags, Intent attr) {
        try {
            mPM.setComponentEnabledSettingForSpecific(componentName, newState, flags,
                    mContext.getUserId(), attr);
        } catch (RemoteException e) {
            // Should never happen!
        }
    }

    /** {@hide} */
    public int movePrimaryEmulatedStorage(VolumeInfo vol) {
        try {
            final String volumeUuid;
            if (VolumeInfo.ID_PRIVATE_INTERNAL.equals(vol.id)) {
                volumeUuid = StorageManager.UUID_PRIVATE_INTERNAL;
            } else if (vol.isPrimaryPhysical()) {
                volumeUuid = StorageManager.UUID_PRIMARY_PHYSICAL;
            } else {
                volumeUuid = Preconditions.checkNotNull(vol.fsUuid);
            }

            return mPM.movePrimaryEmulatedStorage(volumeUuid);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
    }

    /** {@hide} */
    public @Nullable VolumeInfo getPrimaryEmulatedStorageCurrentVolume() {
        final StorageManager storage = mContext.getSystemService(StorageManager.class);
        final String volumeUuid = storage.getPrimaryEmulatedStorageUuid();
        return storage.findVolumeByQualifiedUuid(volumeUuid);
    }

    public static final int BACKUPAPP_SUCCEED = 0;
    public static final int BACKUPAPP_FAILED_UNKNOWN = -1;
    /*
     * restore app, the srcDir is neither exist nor a folder
     */
    public static final int BACKUPAPP_FILE_NOTFOUND = -2;
    /*
     * backup app don't install
     */
    public static final int BACKUPAPP_PKG_DONINSTALL = -3;

    /** {@hide} */
    public int backupAppData(String pkgName, String destDir) {
        try {
            return mPM.backupAppData(pkgName, destDir);
        } catch (RemoteException e) {
            // Should never happen!
            return BACKUPAPP_FAILED_UNKNOWN;
        }
    }

    /** {@hide} */
    public int restoreAppData(String srcDir, String pkgName) {
        try {
            return mPM.restoreAppData(srcDir, pkgName);
        } catch (RemoteException e) {
            // Should never happen!
            return BACKUPAPP_FAILED_UNKNOWN;
        }
    }
    /** {@hide} */
    public List<PackageInfo> getPackageFeatureList(String pkgName) {
        try {
            ParceledListSlice<PackageInfo> parceledList =
                    mPM.getPackageFeatureList(pkgName);
            if (parceledList == null) {
                return Collections.emptyList();
            }
            return parceledList.getList();
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
    }
}
