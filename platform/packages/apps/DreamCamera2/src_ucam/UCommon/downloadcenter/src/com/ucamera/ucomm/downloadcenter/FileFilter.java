/*
 *   Copyright (C) 2010,2011 Thundersoft Corporation
 *   All rights Reserved
 */

package com.ucamera.ucomm.downloadcenter;

import java.io.File;
import java.io.FilenameFilter;

public class FileFilter implements FilenameFilter {
    private static final String FILE_CN_EXT = ".CN.ucam";
    //filter format
    private String mFilterFormat;

    public FileFilter(String filterFormat) {
        mFilterFormat = filterFormat;
    }

    public boolean accept(File dir, String filename) {
        return filename.endsWith(mFilterFormat) && !filename.endsWith(FILE_CN_EXT);
    }

}
