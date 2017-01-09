
package com.sprd.voicetrigger.utils;

import android.content.res.AssetManager;
import android.util.Log;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.text.DateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.Locale;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

public class VocabularyPackageManager {
    private static final String TAG = "VocabPackMan";
    private ArrayList<packClass> packages;
    private String appDirFull = null;
    private static VocabularyPackageManager mVocabularyPackageManager;

    private static class promptClass {
        String name;
        String text;
    }

    private static class commandClass {
        String phrase;
        String prompt;
        String response;
    }

    public static class packClass {
        public String file;
        public String name;
        public ArrayList<String> vocabulary;
        public ArrayList<String> sensitivity;
        public ArrayList<String> search;
        public String netFile;
        public String genderFile;
        public ArrayList<promptClass> prompts;
        public ArrayList<commandClass> commands;
        public String antiDataFile;
        public float paramAOffset;
        public float durMinFactor;
        public float durMaxFactor;
        public float triggerSampPerCat;
        public float sampPerCatWithin;
        public float targetSNR;
        public float learnRate;
        public float learnRateWithin;
        public float dropoutWithin;
        public float epqMin;
        public float useFeat;
        public float checkSNR;
        public float checkVowelDur;
        public boolean isTrigger;
    }

    private VocabularyPackageManager() {
        // do nothing
    }

    public static VocabularyPackageManager getInstance() {
        if (mVocabularyPackageManager == null) {
            mVocabularyPackageManager = new VocabularyPackageManager();
        }
        return mVocabularyPackageManager;
    }

    /**
     * @param isTrigger true = return trigger packs</p>
     *                  false = return non-trigger packs</p>
     * @return return a string array of all vocabulary pack names
     */
    public ArrayList<String> getVocabNames(boolean isTrigger) {
        ArrayList<String> vocabNames = new ArrayList<>();
        vocabNames.clear();
        if (packages != null) {
            for (packClass p : packages) {
                if (p.isTrigger == isTrigger)
                    vocabNames.add(p.name);
            }
        }
        return vocabNames;
    }

    /**
     * return a string array of all vocabulary pack names
     *
     * @param isTrigger true  = return trigger packs<p>
     *                  false = return non-trigger packs
     * @param mask
     * @return
     */
    public ArrayList<String> getVocabNames(boolean isTrigger, String mask) {
        ArrayList<String> vocabNames;
        vocabNames = new ArrayList<String>();
        vocabNames.clear();
        if (packages != null) {
            for (packClass p : packages) {
                if ((p.isTrigger == isTrigger) && (p.name.contains(mask)))
                    vocabNames.add(p.name);
            }
        }
        return vocabNames;
    }

    /**
     * returns a package class from a name
     *
     * @param name the package name
     * @return returns null if not found
     */
    public packClass getPack(String name) {
        packClass result = null;
        for (packClass p : packages) {
            if (p.name.equals(name)) {
                result = p;
                break;
            }
        }
        if (result != null)
            Log.i(TAG, "pack=" + result.name);
        return result;
    }

    /**
     * returns a package class from an index
     *
     * @param index     the package index number
     * @param isTrigger - true = return trigger packs
     * @return null if index < 0 or bigger than the number of packages
     */
    public packClass getPack(int index, boolean isTrigger) {
        packClass result = null;
        ArrayList<String> vocabNames = getVocabNames(isTrigger);
        if ((index >= 0) && (index <= vocabNames.size())) {
            result = getPack(vocabNames.get(index));
        }
        return result;
    }

    /**
     * returns an index from a package name
     *
     * @param name      - the package name
     * @param isTrigger - true = return trigger packs
     * @return 0 id not found
     */
    public int getIndex(String name, boolean isTrigger) {
        int result = 0;
        ArrayList<String> vocabNames = getVocabNames(isTrigger);
        for (packClass p : packages) {
            if (p.name == name) {
                break;
            }
            result++;
        }
        if (result >= vocabNames.size()) {
            result = 0;
        }
        return result;
    }

    /**
     * return the number of packages
     *
     * @param isTrigger :true = return trigger packs </p>
     *                  false = return non-trigger packs
     * @return
     */
    public int getNumPacks(boolean isTrigger) {
        int numPacks = 0;
        for (packClass p : packages) {
            if (p.isTrigger == isTrigger) {
                numPacks++;
            }
        }
        return numPacks;
    }

    /**
     * return the vocabulary from a package name as a return-delimited string
     *
     * @param p the package to parse
     * @return
     */
    public String getVocabulary(packClass p) {
        String result = "";
        try {
            for (int i = 0; i < p.vocabulary.size(); i++) {
                result += p.vocabulary.get(i) + "\n";
            }
        } catch (Exception e) {
            // do nothing, just exit
        }
        Log.i(TAG, "getVocabulary=" + result);
        return result;
    }

    /**
     * return the sensitivity array from a package as an ArrayList
     *
     * @param p the package to parse
     * @return
     */
    public ArrayList<String> getSensitivity(packClass p) {
        ArrayList<String> result = new ArrayList<>();
        result.clear();
        try {
            for (int i = 0; i < p.sensitivity.size(); i++) {
                result.add(p.sensitivity.get(i) + "\n");
            }
        } catch (Exception e) {
            // do nothing, just exit
        }
        return result;
    }

