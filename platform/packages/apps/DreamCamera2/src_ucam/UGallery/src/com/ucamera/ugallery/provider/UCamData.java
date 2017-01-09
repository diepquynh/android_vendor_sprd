/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.ugallery.provider;

import android.net.Uri;
import android.provider.BaseColumns;

public class UCamData {
    private static final String CONTENT  = "content://";
    public static final String AUTHORITY = "com.ucamera.uphoto.provider";
    public static final Uri CONTENT_ALL_URI = Uri.parse(CONTENT + AUTHORITY);
    public static final String THUMBNAILS = "thumbnails";
    public static final String ALBUMS = "albums";

    private UCamData() {

    }

    public static final class Thumbnails implements BaseColumns {
        public static final Uri CONTENT_URI = Uri.parse(CONTENT + AUTHORITY + "/" + THUMBNAILS);

        public static final String DEFAULT_SORT_ORDER = " imageId ASC";

        public static final String THUMB_ID = "imageId";
        public static final String THUMB_PATH = "imagePath";
        public static final String THUMB = "data";
        public static final String THUMB_DATE = "modified";

        private Thumbnails() {

        }
    }

    public static final class Albums implements BaseColumns {
        public static final Uri CONTENT_URI = Uri.parse(CONTENT + AUTHORITY + "/" + ALBUMS);

        public static final String DEFAULT_SORT_ORDER = "album_id  ASC";

        public static final String ALBUM_BUCKET = "album_id";
        public static final String IMAGE_NAME = "image_name";
        public static final String ALBUM = "data";
        private Albums() {

        }
    }
}
