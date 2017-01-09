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

import static android.Manifest.permission.READ_EXTERNAL_STORAGE;
import static android.Manifest.permission.WRITE_EXTERNAL_STORAGE;
import static android.Manifest.permission.WRITE_MEDIA_STORAGE;
import static android.content.pm.PackageManager.COMPONENT_ENABLED_STATE_DEFAULT;
import static android.content.pm.PackageManager.COMPONENT_ENABLED_STATE_DISABLED;
import static android.content.pm.PackageManager.COMPONENT_ENABLED_STATE_DISABLED_UNTIL_USED;
import static android.content.pm.PackageManager.COMPONENT_ENABLED_STATE_DISABLED_USER;
import static android.content.pm.PackageManager.COMPONENT_ENABLED_STATE_ENABLED;
import static android.content.pm.PackageManager.FLAG_PERMISSION_GRANTED_BY_DEFAULT;
import static android.content.pm.PackageManager.FLAG_PERMISSION_POLICY_FIXED;
import static android.content.pm.PackageManager.FLAG_PERMISSION_REVOKE_ON_UPGRADE;
import static android.content.pm.PackageManager.FLAG_PERMISSION_SYSTEM_FIXED;
import static android.content.pm.PackageManager.FLAG_PERMISSION_USER_FIXED;
import static android.content.pm.PackageManager.FLAG_PERMISSION_USER_SET;
import static android.content.pm.PackageManager.INSTALL_EXTERNAL;
import static android.content.pm.PackageManager.INSTALL_FAILED_ALREADY_EXISTS;
import static android.content.pm.PackageManager.INSTALL_FAILED_CONFLICTING_PROVIDER;
import static android.content.pm.PackageManager.INSTALL_FAILED_DEXOPT;
import static android.content.pm.PackageManager.INSTALL_FAILED_DUPLICATE_PACKAGE;
import static android.content.pm.PackageManager.INSTALL_FAILED_DUPLICATE_PERMISSION;
import static android.content.pm.PackageManager.INSTALL_FAILED_INSUFFICIENT_STORAGE;
import static android.content.pm.PackageManager.INSTALL_FAILED_INTERNAL_ERROR;
import static android.content.pm.PackageManager.INSTALL_FAILED_INVALID_APK;
import static android.content.pm.PackageManager.INSTALL_FAILED_INVALID_INSTALL_LOCATION;
import static android.content.pm.PackageManager.INSTALL_FAILED_MISSING_SHARED_LIBRARY;
import static android.content.pm.PackageManager.INSTALL_FAILED_PACKAGE_CHANGED;
import static android.content.pm.PackageManager.INSTALL_FAILED_REPLACE_COULDNT_DELETE;
import static android.content.pm.PackageManager.INSTALL_FAILED_SHARED_USER_INCOMPATIBLE;
import static android.content.pm.PackageManager.INSTALL_FAILED_TEST_ONLY;
import static android.content.pm.PackageManager.INSTALL_FAILED_UID_CHANGED;
import static android.content.pm.PackageManager.INSTALL_FAILED_UPDATE_INCOMPATIBLE;
import static android.content.pm.PackageManager.INSTALL_FAILED_USER_RESTRICTED;
import static android.content.pm.PackageManager.INSTALL_FAILED_VERSION_DOWNGRADE;
import static android.content.pm.PackageManager.INSTALL_FORWARD_LOCK;
import static android.content.pm.PackageManager.INSTALL_INTERNAL;
import static android.content.pm.PackageManager.INSTALL_PARSE_FAILED_INCONSISTENT_CERTIFICATES;
import static android.content.pm.PackageManager.INTENT_FILTER_DOMAIN_VERIFICATION_STATUS_ALWAYS;
import static android.content.pm.PackageManager.INTENT_FILTER_DOMAIN_VERIFICATION_STATUS_ASK;
import static android.content.pm.PackageManager.INTENT_FILTER_DOMAIN_VERIFICATION_STATUS_NEVER;
import static android.content.pm.PackageManager.INTENT_FILTER_DOMAIN_VERIFICATION_STATUS_UNDEFINED;
import static android.content.pm.PackageManager.INTENT_FILTER_DOMAIN_VERIFICATION_STATUS_ALWAYS_ASK;
import static android.content.pm.PackageManager.MATCH_ALL;
import static android.content.pm.PackageManager.MOVE_FAILED_DOESNT_EXIST;
import static android.content.pm.PackageManager.MOVE_FAILED_INTERNAL_ERROR;
import static android.content.pm.PackageManager.MOVE_FAILED_OPERATION_PENDING;
import static android.content.pm.PackageManager.MOVE_FAILED_SYSTEM_PACKAGE;
import static android.content.pm.PackageManager.PERMISSION_DENIED;
import static android.content.pm.PackageManager.PERMISSION_GRANTED;
import static android.content.pm.PackageParser.isApkFile;
import static android.os.Process.PACKAGE_INFO_GID;
import static android.os.Process.SYSTEM_UID;
import static android.system.OsConstants.O_CREAT;
import static android.system.OsConstants.O_RDWR;
import static com.android.internal.app.IntentForwarderActivity.FORWARD_INTENT_TO_MANAGED_PROFILE;
import static com.android.internal.content.NativeLibraryHelper.LIB64_DIR_NAME;
import static com.android.internal.content.NativeLibraryHelper.LIB_DIR_NAME;
import static com.android.internal.util.ArrayUtils.appendInt;
import static com.android.server.pm.InstructionSets.getAppDexInstructionSets;
import static com.android.server.pm.InstructionSets.getDexCodeInstructionSet;
import static com.android.server.pm.InstructionSets.getDexCodeInstructionSets;
import static com.android.server.pm.InstructionSets.getPreferredInstructionSet;
import static com.android.server.pm.InstructionSets.getPrimaryInstructionSet;
import static com.android.server.pm.PermissionsState.PERMISSION_OPERATION_FAILURE;
import static com.android.server.pm.PermissionsState.PERMISSION_OPERATION_SUCCESS;
import static com.android.server.pm.PermissionsState.PERMISSION_OPERATION_SUCCESS_GIDS_CHANGED;