    /**
     * return the sensitivity array from a package as an ArrayList
     *
     * @param pName - the package to parse
     * @return
     */
    public ArrayList<String> getSensitivity(String pName) {
        packClass p = getPack(pName);
        return getSensitivity(p);
    }

    /**
     * return the search file from a sensitivity string
     *
     * @param p   - the package to parse
     * @param sen
     * @return
     */
    public String getSearch(packClass p, String sen) {
        int i = p.sensitivity.indexOf(sen);
        // TODO why get search string by sensitivity string?
        return p.search.get(i);
    }

    /**
     * return a prompt text
     *
     * @param p    - the package to parse
     * @param name
     * @return
     */
    public String getPrompt(packClass p, String name) {
        for (promptClass prompt : p.prompts) {
            Log.i(TAG, "name, prompt=" + name + ", " + prompt.name + "|" + prompt.text);
            if (name.equals(prompt.name)) {
                return prompt.text;
            }
        }
        return "";
    }

    /**
     * return the number of commands in a package
     *
     * @param p - the package to parse
     * @return
     */
    public int getCommandsNum(packClass p) {
        return p.commands.size();
    }

    /**
     * return a command phrase from an integer
     *
     * @param p   - the package to parse
     * @param iDx
     * @return
     */
    public String getCommandPhrase(packClass p, int iDx) {
        if (p.commands.size() == 0) {
            return "";
        }
        if (iDx >= p.commands.size()) {
            return "";
        }
        return p.commands.get(iDx).phrase;
    }

    /**
     * return a command prompt from an integer
     *
     * @param p   - the package to parse
     * @param iDx
     * @return
     */
    public String getCommandPrompt(packClass p, int iDx) {
        if (p.commands.size() == 0) return "";
        if (iDx >= p.commands.size()) return "";
        return p.commands.get(iDx).prompt;
    }

    /**
     * return a command response from an integer
     *
     * @param p   - the package to parse
     * @param iDx
     * @return
     */
    public String getCommandResponse(packClass p, int iDx) {
        DateFormat df = DateFormat.getTimeInstance();

        if (p.commands.size() == 0) return "";
        if (iDx >= p.commands.size()) return "";
        String temp = p.commands.get(iDx).response;
        temp = temp.replace("\\T", df.format(new Date()));
        return temp;
    }

    /**
     * init packages such as unpack packages zip file and get packages object
     *
     * @param amgr          AssetManager ,you can use "getAssets()"
     * @param appDirFull_in the directory you would like to unpack packages zip
     * @return null
     */
    public String[] initPacks(AssetManager amgr, String appDirFull_in) {
        appDirFull = appDirFull_in;
        packages = new ArrayList<packClass>();
        packages.clear();
        Locale locale = Locale.getDefault();
        Log.d(TAG, "initPacks locale = " + locale.getLanguage());
        String language = locale.getLanguage();
        if (!"zh".equals(language)) {
            language = "en";
        }
        readPacks(amgr, "triggerc" + language, true);
        readPacks(amgr, "command" + language, false);
        readPacks(amgr, "trigger", true);
        readPacks(amgr, "command", false);
        return null;
    }

    public String[] initCommandPacks(AssetManager amgr, String appDirFull_in) {
        appDirFull = appDirFull_in;
        if (packages == null) {
            packages = new ArrayList<packClass>();
            packages.clear();
        }
        Locale locale = Locale.getDefault();
        Log.d(TAG, "initCommandPacks locale = " + locale.getLanguage());
        String language = locale.getLanguage();
        if (!"zh".equals(language)) {
            language = "en";
        }
        readPacks(amgr, "triggerc" + language, true);
        readPacks(amgr, "command" + language, false);
        return null;
    }

