package com.sprd.systemupdate;

import android.text.TextUtils;
import org.json.JSONException;
import org.json.JSONObject;

public class VersionInfo {
    String mVersion;
    String mDate;
    int mSize;
    String mUrl;
    String mReleaseNote;
    String mDelta_name;

    public static VersionInfo fromJson(String json) {
        if (TextUtils.isEmpty(json)) {
            return null;
        }
        try {
            JSONObject jsonObj = new JSONObject(json);
            VersionInfo ret = new VersionInfo();
            ret.mVersion = jsonObj.getString("version");
            ret.mDate = jsonObj.getString("date");
            ret.mSize = jsonObj.getInt("size");
            ret.mUrl = jsonObj.getString("delta_path");
            ret.mReleaseNote = jsonObj.getString("release_note");
            ret.mDelta_name = jsonObj.getString("delta_name");
            return ret;
        } catch (JSONException e) {
            e.printStackTrace();
            return null;
        }
    }

    public boolean equals(VersionInfo info) {
        return info != null && info.mVersion.equals(mVersion)
                && mDate.equals(info.mDate) && mSize == info.mSize
                && mUrl.equals(info.mUrl)
                && mReleaseNote.equals(info.mReleaseNote);
    }
}
