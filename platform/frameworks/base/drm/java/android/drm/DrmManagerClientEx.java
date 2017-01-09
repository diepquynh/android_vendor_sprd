/*
 * Copyright (C) 2010 The Android Open Source Project
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

package android.drm;
import android.content.Context;

/**
 * The main programming interface for the DRM framework. An application must instantiate this class
 * to access DRM agents through the DRM framework.
 */

public class DrmManagerClientEx extends DrmManagerClient {

    public DrmManagerClientEx(Context context) {
        super(context);
    }
    public int setPlaybackStatus(DecryptHandle decryptHandle,int playbackStatus) {
        return _setPlaybackStatus(mUniqueId, decryptHandle, playbackStatus);
    }

    public int closeDecryptSession(DecryptHandle handle) {
        return _closeDecryptSession(mUniqueId, handle);
    }

    public DecryptHandle openDecryptSession(String path) {
        return _openDecryptSession(mUniqueId, path, null);
    }

    public byte[] pread(DecryptHandle handle, int offset, int size) {
        return _pread(mUniqueId, handle, offset, size);
    }

    private native int  _closeDecryptSession(int uniqueId, DecryptHandle handle);
    private native DecryptHandle _openDecryptSession(int uniqueId, String path, String mimeType);
    private native int _setPlaybackStatus(int uniqueId, DecryptHandle decryptHandle, int playbackStatus);
    private native byte[] _pread(int uniqueId, DecryptHandle decryptHandle, int size, int offset);
}