import android.Manifest;
import android.app.ActivityManager;
import android.app.ActivityManagerNative;
import android.app.AppGlobals;
import android.app.Application;
import android.app.IActivityManager;
import android.app.PackageManagerEx;
import android.app.admin.IDevicePolicyManager;
import android.app.backup.IBackupManager;
import android.app.usage.UsageStats;
import android.app.usage.UsageStatsManager;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.IIntentReceiver;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.IntentSender;
import android.content.IntentSender.SendIntentException;
import android.content.ServiceConnection;
import android.content.pm.ActivityInfo;
import android.content.pm.ApplicationInfo;
import android.content.pm.FeatureInfo;
import android.content.pm.IOnPermissionsChangeListener;
import android.content.pm.IPackageDataObserver;
import android.content.pm.IPackageDeleteObserver;
import android.content.pm.IPackageDeleteObserver2;
import android.content.pm.IPackageInstallObserver2;
import android.content.pm.IPackageInstaller;
import android.content.pm.IPackageManager;
import android.content.pm.IPackageMoveObserver;
import android.content.pm.IPackageStatsObserver;
import android.content.pm.InstrumentationInfo;
import android.content.pm.IntentFilterVerificationInfo;
import android.content.pm.KeySet;
import android.content.pm.PackageCleanItem;
import android.content.pm.PackageInfo;
import android.content.pm.PackageInfoLite;
import android.content.pm.PackageInstaller;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.LegacyPackageDeleteObserver;
import android.content.pm.PackageManagerInternal;
import android.content.pm.PackageParser;
import android.content.pm.PackageParser.ActivityIntentInfo;
import android.content.pm.PackageParser.PackageLite;
import android.content.pm.PackageParser.PackageParserException;
import android.content.pm.PackageStats;
import android.content.pm.PackageUserState;
import android.content.pm.ParceledListSlice;
import android.content.pm.PermissionGroupInfo;
import android.content.pm.PermissionInfo;
import android.content.pm.ProviderInfo;
import android.content.pm.ResolveInfo;
import android.content.pm.ServiceInfo;
import android.content.pm.Signature;
import android.content.pm.UserInfo;
import android.content.pm.VerificationParams;
import android.content.pm.VerifierDeviceIdentity;
import android.content.pm.VerifierInfo;
import android.content.res.Resources;
import android.hardware.display.DisplayManager;
import android.net.Uri;
import android.os.Debug;
import android.os.Binder;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Environment.UserEnvironment;
import android.os.FileUtils;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.Parcel;
import android.os.ParcelFileDescriptor;
import android.os.Process;
import android.os.RemoteCallbackList;
import android.os.RemoteException;
import android.os.SELinux;
import android.os.ServiceManager;
import android.os.SystemClock;
import android.os.SystemProperties;
import android.os.UserHandle;
import android.os.UserManager;
import android.os.storage.IMountService;
import android.os.storage.MountServiceInternal;
import android.os.storage.StorageEventListener;
import android.os.storage.StorageManager;
import android.os.storage.VolumeInfo;
import android.os.storage.VolumeRecord;
import android.security.KeyStore;
import android.security.SystemKeyStore;
import android.system.ErrnoException;
import android.system.Os;
import android.system.StructStat;
import android.text.TextUtils;
import android.text.format.DateUtils;
import android.util.ArrayMap;
import android.util.ArraySet;
import android.util.AtomicFile;
import android.util.DisplayMetrics;
import android.util.EventLog;
import android.util.ExceptionUtils;
import android.util.Log;
import android.util.LogPrinter;
import android.util.MathUtils;
import android.util.PrintStreamPrinter;
import android.util.Slog;
import android.util.SparseArray;
import android.util.SparseBooleanArray;
import android.util.SparseIntArray;
import android.util.Xml;
import android.view.Display;

