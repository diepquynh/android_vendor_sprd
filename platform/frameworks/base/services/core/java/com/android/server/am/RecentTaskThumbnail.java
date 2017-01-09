/*
 * Copyright (C) 2006-2008 The Android Open Source Project
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
package com.android.server.am;

import static com.android.server.am.ActivityRecord.HOME_ACTIVITY_TYPE;
import static com.android.server.am.ActivityRecord.APPLICATION_ACTIVITY_TYPE;
import static com.android.server.am.ActivityRecord.RECENTS_ACTIVITY_TYPE;

import android.app.Activity;
import android.app.ActivityManager;
import android.app.ActivityManager.TaskThumbnail;
import android.app.ActivityManager.TaskDescription;
import android.app.ActivityOptions;
import android.app.AppGlobals;
import android.content.ComponentName;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.ApplicationInfo;
import android.content.pm.IPackageManager;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.os.ParcelFileDescriptor;
import android.os.RemoteException;
import android.os.UserHandle;
import android.os.Environment;
import android.service.voice.IVoiceInteractionSession;
import android.util.Slog;
import android.util.ArrayMap;
import android.util.ArraySet;
import android.util.AtomicFile;
import android.util.Slog;
import android.util.SparseArray;
import android.util.Xml;

import com.android.internal.app.IVoiceInteractor;
import com.android.internal.util.XmlUtils;
import com.android.server.am.ActivityStack.ActivityState;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;
import org.xmlpull.v1.XmlSerializer;

import java.io.File;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.ArrayList;

import com.android.internal.util.FastXmlSerializer;
import com.android.internal.util.XmlUtils;

import java.io.BufferedReader;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.StringWriter;


import libcore.io.IoUtils;

/*
*@hide
*/
class RecentTaskThumbnail {
    private ActivityManagerService mService;
    final String mPkgName;
    Bitmap mLastThumbnail;
    File mLastThumbnailFile;
    String mFilename;
    Intent mIntent;
    String mClassName;
    String mXMLFileName;
    final int mUserId;
    static String TAG = "ActivityManager";
    private static final String TAG_TASK = "recenttask";
    private static final String TAG_INTENT = "intent";
    private static final String ATTR_PKGNAME = "pkgname";
    private static final String ATTR_CLSNAME = "classname";
    public static final String THUMBNAIL_SUFFIX = "recent_thumbnail_";
    public static final String THUMBNAIL_XML_ENDWITH = ".xml";
    //public static final String THUMBNAIL_DIR = "/data/system/recent_images";
    public static final int RECENT_THUMBNAIL_DELAY = 400;// in ms TODO? we should tune for 512M
    //public static final String IMAGES_DIRNAME = "recent_images";

    RecentTaskThumbnail(ActivityManagerService service, String pkgName, Intent intent, int userId) {
        mService = service;
        mPkgName = pkgName;
        mUserId = userId;
        mIntent = new Intent(intent);
        mClassName = intent.getComponent().getClassName();
        String tmp = mClassName.replace("$", "@");
        mFilename = THUMBNAIL_SUFFIX + tmp + TaskPersister.IMAGE_EXTENSION;
        mXMLFileName = THUMBNAIL_SUFFIX + tmp + THUMBNAIL_XML_ENDWITH;
        mLastThumbnailFile = new File(TaskPersister.getUserImagesDir(userId), mFilename);
    }

    void getLastThumbnail(TaskThumbnail thumbs) {
        thumbs.mainThumbnail = mLastThumbnail;
        if(mLastThumbnail != null && mLastThumbnail.isRecycled()) {
            Slog.w(TAG,"getLastThumbnail--->mLastThumbnail recycled...reset to null ");
            mLastThumbnail = null;
         }
        thumbs.thumbnailFileDescriptor = null;
        if (mLastThumbnail == null) {
            thumbs.mainThumbnail = mService.mRecentTasks.mTaskPersister.getImageFromWriteQueue(mFilename);
        }
        // Only load the thumbnail file if we don't have a thumbnail
        if (thumbs.mainThumbnail == null && mLastThumbnailFile.exists()) {
            try {
                thumbs.thumbnailFileDescriptor = ParcelFileDescriptor.open(mLastThumbnailFile,
                        ParcelFileDescriptor.MODE_READ_ONLY);
                Slog.d(TAG, "getLastThumbnail thumbnailFileDescriptor = " + thumbs.thumbnailFileDescriptor);
            } catch (IOException e) {
            }
        }
    }

    void setLastThumbnail(Bitmap thumbnail) {
        Slog.d(TAG, "setLastThumbnail thumbnail = " + thumbnail + " mFilename = " + mFilename);
        mLastThumbnail = thumbnail;
        if (thumbnail == null) {
            if (mLastThumbnailFile != null) {
                mLastThumbnailFile.delete();
            }
        } else {
            mService.mRecentTasks.mTaskPersister.saveImage(thumbnail, mLastThumbnailFile.getAbsolutePath());
        }
    }

