/*
 * Copyright (C) 2013 The Android Open Source Project
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
package com.sprd.gallery3d.app;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;

import com.android.gallery3d.app.MovieActivity;

/** Trampoline activity that launches the Gallery activity defined in IntentHelper. */
public class Video extends Activity {
    private static final String FLAG_GALLERY="startByGallery";
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Intent in = getIntent();
        Intent intent = new Intent(Video.this , MovieActivity.class);
        // Since this is being launched from a homescreen shortcut,
        // it's already in a new task. Start Movie activity and
        // reset the task to its initial state if needed.
//        intent.setFlags(Intent.FLAG_ACTIVITY_RESET_TASK_IF_NEEDED);
        intent.setData(in.getData());
        intent.putExtra(FLAG_GALLERY, in.getBooleanExtra(FLAG_GALLERY, false));
        intent.putExtra("currentstate", in.getIntExtra("currentstate",0));
        intent.putExtra("position", in.getIntExtra("position", 0));
        intent.putExtra("clearDialog", true);
        intent.putExtra("isConsumed", in.getBooleanExtra("isConsumed", false));
        intent.putExtra(Intent.EXTRA_TITLE, intent.getStringExtra(Intent.EXTRA_TITLE));
        intent.putExtras(in);
        intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
        intent.addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
        startActivity(intent);
        finish();
    }
}
