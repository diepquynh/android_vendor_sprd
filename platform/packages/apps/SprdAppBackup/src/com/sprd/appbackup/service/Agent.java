package com.sprd.appbackup.service;

import android.os.Parcel;
import android.os.Parcelable;

public class Agent implements Parcelable {
    String mPackageName;
    String mApplicationName;
    String mServiceName;
    String mAgentName;


    Agent(String packageName, String applicationName, String serviceName, String agentName) {
	mPackageName = packageName;
	mApplicationName = applicationName;
	mServiceName = serviceName;
	mAgentName = agentName;
    }

    public String getPackageName(){
        return mPackageName;
    }
    @Override
    public int hashCode() {
	return mAgentName.hashCode();
    }

    public String toString() {
	return mAgentName+"/"+mServiceName+"/"+mApplicationName+"/"+mPackageName;
    }

    @Override
    public boolean equals(Object other) {
	if (this == other) {
	    return true;
	}

	if (! (other instanceof Agent)) {
	    return false;
	}
	Agent agent=(Agent)other;
	return mAgentName.equals(agent.mAgentName);
    }

    public int describeContents() {
	return 0;
    }

    public void writeToParcel(Parcel out, int flags) {
	out.writeString(mPackageName);
	out.writeString(mApplicationName);
	out.writeString(mServiceName);
	out.writeString(mAgentName);
    }

    public static final Parcelable.Creator<Agent> CREATOR
    = new Parcelable.Creator<Agent>() {
	    public Agent createFromParcel(Parcel in) {
		return new Agent(in);
	    }

	    public Agent[] newArray(int size) {
		return new Agent[size];
	    }
	};

    private Agent(Parcel in) {
	mPackageName = in.readString();
	mApplicationName = in.readString();
	mServiceName = in.readString();
	mAgentName = in.readString();
    }

    public String getAgentName() {
        return mAgentName;
    }

}
