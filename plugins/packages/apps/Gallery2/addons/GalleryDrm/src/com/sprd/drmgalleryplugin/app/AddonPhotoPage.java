
package com.sprd.drmgalleryplugin.app;

import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.LinkedList;
import java.util.Queue;
import android.app.AddonManager;
import android.content.Context;
import android.drm.DrmManagerClient;
import android.drm.DecryptHandle;
import android.drm.DrmStore;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.widget.Toast;
import com.android.gallery3d.app.PhotoPage;
import com.android.gallery3d.data.MediaItem;
import com.android.gallery3d.data.LocalImage;
import com.android.gallery3d.data.Path;
import com.sprd.drmgalleryplugin.R;
import com.sprd.drmgalleryplugin.util.DrmUtil;
import com.sprd.gallery3d.drm.PhotoPageUtils;

public class AddonPhotoPage extends PhotoPageUtils implements
        AddonManager.InitialCallback {
    private Context mAddonContext;
    private static final int MSG_CONSUME_DRM_RIGHTS = 17;
    private static final int CONSUME_DRM_RIGHTS_DELAY = 1000;
    public static final String KEY_IS_DISPLAY_DRM = "is-play-drm";
    private boolean mFirstPickIsDrmPhoto ;
    private boolean mDrmToastlock ;
    DecryptHandle decryptHandle ;
    private static final String TAG = "AddonPhotoPage";
    private HashMap<Path, MediaItem> mDrmConsumedItems ;
    private Queue<MediaItem> mQueueDrmWaitingConsumed ;

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    @Override
    public void getFirstPickIsDrmPhoto(Bundle data) {
        mFirstPickIsDrmPhoto = data.getBoolean(KEY_IS_DISPLAY_DRM, false);
        mDrmConsumedItems = new LinkedHashMap<Path, MediaItem>();
        mQueueDrmWaitingConsumed = new LinkedList<MediaItem>();
        mFirstPickIsDrmPhoto = false;
        mDrmToastlock = false;
        decryptHandle = null;
    }

    @Override
    public void updateDrmCurrentPhoto(MediaItem mCurrentPhoto, Handler mHandler) {
        Log.d(TAG, "updateDrmCurrentPhoto---start");
        String filePath = mCurrentPhoto.getFilePath();
        if (DrmUtil.isDrmFile(filePath, null)
                && mCurrentPhoto.getMediaType() != LocalImage.MEDIA_TYPE_VIDEO
                && DrmUtil.getDrmFileType(filePath).equals(DrmUtil.FL_DRM_FILE) != true) {
            if (DrmUtil.isDrmValid(filePath)) {
//                if (!mDrmToastlock && !mFirstPickIsDrmPhoto) {
//                    Toast.makeText(mAddonContext,
//                            mAddonContext.getString(R.string.drm_consumed),
//                            Toast.LENGTH_SHORT).show();
//                    mDrmToastlock = true;
//                }
                boolean found = false;
                if(mDrmConsumedItems == null) return;
                for (Path p : mDrmConsumedItems.keySet()) {
                    if (p.toString().equals(mCurrentPhoto.getPath().toString())) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    setDrmPlayStatus(filePath, DrmStore.Playback.START);
                    mQueueDrmWaitingConsumed.offer(mCurrentPhoto);
                    mHandler.removeMessages(MSG_CONSUME_DRM_RIGHTS);
                    mHandler.sendEmptyMessageDelayed(MSG_CONSUME_DRM_RIGHTS,
                            CONSUME_DRM_RIGHTS_DELAY);
                }
            }
        }
    }

    @Override
    public boolean cosumeDrmRights(Message message) {
        if (MSG_CONSUME_DRM_RIGHTS == message.what) {
            MediaItem drmImage;
            while ((drmImage = mQueueDrmWaitingConsumed.poll()) != null) {
                setDrmPlayStatus(drmImage.getFilePath(), DrmStore.Playback.STOP);
                mDrmConsumedItems.put(drmImage.getPath(), drmImage);
                /* SPRD: Add 20150730 Spreadst of bug458241, show toast at proper time @{ */
                if (!mDrmToastlock && !mFirstPickIsDrmPhoto) {
                    Toast.makeText(mAddonContext,
                            mAddonContext.getString(R.string.drm_consumed),
                            Toast.LENGTH_SHORT).show();
                    mDrmToastlock = true;
                }
                /* @} */
            }
            return false;
        } else {
            return true;
        }
    }

    @Override
    public void onDrmDestroy() {
        mQueueDrmWaitingConsumed = null;
        mDrmConsumedItems = null;
    }

    public void setDrmPlayStatus(String filePath, int playbackStatus) {
        Log.d(TAG, "setDrmPlayStatus---start");
        DrmManagerClient mDrmManagerClient = DrmUtil.getDrmManagerClient();
        if (decryptHandle == null) {
            decryptHandle = mDrmManagerClient.openDecryptSession(filePath);
        }

        if (decryptHandle != null) {
            mDrmManagerClient.setPlaybackStatus(decryptHandle, playbackStatus);
            Log.d(TAG, "setDrmPlayStatus playbackStatus = " + playbackStatus
                    + "{" + filePath + "}");
            if (playbackStatus == DrmStore.Playback.STOP) {
                mDrmManagerClient.closeDecryptSession(decryptHandle);
                decryptHandle = null;
            }
        } else {
            Log.e(TAG, "mDecryptHandle open fail :" + filePath);
        }
    }
}
