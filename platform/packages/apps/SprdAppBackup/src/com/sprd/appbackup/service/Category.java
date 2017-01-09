package com.sprd.appbackup.service;

import android.os.Parcel;
import android.os.Parcelable;
public class Category implements Parcelable{
    int mCode;
    String mDescription;

    public Category(int code, String description){
        mCode = code;
        mDescription = description;
    }
    public Category(){
        mCode = 0;
        mDescription = null;
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeInt(mCode);
        dest.writeString(mDescription);
    }

    private Category(Parcel in){
        mCode = in.readInt();
        mDescription = in.readString();
    }

    public static final Parcelable.Creator<Category>
    CREATOR = new Parcelable.Creator<Category>() {
        public Category createFromParcel(Parcel in) {
        return new Category(in);
        }

        public Category[] newArray(int size) {
        return new Category[size];
        }
    };

    @Override
    public String toString() {
        return "mDescription = "+mDescription+"/code = "+mCode;
    }
    public int getmCode() {
        return mCode;
    }

    public String getmDescription() {
        return mDescription;
    }

}
