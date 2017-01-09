/* SPRD: add for bug623517 {@ */
package com.android.mail.browse;

import android.content.Context;
import android.content.CursorLoader;
import android.database.Cursor;
import android.net.Uri;
import android.provider.OpenableColumns;

import com.android.mail.utils.LogTag;
import com.android.mail.utils.LogUtils;

import java.io.InputStream;

public class EmlFilenameLoader extends CursorLoader {
    private static final String LOG_TAG = LogTag.getLogTag();

    private Uri mEmlFileUri;

    public EmlFilenameLoader(Context c, Uri uri) {
        super(c, uri,
                new String[] { OpenableColumns.DISPLAY_NAME, OpenableColumns.SIZE },
                null, null, null);
        mEmlFileUri = uri;
    }

    @Override
    public Cursor loadInBackground() {
        InputStream in = null;
        try {
            in = getContext().getContentResolver().openInputStream(mEmlFileUri);
            return super.loadInBackground();
        } catch (Exception e) {
            LogUtils.w(LOG_TAG, e, "EmlFilenameLoader isEmlFileExists");
        } finally {
            try {
                if (null != in) {
                    in.close();
                }
            } catch (Exception e) {
                LogUtils.w(LOG_TAG, e, "EmlFilenameLoader Failed to close emlfile stream");
            }
        }
        return null;
    }
}
/* @} */
