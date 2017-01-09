package com.sprd.voicetrigger.provider;

import android.net.Uri;
import android.provider.BaseColumns;

public class MyProviderMetaData {
    public static final String AUTHORITY = "com.sprd.voicetrigger.provider";
    public static final String DATABASE_NAME = "config.db";
    public static final int DATABASE_VERSION = 2;

    public static final class ConfigTableMetaData implements BaseColumns {

        public static final String TABLE_NAME = "config";
        public static final Uri CONTENT_URI = Uri.parse("content://" + AUTHORITY + "/config");
        public static final String CONTENT_TYPE = "vnd.android.cursor.dir/com.sprd.voicetrigger.config";
        public static final String CONTENT_ITEM_TYPE = "vnd.android.cursor.item/com.sprd.voicetrigger.config";

        public static final String SENSIBILITY = "sensibility";
        public static final String ISOPENSWITCH = "isOpenSwitch";
        public static final String ISDEFAULTMODE = "isDefaultMode";

        public static final String SQL_CREATE_TABLE = "CREATE TABLE " + TABLE_NAME
                + " ("
                + SENSIBILITY + " INTERGER NOT NULL,"
                + ISOPENSWITCH + " BOOLEAN NOT NULL,"
                + ISDEFAULTMODE + " BOOLEAN NOT NULL "
                + ");";
    }
}