    /**
     * Unpack built-in triggers in assets folder
     *
     * @param amgr
     * @param packType
     * @param isTrigger
     * @return
     */
    private String[] readPacks(AssetManager amgr, final String packType, boolean isTrigger) {
        String all[] = null;
        try {
            all = amgr.list("");
            if (all != null) {
                for (String s : all) {
                    Log.i(TAG, "ASSET: " + s);
                    if (s.startsWith(packType + "_") && s.endsWith(".zip")) {
                        unpack(amgr, s, isTrigger);
                    }
                }
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
        return all;
    }

    /**
     * unpack languages packages zip files
     *
     * @param amgr      AssetManager
     * @param zipFile   the package name
     * @param isTrigger whether it is trigger
     */
    private void unpack(AssetManager amgr, String zipFile, boolean isTrigger) {
        Log.i(TAG, "Unpacking: " + zipFile);
        ZipInputStream in = null;
        try {
            in = new ZipInputStream(amgr.open(zipFile));
            unpack(in, isTrigger);
        } catch (IOException e) {
            Log.e(TAG, "unpack(String): " + e);
        } finally {
            try {
                if (in != null) {
                    in.close();
                }
            } catch (IOException e) {
                // ignored
            }
        }
    }

    /**
     * unpack languages packages zip files
     *
     * @param in        ZipInputStream
     * @param isTrigger whether it is trigger
     */
    private void unpack(ZipInputStream in, boolean isTrigger) {
        //Log.i(TAG, "Unpacking: " + in);
        try {
            for (ZipEntry entry = in.getNextEntry(); entry != null; entry = in.getNextEntry()) {
                // Log.i(TAG,"ZIP ENTRY: "+entry.toString());
                byte[] buffer = new byte[1024];
                int length;
                String dstEntryDir = new File(appDirFull + entry.getName()).getParent()
                        + File.separator;
                if (!new File(dstEntryDir).exists()) {
                    // Log.i(TAG,"CREATING DIR: "+dstEntryDir);
                    new File(dstEntryDir).mkdirs();
                }
                FileOutputStream fout = new FileOutputStream(appDirFull + entry.getName());
                while ((length = in.read(buffer)) > 0) {
                    fout.write(buffer, 0, length);
                }
                in.closeEntry();
                fout.close();
                //Log.i(TAG, "EXTRACTED:" + entry.getName());

                if (entry.getName().endsWith("/config.txt")) {
                    readPack(entry.getName(), isTrigger);
                }
            }
        } catch (IOException e) {
            Log.e(TAG, e.getMessage());
        }
    }

    /**
     * Retrieves information from a pack
     *
     * @param file
     * @param isTrigger
     */
    private void readPack(String file, boolean isTrigger) {
        if (packages.contains(file))
            return;
        try {
            packClass p;
            p = new packClass();
            p.file = file;
            p.name = "";
            p.vocabulary = new ArrayList<>();
            p.vocabulary.clear();
            p.sensitivity = new ArrayList<>();
            p.sensitivity.clear();
            p.search = new ArrayList<>();
            p.search.clear();

            p.prompts = new ArrayList<>();
            p.prompts.clear();
            p.commands = new ArrayList<>();
            p.commands.clear();

            p.isTrigger = isTrigger;
            File fi = new File(appDirFull + file);
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
                } else if (type.equals("::Name")) {
                    p.name = line;
                } else if (type.equals("::Vocabulary")) {
                    p.vocabulary.add(line);
                } else if (type.equals("::Sensitivity")) {
                    p.sensitivity.add(line);
                } else if (type.equals("::Search")) {
                    p.search.add(fi.getParent() + File.separator + line);
                } else if (type.equals("::Net")) {
                    p.netFile = fi.getParent() + File.separator + line;
                } else if (type.equals("::Gender")) {
                    p.genderFile = fi.getParent() + File.separator + line;
                } else if (type.equals("::AntiData")) {
                    p.antiDataFile = fi.getParent() + File.separator + line;
                } else if (type.equals("::PARAMA_OFFSET")) {
                    p.paramAOffset = Float.parseFloat(line);
                } else if (type.equals("::DUR_MIN_FACTOR")) {
                    p.durMinFactor = Float.parseFloat(line);
                } else if (type.equals("::DUR_MAX_FACTOR")) {
                    p.durMaxFactor = Float.parseFloat(line);
                } else if (type.equals("::TRIGGER_SAMP_PER_CAT")) {
                    p.triggerSampPerCat = Float.parseFloat(line);
                } else if (type.equals("::SAMP_PER_CAT_WITHIN")) {
                    p.sampPerCatWithin = Float.parseFloat(line);
                } else if (type.equals("::TARGET_SNR")) {
                    p.targetSNR = Float.parseFloat(line);
                } else if (type.equals("::LEARN_RATE")) {
                    p.learnRate = Float.parseFloat(line);
                } else if (type.equals("::LEARN_RATE_WITHIN")) {
                    p.learnRateWithin = Float.parseFloat(line);
                } else if (type.equals("::DROPOUT_WITHIN")) {
                    p.dropoutWithin = Float.parseFloat(line);
                } else if (type.equals("::EPQ_MIN")) {
                    p.epqMin = Float.parseFloat(line);
                } else if (type.equals("::USE_FEAT")) {
                    p.useFeat = Float.parseFloat(line);
                } else if (type.equals("::CheckSNR")) {
                    p.checkSNR = Float.parseFloat(line);
                } else if (type.equals("::CheckVowelDur")) {
                    p.checkVowelDur = Float.parseFloat(line);
                } else if (type.equals("::Prompts")) {
                    //Log.i(TAG, "line=" + line);
                    promptClass prompt = new promptClass();
                    String[] parts = line.split("\\|", 2);
                    prompt.name = parts[0];
                    prompt.text = parts[1];
                    prompt.text = prompt.text.replace("\\n", "\n");
                    p.prompts.add(prompt);
                } else if (type.equals("::Commands")) {
                    commandClass command = new commandClass();
                    String[] parts = line.split("\\|", 3);
                    command.phrase = parts[0];
                    command.prompt = parts[1];
                    command.prompt = command.prompt.replace("\\n", "\n");
                    command.response = parts[2];
                    command.response = command.response.replace("\\n", "\n");
                    p.commands.add(command);
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
