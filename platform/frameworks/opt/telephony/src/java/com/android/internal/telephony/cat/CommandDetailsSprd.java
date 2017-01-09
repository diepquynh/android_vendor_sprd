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

abstract class ValueObjectSprd {
    abstract ComprehensionTlvTagSprd getTag();
}

/**
 * Class for Command Details object of proactive commands from SIM.
 * {@hide}
 */
/**
 * @orig
 * SPRD: Remove this code for BIP function. @{
class CommandDetails extends ValueObject implements Parcelable {
@} */
public class CommandDetailsSprd extends ValueObjectSprd implements Parcelable {
    public boolean compRequired;
    public int commandNumber;
    public int typeOfCommand;
    public int commandQualifier;

    @Override
    public ComprehensionTlvTagSprd getTag() {
        return ComprehensionTlvTagSprd.COMMAND_DETAILS;
    }

    CommandDetailsSprd() {
    }

    public boolean compareTo(CommandDetailsSprd other) {
        return (this.compRequired == other.compRequired &&
                this.commandNumber == other.commandNumber &&
                this.commandQualifier == other.commandQualifier &&
                this.typeOfCommand == other.typeOfCommand);
    }

    public CommandDetailsSprd(Parcel in) {
        compRequired = in.readInt() != 0;
        commandNumber = in.readInt();
        typeOfCommand = in.readInt();
        commandQualifier = in.readInt();
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeInt(compRequired ? 1 : 0);
        dest.writeInt(commandNumber);
        dest.writeInt(typeOfCommand);
        dest.writeInt(commandQualifier);
    }

    public static final Parcelable.Creator<CommandDetailsSprd> CREATOR =
                                new Parcelable.Creator<CommandDetailsSprd>() {
        @Override
        public CommandDetailsSprd createFromParcel(Parcel in) {
            return new CommandDetailsSprd(in);
        }

        @Override
        public CommandDetailsSprd[] newArray(int size) {
            return new CommandDetailsSprd[size];
        }
    };

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public String toString() {
        return "CmdDetails: compRequired=" + compRequired +
                " commandNumber=" + commandNumber +
                " typeOfCommand=" + typeOfCommand +
                " commandQualifier=" + commandQualifier;
    }
}

class DeviceIdentitiesSprd extends ValueObjectSprd {
    public int sourceId;
    public int destinationId;

    @Override
    ComprehensionTlvTagSprd getTag() {
        return ComprehensionTlvTagSprd.DEVICE_IDENTITIES;
    }
}

// Container class to hold icon identifier value.
class IconIdSprd extends ValueObjectSprd {
    int recordNumber;
    boolean selfExplanatory;

    @Override
    ComprehensionTlvTagSprd getTag() {
        return ComprehensionTlvTagSprd.ICON_ID;
    }
}

// Container class to hold item icon identifier list value.
class ItemsIconIdSprd extends ValueObjectSprd {
    int [] recordNumbers;
    boolean selfExplanatory;

    @Override
    ComprehensionTlvTagSprd getTag() {
        return ComprehensionTlvTagSprd.ITEM_ICON_ID_LIST;
    }
}
