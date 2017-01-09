package plugin.sprd.emailxposed;

import android.content.Context;
import android.drm.DrmManagerClient;
import android.drm.DrmStore;
import android.net.Uri;
import android.os.SystemProperties;
import android.util.Log;

public class DrmUtils {

    private static final String TAG = "DrmUtils";
    private static DrmManagerClient mDrmManagerClient;
    private static Object lock = new Object();

    public DrmUtils(Context context) {
        if(mDrmManagerClient == null) {
            synchronized (lock) {
                if (mDrmManagerClient == null) {
                    mDrmManagerClient = new DrmManagerClient(context.getApplicationContext());
                }
            }
        }
    }

    public boolean isDrmEnabled() {
        String prop = SystemProperties.get("drm.service.enabled");
        return prop != null && prop.equals("true");
    }

    public boolean isDrmType(Uri uri) {
        boolean result = false;
        if (mDrmManagerClient != null) {
            try {
                /* SPRD:bug522288 modify begin @{ */
                if (mDrmManagerClient.canHandle(uri, null) && mDrmManagerClient.getMetadata(uri) != null) {
                    result = true;
                }
                /* @} */
            } catch (IllegalArgumentException ex) {
                Log.e(TAG, "canHandle called with wrong parameters");
            } catch (IllegalStateException ex) {
                Log.e(TAG, "DrmManagerClient didn't initialize properly");
            /* SPRD:bug611315 modify begin @{ */
            } catch (UnsupportedOperationException ex) {
                Log.e(TAG, "UnsupportedOperationException");
            }
            /* @} */
        }
        return result;
    }

    public boolean isDrmType(String path, String mimeType) {
        boolean result = false;
        if (mDrmManagerClient != null) {
            try {
                if (mDrmManagerClient.canHandle(path, mimeType)) {
                    result = true;
                }
            } catch (IllegalArgumentException ex) {
                Log.e(TAG, "canHandle called with wrong parameters");
            } catch (IllegalStateException ex) {
                Log.e(TAG, "DrmManagerClient didn't initialize properly");
            } catch (UnsupportedOperationException ex) {
                Log.e(TAG,"UnsupportedOperationException");
            }
        }
        return result;
    }

    public boolean haveRightsForAction(Uri uri, int action) {
        boolean result = false;
        try {
            if (mDrmManagerClient.canHandle(uri, null)) {
                result = (mDrmManagerClient.checkRightsStatus(uri, action)
                        == DrmStore.RightsStatus.RIGHTS_VALID);
            }
        } catch (Exception e) {
            // Ignore exception and assume it is OK to forward file.
            Log.e(TAG, "Exception happens in haveRightsForAction. \n" + e);
        }
        return result;
    }
}
