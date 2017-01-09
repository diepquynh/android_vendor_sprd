
package com.sprd.voicetrigger.utils;

import android.content.Context;
import android.util.Log;

import com.sprd.voicetrigger.languagesupport.SupportLanguages;
import com.sprd.voicetrigger.provider.ContentProviderHelper;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;

public class UserVoiceManager {
    public static final String MODE_EFT = "eft";
    public static final String MODE_UDT = "udt";
    private static final String UDT_LANGUAGE_DEFAULT = "gl";
    public static final String TAG = "UserVoiceManager";
    private final String VOICE_FILE_DIR;
    private Context mContext;

    public UserVoiceManager(Context context) {
        mContext = context;
        VOICE_FILE_DIR = mContext.getDir("user_voice", Context.MODE_PRIVATE).toString();
        Log.d(TAG, " VOICE_FILE_DIR = " + VOICE_FILE_DIR);
    }

    /**
     * you can add a eft voice data to a file use this method;
     *
     * @param fileDir
     * @param userName
     * @param languageMode
     * @return
     */
    public boolean addEFTVoiceDataToFile(String fileDir, String userName, String languageMode)
            throws IllegalArgumentException {
        return addVoiceDataToFile(fileDir, userName, MODE_EFT, languageMode);
    }

    /**
     * @param fileDir
     * @param userName
     * @return
     */
    public boolean addUDTVoiceDataToFile(String fileDir, String userName)
            throws IllegalArgumentException {
        return addVoiceDataToFile(fileDir, userName, MODE_UDT, null);
    }

