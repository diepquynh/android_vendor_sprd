
package com.sprd.voicetrigger.utils;

import android.content.res.AssetManager;
import android.util.Log;

import java.io.BufferedOutputStream;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

public class LangagePackageManager {
    private static final String TAG = "LangPackMan";

    private AssetManager assets = null;
    private File zipDir = null;
    private File packsDir = null;

    public static class packClass {
        public boolean isAsset;
        public String zipFile;
        public String configFile;
        public String netFile;
        public String enrollnetFile;
        public String phonemeFile;
        public String svsidFile;
        public boolean unpacked;
        public String name;
        public int paramAStart;
        public int paramBStart;
        public int trigThreshold;
        public int trigThresholdStep;
        public float svThreshold;
        public float svThresholdStep;
        public float checkSNR;
    }

    private ArrayList<packClass> packages;
    private volatile static LangagePackageManager mLangagePackageManager;

    private LangagePackageManager() {
        // do nothing
    }

    /**
     * return a instance object for single instance
     *
     * @return
     */
    public static LangagePackageManager getInstance() {
        if (mLangagePackageManager == null) {
            synchronized (LangagePackageManager.class) {
                if (mLangagePackageManager == null) {
                    mLangagePackageManager = new LangagePackageManager();
                }
            }
        }
        return mLangagePackageManager;
    }

    public String[] initPacks(AssetManager amgr, String appDirFull_in) {
        assets = amgr;
        packsDir = new File(appDirFull_in);

        // initialize trigger packs
        packages = new ArrayList<>();
        packages.clear();

        readPacks("langpack");

        return null;
    }

    /**
     * return a string array of all language pack names
     *
     * @return
     */
    public ArrayList<String> getPackNames() {
        ArrayList<String> packNames;
        packNames = new ArrayList<String>();
        packNames.clear();
        for (packClass p : packages) {
            // Log.i(TAG, "getPackName:p=" + p.name);
            packNames.add(p.name);
        }
        return packNames;
    }

    /**
     * returns a package class from a name name
     *
     * @param name - the package name
     * @return pack 0 if not found
     */
    public packClass getPack(String name) {
        // Log.i(TAG, "getPack:Entry");
        packClass result = packages.get(0);
        for (packClass p : packages) {
            Log.i(TAG, "package:" + p.name + "=" + name);
            if (p.name.equals(name)) {
                result = p;
                break;
            }
        }
        return result;
    }

    /**
     * returns a package class from an index
     *
     * @param index - the package index number
     * @return pack 0 if index < 0 or bigger than the number of packages
     */
    public packClass getPack(int index) {
        packClass result = packages.get(0);
        if ((index >= 0) && (index <= packages.size()))
            result = packages.get(index);
        return result;
    }

    /**
     * returns an index from a package name
     *
     * @param name - the package name
     * @return 0 id not found
     */
    public int getIndex(String name) {
        // Log.i(TAG, "name=" + name);
        int result = 0;
        for (packClass p : packages) {
            if (p.name == name) {
                break;
            }
            result++;
        }
        // Log.i(TAG, "result=" + result);
        return result;
    }

    /**
     * @return return the number of packages
     */
    public int getNumPacks() {
        return packages.size();
    }

