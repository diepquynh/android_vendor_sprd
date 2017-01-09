package android.hardware.camera2.impl;

import java.util.List;

import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraCaptureSessionEx;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraConstrainedHighSpeedCaptureSession;

import android.os.Handler;
import android.view.Surface;

/**
 * @hide
 */
public class SprdCameraConstrainedHighSpeedCaptureSessionImpl extends CameraConstrainedHighSpeedCaptureSessionImpl
        implements CameraCaptureSessionEx {

    SprdCameraConstrainedHighSpeedCaptureSessionImpl(int id, List<Surface> outputs,
            CameraCaptureSession.StateCallback callback, Handler stateHandler,
            android.hardware.camera2.impl.CameraDeviceImpl deviceImpl,
            Handler deviceStateHandler, boolean configureSuccess,
            CameraCharacteristics characteristics) {
        super(id, outputs, callback, stateHandler, deviceImpl
                , deviceStateHandler, configureSuccess, characteristics);
    }

    /**
     * SPRD: fix bug 473462 add for burst capture
     */
    public int cancelPicture() throws CameraAccessException {
        return -1;
    }
}
