package com.android.server;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.Objects;

import android.annotation.Nullable;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.IPackageMoveObserver;
import android.content.pm.PackageManager;
import android.hardware.usb.UsbManager;
import android.net.Uri;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.RemoteCallbackList;
import android.os.RemoteException;
import android.os.EnvironmentEx;
import android.os.SystemProperties;
import android.os.UserHandle;
import android.os.storage.IMountService;
import android.os.storage.IMountServiceListener;
import android.os.storage.StorageManager;
import android.os.storage.StorageVolume;
import android.os.storage.VolumeInfo;
import android.util.ArrayMap;
import android.util.Slog;
import com.android.server.MountService.Callbacks;

import com.android.internal.annotations.GuardedBy;
import com.android.internal.os.SomeArgs;

public abstract class MountServiceEx extends IMountService.Stub {
    private static final String TAG = "MountServiceEx";

    protected final Object mShareLock = new Object();
    protected boolean mUMSShared = false;
    protected boolean mUmsAvailable = false;
    protected boolean mUmsEnabling = false;
    /* flag for USB line connected */
    protected boolean mUSBIsConnected = false;
    /* flag for USB line if unpluged after enable UMS */
    protected boolean mUSBIsUnpluged = false;
    /* flag for if need send ums connected on system ready */
    protected boolean mSendUmsConnectedOnBoot = false;
    protected static final boolean ENABLE_UMS = true;

    protected static final String ATTR_PRIMARY_EMULATED_UUID = "primaryEmulatedUuid";
    protected MountService mMountService;
    @GuardedBy("mLock")
    protected String mPrimaryEmulatedUuid;
    @GuardedBy("mLock")
    protected boolean mSetEmulated;
    @GuardedBy("mLock")

    protected static final int H_VOLUME_UNSHARED_BROADCAST = 11;

    protected static final int H_VOLUME_MOUNT = 5;

    protected static final int H_RESET = 10;

    /* SPRD: support double sdcard add for sdcard hotplug @{ */
    class VolumesPresentState {
        boolean mVolumeBadRemoved = false;
    }

