package android.hardware.camera2.impl;

import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.ICameraDeviceUser;

/**
 * @hide
 */
public class SprdICameraDeviceUserWrapper extends ICameraDeviceUserWrapper {

    public SprdICameraDeviceUserWrapper(ICameraDeviceUser remoteDevice) {
        super(remoteDevice);
    }

    public int cancelPicture() throws CameraAccessException {
        try {
            return mRemoteDevice.cancelPicture();
        } catch (Throwable t) {
            CameraManager.throwAsPublicException(t);
            throw new UnsupportedOperationException("Unexpected exception", t);
        }
    }
}
