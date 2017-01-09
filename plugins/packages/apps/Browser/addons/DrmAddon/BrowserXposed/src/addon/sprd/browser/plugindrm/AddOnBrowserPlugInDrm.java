package addon.sprd.browser.plugindrm;

import java.io.File;
import android.net.Uri;
import android.os.Environment;
import android.util.Log;
import android.content.Context;
import android.drm.DrmManagerClient;
import android.app.AddonManager;
import com.sprd.drmplugin.BrowserPlugInDrm;

public class AddOnBrowserPlugInDrm extends BrowserPlugInDrm implements AddonManager.InitialCallback{


    private Context sContext;

    public AddOnBrowserPlugInDrm(){
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        sContext = context;
        return clazz;
    }

    @Override
    public boolean canGetDrmPath(){
        String externalStatus = Environment.getExternalStoragePathState(); //TCard
        String externalStoragePath = Environment.getExternalStoragePath().getPath();//TCard
        if ((null == externalStatus) || (externalStoragePath == null)){
            return false;
        }
        return true;
    }

    @Override
    public Uri getDrmPath(String url, String mimetype , String filename){
        String drmPath = null;
        String status = null;
        String path = null;
        status = Environment.getInternalStoragePathState();//TCard
        path = Environment.getInternalStoragePath().getPath();//TCard
        String externalStatus = Environment.getExternalStoragePathState(); //TCard
        String externalStoragePath = Environment.getExternalStoragePath().getPath();//TCard
        if (isDownloadForDRM(url, mimetype)){
            if (externalStatus.equals(Environment.MEDIA_MOUNTED)){
                drmPath = createDRMdownloadDirectory(externalStoragePath);
                Log.i("FORDRM", "by url external path  "+drmPath);
            }else if (status.equals(Environment.MEDIA_MOUNTED)){
                drmPath = createDRMdownloadDirectory(path);
                Log.i("FORDRM", "by url internal path  "+drmPath);
            }
        }
        if  ((drmPath != null) && (isDownloadForDRM(url, mimetype))) {
            Log.i("FORDRM", "DRM download path  "+(drmPath+filename));
            return Uri.fromFile(new File(drmPath+filename));
        }
        return null;
    }

    @Override
    public CharSequence getFilePath(CharSequence path , String str , String filename){
        if(filename != null && filename.startsWith("content://drm/")){
            path = str + "/data/data/com.android.providers.drm/files/";
        }
        if (filename != null){
           if (filename.endsWith(".dm")){
               filename = filename.replace(".dm", ".dcf");
               path = str + filename;
           }else if (filename.endsWith(".dr")){
               String downloadDrm = sContext.getString(R.string.download_drm_success);
               path = downloadDrm;
           }else if (filename.equals("transfer_failed")){
               path = sContext.getString(R.string.drm_conversion_failed);
           }
        }
        return path;
    }

    @Override
    public boolean getMimeType(String mimetype){
        /* SPRD: 506914 change the order to avoid NullPointerException @{ */
        return (!"application/vnd.oma.drm.rights+xml".equals(mimetype)
                && !"application/vnd.oma.drm.rights+wbxml".equals(mimetype));
        /* @} */
    }

    private boolean isDownloadForDRM(String url, String mimetype){
        if (mimetype != null){
            if (mimetype.equals("application/vnd.oma.drm.message") ||
                      mimetype.equals("application/vnd.oma.drm.rights+xml") ||
                      mimetype.equals("application/vnd.oma.drm.rights+wbxml") ||
                      mimetype.equals("application/vnd.oma.drm.content"))  {
                return true;
            }else{
                return false;
            }
        }
        if (url != null){
            if ((url.endsWith(".dr") || url.endsWith(".dcf") || url.endsWith(".dm"))){
                return true;
            }else{
                return false;
            }
        }
        return false;
    }

    private String createDRMdownloadDirectory(String path){
        String dir = path + "/DrmDownload";
        File file = new File(dir);
        Log.i("FORDRM", "createDRMdownloadDirectory   dir  "+dir);
        if (file.exists()){
            if (file.isDirectory()){
                Log.i("FORDRM", "createDRMdownloadDirectory   file exitst isDirectory  ");
                return (dir + "/");
            }else{
                boolean create = file.mkdirs();

                if (create){
                    Log.i("FORDRM", "createDRMdownloadDirectory   file exitst create true ");
                    return (dir + "/");
                }else{
                    Log.i("FORDRM", "createDRMdownloadDirectory   file exitst return null  ");
                    return null;
                }
            }

        }else{
            boolean sucCreate = file.mkdirs();
            if (sucCreate){
                Log.i("FORDRM", "createDRMdownloadDirectory   file not exitst create true  ");
                return (dir +  "/");
            }else{
                Log.i("FORDRM", "createDRMdownloadDirectory   file not  exitst return null  ");
                return null;
            }
        }
    }
}
