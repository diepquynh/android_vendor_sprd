/** Created by Spreadtrum */
package com.sprd.sprdlauncher2.widget.contacts;

import android.content.Intent;

public class ContactItemInfo {
    long mContactId;
    int mFastNumber;
    String mCallNumber;
    String mNameStr;
    long mPhotoId;
    Intent mIntent;
    boolean mIsMissed = false;
    String mSortkey;
    String mLookUpKey;
}
