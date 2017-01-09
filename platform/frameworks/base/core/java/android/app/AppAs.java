package android.app;


import android.os.Parcel;
import android.os.Parcelable;

import org.xmlpull.v1.XmlSerializer;
import org.xmlpull.v1.XmlPullParserException;
import org.xmlpull.v1.XmlPullParser;

import java.io.IOException;
import java.util.ArrayList;

public class AppAs{
    public static class AppAsData implements Parcelable {
        private String packageName;
        private boolean enabled=true;
        private int launchCount = 0;
        private long lastLaunchTime = 0;
        private String basePackage;
        private ArrayList<AppAsData> mList = null;
        private static final String ATTR_PACKAGE = "callingpackage";
        private static final String ATTR_ENABLED = "enabled";
        private static final String ATTR_COUNT = "count";
        private static final String ATTR_LAST_TICK_TIME = "last_tick_time";

        public AppAsData(String pkg,boolean isBase) {
            packageName = pkg;
            lastLaunchTime = System.currentTimeMillis();
            if (isBase) {
                mList = new ArrayList<AppAsData>();
            }
            lastLaunchTime = System.currentTimeMillis();
        }
        public AppAsData(String pkg, boolean enable, int count) {
            packageName = pkg;
            enabled = enable;
            launchCount = count;
            lastLaunchTime = System.currentTimeMillis();
        }
        public AppAsData(String pkg, boolean enable, int count, long time) {
            packageName = pkg;
            enabled = enable;
            launchCount = count;
            lastLaunchTime = time;
        }
        public AppAsData() {
        }
        @Override
        public int describeContents() {
            return 0;
        }
        public void saveToXml(XmlSerializer out) throws IOException, XmlPullParserException {
            out.attribute(null, ATTR_PACKAGE, packageName);

            if (enabled) {
                out.attribute(null, ATTR_ENABLED, "true");
            } else {
                out.attribute(null, ATTR_ENABLED, "false");
            }

            out.attribute(null, ATTR_COUNT, Integer.toString(launchCount));
            out.attribute(null, ATTR_LAST_TICK_TIME, String.valueOf(lastLaunchTime));
        }
        public AppAsData restoreFromXml(XmlPullParser in) throws IOException, XmlPullParserException {
            String mPackage = null;
            boolean mEnabled = true;
            int mLaunchCount = 0;
            long mLastLaunchTime = 0;
            for (int attrNdx = in.getAttributeCount() - 1; attrNdx >= 0; --attrNdx) {
                final String attrName = in.getAttributeName(attrNdx);
                final String attrValue = in.getAttributeValue(attrNdx);
                if (ATTR_PACKAGE.equals(attrName)) {
                    mPackage = attrValue;
                } else if (ATTR_ENABLED.equals(attrName)) {
                    mEnabled = Boolean.valueOf(attrValue);
                } else if (ATTR_COUNT.equals(attrName)) {
                    mLaunchCount = Integer.parseInt(attrValue);
                } else if (ATTR_LAST_TICK_TIME.equals(attrName)) {
                    mLastLaunchTime = Long.valueOf(attrValue);
                }
            }
            return new AppAsData(mPackage, mEnabled, mLaunchCount, mLastLaunchTime);
        }
        public void setEnabled(boolean checked) {
            enabled=checked;
        }
        public String getPackage() {
            return packageName;
        }
        public void addList(AppAsData app) {
            if (mList == null) {
                mList = new ArrayList<AppAsData>();
            }
            mList.add(app);
        }

        /* SPRD:modify for Bug 653843 associated start interface shows uninstalling app infos. @{ */
        public void remove(AppAsData app) {
            mList.remove(app);
        }
        /* @} */
        public ArrayList<AppAsData> getAllList() {
            return mList;
        }
        public AppAsData getListItem(int index) {
            return (mList != null)?mList.get(index):null;
        }
        public int getListSize() {
            return (mList != null)?mList.size():0;
        }
        public void increaseCount() {
            launchCount++;
        }
        public int getCount() {
            return launchCount;
        }
        public boolean getEnabled() {
            return enabled;
        }
        public void setLastTickTime(long time) {
            lastLaunchTime = time;
        }
        public long getLastTickTime() {
            return lastLaunchTime;
        }

        public void writeToParcel(Parcel dest, int flags) {
            dest.writeString(packageName);
            dest.writeInt(enabled ? 1 : 0);
            dest.writeInt(launchCount);
            dest.writeLong(lastLaunchTime);
        }

