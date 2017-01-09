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

import android.os.Parcel;
import android.os.Parcelable;

public class LanguageMessage implements Parcelable {

    public String languageString = null;

    public LanguageMessage() {
    }

    private LanguageMessage(Parcel in) {
        languageString = in.readString();
    }

    public int describeContents() {
        return 0;
    }

    public void writeToParcel(Parcel dest, int flags) {
        dest.writeString(languageString);
    }

    public static final Parcelable.Creator<LanguageMessage> CREATOR = new Parcelable.Creator<LanguageMessage>() {
        public LanguageMessage createFromParcel(Parcel in) {
            return new LanguageMessage(in);
        }

        public LanguageMessage[] newArray(int size) {
            return new LanguageMessage[size];
        }
    };
}
