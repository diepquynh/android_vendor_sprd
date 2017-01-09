
package com.sprd.voicetrigger.languagesupport;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.preference.PreferenceManager;

import com.sprd.voicetrigger.R;
import com.sprd.voicetrigger.global.SharedPreferencesField;

import java.util.HashMap;
import java.util.Locale;
import java.util.Map;

public class SupportLanguages {
    private Context mContext;
    private Map<String, String> map = new HashMap<String, String>();
    private SharedPreferences mSharedPreferences;
    private Editor mEditor;

    public SupportLanguages(Context context) {
        mContext = context;
        mSharedPreferences = PreferenceManager.getDefaultSharedPreferences(mContext
                .getApplicationContext());
        mEditor = mSharedPreferences.edit();
        try {
            updateMap();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void updateMap() throws Exception {
        String[] key = getKeys(mContext);
        String[] value = getValues(mContext);
        if (key.length != value.length) {
            throw new Exception("support languages key-value are not one to one relations");
        }
        for (int i = 0; i < key.length; i++) {
            map.put(key[i], value[i]);
        }
    }

    public String getCurrentLanguageKey() {
        return mSharedPreferences.getString(SharedPreferencesField.CHOOSED_LANGUAGE,
                getCurrentLauguage());
    }

    public String getCurrentLanguageValue() {
        String key = getCurrentLanguageKey();
        return map.get(key);
    }

    public int getCurrentLanguageIndex() {
        return getIndexWithLanguageKey(getCurrentLanguageKey());
    }

    public String getLanguageKeyString(int index) {
        String[] keys = getKeys(mContext);
        return keys[index];
    }

    public int getIndexWithLanguageKey(String languageKey) {
        String[] keys = getKeys(mContext);
        String key = languageKey;
        int index = 0;
        for (int i = 0; i < keys.length; i++) {
            if (keys[i].equals(key)) {
                index = i;
                break;
            }
        }
        return index;
    }

    public String getLanguageFullName(String key) {
        return map.get(key);
    }

    public static String getCurrentLauguage() {
        String mCurrentLanguage = Locale.getDefault().getLanguage();
        if (mCurrentLanguage.equals("zh")) {
            return "zh-cn";
        } else {
            return "en-us";
        }
    }

    public boolean setCurrentLanguageWithKey(String key) throws Exception {
        if (!map.containsKey(key)) {
            throw new Exception("unSupported languages key!");
        }
        mEditor.putString(SharedPreferencesField.CHOOSED_LANGUAGE, key);
        mEditor.commit();
        return true;
    }

    public boolean setCurrentLanguageWithIndex(int index) {
        if (index < 0 || index > map.size()) {
            throw new IllegalArgumentException();
        }
        String key = getKeys(mContext)[index];
        try {
            return setCurrentLanguageWithKey(key);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return false;
    }

    private String[] getKeys(Context context) {
        return context.getResources().getStringArray(R.array.support_languages_key);
    }

    private String[] getValues(Context context) {
        return context.getResources().getStringArray(R.array.support_languages_value);
    }

    /**
     * get different string form different languages by user defined ,such as when user set
     * the module language en-us ,will return the en-us's wakeup words "hello blue genie",
     * when user set zn-ch will return "芝麻开门"
     *
     * @param format a format string you need to format
     * @return a string dependence different module language
     */
    public String getChoosedLanguageWakeupWordsString(int index, String format) {
        String LanguageDependenceStr = mContext.getResources().getStringArray(R.array.wakeup_words_string_array)[index];
        return String.format(format, LanguageDependenceStr);
    }
}
