/*
 ** Copyright 2015 The Spreadtrum.com
 */
package android.app;

import android.content.Context;
import android.content.Intent;
import android.content.IIntentReceiver;
import android.content.IIntentSender;
import android.content.IntentSender;
import android.os.Bundle;
import android.os.RemoteException;
import android.os.Handler;
import android.os.IBinder;
import android.os.Parcel;
import android.os.Parcelable;
import android.os.UserHandle;
import android.util.AndroidException;

import java.util.Locale;

/**
 * Data needed by alarm manager.
 * @hide internal
 */
public final class PowerGuruAlarmInfo  implements Parcelable {

    public String packageName;
    public String actionName;
    public String componentName;
    public int alarmType;

    public boolean isFromGMS;
    public boolean isAvailable;

    public PowerGuruAlarmInfo(String _packageName, String _actionName,
        String _componentName, int _alarmType, boolean _isGms, boolean _isAvailable) {

        packageName = _packageName;
        actionName = _actionName;
        componentName = _componentName;;
        alarmType = _alarmType;

        isFromGMS = _isGms;
        isAvailable = _isAvailable;
    }

    public PowerGuruAlarmInfo() {
    }

    public PowerGuruAlarmInfo(Parcel in) {
        readFromParcel(in);
    }

    public static final Parcelable.Creator<PowerGuruAlarmInfo> CREATOR =
        new Parcelable.Creator<PowerGuruAlarmInfo>() {
            public PowerGuruAlarmInfo createFromParcel(Parcel in) {
                return new PowerGuruAlarmInfo(in);
            }

            public PowerGuruAlarmInfo[] newArray(int size) {
                return new PowerGuruAlarmInfo[size];
            }
        };

    public int describeContents() {
        return 0;
    }

    public String getPkg() {
        return packageName;
    }

    public String getAction() {
        return actionName;
    }

    public String getCpnt() {
        return componentName;
    }

    public int getAlarmType() {
        return alarmType;
    }

    public boolean getFromGMS(){
        return isFromGMS;
    }
    public boolean getAvailable(){
        return isAvailable;
    }

    public String setPkg(String pkg) {
        return this.packageName = pkg ;
    }

    public String setAction(String action) {
        return this.actionName = action;
    }

    public String setCpnt(String cpnt) {
        return this.componentName = cpnt;
    }

    public int setAlarmType(int type) {
        return this.alarmType = type;
    }
    public void setFromGMS(boolean fromGms){
        this.isFromGMS = fromGms;
    }

    public void setAvailable(boolean avail){
        this.isAvailable= avail;
    }




    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeString(packageName);
        dest.writeString(actionName);
        dest.writeString(componentName);
        dest.writeInt(alarmType );

        if (isFromGMS) {
            dest.writeInt(1);
        } else {
            dest.writeInt(0);
        }

        if (isAvailable) {
            dest.writeInt(1);
        } else {
            dest.writeInt(0);
        }
    }

    public void readFromParcel(Parcel in) {
        packageName = in.readString();
        actionName = in.readString();
        componentName = in.readString();
        alarmType = in.readInt();

        isFromGMS = (in.readInt() == 1);
        isAvailable = (in.readInt() == 1);
    }

    public String toString(){
        return ("p: "+packageName + " a: " + actionName +
                " c: "+componentName+" t: " +alarmType +
                " gms: "+ isFromGMS +  " ava: "+isAvailable);
    }

}

