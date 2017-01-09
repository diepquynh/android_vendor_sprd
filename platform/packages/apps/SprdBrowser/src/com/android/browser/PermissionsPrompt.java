/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.browser;

import android.content.Context;
import android.net.Uri;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.View;
import android.webkit.GeolocationPermissions;
import android.webkit.PermissionRequest;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;

import java.util.Enumeration;
import java.util.Vector;
import android.app.Activity;
import android.util.Log;
import android.Manifest;
import android.content.pm.PackageManager;

public class PermissionsPrompt extends RelativeLayout {
    private TextView mMessage;
    private Button mAllowButton;
    private Button mDenyButton;
    private CheckBox mRemember;
    private PermissionRequest mRequest;
    private Activity mActivity;

    public PermissionsPrompt(Context context) {
        this(context, null);
    }

    public PermissionsPrompt(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        init();
    }

    private void init() {
        mMessage = (TextView) findViewById(R.id.message);
        mAllowButton = (Button) findViewById(R.id.allow_button);
        mDenyButton = (Button) findViewById(R.id.deny_button);
        mRemember = (CheckBox) findViewById(R.id.remember);

        mAllowButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                handleButtonClick(true);
            }
        });
        mDenyButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                handleButtonClick(false);
            }
        });
    }

    public void show(PermissionRequest request, Activity activity) {
        mRequest = request;
        mActivity = activity;
        setMessage();
        mRemember.setChecked(true);
        mAllowButton.setEnabled(true);
        mDenyButton.setEnabled(true);
        setVisibility(View.VISIBLE);
    }

    public void setMessage() {
        String[] resources = mRequest.getResources();
        Vector<String> strings = new Vector<String>();
        for (String resource : resources) {
            if (resource.equals(PermissionRequest.RESOURCE_VIDEO_CAPTURE))
                strings.add(getResources().getString(R.string.resource_video_capture));
            else if (resource.equals(PermissionRequest.RESOURCE_AUDIO_CAPTURE))
                strings.add(getResources().getString(R.string.resource_audio_capture));
            else if (resource.equals(PermissionRequest.RESOURCE_PROTECTED_MEDIA_ID))
                strings.add(getResources().getString(R.string.resource_protected_media_id));
        }
        if (strings.isEmpty()) return;

        Enumeration<String> e = strings.elements();
        StringBuilder sb = new StringBuilder(e.nextElement());
        if (e.hasMoreElements()) {
            sb.append(", ");
            sb.append(e.nextElement());
        }

        mMessage.setText(String.format(
                getResources().getString(R.string.permissions_prompt_message),
                mRequest.getOrigin(), sb.toString()));
    }

    /**
     * Hides the prompt.
     */
    public void hide() {
        setVisibility(View.GONE);
        mAllowButton.setEnabled(false);
        mDenyButton.setEnabled(false);
    }

    /**
     * Handles a click on one the buttons by invoking the callback.
     */
    private void handleButtonClick(boolean allow) {
        hide();
        if (allow) {
            /*modify for camera and record permission*/
            if (mActivity != null) {
                int numPermissionsToRequest = 0;
                boolean requestMicrophonePermission = false;
                boolean requestCameraPermission = false;
                String[] resources = mRequest.getResources();
                for (String resource : resources) {
                    if (resource.equals(PermissionRequest.RESOURCE_VIDEO_CAPTURE))
                        if (!requestCameraPermission && mActivity.checkSelfPermission(Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
                            requestCameraPermission = true;
                            numPermissionsToRequest ++;
                        }
                    else if (resource.equals(PermissionRequest.RESOURCE_AUDIO_CAPTURE))
                        if (!requestMicrophonePermission && mActivity.checkSelfPermission(Manifest.permission.RECORD_AUDIO) != PackageManager.PERMISSION_GRANTED) {
                            requestMicrophonePermission = true;
                            numPermissionsToRequest ++;
                        }
                }
                String[] permissionsToRequest = new String[numPermissionsToRequest];
                int permissionsRequestIndex = 0;
                if (requestCameraPermission) {
                    Log.i("PermissionsPrompt", "no camera permission");
                    permissionsToRequest[permissionsRequestIndex] = Manifest.permission.CAMERA;
                    permissionsRequestIndex++;
                }
                if (requestMicrophonePermission) {
                    Log.i("PermissionsPrompt", "no record audio permission");
                    permissionsToRequest[permissionsRequestIndex] = Manifest.permission.RECORD_AUDIO;
                    permissionsRequestIndex++;
                }
                if (permissionsRequestIndex > 0) {
                    Log.i("PermissionsPrompt", "no permission, Controller.mPermissionObj = " + Controller.mPermissionObj);
                    if (Controller.mPermissionObj == null) {
                        Controller.mPermissionObj = mRequest;
                        Log.i("PermissionsPrompt", "requestPermissions");
                        mActivity.requestPermissions(
                                permissionsToRequest,
                                Controller.PERMISSIONS_REQUEST_MEDIA);
                    }
                } else {
                    mRequest.grant(mRequest.getResources());
                }
            } else {
                mRequest.grant(mRequest.getResources());
            }
        }
        else
            mRequest.deny();
    }
}
