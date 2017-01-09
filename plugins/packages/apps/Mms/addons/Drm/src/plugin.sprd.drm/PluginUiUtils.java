package plugin.sprd.drm;

import android.app.AddonManager;
import android.content.Context;
import android.content.ContentUris;
import android.content.res.Resources;
import android.database.Cursor;
import android.drm.DrmManagerClient;
import android.drm.DrmManagerClient;
import android.drm.DrmStore;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.os.Bundle;
import android.os.SystemProperties;
import android.provider.MediaStore.Images;
import android.provider.Telephony.Mms;
import android.text.TextUtils;
import android.util.Log;
import android.widget.Toast;
import android.view.Menu;

import com.google.android.mms.pdu.PduBody;
import com.google.android.mms.pdu.PduPart;

import com.android.mms.MmsApp;
import com.android.mms.model.SlideshowModel;
import com.android.mms.ui.MessageListItem.ImageLoadedCallback;
import com.android.mms.ui.UriImage;
import com.sprd.mms.drm.DrmUiUtils;
import com.sprd.mms.drm.DrmUtilsSprd;

import plugin.sprd.drm.R;

public class PluginUiUtils extends DrmUiUtils implements
        AddonManager.InitialCallback {
    private static final String TAG = "PluginUiUtils";
    private Context mContext;

    public PluginUiUtils() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mContext = context;
        return clazz;
    }

    @Override
    public boolean setItemImage(ImageLoadedCallback callback, long messageId) {
        Log.d(TAG, "setItemImage");
        if (isMmsWithDrmFile(messageId)) {
            callback.mListItem.setImage(null, null);
            return true;
        }
        return false;
    }

    @Override
    public Bitmap createDrmBitmap(String name, Bitmap bitmap) {
        Log.d(TAG, "createDrmBitmap");
        String extension = getExtensionByName(name);
        if (!isDrmExtension(extension)) {
            return null;
        }
        if (bitmap != null) {
            if (!bitmap.isRecycled()) {
                bitmap.recycle();
            }
            bitmap = null;
        }
        try {
            bitmap = BitmapFactory.decodeResource(mContext.getResources(),
                    R.drawable.ic_missing_thumbnail_picture_drm);
        } catch (java.lang.OutOfMemoryError e) {
            Log.e(TAG, "setImage: out of memory: ", e);
        }
        return bitmap;
    }

    @Override
    public boolean isMmsWithDrmFile(long msgId) {
        Log.d(TAG, " isMmsWithDrmFile");
        PduBody body = null;
        try {
            body = SlideshowModel.getPduBody(mContext,
                    ContentUris.withAppendedId(Mms.CONTENT_URI, msgId));
        } catch (Exception e) {
            Log.e(TAG, "can't load pdu body: " + msgId);
        }
        if (body == null) {
            return false;
        }
        return isDrmBody(body);
    }

    @Override
    public boolean isDrmBody(PduBody body) {
        Log.d(TAG, " isDrmBody");
        if (body == null) {
            return false;
        }
        int partNum = body.getPartsNum();
        for (int i = 0; i < partNum; i++) {
            PduPart part = body.getPart(i);
            if (isDrmPart(part)) {
                return true;
            }
        }
        return false;
    }

    @Override
    public boolean isDrmPart(PduPart part) {
        Log.d(TAG, " isDrmPart");
        if (part == null) {
            return false;
        }
        DrmManagerClient drmManagerClient = MmsApp.getApplication()
                .getDrmManagerClient();
        String type = new String(part.getContentType());
        if (drmManagerClient.canHandle("", type)
                && DrmUtilsSprd.getInstance(mContext).isDrmEnabled()) {
            return true;
        }
        return false;
    }

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

    public boolean isDrmExtension(String entension) {
        Log.d(TAG, "isDrmExtension");
        return entension.toLowerCase().equals("dcf");
    }
}
