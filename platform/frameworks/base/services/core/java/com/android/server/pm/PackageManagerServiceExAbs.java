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

package com.android.server.pm;

import android.content.ComponentName;
import android.content.Intent;
import android.content.pm.IPackageManager;
import android.content.pm.PackageInfo;
import android.content.pm.ParceledListSlice;
import android.os.RemoteException;

/**
 * Keep track of all those .apks everywhere.
 *
 * This is very central to the platform's security; please run the unit
 * tests whenever making modifications here:
 *
mmm frameworks/base/tests/AndroidTests
adb install -r -f out/target/product/passion/data/app/AndroidTests.apk
adb shell am instrument -w -e class com.android.unit_tests.PackageManagerTests com.android.unit_tests/android.test.InstrumentationTestRunner
 *
 * {@hide}
 */
public abstract class PackageManagerServiceExAbs extends IPackageManager.Stub {
    static final String TAG = "PackageManagerServiceExAbs";

    @Override
    public void setComponentEnabledSettingForSetupMenu(ComponentName componentName,
        int flags, int userId ,Intent attr){
        throw new UnsupportedOperationException();
    }

    @Override
    public void setComponentEnabledSettingForSpecific(ComponentName componentName,
            int newState, int flags, int userId ,Intent attr){
        throw new UnsupportedOperationException();
    }
    @Override
    public int backupAppData(String pkgName, String destDir){
        throw new UnsupportedOperationException();
    }
    @Override
    public int restoreAppData(String sourceDir, String pkgName) {
       throw new UnsupportedOperationException();
    }
    @Override
    public int movePrimaryEmulatedStorage(String volumeUuid) throws RemoteException {
       throw new UnsupportedOperationException();
    }
    @Override
    public ParceledListSlice<PackageInfo> getPackageFeatureList(String pkgName) {
       throw new UnsupportedOperationException();
    }
}
