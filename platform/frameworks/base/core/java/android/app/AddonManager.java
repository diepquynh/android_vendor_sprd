package android.app;

import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.os.Bundle;
import android.app.PackageManagerEx;
import android.os.IBinder;
import android.os.Build;
import android.util.ArrayMap;
import android.util.Log;
import android.util.SparseArray;

import dalvik.system.PathClassLoader;

import java.util.ArrayList;
import java.util.List;
import java.util.Set;
import java.lang.ref.WeakReference;

/**
 * The plugin interface for feature management.
 */
public class AddonManager /* TODO extends IAddonManager */ {
    public static interface InitialCallback {
        public Class onCreateAddon(Context context, Class clazz);
    }

    private static final String LOGTAG = "AddonManager";
    private static final boolean DEBUG = true;

    private static final String NO_PLUGIN = "";

    private static AddonManager sInstance;

    //Feature object cache
    private static ArrayMap<String, WeakReference<Object>> sFeatureCache
            = new ArrayMap<String, WeakReference<Object>>();
    private static ArrayMap<String, WeakReference<Class>> sClassCache
            = new ArrayMap<String, WeakReference<Class>>();
    //Feature class name cache. id map may be useless in future.
    private static SparseArray<String> sIdMap = new SparseArray<String>();
    //The installed Addon package information list
    private List<PackageInfo> mManagedList;

    private IBinder mService;

    /**
     * In the normal case, mContext is equals to android.app.ActivityThread.currentApplication().
     * If the APP is shared one process with other apps, the two context
     * maybe differently, because the APP need to create AddonManager itself
     * by the AddonManager Constructor {@link AddonManager(Context context)}.
     */
    private Context mContext;
    static void attachApplication(Context context) {
    }

    /**
     * Get the default addonManager, its connect to the application which
     * context belong to.
     * @return Reture the addonmanager instance for host application.
     */
    public static AddonManager getDefault() {
        if (sInstance != null) return sInstance;
        sInstance = new AddonManager(ActivityThread.currentApplication());
        return sInstance;
    }

    /* TODO for binder service */
    AddonManager(ContextImpl context, IBinder binder) {
        this(context);
        mContext = context;
        mService = binder;
    }

    /**
     * The constructor of AddonManager, App can use it to create its own
     * addonmanager.
     * @param context The context of the host application.
     */
    public AddonManager(Context context) {
        mContext = context;
        if (mManagedList == null) {
            mManagedList = manageFeatureListS(context);
        }
    }

    private static List<PackageInfo> manageFeatureListS(Context context) {
        long now = android.os.SystemClock.uptimeMillis();
        List<PackageInfo> infoList;
        final PackageManagerEx pms = (PackageManagerEx)context.getPackageManager();
        infoList = pms.getPackageFeatureList(context.getPackageName());
        if(DEBUG) {
            Log.d(LOGTAG,"manageFeatureList cost:"+(android.os.SystemClock.uptimeMillis() - now));
        }
        if(infoList == null) {
            infoList = new ArrayList<PackageInfo>();}
        return infoList;
    }
    /**
     * Get the target package information of this AddonManger.
     * @param context The context belong to the host APP
     * @return Return the package information list of Addon.
     */
    private static List<PackageInfo> manageFeatureList(Context context) {
        final PackageManager pms = context.getPackageManager();
        List<PackageInfo> infoList = new ArrayList<PackageInfo>();
        final List<PackageInfo> installedList
                = pms.getInstalledPackages(PackageManager.GET_META_DATA);
        if (installedList != null && !installedList.isEmpty()) {
            String currentPackageName = context.getPackageName();
            for (PackageInfo pkg : installedList) {
                // verify data valid
                if (pkg == null
                        || pkg.applicationInfo == null
                        || !pkg.applicationInfo.enabled
                        || pkg.applicationInfo.metaData == null) {
                    continue;
                }

                // Check meta and verify key isFeatureAddon is true.
                // And ensure targetPackages is current package.
                Bundle metaData = pkg.applicationInfo.metaData;
                if (metaData.containsKey("isFeatureAddon")) {
                    Set<String> metaKeys = metaData.keySet();
                    boolean foundTargetPackages = false;
                    for (String key : metaKeys) {
                        if (key.startsWith("targetPackages")) {
                            if (metaData.getString(key, "").contains(
                                    currentPackageName)) {
                                if (DEBUG) Log.d(LOGTAG, "-> manageFeatureList: The Plugin of <" +
                                        currentPackageName + "> is <" + pkg + "> .");
                                infoList.add(pkg);
                                foundTargetPackages = true;
                            }
                        }
                    }
                    if (foundTargetPackages) {
                        // If targetPackages is not set, that means it may
                        // take effects in framework or system server.
                        // It MUST install to internal only to make it safety
                        // to load class.
                        if (pkg.installLocation == PackageInfo.INSTALL_LOCATION_INTERNAL_ONLY) {
                            if (DEBUG) Log.d(LOGTAG, "-> manageFeatureList: The Plugin of <" +
                                        currentPackageName + "> is <" + pkg + "> .");
                            infoList.add(pkg);
                        } else {
                            Log.w(LOGTAG, "-> manageFeatureList: no target package and not " +
                                    "install internal only, drop " + pkg);
                        }
                    }
                }
            }
        } else {
            // Should not happen
        }
        return infoList;
    }