import dalvik.system.DexFile;
import dalvik.system.VMRuntime;

import libcore.io.IoUtils;
import libcore.util.EmptyArray;

import com.android.internal.R;
import com.android.internal.annotations.GuardedBy;
import com.android.internal.app.IMediaContainerService;
import com.android.internal.app.ResolverActivity;
import com.android.internal.content.NativeLibraryHelper;
import com.android.internal.content.PackageHelper;
import com.android.internal.os.IParcelFileDescriptorFactory;
import com.android.internal.os.SomeArgs;
import com.android.internal.os.Zygote;
import com.android.internal.util.ArrayUtils;
import com.android.internal.util.FastPrintWriter;
import com.android.internal.util.FastXmlSerializer;
import com.android.internal.util.IndentingPrintWriter;
import com.android.internal.util.Preconditions;
import com.android.server.EventLogTags;
import com.android.server.FgThread;
import com.android.server.IntentResolver;
import com.android.server.LocalServices;
import com.android.server.ServiceThread;
import com.android.server.SystemConfig;
import com.android.server.Watchdog;
import com.android.server.pm.PermissionsState.PermissionState;
import com.android.server.pm.Settings.DatabaseVersion;
import com.android.server.pm.Settings.VersionInfo;
import com.android.server.storage.DeviceStorageMonitorInternal;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;
import org.xmlpull.v1.XmlSerializer;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.BufferedReader;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileDescriptor;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.FilenameFilter;
import java.io.IOException;
import java.io.InputStream;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.security.NoSuchAlgorithmException;
import java.security.PublicKey;
import java.security.cert.CertificateEncodingException;
import java.security.cert.CertificateException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.Comparator;
import java.util.Date;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicLong;

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
public class PackageManagerServiceEx extends PackageManagerService {
    static final String TAG = "PackageManagerEx";
    public PackageManagerServiceEx(Context context, Installer installer,
            boolean factoryTest, boolean onlyCore) {
        super(context,installer,factoryTest,onlyCore);
        setUpFeatureList();
    }

    /* SPRD: update label and icon for app @{ */
    @Override
    public void setComponentEnabledSettingForSetupMenu(ComponentName componentName,
        int flags, int userId ,Intent attr){
        Log.w(TAG,"setComponentEnabledSettingForSetupMenu" + componentName.toString());
        if(attr != null){
            updatePropertyForSetupMenu(componentName,attr);
        }
        setComponentEnabledSettingSetupMenu(componentName,flags,userId);
    }

