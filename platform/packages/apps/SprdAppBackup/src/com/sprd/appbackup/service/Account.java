package com.sprd.appbackup.service;

import android.os.Parcel;
import android.os.Parcelable;

public class Account implements Parcelable{

    private String mAccountId;
    private String mAccountName;
    private String mAccountType;
    private boolean mChecked;
    private boolean mAvailable;

    public Account(String mAccountId, String mAccountName, String mAccountType,
            boolean mChecked, boolean mAvailable) {
        super();
        this.mAccountId = mAccountId;
        this.mAccountName = mAccountName;
        this.mAccountType = mAccountType;
        this.mChecked = mChecked;
        this.mAvailable = mAvailable;
    }

    public Account() {}
    private Account(Parcel in){
        mAccountId = in.readString();
        mAccountName = in.readString();
        mAccountType = in.readString();
    }
    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeString(mAccountId);
        dest.writeString(mAccountName);
        dest.writeString(mAccountType);
    }

    public static final Parcelable.Creator<Account>
    CREATOR = new Parcelable.Creator<Account>() {
        public Account createFromParcel(Parcel in) {
        return new Account(in);
        }

        public Account[] newArray(int size) {
        return new Account[size];
        }
    };
    public String getAccountId() {
        return mAccountId;
    }

    public void setAccountId(String mAccountId) {
        this.mAccountId = mAccountId;
    }

    public String getAccountName() {
        return mAccountName;
    }

    public void setAccountName(String mAccountName) {
        this.mAccountName = mAccountName;
    }

    public boolean isChecked() {
        return mChecked;
    }

    public void setChecked(boolean mChecked) {
        this.mChecked = mChecked;
    }

    public boolean isAvailable() {
        return mAvailable;
    }

    public void setAvailable(boolean mAvailable) {
        this.mAvailable = mAvailable;
    }


    public String getAccountType() {
        return mAccountType;
    }

    public void setAccountType(String mAccountType) {
        this.mAccountType = mAccountType;
    }

    @Override
    public String toString() {
        return "Account [mAccountId=" + mAccountId + ", mAccountName="
                + mAccountName + ", mAccountType=" + mAccountType
                + ", mChecked=" + mChecked + ", mAvailable=" + mAvailable + "]";
    }

}
