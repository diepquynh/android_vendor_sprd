package com.android.camera;

import android.Manifest;
import android.app.Activity;
import android.app.Dialog;
import android.app.AlertDialog;
import android.app.KeyguardManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.provider.MediaStore;
import android.view.KeyEvent;
import android.view.Window;
import android.view.WindowManager;

import com.android.camera.app.CameraServicesImpl;
import com.android.camera.debug.Log;
import com.android.camera.settings.Keys;
import com.android.camera.settings.SettingsManager;
import com.android.camera.util.CameraUtil;
import com.android.camera.util.QuickActivity;
import com.android.camera2.R;
import com.dream.camera.settings.DataModuleManager;

/**
 * Activity that shows permissions request dialogs and handles lack of critical permissions.
 */
public class PermissionsActivity extends QuickActivity {
    private static final Log.Tag TAG = new Log.Tag("PermissionsActivity");

    private static int PERMISSION_REQUEST_CODE = 1;
    private static int RESULT_CODE_OK = 1;
    private static int RESULT_CODE_FAILED = 2;

    private int mIndexPermissionRequestCamera;
    private int mIndexPermissionRequestMicrophone;
    private int mIndexPermissionRequestLocation;
    private int mIndexPermissionRequestStorage;
    private boolean mShouldRequestCameraPermission;
    private boolean mShouldRequestMicrophonePermission;
    private boolean mShouldRequestLocationPermission;
    private boolean mShouldRequestStoragePermission;
    private int mNumPermissionsToRequest;
    private boolean mFlagHasCameraPermission;
    private boolean mFlagHasMicrophonePermission;
    private boolean mFlagHasStoragePermission;
    private SettingsManager mSettingsManager;
    private String cameraIntentAction;

