/* Spreadtrum Communication Inc. 2016 */

package android.print;

import android.os.Looper;

/**
 * @hide
 */
public class PrintManagerHelper {

    public static PrintJob print(PrintManager manager, String printJobName, PrintDocumentAdapter documentAdapter,
            PrintAttributes attributes, Looper looper) {
        return manager.print(printJobName, documentAdapter, attributes, looper);
    }

}