    private boolean addVoiceDataToFile(String fileDir, String userName, String mode,
                                       String languageMode) throws IllegalArgumentException {
        String user_name;
        String language;
        if (fileDir == null || mode == null) {
            Log.d(TAG, "fileDir == null || mode == null");
            return false;
        }
        if (userName == null) {
            user_name = "user0";
        } else {
            user_name = userName;
        }
        if (languageMode == null) {
            language = UDT_LANGUAGE_DEFAULT;
        } else {
            language = languageMode;
        }
        if (language.contains("_") || user_name.contains("_") || mode.contains("_")) {
            throw new IllegalArgumentException("Argument name can not cantain \"_\"! ");
        }
        String fileName = user_name + "_" + mode + "_" + language + ".dat";
        Log.d(TAG, "fileName= " + fileName + ", fileDir=" + fileDir);
        File temp = new File(VOICE_FILE_DIR, fileName);
        InputStream inStream = null;
        FileOutputStream fs = null;
        try {
            File oldfile = new File(fileDir);
            if (oldfile.exists()) {
                int byteread = 0;
                inStream = new FileInputStream(fileDir);
                fs = new FileOutputStream(temp);
                byte[] buffer = new byte[1024];
                while ((byteread = inStream.read(buffer)) != -1) {
                    fs.write(buffer, 0, byteread);
                }
                return true;
            } else {
                return false;
            }
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        } finally {
            if (inStream != null) {
                try {
                    inStream.close();
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
            if (fs != null) {
                try {
                    fs.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    /**
     * @param fileDir
     * @param mode
     * @return
     */
    public boolean addWaveDataToFile(String fileDir, String mode) throws IllegalArgumentException {
        if (fileDir == null) {
            Log.d(TAG, "fileDir == null ");
            return false;
        }

        String fileName = "udt.wav";
        Log.d(TAG, "fileName= " + fileName + ", fileDir=" + fileDir);
        File temp = new File(VOICE_FILE_DIR, fileName);
        InputStream inStream = null;
        FileOutputStream fs = null;
        try {
            File oldfile = new File(fileDir);
            if (oldfile.exists()) {
                int byteread = 0;
                inStream = new FileInputStream(fileDir);
                fs = new FileOutputStream(temp);
                byte[] buffer = new byte[1024];
                while ((byteread = inStream.read(buffer)) != -1) {
                    fs.write(buffer, 0, byteread);
                }
                return true;
            } else {
                return false;
            }
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        } finally {
            if (inStream != null) {
                try {
                    inStream.close();
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
            if (fs != null) {
                try {
                    fs.close();
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        }
    }

    /**
     * return if has the user voice files in storage space
     *
     * @return true - has voice file</p> - false hasn't voice file
     */
    public boolean hasUserVoiceDataFile(int currentLanguage) {
        return new File(VOICE_FILE_DIR).listFiles().length > 0 ? true : false;
    }

    /**
     * @param userName
     * @param languageMode
     * @return File the eft data file object
     */
    public File getEFTVoiceDataFile(String userName, String languageMode)
            throws IllegalArgumentException {
        String user_name;
        if (userName == null) {
            user_name = "user0";
        } else {
            user_name = userName;
        }
        if (languageMode == null) {
            throw new IllegalArgumentException("Argument not allow null ");
        }
        return getVoiceDataFile(user_name, MODE_EFT, languageMode);
    }

    public boolean hasEFTVoiceDataFile(String userName, String languageMode) {
        return getEFTVoiceDataFile(userName, languageMode) == null ? false : true;
    }

    public boolean hasUDTVoiceDataFile(String userName) {
        return getUDTVoiceDataFile(userName) == null ? false : true;
    }

    /**
     * @param userName
     * @return
     */
    public File getUDTVoiceDataFile(String userName) {
        String user_name;
        if (userName == null) {
            user_name = "user0";
        } else {
            user_name = userName;
        }
        return getVoiceDataFile(user_name, MODE_UDT, UDT_LANGUAGE_DEFAULT);
    }

    /**
     * @param userName     userName
     * @param languageMode languageMode see
     * @return the path of eft data
     */
    public String getEFTDataPath(String userName, String languageMode) {
        if (getEFTVoiceDataFile(userName, languageMode) != null) {
            return getEFTVoiceDataFile(userName, languageMode).getPath();
        }
        return null;
    }

    public String getUDTDataPath(String userName) {
        if (getUDTVoiceDataFile(userName) != null) {
            return getUDTVoiceDataFile(userName).getPath();
        }
        return null;
    }

    public String getCurrentDataPath(String userName) {
        boolean currentMode = ContentProviderHelper.isDefaultMode(mContext);
        String currentLanguage = new SupportLanguages(mContext).getCurrentLanguageKey();
        if (currentMode) {
            return getEFTDataPath(userName,currentLanguage);
        } else {
            return getUDTDataPath(userName);
        }
    }

    private ArrayList<File> getVoiceDataFiles() {
        ArrayList<File> arr = new ArrayList<File>();
        File[] allFiles = new File(VOICE_FILE_DIR).listFiles();

        String fileEnd;

        for (int i = 0; i < allFiles.length; i++) {
            fileEnd = allFiles[i]
                    .getName()
                    .substring(allFiles[i].getName().lastIndexOf(".") + 1,
                            allFiles[i].getName().length()).toLowerCase();
            Log.d(TAG, "getVoiceDataFiles : " + fileEnd);
            if (fileEnd.equals("dat")) {
                arr.add(allFiles[i]);
            }
        }
        return arr;
    }

    private File getVoiceDataFile(String userName, String mode, String languageMode) {
        ArrayList<File> arr = getVoiceDataFiles();
        File result = null;
        String fileName;
        for (int i = 0; i < arr.size(); i++) {
            fileName = arr.get(i).getName();
            if (!fileName.substring(0, fileName.indexOf("_")).equals(userName)) {
                continue;
            }
            if (!fileName.substring(fileName.indexOf("_") + 1, fileName.lastIndexOf("_")).equals(
                    mode)) {
                continue;
            }
            if (!fileName.substring(fileName.lastIndexOf("_") + 1, fileName.lastIndexOf("."))
                    .equals(languageMode)) {
                continue;
            }
            result = arr.get(i);
        }
        return result;
    }

    public String getWaveDataPath() {
        File[] allFiles = new File(VOICE_FILE_DIR).listFiles();
        File result = null;
        String fileName;
        for (int i = 0; i < allFiles.length; i++) {
            fileName = allFiles[i].getName();
            Log.d(TAG, "getWaveDataPath ,allFiles.length = " + allFiles.length);
            Log.d(TAG, "getWaveDataPath [" + i + "]=" + fileName);
            if (fileName.equals("udt.wav"))
                result = allFiles[i];
        }
        if (result != null) {
            return result.getPath();
        } else {
            return null;
        }
    }
}