    public TaskThumbnail getTaskThumbnailLocked() {
        final TaskThumbnail taskThumbnail = new TaskThumbnail();
        getLastThumbnail(taskThumbnail);
        Slog.d(TAG, "getTaskThumbnailLocked Thumbnail" + taskThumbnail.mainThumbnail);
        return taskThumbnail;
    }

    @Override
    public String toString() {
        return "RecentTaskThumbnail:[" + mIntent.getComponent() + "]" + "[" + mLastThumbnail + "]";
    }

    boolean matchRecord(Intent intent) {
        if (intent != null && intent.getComponent() != null) {
            if (mClassName.equals(intent.getComponent().getClassName())
                    && mPkgName.equals(intent.getComponent().getPackageName())) {
                return intent.hasCategory(Intent.CATEGORY_LAUNCHER);
            }
        }
        return false;

    }

    static RecentTaskThumbnail restoreFromXML(File file, ActivityManagerService service, int userId) {
        BufferedReader reader = null;
        boolean deleteFile = false;
        Intent intent = null;
        String className = null;
        String pkgName = null;
        try {
            reader = new BufferedReader(new FileReader(file));
            final XmlPullParser in = Xml.newPullParser();
            in.setInput(reader);
            int event;
            while (((event = in.next()) != XmlPullParser.END_DOCUMENT) &&
                    event != XmlPullParser.END_TAG) {
                final String name = in.getName();
                if (event == XmlPullParser.START_TAG) {
                    if (TAG_TASK.equals(name)) {
                        for (int attrNdx = in.getAttributeCount() - 1; attrNdx >= 0; --attrNdx) {
                            final String attrName = in.getAttributeName(attrNdx);
                            final String attrValue = in.getAttributeValue(attrNdx);
                            if (ATTR_PKGNAME.equals(attrName)) {
                                pkgName = attrValue;
                            } else if (ATTR_CLSNAME.equals(attrName)) {
                                className = attrValue;
                            }
                        }
                    } else if (TAG_INTENT.equals(name)) {
                        intent = Intent.restoreFromXml(in);
                    } else {
                        Slog.wtf(TAG, "restoreTasksLocked Unknown xml event=" + event +
                                " name=" + name);
                    }
                }
                //XmlUtils.skipCurrentTag(in);
            }
        } catch (Exception e) {
            Slog.wtf(TAG, "Unable to parse " + file + ". Error ", e);
            deleteFile = true;
        } finally {
            IoUtils.closeQuietly(reader);
            if (deleteFile) {
                Slog.d(TAG, "Deleting file=" + file.getName());
                file.delete();
            }
            if (intent != null && pkgName != null && className != null) {
                Slog.d(TAG, "restore----->" + pkgName + "---->intent:" + intent);
                RecentTaskThumbnail task = new RecentTaskThumbnail(service, pkgName, intent, userId);
                return task;
            }
            return null;
        }
    }

    void tryToSave() {
        Slog.e(TAG, "try to save---->" + this);
        StringWriter writer = null;
        try {
            writer = saveToXml();
        } catch (IOException e) {
            e.printStackTrace();
        } catch (XmlPullParserException e) {
            e.printStackTrace();
        }
        if (writer != null) {
            FileOutputStream file = null;
            AtomicFile atomicFile = null;
            try {
                atomicFile = new AtomicFile(new File(TaskPersister.getUserImagesDir(mUserId).getAbsolutePath(), mXMLFileName));
                file = atomicFile.startWrite();
                file.write(writer.toString().getBytes());
                file.write('\n');
                atomicFile.finishWrite(file);
            } catch (IOException e) {
                if (file != null) {
                    atomicFile.failWrite(file);
                }

            }

        }

    }

    private StringWriter saveToXml() throws IOException, XmlPullParserException {
        final XmlSerializer xmlSerializer = new FastXmlSerializer();
        StringWriter stringWriter = new StringWriter();
        xmlSerializer.setOutput(stringWriter);
        // save task
        xmlSerializer.startDocument(null, true);
        xmlSerializer.startTag(null, TAG_TASK);
        xmlSerializer.attribute(null, ATTR_PKGNAME, String.valueOf(mPkgName));
        xmlSerializer.attribute(null, ATTR_CLSNAME, String.valueOf(mClassName));
        xmlSerializer.startTag(null, TAG_INTENT);
        mIntent.saveToXml(xmlSerializer);
        xmlSerializer.endTag(null, TAG_INTENT);
        xmlSerializer.endTag(null, TAG_TASK);
        xmlSerializer.endDocument();
        xmlSerializer.flush();
        return stringWriter;
    }
}
