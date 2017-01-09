
package com.sprd.contacts;

public class DeduplicationCandidate {
    public long mRawContactId;
    public String mName;
    public String mNumber;
    public long mPhotoId;
    public long mAccountId;
    public String mAccountName;

    public DeduplicationCandidate(long mRawContactId, String mName,
            String mNumber, long mPhotoId, long mAccountId, String accountName) {
        this.mRawContactId = mRawContactId;
        this.mName = mName;
        this.mNumber = mNumber;
        this.mPhotoId = mPhotoId;
        this.mAccountId = mAccountId;
        this.mAccountName = accountName;
    }

}
