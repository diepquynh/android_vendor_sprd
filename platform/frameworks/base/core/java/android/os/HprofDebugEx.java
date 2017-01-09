
package android.os;

import java.io.File;
import java.io.IOException;
import java.util.Date;

import dalvik.system.VMDebug;

import android.app.ActivityManager;
import android.util.Log;
import android.util.Slog;

public class HprofDebugEx extends HprofDebug{

    private static final String TAG = "HprofDebugEx";
    // The product whether is debug version
    private static Boolean IS_DEBUG_BUILD = null;
    private static final boolean DO_SET_DUMP_HPROF = SystemProperties.getBoolean(
            "persist.sys.sprd.monkey", false);

    /**
     * @SPRD: Detect whether we are in monkey mode. @{ I think the best usage is
     *        in "catch" block, like this: try { ... ... } catch (XxxException
     *        e) { if (android.os.Debug.isMonkey()) { ... ... } else { throw e;
     *        } } Because it needs some time to get the system property, pls
     *        don't use it too frequently.
     * @return Whether in monkey mode {@hide}
     */
    public static boolean isMonkey() {
        return ActivityManager.isUserAMonkey();
    }

    /**
     * @SPRD: Should dump hprofile. @{ {@hide}
     */
    public static boolean shouldDumpHProfile() {
        return (DO_SET_DUMP_HPROF || isDebug() || isMonkey());
    }

    /**
     * @SPRD: Dump the hprof file of the current process. @{ Don not use this in
     *        non-debug mode, because the hprof file will be dump to the /data
     *        block, and these files may be large.
     * @param pname The current process name. {@hide}
     */
    public static void dumpHprof(String pname) {
        final String HPROFILE_PATH = Environment.getDataDirectory() + "/misc/hprofs/";
        File dir = new File(HPROFILE_PATH);

        if (dir.exists() && dir.isDirectory() && dir.canWrite()) {
            File[] files = dir.listFiles();
            for (File f : files) {
                String p = f.getPath();
                if (f.isFile() && p.contains(pname) && p.endsWith("hprof")) {
                    Log.w(TAG, "Delete old hprofile: " + p);
                    f.delete();
                }
            }
            int pid = Process.myPid();
            java.text.SimpleDateFormat df = new java.text.SimpleDateFormat("yyyy_MM_dd_HH_mm_ss");
            String filename = HPROFILE_PATH + pname + "_" + pid + "_" + df.format(new Date())
                    + ".hprof";

            try {
                Log.w(TAG, "Dump hprofile: " + filename);
                dumpHprofData(filename);
            } catch (IOException e) {
                Log.w(TAG, "Fail to dump hprofile - " + filename, e);
            }
        } else {
            Log.w(TAG,
                    "Fail to dump hprofile " + pname + ": " + " - " + dir.exists() + ", "
                            + dir.canWrite());
        }
    }

    /**
     * Dump "hprof" data to the specified file. This may cause a GC.
     *
     * @param fileName Full pathname of output file (e.g. "/sdcard/dump.hprof").
     * @throws UnsupportedOperationException if the VM was built without HPROF
     *             support.
     * @throws IOException if an error occurs while opening or writing files.
     */
    public static void dumpHprofData(String fileName) throws IOException {
        VMDebug.dumpHprofData(fileName);
    }

    /**
     * @SPRD: Detect whether we are in debug mode. @{ I think the best usage is
     *        in "catch" block, like this: try { ... ... } catch (XxxException
     *        e) { if (android.os.Debug.isDebug()) { ... ... } else { throw e; }
     *        } Because it needs some time to get the system property, pls don't
     *        use it too frequently.
     * @return Whether in debug mode {@hide}
     */
    public static boolean isDebug() {
        if (IS_DEBUG_BUILD == null) {
            IS_DEBUG_BUILD = Build.TYPE.equals("eng") || Build.TYPE.equals("userdebug");
        }
        return IS_DEBUG_BUILD;
    }

    /**
     * SPRD: Dump hprofile when OOM. @{
     *
     * @param: name The name of process
     * @param: e The exception
     */
    public void dumpHProfile(String name, Throwable e) {
        if (e instanceof OutOfMemoryError) {
            if (shouldDumpHProfile()) {
                dumpHprof(name);
            } else {
                Slog.d(TAG,
                        "There is a OOM , but did not dumped, because of the present version is not USERDEBUG or did not tested by MONKEY.");
            }
        } else if (e instanceof RuntimeException) {
            String cause = Log.getStackTraceString(e.getCause());
            if (null != cause && cause.contains("OutOfMemoryError")) {
                if (shouldDumpHProfile()) {
                    dumpHprof(name);
                } else {
                    Slog.d(TAG,
                            "There is a OOM , but did not dumped, because of the present version is not USERDEBUG or did not tested by MONKEY.");
                }
            }
        }
    }

}
