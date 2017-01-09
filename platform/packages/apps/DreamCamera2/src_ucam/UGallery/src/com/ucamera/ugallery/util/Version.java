/**
 *   Copyright (C) 2010,2011 Thundersoft Corporation
 *   All rights Reserved
 */
package com.ucamera.ugallery.util;

public class Version {

    private int major;
    private int minor;
    private int micro;
    private String build;

    private Version(int major, int minor, int micro, String build) {
        this.major = major;
        this.minor = minor;
        this.micro = micro;
        this.build = build;
    }

    public static Version fromString(String version) {
        String parts[] = version.split("\\.");
        int major = parts.length > 0 ? getInt(parts[0]) : 0;
        int minor = parts.length > 1 ? getInt(parts[1]) : 0;
        int micro = parts.length > 2 ? getInt(parts[2]) : 0;
        String build = parts.length > 3 ? parts[3] : "";
        return new Version(major,minor,micro,build);
    }

    private static int getInt(String s) {
        try {
            return Integer.parseInt(s);
        }catch (Exception e) {
            return 0;
        }
    }

    public boolean isNewerThan(Version o) {
        if (major > o.major)
            return true;
        if (major < o.major)
            return false;

        if (minor > o.minor)
            return true;
        if (minor < o.minor)
            return false;

        if (micro > o.micro)
            return true;
        if (micro < o.micro)
            return false;

        return build.compareTo(o.build) > 0;
    }
}
