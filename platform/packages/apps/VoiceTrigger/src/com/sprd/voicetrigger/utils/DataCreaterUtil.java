
package com.sprd.voicetrigger.utils;

import android.content.Context;
import android.util.Log;

import java.io.BufferedInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

/**
 * Description: creat a data file for device to use which is combined with
 * gram.bin and net.bin
 */
public class DataCreaterUtil {
    private static final String WAKE_UP_WORDS_DATA_FILE_NAME = "SMicTD1.dat";
    private static final String TAG = "DataCreaterUtil";

    /**
     * the static method to make a file , it will be saved into your app's
     * private "file" dir;
     *
     * @param context the activity context or application context
     * @param gram    the File object of gram.bin
     * @param net     the File object of net.bin
     * @throws IOException throw a IOException if the directions is disability etc.
     */
    public static void createHeader(Context context, File gram, File net) throws IOException {
        File fd_gram = gram;
        File fd_net = net;
        byte[] dummy;

        long NetStartOffset = fd_gram.length() % 8 == 0 ? fd_gram.length()
                : ((fd_gram.length() / 8) + 1) * 8;
        long NetEndOffset = NetStartOffset
                + (fd_net.length() % 8 == 0 ? fd_net.length() : ((fd_net.length() / 8) + 1) * 8);

        byte[] header = new byte[16];
        header[0] = 0x79;
        header[1] = 0x23;
        header[2] = 0x01;
        header[3] = 0x00;
        header[4] = (byte) (NetStartOffset & 0x000000ff);
        header[5] = (byte) ((NetStartOffset & 0x0000ff00) >> 8);
        header[6] = (byte) ((NetStartOffset & 0x00ff0000) >> 16);
        header[7] = (byte) ((NetStartOffset & 0xff000000) >> 24);
        header[8] = (byte) (NetEndOffset & 0x000000ff);
        header[9] = (byte) ((NetEndOffset & 0x0000ff00) >> 8);
        header[10] = (byte) ((NetEndOffset & 0x00ff0000) >> 16);
        header[11] = (byte) ((NetEndOffset & 0xff000000) >> 24);

        FileInputStream fGRAM = new FileInputStream(fd_gram);
        FileInputStream fNET = new FileInputStream(fd_net);
        FileOutputStream fTD = context.openFileOutput(WAKE_UP_WORDS_DATA_FILE_NAME,
                Context.MODE_PRIVATE);

        DataInputStream isr_gram = new DataInputStream(fGRAM);
        DataInputStream isr_net = new DataInputStream(fNET);
        DataOutputStream osw_td = new DataOutputStream(fTD);

        osw_td.write(header);

        byte[] buf1 = new byte[fGRAM.available()];
        while (isr_gram.read(buf1) > 0)
            osw_td.write(buf1);

        if (fd_gram.length() % 8 != 0) {
            dummy = new byte[(int) (8 - (fd_gram.length() % 8))];
            osw_td.write(dummy);
        }

        byte[] buf2 = new byte[fNET.available()];
        while (isr_net.read(buf2) > 0)
            osw_td.write(buf2);

        if (fd_net.length() % 8 != 0) {
            dummy = new byte[(int) (8 - (fd_net.length() % 8))];
            osw_td.write(dummy);
        }
        fTD.flush();
        fTD.close();
        osw_td.flush();
        osw_td.close();
    }


    private void copyData(Context context, int res, String filename) {
        InputStream ins;
        int size;
        byte[] buffer;
        try {
            ins = context.getResources().openRawResource(res);
            size = ins.available();
            Log.i(TAG, "max=" + Runtime.getRuntime().maxMemory());
            Log.i(TAG, "total=" + Runtime.getRuntime().totalMemory());
            Log.i(TAG, "free=" + Runtime.getRuntime().freeMemory());

            buffer = new byte[size];
            ins.read(buffer);
            ins.close();
        } catch (Exception e) {
            e.printStackTrace();
            return;
        }
        if (!EnvironmentUtils.writeStorage(context, buffer, filename)) {
            Log.e(TAG, "COPYDATA FAILED: file=" + filename);
        }
        buffer = null;
    }

    public void deleteDir(String dir) {
        Log.i(TAG, "deleteDir=" + dir);
        File file = new File(dir);
        if (file.exists() && file.isDirectory()) {
            File[] all = file.listFiles();
            for (int i = 0; i < all.length; i++) {
                all[i].delete();
                Log.i(TAG, "Delete file: " + all[i].toString());
            }
        }
        file.delete();
    }

    public static Boolean createDir(String dir) {
        Log.i(TAG, "createDir=" + dir);
        Boolean res = true;
        File file = new File(dir);
        if (!file.exists()) {
            if (!file.mkdirs()) {
                res = false;
            } else
                Log.i(TAG, "Directory created: " + dir);
        } else {
            Log.i(TAG, "Directory exists: " + dir);
        }
        String all[] = file.list();
        Log.i(TAG, "DIRECTORY=" + dir);
        for (int i = 0; i < all.length; i++)
            Log.i(TAG, "DIR: " + all[i]);
        return res;
    }

    public static byte[] getByteArrayFromFileDir(String datFileDir) throws IOException {

        File f = new File(datFileDir);
        if (!f.exists()) {
            throw new FileNotFoundException(datFileDir);
        }
        Log.d(TAG, "toByteArray f.length()=" + f.length());
        ByteArrayOutputStream bos = new ByteArrayOutputStream((int) f.length());
        BufferedInputStream in = null;
        try {
            in = new BufferedInputStream(new FileInputStream(f));
            int buf_size = 1024;
            byte[] buffer = new byte[buf_size];
            int len = 0;
            while (-1 != (len = in.read(buffer, 0, buf_size))) {
                bos.write(buffer, 0, len);
            }
            return bos.toByteArray();
        } catch (IOException e) {
            e.printStackTrace();
            throw e;
        } finally {
            if (in != null){
                try {
                    in.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
            bos.close();
        }
    }

    /**
     * @param file
     * @return
     * @throws IOException
     */
    @Deprecated
    public static byte[] getBytesFromFile(File file) throws IOException {
        FileInputStream is = new FileInputStream(file);
        long length = file.length();
        if (length > Integer.MAX_VALUE) {
            throw new IOException("File is to large " + file.getName());
        }

        byte[] bytes = new byte[(int) length];
        int offset = 0;
        int numRead = 0;
        while (offset < bytes.length
                && (numRead = is.read(bytes, offset, bytes.length - offset)) >= 0) {
            offset += numRead;
        }

        if (offset < bytes.length) {
            throw new IOException("Could not completely read file " + file.getName());
        }
        is.close();
        return bytes;
    }

    /**
     * copy a file to another dir
     * @param from the form file path
     * @param to the to file path
     */
    public static void copyFile(String from, String to){
        FileInputStream input = null;
        FileOutputStream output = null;
        try{
            input=new FileInputStream(from);
            output=new FileOutputStream(to);
            int in=input.read();
            while(in!=-1){
                output.write(in);
                in=input.read();
            }
            Log.d(TAG, "copy data File: successful");
        }catch (IOException e){
            Log.d(TAG, "copy data File: failed");
            e.printStackTrace();
        } finally {
            if (input != null){
                try {
                    input.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
            if (output != null){
                try {
                    output.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
    }
}
