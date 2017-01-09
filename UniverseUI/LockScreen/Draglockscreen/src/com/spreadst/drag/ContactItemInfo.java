/** Create by Spreadst */

package com.spreadst.drag;

import android.content.Intent;

public class ContactItemInfo {
    int mFastNumber; // 1,2,3,4...,9
    boolean mIsMissed = false; // has number or not
    String mCallNumber; // number if has
    long mContactId; // contact id
    String mNameStr; // name to show at bottom
    long mPhotoId; // photo id to find picture
    Intent mIntent; // extra info
    int voiceMailPhoneId;
}