    protected VolumesPresentState mVolumesPresentState = new VolumesPresentState();
    /* @} */
    protected final BroadcastReceiver mUsbReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            mUSBIsConnected = intent.getBooleanExtra(UsbManager.USB_CONNECTED, false);
            boolean available = (mUSBIsConnected &&
                    intent.getBooleanExtra("mass_storage", false));
            synchronized (mShareLock) {
                if (!mUSBIsConnected && !mUSBIsUnpluged) {
                    mUSBIsUnpluged = true;
                }
            }
            notifyShareAvailabilityChange(available);
        }
    };

    protected void sendPriEmulatedVolumeMounted(VolumeInfo vol, VolumeInfo privateVol){
        if (Objects.equals(StorageManager.UUID_PRIVATE_INTERNAL, mPrimaryEmulatedUuid)
                && VolumeInfo.ID_PRIVATE_INTERNAL.equals(privateVol.id)) {
            Slog.v(TAG, "Found primary emulated storage at " + vol);
            vol.mountFlags |= VolumeInfo.MOUNT_FLAG_PRI_EMU;
            vol.mountFlags |= VolumeInfo.MOUNT_FLAG_VISIBLE;
            mMountService.mHandler.obtainMessage(H_VOLUME_MOUNT, vol).sendToTarget();
       } else if (Objects.equals(privateVol.fsUuid, mPrimaryEmulatedUuid)) {
            Slog.v(TAG, "Found primary emulated storage at " + vol);
            vol.mountFlags |= VolumeInfo.MOUNT_FLAG_PRI_EMU;
            vol.mountFlags |= VolumeInfo.MOUNT_FLAG_VISIBLE;
            mMountService.mHandler.obtainMessage(H_VOLUME_MOUNT, vol).sendToTarget();
       }
    }

    private void sendUmsIntent(boolean c) {
        mMountService.mContext.sendBroadcastAsUser(
                new Intent((c ? Intent.ACTION_UMS_CONNECTED : Intent.ACTION_UMS_DISCONNECTED)),
                UserHandle.ALL);
    }

    private boolean isEnableUMSBroken() {
        synchronized (mShareLock) {
            return !mUSBIsConnected || mUSBIsUnpluged;
        }
    }

    protected void notifyShareAvailabilityChange(final boolean avail) {
         Slog.d(TAG,"notifyShareAvailabilityChange :avail="+avail+",mSystemReady="+mMountService.mSystemReady);
        synchronized (mMountService.mLock) {
            mMountService.mCallbacks.notifyUMSConnectionChanged(avail);
            synchronized (mShareLock) {
                mUmsAvailable = avail;
            }
        }

        if (mMountService.mSystemReady == true) {
            sendUmsIntent(avail);
        } else {
            mSendUmsConnectedOnBoot = avail;
        }

        if (avail == false && mUMSShared) {
            /*
             * USB mass storage disconnected while enabled
             */
            new Thread("MountService#AvailabilityChange") {
                @Override
                public void run() {
                    try {
                        Slog.w(TAG, "Disabling UMS after cable disconnect");
                        setUMSEnabled(false);
                    } catch (Exception ex) {
                        Slog.w(TAG, "Failed to mount media on UMS enabled-disconnect", ex);
                    }
                }
            }.start();
        }
    }

    protected boolean isUMSConnected() {
        mMountService.waitForReady();
        synchronized (mShareLock) {
            if (mUmsEnabling) {
                return true;
            }
            return mUmsAvailable;
        }
    }

    protected boolean isUMSEnabled() {
        mMountService.waitForReady();
        synchronized (mShareLock) {
            return mUMSShared;
        }
    }

    protected void setUMSEnabled(boolean enable) {
        boolean broken = false;
        mMountService.enforcePermission(android.Manifest.permission.MOUNT_UNMOUNT_FILESYSTEMS);
        mMountService.waitForReady();

        final ArrayList<VolumeInfo> publicVolumes = new ArrayList<>();
        synchronized (mMountService.mLock) {
            for (int i = 0; i < mMountService.mVolumes.size(); i++) {
                final VolumeInfo vol = mMountService.mVolumes.valueAt(i);
                if (vol.type == VolumeInfo.TYPE_PUBLIC) {
                    publicVolumes.add(vol);
                }
            }
        }

        synchronized (mShareLock) {
            if (enable) {
                mUSBIsUnpluged = false;
                mUmsEnabling = true;
            }

            int publicCount = publicVolumes.size();
            if (enable) {
                boolean prepareOK = prepareEnableUMS(publicCount);
                if (isEnableUMSBroken() || !prepareOK) {
                    if (prepareOK) {
                        disableUMSOver();
                    }
                    mUmsEnabling = false;
                    return;
                }
                int i;
                for (i = 0; i < publicCount; i++) {
                    if (isEnableUMSBroken()) {
                        broken = true;
                        break;
                    }
                    final VolumeInfo vol = publicVolumes.get(i);
                    boolean enableOK = enableUMSLocked(vol);
                    if (!enableOK || isEnableUMSBroken()) {
                        i++;
                        broken = true;
                        break;
                    }
                }
                if (broken || isEnableUMSBroken()) {
                    mUmsEnabling = false;
                    // if usb line is unconnected when sharing, roll back
                    // opration
                    for (int j = 0; j < i; j++) {
                        final VolumeInfo vol = publicVolumes.get(j);
                        disableUMSLocked(vol);
                    }
                    disableUMSOver();
                } else {
                    mUmsEnabling = false;
                    mUMSShared = true;
                }
            } else {
                int i;
                for (i = 0; i < publicCount; i++) {
                    final VolumeInfo vol = publicVolumes.get(i);
                    disableUMSLocked(vol);
                }
                if (i < publicCount) {
                    // maybe cannot get here
                    Slog.w(TAG, "Some thing wrong!", new Exception("disable UMS"));
                }
                disableUMSOver();
                mUMSShared = false;
            }
        }
    }

    private boolean prepareEnableUMS(int shareCount) {
        try {
            mMountService.mConnector.execute("volume", "pre_share", shareCount);
        } catch (NativeDaemonConnectorException e) {
            Slog.w(TAG, "prepare enable ums error:", new Exception(e));
            return false;
        }
        return true;
    }

    private boolean enableUMSLocked(final VolumeInfo vol) {
        try {
            vol.stateBeforeUMS = vol.state;
            if (vol.state == VolumeInfo.STATE_MOUNTED) {
                mMountService.unmount(vol);
            }
            mMountService.mConnector.execute("volume", "share", vol.id);
        } catch (Exception e) {
            Slog.w(TAG, "enable ums error:", new Exception(e));
            return false;
        }
        return true;
    }

    private boolean disableUMSLocked(final VolumeInfo vol) {
        try {
            mMountService.mConnector.execute("volume", "unshare", vol.id);
            if (vol.stateBeforeUMS == VolumeInfo.STATE_MOUNTED) {
                mMountService.mount(vol);
            }
        } catch (Exception e) {
            Slog.w(TAG, "disable ums error:", new Exception(e));
            return false;
        }
        return true;
    }

    private boolean disableUMSOver() {
        try {
            mMountService.mConnector.execute("volume", "unshare_over");
        } catch (NativeDaemonConnectorException e) {
            Slog.w(TAG, "disable ums over error:", new Exception(e));
            return false;
        }
        return true;
    }

    protected void handleUmsSystemReady() {
        if (mSendUmsConnectedOnBoot) {
            sendUmsIntent(true);
            mSendUmsConnectedOnBoot = false;
        }
    }

    protected void sendUnsharedBroadcastAndUpdatePms(int oldState, int newState, VolumeInfo vol,
            int[] startedUsers) {
        if (oldState == VolumeInfo.STATE_SHARED && newState != oldState) {
            for (int userId : startedUsers) {
                if (vol.isVisibleForRead(userId)) {
                    final StorageVolume userVol = vol.buildStorageVolume(mMountService.mContext, userId, false);
                    mMountService.mHandler.obtainMessage(H_VOLUME_UNSHARED_BROADCAST, userVol).sendToTarget();
                }
            }
        }
        /* SPRD: support double sdcard add for sdcard hotplug @{*/
        if (vol.type == VolumeInfo.TYPE_PUBLIC && (Objects.equals(vol.linkName,"sdcard0")
                || Objects.equals(vol.linkName,"sdcard1")) && newState == VolumeInfo.STATE_MOUNTED) {
            synchronized (mVolumesPresentState) {
                if(mVolumesPresentState.mVolumeBadRemoved){
                    synchronized (mMountService.mAsecMountSet) {
                        mMountService.mAsecMountSet.clear();
                    }
                    mVolumesPresentState.mVolumeBadRemoved = false;
                }
                }
        } else if(vol.type == VolumeInfo.TYPE_PUBLIC && (Objects.equals(vol.linkName,"sdcard0")
                || Objects.equals(vol.linkName,"sdcard1")) && newState == VolumeInfo.STATE_EJECTING){
            synchronized (mVolumesPresentState) {
                mVolumesPresentState.mVolumeBadRemoved = true;
            }
        }
    }

    protected void setPrimaryExternalProperties() {
        if (SystemProperties.getBoolean(StorageManager.PROP_PRIMARY_PHYSICAL, false)) {
            SystemProperties.set(StorageManager.PROP_PRIMARY_TYPE,
                    String.valueOf(EnvironmentEx.STORAGE_PRIMARY_EXTERNAL));
        }
    }
    protected VolumeInfo findSourceVolume(VolumeInfo to){
        VolumeInfo from;
        if ((to != null && to.getType() == VolumeInfo.TYPE_EMULATED)
                && !Objects.equals(mMountService.mMoveTargetUuid, mPrimaryEmulatedUuid)) {
            from = mMountService.findStorageForUuid(mPrimaryEmulatedUuid);
        } else {
            from = mMountService.findStorageForUuid(mMountService.mPrimaryStorageUuid);
        }
        return from;
    }
    protected boolean skipMove(VolumeInfo from ,VolumeInfo to){
        return (to != null && (to.getType() == VolumeInfo.TYPE_PUBLIC
                || (from == null && Objects.equals(StorageManager.UUID_PRIMARY_PHYSICAL, mMountService.mPrimaryStorageUuid))
                || (from != null && from.getType() == VolumeInfo.TYPE_PUBLIC)));
    }
    protected void sprdJustResetOrNot(String fsUuid){
        boolean needReset = false;
        if (Objects.equals(mMountService.mPrimaryStorageUuid, fsUuid)) {
            mMountService.mPrimaryStorageUuid = mMountService.getDefaultPrimaryStorageUuid();
            needReset = true;
            if (SystemProperties.getBoolean(StorageManager.PROP_PRIMARY_PHYSICAL, false)) {
                SystemProperties.set(StorageManager.PROP_PRIMARY_TYPE,
                        String.valueOf(EnvironmentEx.STORAGE_PRIMARY_EXTERNAL));
            }
        }
        if (Objects.equals(mPrimaryEmulatedUuid, fsUuid)) {
            mPrimaryEmulatedUuid = StorageManager.UUID_PRIVATE_INTERNAL;
            needReset = true;
        }
        if (needReset) {
        	mMountService.mHandler.obtainMessage(H_RESET).sendToTarget();
        }
    }

    @Override
    public String getPrimaryEmulatedStorageUuid() {
        mMountService.enforcePermission(android.Manifest.permission.MOUNT_UNMOUNT_FILESYSTEMS);
        mMountService.waitForReady();
        synchronized (mMountService.mLock) {
            return mPrimaryEmulatedUuid;
        }
    }
    protected MountService getMountService(){
        MountService ms = MountService.sSelf;
            return ms;
    }
    /* SPRD: add for emulated storage @{ */
    @Override
    public void setPrimaryEmulatedStorageUuid(String volumeUuid, IPackageMoveObserver callback) {
         mMountService.enforcePermission(android.Manifest.permission.MOUNT_UNMOUNT_FILESYSTEMS);
         mMountService.waitForReady();
        final VolumeInfo target = mMountService.findStorageForUuid(volumeUuid);

        if (target == null || target.getType() != VolumeInfo.TYPE_EMULATED) {
            throw new IllegalArgumentException("Target volume[" + volumeUuid + "] is not emulated");
        }
        final VolumeInfo from;
        final VolumeInfo to;
        synchronized (mMountService.mLock) {
            if (Objects.equals(mPrimaryEmulatedUuid, volumeUuid)) {
                throw new IllegalArgumentException("Primary emulated storage already at " + volumeUuid);
            }

            if (mMountService.mMoveCallback != null) {
                throw new IllegalStateException("Move already in progress");
            }
            mMountService.mMoveCallback = callback;
            mMountService.mMoveTargetUuid = volumeUuid;
            mSetEmulated = true;

            // Here just move from emulated storage to emulated storage
            {
                from = mMountService.findStorageForUuid(mPrimaryEmulatedUuid);
                to = mMountService.findStorageForUuid(volumeUuid);

                if (from == null) {
                    Slog.w(TAG, "Failing move due to missing from volume " + mPrimaryEmulatedUuid);
                    mMountService.onMoveStatusLocked(PackageManager.MOVE_FAILED_INTERNAL_ERROR);
                    return;
                } else if (to == null) {
                    Slog.w(TAG, "Failing move due to missing to volume " + volumeUuid);
                    mMountService.onMoveStatusLocked(PackageManager.MOVE_FAILED_INTERNAL_ERROR);
                    return;
                }
            }
        }
 
        try {
             mMountService.mConnector.execute("volume", "move_storage", from.id, to.id);
        } catch (NativeDaemonConnectorException e) {
             throw e.rethrowAsParcelableException();
        }
    }
    public String onMovestatusLockedEx(){
        if (mSetEmulated) {
            /* when we migrate data, if primary is emulated, we should
             * keep primary storage as same as mounted emulated storage
             */
            if (Objects.equals(mMountService.mPrimaryStorageUuid, mPrimaryEmulatedUuid)) {
                mMountService.mPrimaryStorageUuid = mMountService.mMoveTargetUuid;
            }
            mPrimaryEmulatedUuid = mMountService.mMoveTargetUuid;
        } else {
            final VolumeInfo target = mMountService.findStorageForUuid(mMountService.mMoveTargetUuid);
            /* when we set primary, if target is phsical, we should
             * set mPrimaryStorageUuid as UUID_PRIMARY_PHYSICAL and
             */
            if (target != null && target.getType() == VolumeInfo.TYPE_PUBLIC) {
                mMountService.mPrimaryStorageUuid = StorageManager.UUID_PRIMARY_PHYSICAL;
                /* set priamry type as external primary */
                SystemProperties.set(StorageManager.PROP_PRIMARY_TYPE,
                                         String.valueOf(EnvironmentEx.STORAGE_PRIMARY_EXTERNAL));
            } else {
                /* when we set primary, if target is emulated, we should
                 * keep mounted emulated storage as same as primary storage
                 */
                if (target == null || target.getType() == VolumeInfo.TYPE_EMULATED) {
                    mPrimaryEmulatedUuid = mMountService.mMoveTargetUuid;
                    /* set priamry type as internal primary */
                    SystemProperties.set(StorageManager.PROP_PRIMARY_TYPE,
                            String.valueOf(EnvironmentEx.STORAGE_PRIMARY_INTERNAL));
                }
                mMountService.mPrimaryStorageUuid = mMountService.mMoveTargetUuid;
            }
        }
        return mMountService.mPrimaryStorageUuid;
    }
    /* @} */
}
