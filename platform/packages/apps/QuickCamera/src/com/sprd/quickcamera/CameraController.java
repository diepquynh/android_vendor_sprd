/*
 * Copyright (C) 2013 The Android Open Source Project
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

package com.sprd.quickcamera;

import android.content.Context;
import android.os.Handler;
import android.util.Log;

import com.android.ex.camera2.portability.CameraAgent;
import com.android.ex.camera2.portability.CameraDeviceInfo;

public class CameraController implements CameraAgent.CameraOpenCallback {
    private static final String TAG = "CameraController";
    private static final int EMPTY_REQUEST = -1;
    private final Handler mCallbackHandler;
    private final CameraAgent mCameraAgent;
    private final CameraAgent mCameraAgentNg;

    private CameraAgent.CameraOpenCallback mCallbackReceiver;

    /** The one for the API that is currently in use (deprecated one by default). */
    private CameraDeviceInfo mInfo;

    private CameraAgent.CameraProxy mCameraProxy;
    private int mRequestingCameraId = EMPTY_REQUEST;

    /**
     * Determines which of mCameraAgent and mCameraAgentNg is currently in use.
     * <p>It's only possible to enable this if the new API is actually
     * supported.</p>
     */
    private boolean mUsingNewApi = false;

    /**
     * Constructor.
     *
     * @param handler The {@link android.os.Handler} to post the camera
     *                callbacks to.
     * @param cameraManager Used for camera open/close.
     * @param cameraManagerNg Used for camera open/close with the new API. If
     *                        {@code null} or the same object as
     *                        {@code cameraManager}, the new API will not be
     *                        exposed and requests for it will get the old one.
     */
    public CameraController(CameraAgent.CameraOpenCallback callbackReceiver,
          Handler handler,
          CameraAgent cameraManager,
          CameraAgent cameraManagerNg) {
        mCallbackReceiver = callbackReceiver;
        mCallbackHandler = handler;
        mCameraAgent = cameraManager;
        // If the new implementation is the same as the old, the
        // CameraAgentFactory decided this device doesn't support the new API.
        mCameraAgentNg = cameraManagerNg != cameraManager ? cameraManagerNg : null;
        mInfo = mCameraAgent.getCameraDeviceInfo();
        if (mInfo == null && mCallbackReceiver != null) {
            mCallbackReceiver.onDeviceOpenFailure(-1, "GETTING_CAMERA_INFO");
        }
    }

    public CameraDeviceInfo.Characteristics getCharacteristics(int cameraId) {
        if (mInfo == null) {
            return null;
        }
        return mInfo.getCharacteristics(cameraId);
    }

    public int getNumberOfCameras() {
        if (mInfo == null) {
            return 0;
        }
        return mInfo.getNumberOfCameras();
    }

    @Override
    public void onCameraOpened(CameraAgent.CameraProxy camera) {
        Log.i(TAG, "onCameraOpened,CameraProxy = " + camera);
        if (mRequestingCameraId != camera.getCameraId()) {
            return;
        }

        // SPRD: Fix bug 572631, optimize camera launch time
        initCameraDeviceInfo();
        mCameraProxy = camera;
        mRequestingCameraId = EMPTY_REQUEST;
        if (mCallbackReceiver != null) {
            mCallbackReceiver.onCameraOpened(camera);
        }
    }

    @Override
    public void onCameraDisabled(int cameraId) {
        if (mCallbackReceiver != null) {
            mCallbackReceiver.onCameraDisabled(cameraId);
        }
    }

    @Override
    public void onDeviceOpenFailure(int cameraId, String info) {
        if (mCallbackReceiver != null) {
            mCallbackReceiver.onDeviceOpenFailure(cameraId, info);
        }
    }

    @Override
    public void onDeviceOpenedAlready(int cameraId, String info) {
        if (mCallbackReceiver != null) {
            mCallbackReceiver.onDeviceOpenedAlready(cameraId, info);
        }
    }

    @Override
    public void onReconnectionFailure(CameraAgent mgr, String info) {
        if (mCallbackReceiver != null) {
            mCallbackReceiver.onReconnectionFailure(mgr, info);
        }
    }

    public void requestCamera(int id) {
        requestCamera(id, false);
    }

    public void requestCamera(int id, boolean useNewApi) {
        Log.i(TAG, "requestCamera");
        // Based on
        // (mRequestingCameraId == id, mRequestingCameraId == EMPTY_REQUEST),
        // we have (T, T), (T, F), (F, T), (F, F).
        // (T, T): implies id == EMPTY_REQUEST. We don't allow this to happen
        //         here. Return.
        // (F, F): A previous request hasn't been fulfilled yet. Return.
        // (T, F): Already requested the same camera. No-op. Return.
        // (F, T): Nothing is going on. Continue.
        Log.i(TAG, "mRequestingCameraId = " + mRequestingCameraId + "mInfo = " + mInfo);
        if (mRequestingCameraId != EMPTY_REQUEST || mRequestingCameraId == id) {
            return;
        }
        if (mInfo == null) {
            return;
        }

        mRequestingCameraId = id;

        // Only actually use the new API if it's supported on this device.
        useNewApi = mCameraAgentNg != null && useNewApi;
        CameraAgent cameraManager = useNewApi ? mCameraAgentNg : mCameraAgent;
        if (mCameraProxy != null) {
            Log.i(TAG, "mCameraProxy.getCameraId() = " + mCameraProxy.getCameraId() + ";id = " + id
                    + ";useNewApi = " + useNewApi + ";mUsingNewApi= " + mUsingNewApi);
        }
        if (mCameraProxy == null) {
            // No camera yet.
            checkAndOpenCamera(cameraManager, id, mCallbackHandler, this);
        } else if (mCameraProxy.getCameraId() != id || mUsingNewApi != useNewApi
                    || cameraManager == mCameraAgentNg) {
                mCameraAgentNg.closeCamera(mCameraProxy, true);
            checkAndOpenCamera(cameraManager, id, mCallbackHandler, this);
        } else {
            // The same camera, just do a reconnect.
            Log.v(TAG, "reconnecting to use the existing camera");
            mCameraProxy.reconnect(mCallbackHandler, this);
            mCameraProxy = null;
        }

        mUsingNewApi = useNewApi;
    }

    public void removeCallbackReceiver() {
        mCallbackReceiver = null;
    }

    /**
     * Closes the opened camera device.
     * TODO: Make this method package private.
     */
    public void closeCamera(boolean synced) {
        Log.i(TAG, "Closing camera");
        mCameraProxy = null;
        if (mUsingNewApi) {
            mCameraAgentNg.closeCamera(mCameraProxy, synced);
        } else {
            mCameraAgent.closeCamera(mCameraProxy, synced);
        }
        mRequestingCameraId = EMPTY_REQUEST;
        mUsingNewApi = false;
    }

    private static void checkAndOpenCamera(CameraAgent cameraManager,
            final int cameraId, Handler handler, final CameraAgent.CameraOpenCallback cb) {
        Log.i(TAG, "checkAndOpenCamera");
        try {
            cameraManager.openCamera(handler, cameraId, cb);
        } catch (Exception ex) {
            handler.post(new Runnable() {
                @Override
                public void run() {
                    cb.onCameraDisabled(cameraId);
                }
            });
        }
    }

    public boolean isNewApi() {
        return mUsingNewApi;
    }

    private void initCameraDeviceInfo() {
        CameraAgent cameraManager = mUsingNewApi ? mCameraAgentNg : mCameraAgent;
        mInfo = cameraManager.getCameraDeviceInfo();
    }
}
