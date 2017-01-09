/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.internal.telephony.cat;

import com.android.internal.telephony.cat.TextMessage;

import android.os.Parcel;
import android.os.Parcelable;

public class DtmfMessage implements Parcelable {

    public String mdtmfString = null;
    TextMessage mTextMsg = null;

    public DtmfMessage() {
    }

    private DtmfMessage(Parcel in) {
        mdtmfString = in.readString();
        mTextMsg = in.readParcelable(null);
    }

    public int describeContents() {
        return 0;
    }

    public void writeToParcel(Parcel dest, int flags) {
        dest.writeString(mdtmfString);
        dest.writeParcelable(mTextMsg, 0);
    }

    public static final Parcelable.Creator<DtmfMessage> CREATOR = new Parcelable.Creator<DtmfMessage>() {
        public DtmfMessage createFromParcel(Parcel in) {
            return new DtmfMessage(in);
        }

        public DtmfMessage[] newArray(int size) {
            return new DtmfMessage[size];
        }
    };
}
