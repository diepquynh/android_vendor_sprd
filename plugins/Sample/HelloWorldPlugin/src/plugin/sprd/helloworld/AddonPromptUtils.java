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

import test.sprd.helloworld.plugin.PromptUtils;

public class AddonPromptUtils extends PromptUtils implements AddonManager.InitialCallback {
    private Context mAddonContext;

    public AddonPromptUtils() {
    }

    /**
     * Create the Addon Object of this class Or Redirect it to the Real AddonClass.
     *
     * @param context The Context of Plugin apk.
     * @param clazz The Class Object of this Class(eg.AddonPromptUtils).
     * @return keep the default or Redirect the Addon Class.
     */
    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    @Override
    public void showPrompt(Context context) {
        //Use the proper context to get the right resource
        context.getString(test.sprd.helloworld.R.string.str_from_host);
        Toast.makeText(context, mAddonContext.getString(R.string.toast_message), Toast.LENGTH_LONG).show();
    }
}
