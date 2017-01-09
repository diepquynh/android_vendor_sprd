package android.content.pm;

import static android.content.pm.PackageManager.INSTALL_PARSE_FAILED_INCONSISTENT_CERTIFICATES;
import static android.content.pm.PackageManager.INSTALL_PARSE_FAILED_NO_CERTIFICATES;

import android.content.pm.PackageParser.Package;
import android.content.pm.PackageParser.PackageParserException;
import android.util.ArraySet;
import android.util.Log;
import android.util.Slog;
import com.android.internal.util.ArrayUtils;

import java.io.File;
import java.security.GeneralSecurityException;
import java.security.PublicKey;
import java.security.cert.Certificate;
import java.util.Iterator;
import java.util.List;
import android.util.jar.StrictJarFile;
import java.util.zip.ZipEntry;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

public class PackageParserExUtils {
    private static final String TAG = "PackageParserUtils";
    private static boolean mIsasync1 = true;
    private static boolean mIsasync2 = true;
    private static boolean mIsSync1Finish = false;
    private static int mThreshold;
    /* SPRD: Modify the process of the certificate verification to improve the speed of installing a
pplication{@ */
    public static void collectNonSystemAppCertificates(Package pkg,final List<ZipEntry> toVerify,
            final StrictJarFile jarFile,File apkFile) throws PackageParserException{
        int length = toVerify.size();
        final Package mPkg=pkg;
        Slog.i(TAG, "collectCertificates length = " + length);
        mThreshold = length / 2;
        final CountDownLatch mConnectedSignal = new CountDownLatch(2);
        new Thread("asynccollectCertificates1") {
            @Override
            public void run() {
                mIsasync1=asynccollectCertificates(mPkg,toVerify, jarFile, 1);
                mIsSync1Finish = true;
                if (!mIsasync1) {
                       mConnectedSignal.countDown();
                   }
                mConnectedSignal.countDown();
            }
        }.start();

        new Thread("asynccollectCertificates2") {
            @Override
            public void run() {
                mIsasync2=asynccollectCertificates(mPkg,toVerify, jarFile, 2);
                if(mIsSync1Finish){
                    mIsSync1Finish = false;
                }
                if (!mIsasync2) {
                    mConnectedSignal.countDown();
                   }
                mConnectedSignal.countDown();
            }
        }.start();
        waitForLatch(mConnectedSignal);
        Slog.e(TAG,"mIsasync1 = " + mIsasync1 + " mIsasync2 = " + mIsasync2);
        if (!mIsasync1 || !mIsasync2) {
            throw new PackageParserException(INSTALL_PARSE_FAILED_NO_CERTIFICATES,
                     "Package " + apkFile.getAbsolutePath() + " has no certificates ");
          }
    }

    /* SPRD: Modify the process of the certificate verification to improve the speed of installing application{@ */
    private static void waitForLatch(CountDownLatch latch) {
        for (;;) {
            try {
                if (latch.await(1000 * 500, TimeUnit.MILLISECONDS)) {
                    Slog.e(TAG, "waitForLatch done!");
                    return;
                } else {
                    Slog.e(TAG, "Thread " + Thread.currentThread().getName()
                            + " still waiting for asynccollectCertificates ready...");
                }
            } catch (InterruptedException e) {
                Slog.e(TAG, "Interrupt while waiting for asynccollectCertificates to be ready.");
            }
        }
    }

    private static boolean asynccollectCertificates(Package pkg,List<ZipEntry> entries, StrictJarFile jarFile,
            int asyncNum){
        int elementNum = 0;
        Slog.e(TAG, "asyncNum = " + asyncNum);
        try{
            final Iterator<ZipEntry> i = entries.iterator();
            while (i.hasNext()) {
                final ZipEntry entry = i.next();
                elementNum++;
                if (asyncNum == 1) {
                    if (elementNum > mThreshold) {
                        break;
                    }
                } else {
                    if (elementNum <= mThreshold) {
                        continue;
                    }
                }
                if (entry.isDirectory()) continue;
                if (entry.getName().startsWith("META-INF/")) continue;



               final Certificate[][] localCerts = PackageParser.loadCertificates(jarFile, entry);

               if (ArrayUtils.isEmpty(localCerts)) {
                   throw new PackageParserException(INSTALL_PARSE_FAILED_NO_CERTIFICATES,
                           "Package " + pkg.packageName + " has no certificates at entry "
                           + entry.getName());
               }
               if((pkg.mCertificates == null) && (asyncNum == 2)){
                   Slog.e(TAG, "certs == null");
                   while(pkg.mCertificates == null){
                       try{
                           Thread.sleep(20);
                           if(mIsSync1Finish){
                               break;
                           }
                           continue;
                       }catch(Exception e){
                           Slog.i(TAG, "exception occured");
                       }
                   }
               }
               final Signature[] entrySignatures = PackageParser.convertToSignatures(localCerts);

               if (pkg.mCertificates == null) {
                   pkg.mCertificates = localCerts;
                   pkg.mSignatures = entrySignatures;
                   Log.d(TAG, "pkg: "+pkg.packageName+"pkg.mCertificates:  "+pkg.mCertificates+
                           "  pkg.mSignatures: "+pkg.mSignatures);
                   pkg.mSigningKeys = new ArraySet<PublicKey>();
                   for (int i1=0; i1 < localCerts.length; i1++) {
                       pkg.mSigningKeys.add(localCerts[i1][0].getPublicKey());
                     }
               } else {
                   if (!Signature.areExactMatch(pkg.mSignatures, entrySignatures)) {
                       throw new PackageParserException(
                               INSTALL_PARSE_FAILED_INCONSISTENT_CERTIFICATES, "Package " + pkg
                                       + " has mismatched certificates at entry "
                                       + entry.getName());
                   }
               }
           }

       } catch (GeneralSecurityException e) {
            Slog.w(TAG, "Failed to collect certificates from " + pkg.packageName, e);
            return false;
       } catch (Exception  e) {
            Slog.w(TAG, "Failed to collect certificates from " + pkg.packageName, e);
            return false;
        }
       return true;
    }
    /* @} */
}
