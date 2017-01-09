/*
 * Copyright (C) 2016 The Android Open Source Project
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
 * limitations under the License.
 */
package com.android.browser.sitenavigation;

import android.net.Uri;

public class SiteNavigation {

    public static final int WEBSITE_NUMBER = 9;

    public static final int WEBSITE_NUMBER_FOR_TABLET = 8;

    public static final String AUTHORITY = "com.android.browser.site_navigation";

    public static final String BROWSER_NAVIGATION = "browser:navigation";

    //site navigation url
    public static final String SITE_NAVIGATION = "content://" + AUTHORITY + "/" + "websites"; // + "/";

    public static final Uri SITE_NAVIGATION_URI = Uri.parse("content://com.android.browser.site_navigation/websites");

    public static final String ID = "_id";

    public static final String URL = "url";

    public static final String TITLE = "title";

    public static final String DATE_CREATED = "created";

    public static final String WEBSITE = "website";

    public static final String FAVICON = "favicon";

    public static final String THUMBNAIL = "thumbnail";

    public static final String DEFAULT_THUMB = "default_thumb";

    public static final String DEFAULT_TITLE = "";

}
