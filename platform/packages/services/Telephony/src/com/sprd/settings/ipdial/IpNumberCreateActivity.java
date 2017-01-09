package com.sprd.phone.settings.ipdial;

import com.android.phone.R;

import android.widget.Toast;
import android.content.Context;
import android.content.Intent;
import android.app.ActionBar;
import android.app.Activity;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.Menu;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;

/***********************************************************
 *              SPRD: FEATURE_IP_DIAL
 * *********************************************************/
public class IpNumberCreateActivity extends Activity {

    private EditText mEditText;
    private final static int RESAULT_OK = 0;
    private final static int RESAULT_ERROR = -1;
    protected static final int MENU_OK = Menu.FIRST;
    protected static final int MENU_CANCLE = Menu.FIRST + 1;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        final ActionBar actionBar = getActionBar();
        if(actionBar != null){
            actionBar.setHomeAsUpIndicator(R.drawable.ic_back_arrow);
            actionBar.setDisplayHomeAsUpEnabled(true);
        }
        setContentView(R.layout.ip_dialing_edit_activity_ex);
        mEditText = (EditText) findViewById(R.id.ip_editor);
        mEditText.addTextChangedListener(new TextWatcher() {

            @Override
            public void afterTextChanged(Editable s) {
                String ipNumber = mEditText.getText().toString();
                if (ipNumber != null && ipNumber.length() == 20) {
                    Toast toast = Toast.makeText(IpNumberCreateActivity.this,
                            R.string.ip_number_is_too_long, Toast.LENGTH_SHORT);
                    toast.setGravity(Gravity.CENTER, 0, 0);
                    toast.show();
                }
                IpNumberCreateActivity.this.invalidateOptionsMenu();
            }

            @Override
            public void beforeTextChanged(CharSequence s, int start, int count,
                    int after) {

            }

            @Override
            public void onTextChanged(CharSequence s, int start, int before,
                    int count) {

            }

        });
    }

    @Override
    protected void onResume() {
        super.onResume();
        mEditText.requestFocus();
    }

    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        super.onCreateOptionsMenu(menu);
        menu.clear();
        if (mEditText != null) {
            String ipNumber = mEditText.getText().toString();
            if (ipNumber != null && !ipNumber.isEmpty()) {
                menu.add(0, MENU_OK, 0, R.string.doneButton).setShowAsAction(
                        MenuItem.SHOW_AS_ACTION_ALWAYS);
            }
        }
        menu.add(0, MENU_CANCLE, 0, R.string.cancel).setShowAsAction(
                MenuItem.SHOW_AS_ACTION_ALWAYS);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home:
                finish();
                return true;
            case MENU_OK:
                Intent intent = new Intent();
                String text = mEditText.getText().toString();
                intent.putExtra("ipNumber", text);
                setResult(RESAULT_OK, intent);
                finish();
                break;
            case MENU_CANCLE:
                setResult(RESAULT_ERROR);
                finish();
                break;
        }
        return super.onOptionsItemSelected(item);
    }
}
