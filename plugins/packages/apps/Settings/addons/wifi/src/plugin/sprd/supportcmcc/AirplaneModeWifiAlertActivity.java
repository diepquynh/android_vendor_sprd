package plugin.sprd.supportcmcc;

import android.os.Bundle;
import android.provider.Settings;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;

import com.android.internal.app.AlertActivity;
import com.android.internal.app.AlertController;

public class AirplaneModeWifiAlertActivity extends AlertActivity implements
        android.view.View.OnClickListener {

    private CheckBox checkBox;
    private Button yesButton;
    private Button noButton;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        final AlertController.AlertParams p = mAlertParams;
        p.mTitle = getString(R.string.airplane_mode_wifi_notification);
        p.mMessage = getString(R.string.airplane_mode_wifi_notification_message);
        p.mView = getLayoutInflater().inflate(R.layout.airplane_wifi_alert, null, false);
        setupAlert();

        checkBox = (CheckBox) findViewById(R.id.no_ask);
        checkBox.setOnClickListener(this);
        yesButton = (Button) findViewById(R.id.yes);
        yesButton.setOnClickListener(this);
        // SPRD : Delete the Button of no
        //noButton = (Button) findViewById(R.id.no);
        //noButton.setOnClickListener(this);
    }

    @Override
    public void onClick(View v) {
        // TODO Auto-generated method stub
        if (v == yesButton) {
            if (checkBox.isChecked()) {
                Settings.Global.putInt(getContentResolver(),
                        WifiConnectionPolicy.AIRPLANE_MODE_WIFI_NOTIFICATION_FLAG, 1);
            }
            finish();
        } else if (v == noButton) {
            finish();
        }
    }
}
