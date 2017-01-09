package plugin.sprd.emailxposed;

import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import com.android.emailcommon.utility.AttachmentUtilities;
import com.android.mail.compose.AttachmentsView;
import com.android.mail.compose.AttachmentsView.AttachmentFailureException;
import com.android.mail.compose.ComposeActivity;
import com.android.mail.providers.Account;
import com.android.mail.providers.Attachment;
import com.sprd.xposed.DrmXposedUtils;

import plugin.sprd.emailxposed.R;

import android.app.AddonManager;
import android.content.Context;
import android.drm.DrmStore;
import android.net.Uri;
import android.text.TextUtils;
import android.os.Environment;
import android.util.Log;
import android.widget.Toast;


public class AddonEmailDrmUtils extends DrmXposedUtils implements
    AddonManager.InitialCallback {

    private Context mAddonContext;
    private DrmUtils mDrmUtils;
    private static final String TAG = "AddonEmailDrmUtils";

    public AddonEmailDrmUtils() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        //mDrmUtils = new DrmUtils(context);
        return clazz;
    }

    @Override
    public boolean isSDFile(Context context, Uri uri) {
        boolean result = true;
        if (mDrmUtils.isDrmType(uri)
                && !mDrmUtils.haveRightsForAction(uri, DrmStore.Action.TRANSFER)) {
            Log.d(TAG, "Enter ComposeActivity: isSDFile=false");
            Toast.makeText(context,
                    mAddonContext.getString(R.string.drm_protected_file), Toast.LENGTH_LONG).show();
            result = false;
        }
        return result;
    }

    @Override
    public boolean isDrmFile(String attName, String attType) {
        Log.d(TAG, "Enter AttachmentTile: isDrmType=" + mDrmUtils.isDrmType(attName, attType));
        return mDrmUtils.isDrmType(attName, attType);
    }

    @Override
    public boolean canOpenDrm(Context context, String attName, String attType) {
        Log.d(TAG, "Enter MessageAttachmentBar: canOpenDrm =" + mDrmUtils.isDrmType(attName, attType));
        boolean result = true;
        if (mDrmUtils.isDrmType(attName, attType)) {
            Toast.makeText(context, mAddonContext.getString(R.string.not_support_open_drm), Toast.LENGTH_LONG).show();
            result = false;
        }
        return result;
    }

    @Override
    public boolean drmPluginEnabled() {
        return true;
    }

    /* SPRD: Modify for bug513132 {@ */
    public long addAttachmentsXposed(final ComposeActivity composeActivity,
            final List<Attachment> attachments, final AttachmentsView mAttachmentsView,
            final Account mAccount) {
        long size = 0;
        AttachmentFailureException error = null;
        int drmNum = 0;
        final List<Attachment> mDrmMaybeForwardAttachments = new ArrayList<Attachment>();
        for (Attachment a : attachments) {
            if (mDrmUtils.isDrmType(a.getName(), a.getContentType())) {
                if (isAttachmentProviderUri(a.contentUri)) {
                    deleteFile(a.getName());
                    mDrmMaybeForwardAttachments.add(a);
                    continue;
                /* SPRD:bug522288 modify begin @{ */
                } else if (mDrmUtils.isDrmType(a.contentUri) && !mDrmUtils.haveRightsForAction(a.contentUri,
                        DrmStore.Action.TRANSFER)) {
                    drmNum++;
                    continue;
                }
                /* @} */
            }
            try {
                size += mAttachmentsView.addAttachment(mAccount, a);
            } catch (AttachmentFailureException e) {
                error = e;
            }
        }

        if (mDrmMaybeForwardAttachments.size() > 0) {
            final Map<String, Object> data = new HashMap<String, Object>();
            data.put("size", size);
            data.put("drmNum", drmNum);
            data.put("completedCopyNum", 0);
            data.put("error", error);
            for (final Attachment a : mDrmMaybeForwardAttachments) {
                AttachmentUtilities.copyAttachmentFromInternalToExternal(composeActivity,
                        a.uri, a.contentUri,false,false,
                        new AttachmentUtilities.CopyAttachmentCallback() {
                            @Override
                            public void onCopyCompleted(String uri) {
                                data.put("completedCopyNum",((int)data.get("completedCopyNum")) + 1);
                                /* SPRD:bug522288 modify begin @{ */
                                if(uri != null && mDrmUtils.isDrmType(Uri.parse(uri))
                                        && !mDrmUtils.haveRightsForAction(Uri.parse(uri), DrmStore.Action.TRANSFER)) {
                                    data.put("drmNum",((int)data.get("drmNum")) + 1);
                                /* @} */
                                } else {
                                    try {
                                        long size = mAttachmentsView.addAttachment(mAccount,a);
                                        data.put("size", ((long)data.get("size")) + size);
                                    } catch (AttachmentFailureException e) {
                                        data.put("error", e);
                                    }
                                }
                                if ((int)data.get("completedCopyNum") == mDrmMaybeForwardAttachments.size()) {
                                    addAttachmentsToast((int)data.get("drmNum"), data.get("error"), composeActivity, attachments);
                                }
                            }
                        });
            }
        } else {
            addAttachmentsToast(drmNum, error, composeActivity, attachments);
        }
        return size;
    }

    public void addAttachmentsToast(int drmNum, Object error,
            ComposeActivity composeActivity,List<Attachment> attachments) {
        if (drmNum > 0) {
            composeActivity.showErrorToast(mAddonContext.getString(R.string.not_add_drm_file));
        }

        if (error != null) {
            Log.e(TAG, "Error adding attachment: " + error);
            if (attachments.size() > 1) {
                composeActivity.showAttachmentTooBigToast(com.android.mail.R.string.too_large_to_attach_multiple);
            } else {
                composeActivity.showAttachmentTooBigToast(((AttachmentFailureException)error).getErrorRes());
            }
        }
    }

    private boolean deleteFile(String fileName) {
        File downloads = Environment
                .getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS + "/" + fileName);
        if (downloads.exists()) {
            return downloads.delete();
        }
        return false;
    }

    public static boolean isAttachmentProviderUri(Uri uri) {
        return "com.android.email.attachmentprovider".equals(uri.getAuthority());
    }
    /* @} */
    public void sendContext(Context context) {
        mDrmUtils = new DrmUtils(context);
    }
}
