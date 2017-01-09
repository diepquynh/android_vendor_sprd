
package com.sprd.voicetrigger;

import android.app.ActionBar;
import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.os.Bundle;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.view.MenuItem;

public class SupportActivity extends PreferenceActivity implements
        Preference.OnPreferenceClickListener {
    @SuppressWarnings("deprecation")
    @Override
    public void onCreate(Bundle bundle) {
        super.onCreate(bundle);
        addPreferencesFromResource(R.xml.voicetrigger_support);
        ActionBar actionBar = getActionBar();

        actionBar.setTitle(R.string.audio_voicetrigger_support_fun);
        actionBar.setDisplayHomeAsUpEnabled(true);
        findPreference("support_find_phone").setOnPreferenceClickListener(this);
        findPreference("support_call_phone").setOnPreferenceClickListener(this);
        findPreference("support_send_mms").setOnPreferenceClickListener(this);
        findPreference("support_camera").setOnPreferenceClickListener(this);
        findPreference("support_open_music").setOnPreferenceClickListener(this);
        findPreference("support_open_app").setOnPreferenceClickListener(this);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == android.R.id.home) {
            finish();
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    @SuppressWarnings("deprecation")
    @Override
    public boolean onPreferenceClick(Preference preference) {
        if (preference == findPreference("support_find_phone")) {
            dialog(R.string.support_find_phone, getString(R.string.find_phone_content));
        }
        if (preference == findPreference("support_call_phone")) {
            dialog(R.string.support_call_phone, getString(R.string.call_phone_content));
        }
        if (preference == findPreference("support_send_mms")) {
            dialog(R.string.support_send_mms, getString(R.string.send_mms_content));
        }
        if (preference == findPreference("support_camera")) {
            dialog(R.string.support_camera, getString(R.string.open_camera_content));
        }
        if (preference == findPreference("support_open_music")) {
            dialog(R.string.support_open_music, getString(R.string.open_music_content));
        }
        if (preference == findPreference("support_open_app")) {
            dialog(R.string.support_open_app, getString(R.string.open_app_content));
        }
        return true;
    }

    protected void dialog(int title, String message) {
        AlertDialog.Builder builder = new Builder(this);
        builder.setMessage(message);
        builder.setTitle(title);
        builder.setPositiveButton(android.R.string.ok, new OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                dialog.dismiss();
                // finish();
            }
        });
        builder.create().show();
    }
}
