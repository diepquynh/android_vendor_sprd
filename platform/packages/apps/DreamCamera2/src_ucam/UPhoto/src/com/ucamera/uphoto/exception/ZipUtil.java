/*
 * Copyright (C) 2010,2012 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.uphoto.exception;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;

public final class ZipUtil {
    private ZipUtil() {}

    /**
     * add provided path to zip file
     *
     * @param srcFile may be file/dir name
     * @param outZipFile output zip file
     */
    public static void zipFile(String srcFile, String outZipFile) throws Exception {
        if (srcFile == null || outZipFile == null) return;

        ZipOutputStream outZip = new ZipOutputStream(new FileOutputStream(outZipFile));
        File file = new File(srcFile);
        /* SPRD: CID 111086 (#1 of 1): Resource leak on an exceptional path (RESOURCE_LEAK) @{*/
        try {
            addToZip(file.getParent() + File.separator, file.getName(), outZip);
        }catch (Exception e){
            if(outZip != null){
                outZip.close();
            }
            throw e;
        }
        /* @} */
        outZip.close();
    }

    /**
     * @param basepath  zip file base dir (end with "/")
     * @param filename  filename related to <code>basepath</code>
     */
    private static void addToZip(String basepath, String filename, final ZipOutputStream zipOut)
            throws Exception {
        if (zipOut == null) return;

        File file = new File(basepath + filename);

        // IF IS FILE, just add to zip
        if (file.isFile()) {
            zipOut.putNextEntry(new ZipEntry(filename));
            FileInputStream is = new FileInputStream(file);
            int len = -1;
            byte[] buffer = new byte[4096];
            // add file contents to zip file
            while ((len = is.read(buffer)) != -1) {
                zipOut.write(buffer, 0, len);
            }
            is.close();
            zipOut.closeEntry();
            return;
        }

        // process folder
        String childFiles[] = file.list();

        // add fold entry
        zipOut.putNextEntry(new ZipEntry(filename + File.separator));
        zipOut.closeEntry();

        // add child recursive
        for (int i = 0; i < childFiles.length; i++) {
            addToZip(basepath, filename + File.separator + childFiles[i], zipOut);
        }
    }

}
