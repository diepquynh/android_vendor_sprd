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

package addon.sprd.browser.custom;

import com.sprd.custom.SprdBrowserSiteNavigationAddonStub;

import android.content.Context;
import android.util.Log;
import android.app.AddonManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;

import java.io.ByteArrayOutputStream;

public class SprdBrowserSiteNavigationAddon extends SprdBrowserSiteNavigationAddonStub implements AddonManager.InitialCallback {

    private Context mContext;
    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mContext = context;
        return clazz;
    }

    @Override
    public CharSequence[] getPredefinedWebsites() {
        return mContext.getResources().getTextArray(R.array.predefined_websites);
    }

    @Override
    public Bitmap readRaw(String name) {
        Bitmap bm = null;
        ByteArrayOutputStream os = null;
        int id = mContext.getResources().getIdentifier(name, "raw", mContext.getPackageName());
        bm = BitmapFactory.decodeResource(mContext.getResources(), id);
        
        return bm;
    }
}