    private void setComponentEnabledSettingSetupMenu(ComponentName componentName,int flags, int userId) {
        if (!sUserManager.exists(userId)) return;
        String packageName = componentName.getPackageName();
        Log.w(TAG, "setComponentEnabledSettingSetupMenu:"+packageName+" == "+" == "+flags+" "+userId);
        if(packageName.startsWith("com.android.stk")){
           mPackageManagerServiceExUtils.isOpen = true;
        }
        setEnabledSettingForSetupMenu(componentName.getPackageName(),componentName.getClassName(), flags, userId, null);
    }

    private void setEnabledSettingForSetupMenu(final String packageName, String className,
            final int
            flags, int userId, String callingPackage) {
        PackageSetting pkgSetting;
        final int uid = Binder.getCallingUid();
        final int permission;
        if (uid == Process.SYSTEM_UID) {
            permission = PackageManager.PERMISSION_GRANTED;
        } else {
            permission = mContext
                .checkCallingOrSelfPermission(android.Manifest.permission.CHANGE_COMPONENT_ENABLED_STATE);
        }
        enforceCrossUserPermission(uid, userId, false, true, "set enabled");
        final boolean allowedByPermission = (permission == PackageManager.PERMISSION_GRANTED);
        boolean sendNow = false;
        boolean isApp = (className == null);
        String componentName = isApp ? packageName : className;
        int packageUid = -1;
        ArrayList<String> components;
        // writer
        synchronized (mPackages) {
            pkgSetting = mSettings.mPackages.get(packageName);
            if (pkgSetting == null) {
                if (className == null) {
                    throw new IllegalArgumentException("Unknown package: " + packageName);
                }
                throw new IllegalArgumentException("Unknown component: " + packageName + "/"
                        + className);
            }
        // Limit who can change which apps
        if (!UserHandle.isSameApp(uid, pkgSetting.appId)) {
            // Don't allow apps that don't have permission to modify other apps
            if (!allowedByPermission) {
                throw new SecurityException(
                        "Permission Denial: attempt to change component state from pid="
                        + Binder.getCallingPid()
                        + ", uid=" + uid + ", package uid=" + pkgSetting.appId);
            }
            // Don't allow changing profile and device owners.
            if (mProtectedPackages.canPackageStateBeChanged(userId, packageName)) {
                throw new SecurityException("Cannot disable a device owner or a profile owner");
            }
        }
            // We're dealing with a component level state change
            // First, verify that this is a valid class name.
            PackageParser.Package pkg = pkgSetting.pkg;
            if (pkg == null || className == null || !pkg.hasComponentClassName(className)) {
                if (pkg != null && pkg.applicationInfo.targetSdkVersion >= Build.VERSION_CODES.JELLY_BEAN) {
                    throw new IllegalArgumentException("Component class " + className
                            + " does not exist in " + packageName);
                } else {
                    Slog.w(TAG, "Failed setComponentEnabledSetting: component class " + className
                            + " does not exist in " + packageName);
                }
            }

            mSettings.writePackageRestrictionsLPr(userId);
            components = mPendingBroadcasts.get(userId, packageName);
            final boolean newPackage = components == null;
            if (newPackage) {
                components = new ArrayList<String>();
            }
            if (!components.contains(componentName)) {
                components.add(componentName);
            }
            if ((flags & PackageManager.DONT_KILL_APP) == 0) {
                sendNow = true;
                // Purge entry from pending broadcast list if another one exists already
                // since we are sending one right away.
                mPendingBroadcasts.remove(userId, packageName);
            } else {
                if (newPackage) {
                    mPendingBroadcasts.put(userId, packageName, components);
                }
                if (!mHandler.hasMessages(SEND_PENDING_BROADCAST)) {
                    // Schedule a message
                    mHandler.sendEmptyMessageDelayed(SEND_PENDING_BROADCAST, BROADCAST_DELAY);
                }
            }
        }

        long callingId = Binder.clearCallingIdentity();
        try {
            if (sendNow) {
                packageUid = UserHandle.getUid(userId, pkgSetting.appId);
                sendPackageChangedBroadcast(packageName,
                        (flags & PackageManager.DONT_KILL_APP) != 0, components, packageUid);
            }
        } finally {
            Binder.restoreCallingIdentity(callingId);
        }
    }

