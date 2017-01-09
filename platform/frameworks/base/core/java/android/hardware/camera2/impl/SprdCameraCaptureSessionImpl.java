package android.hardware.camera2.impl;

import java.util.List;

import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraCaptureSessionEx;
import android.hardware.camera2.CameraDevice;
import android.os.Handler;
import android.view.Surface;

/**
 * @hide
 */
public class SprdCameraCaptureSessionImpl extends CameraCaptureSessionImpl implements CameraCaptureSessionEx {

    SprdCameraCaptureSessionImpl(int id, Surface input, List<Surface> outputs,
            CameraCaptureSession.StateCallback callback, Handler stateHandler,
            android.hardware.camera2.impl.CameraDeviceImpl deviceImpl,
            Handler deviceStateHandler, boolean configureSuccess) {
        super(id, input, outputs, callback, stateHandler, deviceImpl
                , deviceStateHandler, configureSuccess);
    }

    /**
     * SPRD: Fix bug 473462 add for burst capture
     */
    public int cancelPicture() throws CameraAccessException {
        if (getDevice() instanceof SprdCameraDeviceImpl) {
            return ((SprdCameraDeviceImpl)getDevice()).cancelPicture();
        }
        return -1;
    }
}