        public void readFromParcel(Parcel source) {
            packageName = source.readString();
            enabled = source.readInt() != 0;
            launchCount = source.readInt();
            lastLaunchTime = source.readLong();
        }

        public static final Creator<AppAsData> CREATOR = new Creator<AppAsData>() {
            public AppAsData createFromParcel(Parcel source) {
                return new AppAsData(source);
            }
            public AppAsData[] newArray(int size) {
                return new AppAsData[size];
            }
        };

        private AppAsData(Parcel source) {
            readFromParcel(source);
        }
    }


    public static class AppAsRecord implements Parcelable {
        private String base;
        private String calling;
        private boolean enabled;
        private int launchCount = 0;
        private long lastLaunchTime = 0;
        private static final String ATTR_PACKAGE = "basepackage";
        private static final String ATTR_CALLPACKAGE = "callingpackage";
        private static final String ATTR_ENABLED = "enabled";
        private static final String ATTR_LAUNCH_COUNT = "launchCount";
        private static final String ATTR_LAST_TICK_TIME = "last_launch_tick_time";

        public AppAsRecord(String pkg, String call, boolean enable) {
            base = pkg;
            calling = call;
            enabled = enable;
        }
        public AppAsRecord(String pkg, String call, boolean enable, int count, long time) {
            base = pkg;
            calling = call;
            enabled = enable;
            launchCount = count;
            lastLaunchTime = time;
        }
        public AppAsRecord() {
        }
        @Override
        public int describeContents() {
            return 0;
        }
        public void saveToXml(XmlSerializer out) throws IOException, XmlPullParserException {
            out.attribute(null, ATTR_PACKAGE, base);
            out.attribute(null, ATTR_CALLPACKAGE, calling);
            if (enabled) {
                out.attribute(null, ATTR_ENABLED, "true");
            } else {
                out.attribute(null, ATTR_ENABLED, "false");
            }
            out.attribute(null, ATTR_LAUNCH_COUNT, Integer.toString(launchCount));
            out.attribute(null, ATTR_LAST_TICK_TIME, String.valueOf(lastLaunchTime));
        }
        public AppAsRecord restoreFromXml(XmlPullParser in) throws IOException, XmlPullParserException {
            String mPackage = null;
            String mCalling = null;
            boolean mEnabled = true;
            int mLaunchCount = 0;
            long mLastLaunchTime = 0;
            for (int attrNdx = in.getAttributeCount() - 1; attrNdx >= 0; --attrNdx) {
                final String attrName = in.getAttributeName(attrNdx);
                final String attrValue = in.getAttributeValue(attrNdx);
                if (ATTR_PACKAGE.equals(attrName)) {
                    mPackage = attrValue;
                } else if (ATTR_CALLPACKAGE.equals(attrName)) {
                    mCalling = attrValue;
                } else if (ATTR_ENABLED.equals(attrName)) {
                    mEnabled = Boolean.valueOf(attrValue);
                } else if (ATTR_LAUNCH_COUNT.equals(attrName)) {
                    mLaunchCount = Integer.parseInt(attrValue);
                } else if (ATTR_LAST_TICK_TIME.equals(attrName)) {
                    mLastLaunchTime = Long.valueOf(attrValue);
                }
            }
            return new AppAsRecord(mPackage, mCalling, mEnabled, mLaunchCount, mLastLaunchTime);
        }
        public String getBasePackage() {
            return base;
        }
        public boolean getEnabled() {
            return enabled;
        }
        public String getCallingPackage() {
            return calling;
        }
        public void increaseCount() {
            launchCount++;
            lastLaunchTime = System.currentTimeMillis();
        }
        public int getCount() {
            return launchCount;
        }
        public long getLastTickTime() {
            return lastLaunchTime;
        }

        public void writeToParcel(Parcel dest, int flags) {
            dest.writeString(base);
            dest.writeString(calling);
            dest.writeInt(enabled ? 1 : 0);
            dest.writeInt(launchCount);
            dest.writeLong(lastLaunchTime);
        }

        public void readFromParcel(Parcel source) {
            base = source.readString();;
            calling = source.readString();;
            enabled = source.readInt() != 0;
            launchCount = source.readInt();
            lastLaunchTime = source.readLong();
        }

        public static final Creator<AppAsRecord> CREATOR = new Creator<AppAsRecord>() {
            public AppAsRecord createFromParcel(Parcel source) {
                return new AppAsRecord(source);
            }
            public AppAsRecord[] newArray(int size) {
                return new AppAsRecord[size];
            }
        };

        private AppAsRecord(Parcel source) {
            readFromParcel(source);
        }
    }

}