    private static final String getCachedId(int featureId, Context context) {
        String value = sIdMap.get(featureId);
        // The value being null means the cache has been not created.
        if (value == null) {
            value = context.getString(featureId, NO_PLUGIN);
            sIdMap.put(featureId, value);
        }
        return value;
    }

    /**
     * Get the cached feature object.
     * @param featureName The feature class name.
     * @return Reture the cached feature object or null if the feature
     * object has not cached before.
     */
    private static final Object getCachedFeature(String featureName) {
        if (featureName == null || NO_PLUGIN.equals(featureName)) return null;
        WeakReference<Object> feature = sFeatureCache.get(featureName);
        if (feature != null && feature.get() != null) {
            return feature.get();
        } else {
            sFeatureCache.remove(featureName);
            return null;
        }
    }

    /**
     *
     * Accept null object and return null
     */
     /*
    private static final Object getCachedClass(String className) {
        if (featureName == null || NO_PLUGIN.equals(className)) return null;
        WeakReference<Class> feature = sFeatureCache.get(className);
        if (feature != null && feature.get() != null) {
            return feature.get();
        } else {
            sFeatureCache.remove(featureName);
            return null;
        }
    }*/

    private Object createDefaultObject(Class defClazz) {
        try {
            return defClazz.newInstance();
        } catch (Exception e) {
            Log.e(LOGTAG, "Terrible, create default instance of class failed!", e);
        }
        return null;
    }

    /**
     * Get default addon without specified feature id / feature class. The class
     *
     * Same as AddonManager#getAddon(String, Class), the feature id
     * will be converted to raw string to reflect.
     *
     * @param defClazz The defalut class in the host APP that be extended by Addon class.
     * @return Return the Addon class instance or DefClazz instance.
     */
    public Object getAddon(Class defClazz) {
        return getAddon(0, defClazz);
    }

    /**
     * Get the Addon feature object if it exists, if not return the defClazz
     * Object. Same as AddonManager#getAddon(String, Class), the feature id
     * will be converted to raw string to reflect.
     *
     * @param featureId The string ID of feature class name.
     * @param defClazz The defalut class in the host APP that be extended by Addon class.
     * @return Return the Addon class instance or DefClazz instance.
     */
    public Object getAddon(int featureId, Class defClazz) {
        // If feature id is a valid id, try to reflect the class you defended in xml resources.
        if (featureId > 0) {
            return getAddon(mContext.getString(featureId, NO_PLUGIN), defClazz);
        } else {
            // If feature id is invalid, it means we reflect by default way, new feature added
            // in 2016.02.14.
            String[] classNameSuffixes = getClassNameSuffix();
            // Get feature class name suffixes and append them and try to
            // reflection.
            if (classNameSuffixes != null && classNameSuffixes.length >= 0) {
                Object result = null;
                String featureClassName = defClazz.getName();
                for (String classNameSuffix : classNameSuffixes) {
                    result = getAddon(featureClassName + classNameSuffix, defClazz);
                    // if the class of new instance is absolutely same as default
                    // maybe it is not found, try to find a new one.
                    // TODO that is a risk of big cost or memory leak to create again.
                    if (!result.getClass().equals(defClazz)) {
                        break;
                    }
                }
                return result;
            } else {
                return createDefaultObject(defClazz);
            }
        }
    }

    /**
     * Get class name of suffix, it/they will be auto append to the name of defClass
     * and use appended to reflect.
     * Currently is "Impl", can be more but not suggest because performance, don't
     * seek class more times.
     * For example, if your default class is named com.sprd.Aclass
     * it will try to load and reflect com.sprd.AclassImpl.
     */
    String[] getClassNameSuffix() {
        return new String[] { "Impl" };
    }

