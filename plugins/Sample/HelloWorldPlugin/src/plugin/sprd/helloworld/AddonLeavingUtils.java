package plugin.sprd.helloworld;

import android.app.Activity;
import android.app.AddonManager;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.graphics.Color;
import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;
import android.widget.Toast;

import test.sprd.helloworld.plugin.LeavingUtils;

public class AddonLeavingUtils extends LeavingUtils implements AddonManager.InitialCallback {
    private Context mAddonContext;
    private AlertDialog mLeavingDialog;

    public AddonLeavingUtils() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    @Override
    public boolean canIGo(final Activity activity) {
        if (mLeavingDialog == null) {
            mLeavingDialog = new AlertDialog.Builder(activity)
                    .setTitle(mAddonContext.getString(R.string.leaving_dialog_title))
                    .setMessage(mAddonContext.getString(R.string.leaving_dialog_msg))
                    .setPositiveButton(mAddonContext.getString(R.string.exit_btn_ok),
                            new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                activity.finish();
                            }
                    }
                    )
                    .create();

        }
        mLeavingDialog.show();
        return false;
    }

}
