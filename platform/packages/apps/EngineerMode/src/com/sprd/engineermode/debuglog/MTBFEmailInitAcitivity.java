
package com.sprd.engineermode.debuglog;

import java.io.File;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import android.util.Log;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.net.Uri;
import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.PreferenceActivity;
import android.preference.PreferenceGroup;
import android.preference.PreferenceScreen;
import android.preference.TwoStatePreference;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.EditText;
import android.view.View.OnClickListener;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.CheckBox;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.Spinner;
import android.app.Activity;

import com.sprd.engineermode.R;
import com.sprd.engineermode.R.xml;

public class MTBFEmailInitAcitivity extends Activity {
    private static final String TAG = "MTBFEmailInitAcitivity";
    private static final int POSITION_SIMTYPE_CMCC = 0;
    private static final int POSITION_SIMTYPE_CUCC = 1;

    private Spinner mSpSimType = null;
    private TextView mTvEmailAddSuffix = null;
    private EditText mEtEmailAdd = null;
    private EditText mEtEmailPsw = null;

    private void initDefaultEmailInfo() {
        // The apps which shoud be checked
        Intent intent = getIntent();
        Bundle bundle = intent.getExtras();
        if(bundle == null){
            return;
        }
        int emailSimType = bundle.getInt(MTBFActivity.EMAIL_SIMTYPE);
        String emailAddrs = bundle.getString(MTBFActivity.EMAIL_ADDRS);
        String emailPsw = bundle.getString(MTBFActivity.EMAIL_PSW);

        if (emailSimType == MTBFActivity.EMAIL_SIMTYPE_CUCC) {
            mSpSimType.setSelection(POSITION_SIMTYPE_CUCC);
        } else {
            mSpSimType.setSelection(POSITION_SIMTYPE_CMCC);
        }

        if (emailAddrs != null) {
            String[] email = emailAddrs.split("@");
            mEtEmailAdd.setText(email[0]);
        }

        if (emailPsw != null) {
            mEtEmailPsw.setText(emailPsw);
        }
    }

    @Override
    public void onBackPressed() {
        Intent intent = getIntent();
        Bundle bundle = new Bundle();
        String emailSUffix = mTvEmailAddSuffix.getText().toString();

        if (0 == MTBFActivity.CUCC_ADDRESS_SUFFIX.compareTo(emailSUffix)) {
            bundle.putInt(MTBFActivity.EMAIL_SIMTYPE, MTBFActivity.EMAIL_SIMTYPE_CUCC);
        } else {
            bundle.putInt(MTBFActivity.EMAIL_SIMTYPE, MTBFActivity.EMAIL_SIMTYPE_CMCC);
        }
        if (mEtEmailAdd.getText().toString().length() != 0) {
            bundle.putString(MTBFActivity.EMAIL_ADDRS, mEtEmailAdd.getText().toString()
                    + emailSUffix);
        } else {
            bundle.putString(MTBFActivity.EMAIL_ADDRS, mEtEmailAdd.getText().toString());
        }
        bundle.putString(MTBFActivity.EMAIL_PSW, mEtEmailPsw.getText().toString());
        intent.putExtras(bundle);
        this.setResult(0, intent);

        finish();
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.mtbf_email_init);

        mEtEmailPsw = (EditText) findViewById(R.id.et_email_psw);
        mEtEmailAdd = (EditText) findViewById(R.id.et_email_addrs);
        mTvEmailAddSuffix = (TextView) findViewById(R.id.tv_email_addrs_suffix);
        mSpSimType = (Spinner) findViewById(R.id.sp_email_simtype);
        mSpSimType.setOnItemSelectedListener(new Spinner.OnItemSelectedListener() {

            @Override
            public void onItemSelected(AdapterView<?> arg0, View arg1, int position, long arg3) {
                if (position == POSITION_SIMTYPE_CMCC) {
                    mTvEmailAddSuffix.setText(MTBFActivity.CMCC_ADDRESS_SUFFIX);
                } else if (position == POSITION_SIMTYPE_CUCC) {
                    mTvEmailAddSuffix.setText(MTBFActivity.CUCC_ADDRESS_SUFFIX);
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> arg0) {
            }
        });

        initDefaultEmailInfo();
    }
}
