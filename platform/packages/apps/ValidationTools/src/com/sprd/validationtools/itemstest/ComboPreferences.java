
package com.sprd.validationtools.itemstest;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.preference.PreferenceManager;

import java.util.Map;
import java.util.Set;
import java.util.WeakHashMap;
import java.util.concurrent.CopyOnWriteArrayList;

public class ComboPreferences implements SharedPreferences, OnSharedPreferenceChangeListener {
    private SharedPreferences mPrefGlobal;
    private SharedPreferences mPrefLocal;
    private CopyOnWriteArrayList<OnSharedPreferenceChangeListener> mListeners;
    private static WeakHashMap<Context, ComboPreferences> sMap =
            new WeakHashMap<Context, ComboPreferences>();

    public ComboPreferences(Context context) {
        mPrefGlobal = PreferenceManager.getDefaultSharedPreferences(context);
        mPrefGlobal.registerOnSharedPreferenceChangeListener(this);
        synchronized (sMap) {
            sMap.put(context, this);
        }
        mListeners = new CopyOnWriteArrayList<OnSharedPreferenceChangeListener>();
    }

    public void setLocalId(Context context, int cameraId) {
        String prefName = context.getPackageName() + "_preferences_" + cameraId;
        if (mPrefLocal != null) {
            mPrefLocal.unregisterOnSharedPreferenceChangeListener(this);
        }
        mPrefLocal = context.getSharedPreferences(prefName, Context.MODE_PRIVATE);
        mPrefLocal.registerOnSharedPreferenceChangeListener(this);
    }

    public SharedPreferences getGlobal() {
        return mPrefGlobal;
    }

    public SharedPreferences getLocal() {
        return mPrefLocal;
    }

    @Override
    public Map<String, ?> getAll() {
        throw new UnsupportedOperationException();
    }

    @Override
    public String getString(String key, String defValue) {
        if (!mPrefLocal.contains(key)) {
            return mPrefGlobal.getString(key, defValue);
        } else {
            return mPrefLocal.getString(key, defValue);
        }
    }

    @Override
    public Set<String> getStringSet(String key, Set<String> defValues) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void registerOnSharedPreferenceChangeListener(
            OnSharedPreferenceChangeListener listener) {
        mListeners.add(listener);
    }

    @Override
    public void unregisterOnSharedPreferenceChangeListener(
            OnSharedPreferenceChangeListener listener) {
        mListeners.remove(listener);
    }

    @Override
    public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
        for (OnSharedPreferenceChangeListener listener : mListeners) {
            listener.onSharedPreferenceChanged(this, key);
        }
    }

    @Override
    public int getInt(String key, int defValue) {
        return 0;
    }

    @Override
    public long getLong(String key, long defValue) {
        return 0;
    }

    @Override
    public float getFloat(String key, float defValue) {
        return 0;
    }

    @Override
    public boolean getBoolean(String key, boolean defValue) {
        return false;
    }

    @Override
    public boolean contains(String key) {
        return false;
    }

    @Override
    public Editor edit() {
        return null;
    }
}