    private void updatePropertyForSetupMenu(ComponentName componentName, Intent attr) {
        String packageName = componentName.getPackageName();
        Log.w("SetupMenu", " packagename is " + packageName);
        if (packageName.startsWith("com.android.stk")) {
            String labelName = null;
            Object temp = attr.getExtra("setup.menu.labelName");
            if (temp != null)
                labelName = (String) temp;
            Log.w("SetupMenu", " labelName is " + labelName);
            PackageParser.Activity a = mActivities.mActivities.get(componentName);
            if (a != null) {
                if (labelName != null) {
                    Log.w("SetupMenu", "setup labelName is " + labelName);
                    a.info.labelName = labelName;
                }
            }
            mPackageManagerServiceExUtils.isOpen = true;
        }
    }

    @Override
    public void setComponentEnabledSettingForSpecific(ComponentName componentName,
            int newState, int flags, int userId ,Intent attr){
          Log.w("set enable for specific apk ",componentName.toString());
          if(attr != null){
            updateProperty(componentName,attr);
          }
          setComponentEnabledSetting(componentName,newState,flags,userId);
    }

    public void updateProperty(ComponentName componentName,Intent attr){
        String packageName = componentName.getPackageName();
        if(packageName.startsWith("com.android.stk")){
            int resIconID = 0,resLabelID = 0;
            Object temp = attr.getExtra("gsm.stk.icon");
            if(temp != null)
                resIconID = (Integer)temp;
            temp = attr.getExtra("gsm.stk.label");
            if(temp != null)
                resLabelID = (Integer)temp;
            Log.w("resID icon/label ",resIconID+" ~ "+resLabelID);
            PackageParser.Activity a = mActivities.mActivities.get(componentName);
            if(a != null){
                if(resIconID !=0 || resLabelID !=0){
                    int length = a.intents.size();
                    for(int i =0 ; i < length ; i++){
                        PackageParser.ActivityIntentInfo intent = a.intents.get(i);
                        intent.labelRes = resLabelID != 0 ? resLabelID : intent.labelRes;
                        intent.icon = resIconID != 0 ? resIconID : intent.icon;
                        a.info.icon = resIconID != 0 ? resIconID : a.info.icon;
                    }
                }
            }
            mPackageManagerServiceExUtils.isOpen = true;
        }
    }
    /* @} */

    /*
     * SPRD: add for backupApp
     * @{
     */
    @Override
    public int backupAppData(String pkgName, String destDir) {
        Log.w(TAG,"backupAppData:pkgName="+ pkgName + ",destDir="+destDir);
        if(pkgName == null || destDir == null) {
            throw new IllegalArgumentException("pkgName: " + pkgName + " or destDir: " + destDir + " is null");
        }
        int ret;
        synchronized (mPackages) {
            PackageParser.Package pkg = mPackages.get(pkgName);
            if(pkg == null) {
                return PackageManagerEx.BACKUPAPP_PKG_DONINSTALL;
            }
        }
            int callUid = Binder.getCallingUid();
            ret = ((InstallerEx)mInstaller).backupApp(pkgName, destDir, callUid, callUid);

        return ret;
    }

