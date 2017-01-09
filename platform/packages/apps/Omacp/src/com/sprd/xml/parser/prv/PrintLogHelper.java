
package com.sprd.xml.parser.prv;

import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;

public class PrintLogHelper {

    private PrintLogHelper() {
    }

    synchronized public static PrintWriter getPrintWriter(String path) {
        setFilePath(path);
        if (null == mPrintWriter) {
            try {
                mPrintWriter = new PrintWriter(new OutputStreamWriter(new FileOutputStream(
                        mFilePath)), true);
            } catch (FileNotFoundException e) {
                e.printStackTrace();
            }

        }
        return mPrintWriter;
    }

    synchronized public void releasPrintWriter() {
        if (mPrintWriter != null) {
            mPrintWriter.close();
            mPrintWriter = null;
        }
    }

    public void printInfo(String logInfo, boolean isNewline) {
        if (mPrintWriter != null) {
            mPrintWriter.println(logInfo);
            if (isNewline)
                mPrintWriter.println();
        } else {
            System.out.println("PrintWriter is nul");
        }
    }

    public static String getFilePath() {
        return mFilePath;
    }

    public static void setFilePath(String path) {
        mFilePath = path;
    }

    private static String mFilePath = null;
    private static PrintWriter mPrintWriter = null;
}
