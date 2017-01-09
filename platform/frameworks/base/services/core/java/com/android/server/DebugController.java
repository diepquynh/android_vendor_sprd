
package com.android.server;

import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.lang.reflect.Field;

import android.os.Build;
import android.text.TextUtils;

/***
 * A convenient class to open or close debug parameter. This will work well only in user-debug mode.
 * the commands should be like this:
 * adb shell dumpsys [service] [cmd] [parameter] [value]
 *
 */
public class DebugController {
    private static Boolean IS_DEBUG_BUILD = null;
    public static boolean isDebug() {
        if (IS_DEBUG_BUILD == null) {
            IS_DEBUG_BUILD = Build.TYPE.equals("eng") || Build.TYPE.equals("userdebug");
        }
        return IS_DEBUG_BUILD;
    }


    public static boolean dump(FileDescriptor fd, PrintWriter pw, String[] args, Object service) {
        // should only do this in debug mode.
        if (!isDebug()) {
            pw.println("not user-debug.");
            return false;
        }

        int opti = 0;
        String cmd = null;
        String valueStr = null;
        boolean value = false;
        boolean output = false; // whether to out put the parameter
        while (opti < args.length) {
            String opt = args[opti];
            if (opt == null || opt.length() <= 0 || opt.charAt(0) != '-') {
                break;
            }
            opti++;
            if ("-set".equals(opt)) {
                if (opti < args.length - 1) {
                    cmd = args[opti++];
                    valueStr = args[opti++];
                    break;
                } else if (opti < args.length) {
                    cmd = args[opti++];
                    break;
                } else {
                    pw.println("  Error: -p option requires argument.");
                    return false;
                }
            } else if ("-out".equals(opt)) {
                output = true;
                cmd = args[opti++];
                break;
            }
        }

        // empty command
        if (TextUtils.isEmpty(cmd)) {
            return false;
        }

        boolean setAll =  "debug".equalsIgnoreCase(cmd);

        //should only do this with debug parameter
        if(!cmd.startsWith("DEBUG") && !setAll){
            return false;
        }

        if(setAll){
            cmd = "localLOGV";
        }

        if ("0".equals(valueStr) || "false".equalsIgnoreCase(valueStr)) {
            value = false;
        } else if ("1".equals(valueStr) || "true".equalsIgnoreCase(valueStr)) {
            value = true;
        }

        try {
            Field field = null;
            Class cls = null;
            try {
                cls = service.getClass();
                field = cls.getDeclaredField(cmd);
            } catch (NoSuchFieldException e) {
                cls = cls.getSuperclass();
                field = cls.getDeclaredField(cmd);
            }

            boolean access = field.isAccessible();
            field.setAccessible(true);
            if (output) {
                pw.println(cmd + " = " + field.getBoolean(service));
            } else {
                field.setBoolean(service, value);
            }
            field.setAccessible(access);
        } catch (Exception e) {
            pw.println("error, " + e.getMessage());
            return false;
        }
        return true;
    }
}
