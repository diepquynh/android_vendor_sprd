/*
 * Copyright (C) 2010 he Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License
 */

package addon.sprd.browser.bookmark;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;

import com.sprd.bookmark.SprdBrowserBookmarkAddonStub;

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
