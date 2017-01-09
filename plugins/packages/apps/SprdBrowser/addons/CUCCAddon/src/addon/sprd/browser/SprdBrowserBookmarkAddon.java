
package addon.sprd.browser;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;

import com.sprd.custom.SprdBrowserBookmarkAddonStub;

import android.content.Context;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.util.Log;
import android.app.AddonManager;

public class SprdBrowserBookmarkAddon extends SprdBrowserBookmarkAddonStub implements AddonManager.InitialCallback {

    private Resources mRes;
    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mRes = context.getResources();
        return clazz;
    }

    @Override
    public CharSequence[] getBookmarks() {
        return mRes.getTextArray(R.array.bookmarks);
    }

    @Override
    public TypedArray getPreloads(){
        return mRes.obtainTypedArray(R.array.bookmark_preloads);
    }

    @Override
    public byte[] readRaw(int id) throws IOException {
        if (id == 0) {
            return null;
        }
        InputStream is = mRes.openRawResource(id);
        try {
            ByteArrayOutputStream bos = new ByteArrayOutputStream();
            byte[] buf = new byte[4096];
            int read;
            while ((read = is.read(buf)) > 0) {
                bos.write(buf, 0, read);
            }
            bos.flush();
            return bos.toByteArray();
        } finally {
            is.close();
        }
    }
}