    /**
     * Get the Addon feature object if it exists, if not return the defClazz
     * Object.
     * @param featureClassName The raw String of feature class name.
     * @param defClazz The defalut class in the host APP that be extended by Addon class.
     * @return Return the Addon class instance or DefClazz instance.
     */
    public Object getAddon(String featureClassName, Class defClazz) {
        if (DEBUG) {
            Log.d(LOGTAG, "-> getAddon: featureClassName:" + featureClassName);
            if (defClazz != null)
            Log.i(LOGTAG, "-> getAddon,The classloader of defClazz is " + defClazz.getClassLoader());
        }

        ApplicationInfo applicationInfo = mContext.getApplicationInfo();
        Object featureObject = null;
        // If reflecting classname is not valid, create instance by default directly.
        if (NO_PLUGIN.equals(featureClassName)) {
            featureObject = createDefaultObject(defClazz);
        } else {
            // Find best matching class loader by previous scanning package info list.
            ClassLoader loader = defClazz.getClassLoader();
            if (mManagedList.isEmpty()) {
                // TODO Oh sorry, it seems do nothing currently.
            } else {
                for (PackageInfo pkg : mManagedList) {
                    // Check the featureClassNames has defined in AndroidManifest
                    final Bundle metaData = pkg.applicationInfo.metaData;

                    // Feature changed in 2014.02.14.
                    // The full class path may make meta data too long and can't
                    // be readale, we search all meta which key is like
                    // pattern ^featureClassNames.*, and try to match.
                    boolean hasDefinedFeatureClass = false;
                    Set<String> metaKeys = metaData.keySet();
                    for (String key : metaKeys) {
                        if (key.startsWith("featureClassNames")) {
                            if (metaData.getString(key, NO_PLUGIN).contains(
                                    featureClassName)) {
                                hasDefinedFeatureClass = true;
                                applicationInfo = pkg.applicationInfo;
                                break;
                            }
                        }
                    }

                    // If contains that class, load that apk package into classloader
                    if (hasDefinedFeatureClass) {
                        loader = ApplicationLoaders.getDefault().getClassLoader(
                                pkg.applicationInfo.sourceDir, Build.VERSION_CODES.N, false,
                                null, null, mContext.getClassLoader());
                        break;
                    }
                }
            }

            Class<?> clazz = null;
            try {
                clazz = loader.loadClass(featureClassName);
            } catch (ClassNotFoundException e) {
                Log.e(LOGTAG, "Load class " + featureClassName + " failed!", e);
            }
            if (DEBUG) Log.d(LOGTAG, "->getAddon, createAddon app: " + applicationInfo
                    + ", clazz: " + clazz+ ", classloader:" + loader);

            featureObject = createAddon(applicationInfo, clazz);
        }

        // Create instance by all methods failed? Try to create default one.
        if (featureObject == null) {
            try {
                if (DEBUG) Log.d(LOGTAG, "->getAddon: Create feature <" + featureClassName +
                        "> object Failed," + " AddonManager will return <" + defClazz + "> object.");
                if (defClazz != null) featureObject = createDefaultObject(defClazz);
            } catch (Exception e) {
                Log.e(LOGTAG, "Create default instance failed", e);
            }
        }

        return featureObject;
    }

    private Object createAddon(ApplicationInfo applicationInfo, Class clazz) {
        Object featureObject = null;
        if (clazz != null) {
            try {
                featureObject = clazz.newInstance();
                if (featureObject instanceof InitialCallback) {
                    InitialCallback callback = (InitialCallback) featureObject;
                    Context pluginContext = null;
                    try {
                        pluginContext = mContext.getApplicationContext()
                                .createPackageContext(
                                        applicationInfo.packageName,
                                        Context.CONTEXT_IGNORE_SECURITY | Context.CONTEXT_INCLUDE_CODE);
                    } catch (Exception e) {
                        Log.e(LOGTAG, "Create addon context failed");
                    }
                    // TODO processing below can't be readable, maybe removed
                    // in future.
                    Class wantedClass = callback.onCreateAddon(pluginContext, clazz);
                    if (DEBUG) Log.d(LOGTAG, "->createAddon, wantedClass <" + wantedClass +
                            ">, pluginContext: " + pluginContext);
                    if (wantedClass != null && !wantedClass.equals(clazz)) {
                        featureObject = createAddon(applicationInfo, wantedClass);
                    }
                }
            } catch (Exception e) {
                Log.e(LOGTAG, "Createing addon failed", e);
            }
        }
        return featureObject;
    }

    /**
     * @Deprecated not completed
     */
    public boolean isFeatureEnabled(int featureId) {
        // TODO should be remote and be controlled
        return true;
    }

    /**
     * @Deprecated not completed
     */
    public boolean setFeatureEnabled(boolean enabled, int featureId) {
        // TODO should be remote and be controlled
        return true;
    }
}
