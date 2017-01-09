package android.hardware.camera2.impl;

import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraDevice;
import android.os.Handler;
import android.os.ServiceSpecificException;
import android.hardware.camera2.CameraDevice.StateCallback;
import android.hardware.camera2.impl.ICameraDeviceUserWrapper;
import android.net.StaticIpConfiguration;
import android.os.RemoteException;
import android.util.Log;

/**
 * @hide
 */
public class SprdCameraDeviceImpl extends CameraDeviceImpl {

    private static String TAG = null;

    public SprdCameraDeviceImpl(String cameraId, StateCallback callback, Handler handler,
            CameraCharacteristics characteristics) {
        super(cameraId, callback, handler, characteristics);
    }

    /**
     * SPRD:fix bug 473462 add for burst capture
     */
    public int cancelPicture() throws CameraAccessException {
        if (TAG == null) {
            final int MAX_TAG_LEN = 23;
            String tag = String.format("CameraDevice-JV-%s", getId());
            if (tag.length() > MAX_TAG_LEN) {
                tag = tag.substring(0, MAX_TAG_LEN);
            }
            TAG = tag;
        }
        
        int count = 0;
        try {
            Log.d(TAG, "cancelPicture cameraDeviceImpl ");
            if (mRemoteDevice != null) {
                count = mRemoteDevice.cancelPicture();
            }
            Log.i(TAG, "cameraDeviceImpl cancelPicture count=" + count);
        } catch (ServiceSpecificException e) {
            Log.d(TAG, "cameraDeviceImpl cancelPicture exception");
            throw new IllegalStateException("cameraDeviceImpl cancelPicture exception", e);
        }
        return count;
    }
}
