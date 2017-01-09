package plugin.sprd.drm;

import android.app.AddonManager;
import android.content.ContentUris;
import android.content.Context;
import android.content.res.Resources;
import android.drm.DrmManagerClient;
import android.drm.DrmStore;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.os.SystemProperties;
import android.provider.Telephony.Mms;
import android.util.Log;
import android.widget.Toast;
import android.webkit.MimeTypeMap;

import com.google.android.mms.pdu.PduBody;
import com.google.android.mms.pdu.PduPart;
import com.google.android.mms.MmsException;

import com.android.mms.MmsApp;
import com.android.mms.ui.UriImage;
import com.android.mms.model.SlideshowModel;
import com.sprd.mms.drm.DrmUtilsSprd;

import plugin.sprd.drm.R;

public class PluginDrmUtils extends DrmUtilsSprd implements
        AddonManager.InitialCallback {
    private static final String TAG = "PluginDrmUtils";
    private Context mContext;

    public PluginDrmUtils() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mContext = context;
        return clazz;
    }

    @Override
    public boolean isDrmSrc(String src) {
        Log.d(TAG, "isDrmSrc");
        return (src.endsWith("dcf"));
    }

    @Override
    public boolean isDrmExtension(String entension) {
        Log.d(TAG, "isDrmExtension");
        return entension.toLowerCase().equals("dcf");
    }

    @Override
    public boolean isDrmType(String mimeType) {
        Log.d(TAG, "isDrmType");
        return isDrmType("", mimeType);
    }

    @Override
    public boolean isDrmType(String path, String mimeType) {
        Log.d(TAG, "isDrmType,path = " + path + ",mimeTYpe = " + mimeType);
        boolean result = false;
        DrmManagerClient drmManagerClient = MmsApp.getApplication()
                .getDrmManagerClient();
        if (drmManagerClient != null) {
            try {
                if (drmManagerClient.canHandle(path, mimeType)
                        && isDrmEnabled()) {
                    result = true;
                    if(isDrmExtension(MimeTypeMap.getFileExtensionFromUrl(path))){
                        Toast.makeText(mContext, R.string.drm_type_error, Toast.LENGTH_LONG)
                        .show();
                        result = false;
                    }
                }
            } catch (IllegalArgumentException ex) {
                Log.w(TAG, "canHandle called with wrong parameters");
            } catch (IllegalStateException ex) {
                Log.w(TAG, "DrmManagerClient didn't initialize properly");
            }
        }
        Log.d(TAG, "isDrmType ,result = " + result);
        return result;
    }

    @Override
    public boolean isDrmType(Uri uri, String mimeType) {
        Log.d(TAG, "isDrmType,uri = " + uri + ",mimeType = " + mimeType);
        boolean result = false;
        DrmManagerClient drmManagerClient = MmsApp.getApplication()
                .getDrmManagerClient();
        if (drmManagerClient != null) {
            try {
                if (drmManagerClient.canHandle(uri, mimeType) && isDrmEnabled()) {
                    result = true;
                }
            } catch (IllegalArgumentException ex) {
                Log.w(TAG, "canHandle called with wrong parameters");
            } catch (IllegalStateException ex) {
                Log.w(TAG, "DrmManagerClient didn't initialize properly");
            } catch (SecurityException e) {
                Log.e(TAG, "SecurityException occurs when add image", e);
            }
        }
        Log.d(TAG, "isDrmType ,result = " + result);
        return result;
    }

    @Override
    public boolean haveRightsForAction(Uri uri, int action) {
        Log.d(TAG, "haveRightsForAction");
        DrmManagerClient drmManagerClient = MmsApp.getApplication()
                .getDrmManagerClient();

        try {
            if (drmManagerClient.canHandle(uri, null) && isDrmEnabled()) {
                int check = drmManagerClient.checkRightsStatus(uri, action);
                return (check == DrmStore.RightsStatus.RIGHTS_VALID);
            }
        } catch (Exception e) {
            // Ignore exception and assume it is OK to forward file.
        } finally {
        }
        return true;
    }

    @Override
    public boolean isDrmEnabled() {
        Log.d(TAG, " isDrmEnabled");
        String prop = SystemProperties.get("drm.service.enabled");
        return prop != null && prop.equals("true");
    }

    @Override
    public boolean isAttachHasRight(Uri uri) {
        Log.d(TAG, " isAttachHasRight uri=" + uri);
        boolean isDrmType = uri != null && isDrmType(uri, null);
        boolean haveRights = uri != null
                && !haveRightsForAction(uri, DrmStore.Action.TRANSFER);
        boolean result = isDrmType && !haveRights;
        Log.d(TAG, " isAttachHasRight,result=" + result);
        if (!isDrmType) {
            return true;
        }
        if (!result) {
            Toast.makeText(mContext, R.string.protected_file, Toast.LENGTH_LONG)
                    .show();
        }
        return false;
    }

    @Override
    public boolean isAttachHasRight(Uri uri, int type) {
        Log.d(TAG, " isAttachHasRight, uri = " + uri + ",type=" + type);
        boolean isDrmType = uri != null && isDrmType(uri, null);
        boolean haveRights = uri != null
                && !haveRightsForAction(uri, DrmStore.Action.TRANSFER);
        boolean result = isDrmType && !haveRights;
        Log.d(TAG, " isAttachHasRight, result=" + result);
        if (!isDrmType) {
            return true;
        }
        if (!result) {
            Resources res = mContext.getResources();
            int resId = 0;
            if (type == TYPE_IMAGE) {
                resId = R.string.type_picture;
            } else if (type == TYPE_AUDIO) {
                resId = R.string.type_audio;
            } else if (type == TYPE_VIDEO) {
                resId = R.string.type_video;
            }
            String message = res.getString(R.string.failed_to_add_media,
                    res.getString(resId));
            Toast.makeText(mContext, message, Toast.LENGTH_SHORT).show();

        }
        return result;
    }

    @Override
    public boolean isForwardable(long msgId) {
        Log.i(TAG, "isForwardable() msgId = " + msgId);
        if (msgId < 0) {
            return false;
        }
        PduBody body = null;
        try {
            body = SlideshowModel.getPduBody(mContext,
                    ContentUris.withAppendedId(Mms.CONTENT_URI, msgId));
        } catch (MmsException e) {
            Log.e(TAG, "getDrmMimeType can't load pdu body: " + msgId);
        }
        if (body == null) {
            return false;
        }

        int partNum = body.getPartsNum();
        for (int i = 0; i < partNum; i++) {
            PduPart part = body.getPart(i);
            String type = new String(part.getContentType());

            if (isDrmType(type)
                    && !haveRightsForAction(part.getDataUri(),
                            DrmStore.Action.TRANSFER)) {
                return false;
            }
        }
        return true;
    }

    @Override
    public Bitmap createEmptyImageBitmapForDrm() {
        Log.d(TAG, " createEmptyImageBitmapForDrm");
        return BitmapFactory.decodeResource(mContext.getResources(),
                R.drawable.ic_missing_thumbnail_picture_drm);
    }

    @Override
    public String getExtensionByName(String name) {
        Log.d(TAG, "getExtensionByName");
        String extension = "";
        if (name != null) {
            int dotPos = name.lastIndexOf('.');
            if (0 <= dotPos) {
                extension = name.substring(dotPos + 1);
            }
        }
        return extension;
    }
}
