/*
 *   Copyright (C) 2010,2012 Thundersoft Corporation
 *   All rights Reserved
 */

package com.ucamera.uphoto;

import java.io.File;
import java.io.FilenameFilter;

/**
 * filter file by special format
 */
public class FileAccept implements FilenameFilter {
    //filter format
    private String mAcceptFormat;

    public FileAccept(String acceptFormat) {
        mAcceptFormat = acceptFormat;
    }

    public boolean accept(File dir, String filename) {
        return filename.endsWith(mAcceptFormat);
    }
}
