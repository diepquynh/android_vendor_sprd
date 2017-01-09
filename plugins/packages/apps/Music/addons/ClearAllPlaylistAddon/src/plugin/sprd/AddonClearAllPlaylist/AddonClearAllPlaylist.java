package plugin.sprd.AddonClearAllPlaylist;

import android.app.AddonManager;
import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.app.Dialog;
import android.app.DialogFragment;
import android.app.DialogFragment;
import android.app.FragmentManager;
import android.content.Context;
import android.content.DialogInterface;
import android.database.Cursor;
import android.os.Bundle;
import android.provider.MediaStore;
import android.util.Log;

import com.android.music.MusicUtils;
import com.sprd.music.plugin.*;

public class AddonClearAllPlaylist extends ClearAllPlaylistPlugin implements AddonManager.InitialCallback{
    private static final String LOGTAG = "AddonClearAllPlaylist";
    private Context mAddonContext;
    // SPRD 573064
    private static ClearAllDialog dialogFragment;
    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        Log.e(LOGTAG, "onCreateAddon");
        mAddonContext = context;
        return clazz;
    }

    @Override
    public boolean hasPlaylist(Context context) {
        Cursor c  = null;
        try {
            c = MusicUtils.query(context, MediaStore.Audio.Playlists.EXTERNAL_CONTENT_URI,
                    new String[] {
                        MediaStore.Audio.Playlists._ID
                    }, null, null, null);
            if( c!= null) {
                int count = c.getCount();
                return count > 0;
            }
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            if(c != null) {
                c.close();
            }
        }
        return false;
    }

    @Override
    public void showClearAllDialog(Context context, FragmentManager manager, boolean isresumed) {
        if (isresumed) {
            // SPRD 573064
            dialogFragment = new ClearAllDialog(context);
            dialogFragment.show(manager, "null");
        }
    }
    /* SPRD 573064 @{ */
    @Override
    public void dismissClearAllDialog(){
        if(dialogFragment != null){
            dialogFragment.dismissAllowingStateLoss();
        }
    }
    /* @} */
    public class ClearAllDialog extends DialogFragment implements DialogInterface.OnClickListener {
        Context mContext;
        public ClearAllDialog(Context context){
            mContext = context;
        }
        @Override
        public Dialog onCreateDialog(Bundle savedInstanceState) {
            Builder builder = new AlertDialog.Builder(mContext);
            builder.setTitle(android.R.string.dialog_alert_title);
            builder.setIcon(android.R.drawable.ic_dialog_alert);
            builder.setMessage(mAddonContext.getString(R.string.delete_all_playlist));
            builder.setPositiveButton(android.R.string.ok, this);
            builder.setNegativeButton(android.R.string.cancel, this);
            return builder.create();
        }

        public void onClick(DialogInterface dialog, int which) {
            switch (which) {
                case DialogInterface.BUTTON_POSITIVE:
                    try {
                        mContext.getContentResolver().delete(
                            MediaStore.Audio.Playlists.EXTERNAL_CONTENT_URI,
                            null, null);
                    } catch (Exception e) {
                        Log.e(LOGTAG, "Delete all play list error");
                    }
                    break;
                default:
                    break;
            }
        }
    }
}
