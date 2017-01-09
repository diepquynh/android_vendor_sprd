
package com.sprd.firewall.db;

import android.net.Uri;
import android.provider.BaseColumns;

public class BlackColumns {
    public static final String AUTHORITY = "com.sprd.providers.block";

    public static final class BlackMumber implements BaseColumns {
        public static final Uri CONTENT_URI = Uri
                .parse("content://com.sprd.providers.block/black_mumbers");

        public static final String MUMBER_VALUE = "mumber_value";

        public static final String BLOCK_TYPE = "block_type";

        public static final String NOTES = "notes";

        public static final String NAME = "name";

        public static final String MIN_MATCH = "min_match";
    }

    public static final class BlockRecorder implements BaseColumns {
        public static final Uri CONTENT_URI = Uri
                .parse("content://com.sprd.providers.block/block_recorded");

        public static final String NUMBER_VALUE = "number_value";

        public static final String CALL_TYPE = "call_type";

        public static final String BLOCK_DATE = "block_date";

        public static final String NAME = "name";
    }

    public static final class SmsBlockRecorder implements BaseColumns {
        public static final Uri CONTENT_URI = Uri
                .parse("content://com.sprd.providers.block/sms_block_recorded");

        public static final String NUMBER_VALUE = "number_value";

        public static final String BLOCK_SMS_CONTENT = "sms_content";

        public static final String BLOCK_DATE = "block_date";

        public static final String NAME = "name";
    }
}