    private void readPacks(final String packType) {

        // Unpack built-in triggers in assets folder
        try {
            // Log.i(TAG, "READING ASSETS...");
            String all[] = assets.list("");
            for (String s : all) {
                if (s.startsWith(packType + "_") && s.endsWith(".zip")) {
                    // Log.i(TAG, "ASSET: " + s);
                    ZipInputStream in = new ZipInputStream(assets.open(s));
                    unpack(in, true);
                    in.close();
                }
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private void unpack(ZipInputStream in, boolean isAsset) {
        // Log.i(TAG, "Unpacking: " + in);
        try {
            for (ZipEntry entry = in.getNextEntry(); entry != null; entry = in.getNextEntry()) {
                extractFile(in, entry);
                if (entry.getName().endsWith("/config.txt")) {
                    readPack(in.toString(), entry.getName(), isAsset);
                }
            }
        } catch (IOException e) {
            Log.e(TAG, e.getMessage());
        }
    }

    private void extractFile(ZipInputStream in, ZipEntry entry) {
        try {
            Log.i(TAG, "ZIP ENTRY: " + entry.toString());
            byte[] buffer = new byte[1024];
            int length;
            String dstEntryDir = new File(packsDir + File.separator + entry.getName()).getParent()
                    + File.separator;
            Log.i(TAG, "File: " + dstEntryDir);
            String dstEntryFile = packsDir + File.separator + entry.getName();
            Log.i(TAG, "File: " + dstEntryFile);

            if (!new File(dstEntryDir).exists()) {
                Log.i(TAG, "CREATING DIR: " + dstEntryDir);
                new File(dstEntryDir).mkdirs();
            } else {
                Log.i(TAG, "Folder already exists:" + dstEntryDir);
            }

            if (!new File(dstEntryFile).exists()) {
                FileOutputStream fout = new FileOutputStream(dstEntryFile);
                BufferedOutputStream bufout = new BufferedOutputStream(fout);
                while ((length = in.read(buffer)) > 0) {
                    bufout.write(buffer, 0, length);
                }
                in.closeEntry();
                bufout.close();
                Log.i(TAG, "EXTRACTED: " + dstEntryFile);
            } else {
                Log.i(TAG, "File already exists:" + dstEntryFile);
            }

        } catch (IOException e) {
            Log.e(TAG, e.getMessage());
        }
    }

    /**
     * Retrieves information from a pack
     *
     * @param zipFile
     * @param configfile
     * @param isAsset
     */
    void readPack(String zipFile, String configfile, boolean isAsset) {
        // Log.i(TAG, "initPack: " + configfile);
        if (packages.contains(configfile))
            return;
        try {
            packClass p = new packClass();
            p.isAsset = isAsset;
            p.zipFile = zipFile;
            p.configFile = configfile;
            p.netFile = "";
            p.enrollnetFile = "";
            p.phonemeFile = "";
            p.svsidFile = "";
            p.unpacked = false;
            p.name = "";
            p.paramAStart = 0;
            p.paramBStart = 0;
            p.svThreshold = 0.f;
            p.svThresholdStep = 0.f;
            File fi = new File(packsDir + File.separator + configfile);
            FileInputStream fis = new FileInputStream(fi);
            BufferedReader dataIO = new BufferedReader(new InputStreamReader(fis));
            String line = null;
            String type = null;
            while ((line = dataIO.readLine()) != null) {
                line = line.trim();
                if (line.length() == 0)
                    continue;
                if (line.startsWith("::")) {
                    type = line;
                } else if (type != null && type.equals("::Name")) {
                    p.name = line;
                } else if (type != null && type.equals("::Net")) {
                    p.netFile = fi.getParent() + File.separator + line;
                } else if (type != null && type.equals("::EnrollNet")) {
                    p.enrollnetFile = fi.getParent() + File.separator + line;
                } else if (type != null && type.equals("::Phoneme")) {
                    p.phonemeFile = fi.getParent() + File.separator + line;
                } else if (type != null && type.equals("::SVSID")) {
                    p.svsidFile = fi.getParent() + File.separator + line;
                } else if (type != null && type.equals("::ParamAstart")) {
                    p.paramAStart = Integer.parseInt(line);
                } else if (type != null && type.equals("::ParamBstart")) {
                    p.paramBStart = Integer.parseInt(line);
                } else if (type != null && type.equals("::TrigThreshold")) {
                    p.trigThreshold = Integer.parseInt(line);
                } else if (type != null && type.equals("::TrigThresholdStep")) {
                    p.trigThresholdStep = Integer.parseInt(line);
                } else if (type != null && type.equals("::SVThreshold")) {
                    p.svThreshold = Float.parseFloat(line);
                } else if (type != null && type.equals("::SVThresholdStep")) {
                    p.svThresholdStep = Float.parseFloat(line);
                } else if (type != null && type.equals("::CheckSNR")) {
                    p.checkSNR = Float.parseFloat(line);
                } else {
                    Log.e(TAG, "ERROR: parsePack: unexpected entry: " + line);
                }
            }
            packages.add(p);
            dataIO.close();
            fis.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
