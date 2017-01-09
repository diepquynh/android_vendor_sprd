package plugin.sprd.drm;

import android.app.AddonManager;
import android.content.Context;
import android.database.Cursor;
import android.drm.DrmManagerClient;
import android.drm.DrmManagerClient;
import android.drm.DrmStore;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.provider.MediaStore.Images;
import android.provider.Telephony.Mms;
import android.text.TextUtils;
import android.util.Log;
import android.widget.Toast;

import com.google.android.mms.pdu.PduBody;
import com.google.android.mms.pdu.PduPart;

import com.android.mms.MmsApp;
import com.android.mms.model.AudioModel;
import com.android.mms.model.VideoModel;
import com.android.mms.ui.UriImage;
import com.sprd.mms.drm.DrmModelUtils;
import com.sprd.mms.drm.DrmUtilsSprd;

public class PluginModelUtils extends DrmModelUtils implements
        AddonManager.InitialCallback {
    private static final String TAG = "PluginModelUtils";
    private Context mContext;

    public PluginModelUtils() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mContext = context;
        return clazz;
    }

    @Override
    public void setImageType(UriImage uriImage, Uri uri, String extension) {
        Log.d(TAG, " setImageType");
        if (DrmUtilsSprd.getInstance(mContext).isDrmType(uri, null)
                && extension.toLowerCase().equals("dcf")) {
            uriImage.mContentType = "image/dcf";
        }
    }

    @Override
    public void setImageTypeBySrc(UriImage uriImage, String src) {
        Log.d(TAG, " setImageTypeBySrc");
        if (DrmUtilsSprd.getInstance(mContext).isDrmSrc(src)) {
            uriImage.mSrc = uriImage.mSrc.substring(uriImage.mSrc
                    .lastIndexOf('/') + 1);
            uriImage.mContentType = "image/dcf";
        }
    }

    @Override
    public void setAudioTypeBySrc(AudioModel model, String src) {
        Log.d(TAG, " setAudioTypeBySrc");
        if (DrmUtilsSprd.getInstance(mContext).isDrmSrc(src)) {
            model.mContentType = "audio/dcf";
        }
    }
    @Override
    public void setAudioTypeByExt(AudioModel model, String extension) {
        Log.d(TAG, "setAudioTypeByExt");
        if (extension.toLowerCase().equals("dcf")) {
            model.mContentType = "audio/dcf";
        }
    }

    @Override
    public void setVideoTypeBySrc(VideoModel model, String src) {
        Log.d(TAG, " setVideoTypeBySrc");
        if (src.endsWith("dcf")) {
            model.mContentType = "video/dcf";
        }
    }

    @Override
    public void setVideoTypeByExt(VideoModel model, String extension) {
        Log.d(TAG, " setVideoTypeByExt");
        if (extension.toLowerCase().equals("dcf")) {
            model.mContentType = "video/dcf";
        }
    }

    @Override
    public void setVideoSrc(VideoModel model, Cursor c) {
        Log.d(TAG, " setVideoSrc");
        int nameIndex = c.getColumnIndex(Images.Media.DISPLAY_NAME);
        if (nameIndex != -1) {
            String fileName = c.getString(nameIndex);
            if (!TextUtils.isEmpty(fileName)) {
                fileName = fileName.replace(' ', '_');
                //SPRD：ADD FOR BUG 403026
                model.mSrc = fileName.substring(fileName.lastIndexOf('/') + 1);
            }
        }
    }
}