    /**
     * Close activity when secure app passes lock screen or screen turns
     * off.
     */
    private final BroadcastReceiver mShutdownReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
          Log.v(TAG, "received intent, finishing: " + intent.getAction());
          finish();
        }
    };

    @Override
    protected void onCreateTasks(Bundle savedInstanceState) {
        setContentView(R.layout.permissions);
        mSettingsManager = CameraServicesImpl.instance().getSettingsManager();

        // Filter for screen off so that we can finish permissions activity
        // when screen is off.
        IntentFilter filter_screen_off = new IntentFilter(Intent.ACTION_SCREEN_OFF);
        registerReceiver(mShutdownReceiver, filter_screen_off);

        // Filter for phone unlock so that we can finish permissions activity
        // via this UI path:
        //    1. from secure lock screen, user starts secure camera
        //    2. user presses home button
        //    3. user unlocks phone
        IntentFilter filter_user_unlock = new IntentFilter(Intent.ACTION_USER_PRESENT);
        registerReceiver(mShutdownReceiver, filter_user_unlock);

        Window win = getWindow();
        if (isKeyguardLocked()) {
            win.addFlags(WindowManager.LayoutParams.FLAG_SHOW_WHEN_LOCKED);
        } else {
            win.clearFlags(WindowManager.LayoutParams.FLAG_SHOW_WHEN_LOCKED);
        }
        //SPRD:fix bug501493,502701,498680,499555 problem caused by permission strategy
        Intent cameraIntent = (Intent)getIntent().getParcelableExtra("cameraIntent");
        cameraIntentAction = cameraIntent.getAction();

        // SPRD Bug:508576 The dialog is created repeatly when rotating the screen.
        mNumPermissionsToRequest = 0;
        checkPermissions();
    }

    @Override
    protected void onResumeTasks() {
        /*
         * SPRD Bug:508576 The dialog is created repeatly when rotating the screen. @{
         * Original Android code:

        mNumPermissionsToRequest = 0;
        checkPermissions();

         */
        /* @} */
    }

    @Override
    protected void onDestroyTasks() {
        Log.v(TAG, "onDestroy: unregistering receivers");
        unregisterReceiver(mShutdownReceiver);
    }

    private void checkPermissions() {
        if (checkSelfPermission(Manifest.permission.CAMERA)
                != PackageManager.PERMISSION_GRANTED) {
            mNumPermissionsToRequest++;
            mShouldRequestCameraPermission = true;
        } else {
            mFlagHasCameraPermission = true;
        }

        if (checkSelfPermission(Manifest.permission.RECORD_AUDIO)
                != PackageManager.PERMISSION_GRANTED) {
            mNumPermissionsToRequest++;
            mShouldRequestMicrophonePermission = true;
        } else {
            mFlagHasMicrophonePermission = true;
        }

        if (checkSelfPermission(Manifest.permission.READ_EXTERNAL_STORAGE)
                != PackageManager.PERMISSION_GRANTED) {
            mNumPermissionsToRequest++;
            mShouldRequestStoragePermission = true;
        } else {
            mFlagHasStoragePermission = true;
        }

        // SPRD: Fix bug 572309 camera GPS function
        if (CameraUtil.isRecordLocationEnable() && checkSelfPermission(Manifest.permission.ACCESS_COARSE_LOCATION)
                != PackageManager.PERMISSION_GRANTED) {
            mNumPermissionsToRequest++;
            mShouldRequestLocationPermission = true;
        }

        if (mNumPermissionsToRequest != 0) {
            /* SPRD:fix bug519999 Create header photo, pop permission error
             * original code
             * if (!isKeyguardLocked() && !mSettingsManager.getBoolean(SettingsManager.SCOPE_GLOBAL,
             *      Keys.KEY_HAS_SEEN_PERMISSIONS_DIALOGS)) {
             */
            if (!isKeyguardLocked()) {
                buildPermissionsRequest();
            } else {
                // Permissions dialog has already been shown, or we're on
                // lockscreen, and we're still missing permissions.
                handlePermissionsFailure();
            }
        } else {
            handlePermissionsSuccess();
        }
    }

    private void buildPermissionsRequest() {
        String[] permissionsToRequest = new String[mNumPermissionsToRequest];
        int permissionsRequestIndex = 0;

        if (mShouldRequestCameraPermission) {
            permissionsToRequest[permissionsRequestIndex] = Manifest.permission.CAMERA;
            mIndexPermissionRequestCamera = permissionsRequestIndex;
            permissionsRequestIndex++;
        }
        if (mShouldRequestMicrophonePermission) {
            permissionsToRequest[permissionsRequestIndex] = Manifest.permission.RECORD_AUDIO;
            mIndexPermissionRequestMicrophone = permissionsRequestIndex;
            permissionsRequestIndex++;
        }
        if (mShouldRequestStoragePermission) {
            permissionsToRequest[permissionsRequestIndex] = Manifest.permission.READ_EXTERNAL_STORAGE;
            mIndexPermissionRequestStorage = permissionsRequestIndex;
            permissionsRequestIndex++;
        }
        if (mShouldRequestLocationPermission) {
            permissionsToRequest[permissionsRequestIndex] = Manifest.permission.ACCESS_COARSE_LOCATION;
            mIndexPermissionRequestLocation = permissionsRequestIndex;
        }

        Log.v(TAG, "requestPermissions count: " + permissionsToRequest.length);
        requestPermissions(permissionsToRequest, PERMISSION_REQUEST_CODE);
    }

    @Override
    public void onRequestPermissionsResult(int requestCode,
                                           String permissions[], int[] grantResults) {
        Log.v(TAG, "onPermissionsResult counts: " + permissions.length + ":" + grantResults.length);
        DataModuleManager.getInstance(this).getDataModuleCamera().set(Keys.KEY_HAS_SEEN_PERMISSIONS_DIALOGS,
                true);

        if (mShouldRequestCameraPermission) {
            if (grantResults.length > 0 && grantResults[mIndexPermissionRequestCamera] ==
                    PackageManager.PERMISSION_GRANTED) {
                mFlagHasCameraPermission = true;
            } else {
                handlePermissionsFailure();
            }
        }
        if (mShouldRequestMicrophonePermission) {
            if (grantResults.length > 0 && grantResults[mIndexPermissionRequestMicrophone] ==
                    PackageManager.PERMISSION_GRANTED) {
                mFlagHasMicrophonePermission = true;
            } else {
                handlePermissionsFailure();
            }
        }
        if (mShouldRequestStoragePermission) {
            if (grantResults.length > 0 && grantResults[mIndexPermissionRequestStorage] ==
                    PackageManager.PERMISSION_GRANTED) {
                mFlagHasStoragePermission = true;
            } else {
                handlePermissionsFailure();
            }
        }

        if (mShouldRequestLocationPermission) {
            if (grantResults.length > 0 && grantResults[mIndexPermissionRequestLocation] ==
                    PackageManager.PERMISSION_GRANTED) {
                // Do nothing
            } else {
                // Do nothing
            }
        }

        if (mFlagHasCameraPermission && mFlagHasMicrophonePermission && mFlagHasStoragePermission) {
            handlePermissionsSuccess();
        }
    }

    private void handlePermissionsSuccess() {
        //SPRD:fix bug519999 Create header photo, pop permission error
        if (!isCaptureIntent()) {
            Intent intent = new Intent(this, CameraActivity.class);
            Bundle bundle = new Bundle();
            bundle.putString("result", CameraActivity.DAFAULT_PERMISSION_OK);
            intent.putExtras(bundle);
            intent.setAction(cameraIntentAction); // SPRD: Fix bug 589908
            startActivity(intent);
        } else {
            setResult(RESULT_OK);
        }
        finish();
    }

    private void handlePermissionsFailure() {
        new AlertDialog.Builder(this).setTitle(getResources().getString(R.string.camera_error_title))
                .setMessage(getResources().getString(R.string.error_permissions))
                .setCancelable(false)
                .setOnKeyListener(new Dialog.OnKeyListener() {
                    @Override
                    public boolean onKey(DialogInterface dialog, int keyCode, KeyEvent event) {
                        if (keyCode == KeyEvent.KEYCODE_BACK) {
                            finish();
                        }
                        return true;
                    }
                })
                .setPositiveButton(getResources().getString(R.string.dialog_dismiss),
                        new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        if (isCaptureIntent()) {
                            setResult(RESULT_CANCELED);
                        }
                        finish();
                    }
                })
                .show();
    }

    //SPRD:fix bug501493,502701,498680,499555 problem caused by permission strategy
    private boolean isCaptureIntent() {
        if (MediaStore.INTENT_ACTION_VIDEO_CAMERA.equals(cameraIntentAction)
                || MediaStore.ACTION_VIDEO_CAPTURE.equals(cameraIntentAction)
                || MediaStore.ACTION_IMAGE_CAPTURE.equals(cameraIntentAction)
                || MediaStore.ACTION_IMAGE_CAPTURE_SECURE.equals(cameraIntentAction)) {
            return true;
        } else {
            return false;
        }
    }
}