    @Override
    public int restoreAppData(String sourceDir, String pkgName) {
        Log.w(TAG,"restoreAppData:sourceDir="+ sourceDir + ",pkgName="+pkgName);
        if(sourceDir == null || pkgName == null) {
            throw new IllegalArgumentException("sourceDir: " + sourceDir + " or pkgName: " + pkgName + " is null");
        }
        int ret;
        PackageParser.Package pkg;
        synchronized (mPackages) {
            pkg = mPackages.get(pkgName);
            if(pkg == null) {
                return PackageManagerEx.BACKUPAPP_PKG_DONINSTALL;
            }
        }
        ret = ((InstallerEx) mInstaller).restoreApp(sourceDir, pkgName, pkg.applicationInfo.uid, pkg.applicationInfo.uid);
        try {
            mInstaller.restoreconAppData(pkg.volumeUuid, pkg.packageName, UserHandle.getUserId(pkg.applicationInfo.uid), StorageManager.FLAG_STORAGE_CE | StorageManager.FLAG_STORAGE_DE , UserHandle.getAppId(pkg.applicationInfo.uid), pkg.applicationInfo.seinfo);
        } catch (Exception e) {
            Slog.e(TAG, "Failed to restorecon for " + pkg.packageName + ": " + e);
        }
        return ret;
    }
    /*
     * @}
     */
    /* SPRD: add for emulated storage */
    public int movePrimaryEmulatedStorage(String volumeUuid) throws RemoteException {
        mContext.enforceCallingOrSelfPermission(android.Manifest.permission.MOVE_PACKAGE, null);
        Log.w(TAG,"movePrimaryEmulatedStorage:volumeUuid= "+volumeUuid);
        final int realMoveId = mNextMoveId.getAndIncrement();
        final Bundle extras = new Bundle();
        extras.putString(VolumeRecord.EXTRA_FS_UUID, volumeUuid);
        mMoveCallbacks.notifyCreated(realMoveId, extras);

        final IPackageMoveObserver callback = new IPackageMoveObserver.Stub() {
            @Override
            public void onCreated(int moveId, Bundle extras) {
                // Ignored
            }

            @Override
            public void onStatusChanged(int moveId, int status, long estMillis) {
                mMoveCallbacks.notifyStatusChanged(realMoveId, status, estMillis);
            }
        };

        final StorageManager storage = mContext.getSystemService(StorageManager.class);
        storage.setPrimaryEmulatedStorageUuid(volumeUuid, callback);
        return realMoveId;
    }
    /* @} */
    private java.util.HashMap<String,ArrayList<PackageInfo>> mFeatureList = new java.util.HashMap<String, ArrayList<PackageInfo>>();
    private void setUpFeatureList() {
        long now = SystemClock.uptimeMillis();
        int flags = updateFlagsForPackage(PackageManager.GET_META_DATA, mContext.getUserId(), null);
        synchronized (mPackages) {
            for (PackageParser.Package p : mPackages.values()) {
                final PackageInfo pi =
                        generatePackageInfo((PackageSetting) p.mExtras, flags, mContext.getUserId());
                if (pi != null) {
                    if (pi.applicationInfo == null ||
                            !pi.applicationInfo.enabled ||
                            pi.applicationInfo.metaData == null) {
                        continue;
                    }
                    try {
                        Bundle metaData = pi.applicationInfo.metaData;
                        if (metaData.containsKey("isFeatureAddon")) {
                            Set<String> metaKeys = metaData.keySet();
                            for (String key : metaKeys) {
                                if (key.startsWith("targetPackages")) {
                                    // maybe like  com.android.providers.downloads;com.android.providers.downloads.ui
                                    String[] targetPkgNames = metaData.getString(key).trim().split(";");
                                    for (String targetPkg : targetPkgNames) {
                                        ArrayList<PackageInfo> targetFeatureList = mFeatureList.get(targetPkg.trim());//modify for Bug#620077
                                        if (targetFeatureList == null) {
                                            targetFeatureList = new ArrayList<PackageInfo>();
                                        }
                                        targetFeatureList.add(pi);
                                        mFeatureList.put(targetPkg, targetFeatureList);
                                    }
                                }
                            }
                        }
                    } catch (Exception e){
                        e.printStackTrace();
                    }
                }
            }
        }
        Slog.d(TAG,"setUpFeatureList cost :" + (SystemClock.uptimeMillis() - now));
    }
    /**
     * @hide
     */
    @Override
    public ParceledListSlice<PackageInfo> getPackageFeatureList(String pkgName) {
        synchronized (mPackages) {
            ArrayList<PackageInfo> list = mFeatureList.get(pkgName);
            if(list == null){
                list = new ArrayList<PackageInfo>();
            }
             Slog.e(TAG,"getPackageFeatureList --> "+list);
            return new ParceledListSlice<PackageInfo>(list);
        }
    }
}
