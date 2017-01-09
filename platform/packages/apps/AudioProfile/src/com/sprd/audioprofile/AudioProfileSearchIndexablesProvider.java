package com.sprd.audioprofile;

import android.database.Cursor;
import android.database.MatrixCursor;
import android.os.UserHandle;
import android.provider.SearchIndexableResource;
import android.provider.SearchIndexablesProvider;
import android.util.Log;
import static android.provider.SearchIndexablesContract.COLUMN_INDEX_XML_RES_RANK;
import static android.provider.SearchIndexablesContract.COLUMN_INDEX_XML_RES_RESID;
import static android.provider.SearchIndexablesContract.COLUMN_INDEX_XML_RES_CLASS_NAME;
import static android.provider.SearchIndexablesContract.COLUMN_INDEX_XML_RES_ICON_RESID;
import static android.provider.SearchIndexablesContract.COLUMN_INDEX_XML_RES_INTENT_ACTION;
import static android.provider.SearchIndexablesContract.COLUMN_INDEX_XML_RES_INTENT_TARGET_PACKAGE;
import static android.provider.SearchIndexablesContract.COLUMN_INDEX_XML_RES_INTENT_TARGET_CLASS;
import static android.provider.SearchIndexablesContract.INDEXABLES_RAW_COLUMNS;
import static android.provider.SearchIndexablesContract.INDEXABLES_XML_RES_COLUMNS;
import static android.provider.SearchIndexablesContract.NON_INDEXABLES_KEYS_COLUMNS;

/* SPRD: Bug 450050 Settins can't search Audio Profiles @{ */
public class AudioProfileSearchIndexablesProvider extends SearchIndexablesProvider {
    private static final String TAG = "AudioProfileSearchIndexablesProvider";
    // SPRD: Modified for bug 597592, some application should not be searched under guest mode
    private boolean mIsOwner;

    private static SearchIndexableResource[] INDEXABLE_RES = new SearchIndexableResource[] {
            new SearchIndexableResource(1, R.xml.audio_profile_sound_settings,
                    AudioProfileSettings.class.getName(),
                    R.drawable.ic_settings_notifications),
    };
    @Override
    public boolean onCreate() {
        // SPRD: Modified for bug 597592, some application should not be searched under guest mode
        mIsOwner = UserHandle.myUserId() == UserHandle.USER_OWNER;
        return true;
    }

    @Override
    public Cursor queryXmlResources(String[] projection) {
        MatrixCursor cursor = new MatrixCursor(INDEXABLES_XML_RES_COLUMNS);
        /* SPRD: Modified for bug 597592, some application should not be searched under guest mode @{ */
        if (!mIsOwner) {
            return cursor;
        }
        /* @} */
        final int count = INDEXABLE_RES.length;
        for (int n = 0; n < count; n++) {
            Object[] ref = new Object[7];
            ref[COLUMN_INDEX_XML_RES_RANK] = INDEXABLE_RES[n].rank;
            ref[COLUMN_INDEX_XML_RES_RESID] = INDEXABLE_RES[n].xmlResId;
            // SPRD: Add className for AudioProfiles for bug466592
            ref[COLUMN_INDEX_XML_RES_CLASS_NAME] = INDEXABLE_RES[n].className;
            ref[COLUMN_INDEX_XML_RES_ICON_RESID] = INDEXABLE_RES[n].iconResId;
            ref[COLUMN_INDEX_XML_RES_INTENT_ACTION] = "android.intent.action.MAIN";
            ref[COLUMN_INDEX_XML_RES_INTENT_TARGET_PACKAGE] = "com.sprd.audioprofile";
            ref[COLUMN_INDEX_XML_RES_INTENT_TARGET_CLASS] = INDEXABLE_RES[n].className;
            cursor.addRow(ref);
        }
        return cursor;
    }

    @Override
    public Cursor queryRawData(String[] projection) {
        MatrixCursor cursor = new MatrixCursor(INDEXABLES_RAW_COLUMNS);
        return cursor;
    }
    @Override
    public Cursor queryNonIndexableKeys(String[] projection) {
        MatrixCursor cursor = new MatrixCursor(NON_INDEXABLES_KEYS_COLUMNS);
        return cursor;
    }
}
/* @} */