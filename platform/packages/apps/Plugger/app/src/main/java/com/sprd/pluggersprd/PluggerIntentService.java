/*
 * Copyright (C) 2012 The Android Open Source Project
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

package com.sprd.pluggersprd;

import android.app.IntentService;
import android.content.ContentResolver;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.provider.MediaStore.Audio;
import android.provider.MediaStore.Images;
import android.telephony.SmsManager;
import android.util.Log;
import android.widget.Toast;

import com.android.dom.smil.parser.SmilXmlSerializer;
import com.android.sms.MmsFailureException;
import com.android.sms.MmsSender;
import com.android.util.ContentType;
import com.android.util.MmsUtils;
import com.android.util.ResizeImageResultCallback;
import com.google.android.mms.pdu.EncodedStringValue;
import com.google.android.mms.pdu.PduBody;
import com.google.android.mms.pdu.PduComposer;
import com.google.android.mms.pdu.PduPart;
import com.google.android.mms.pdu.SendReq;
import com.sprd.pluggersprd.datamodel.SmilHelper;

import org.w3c.dom.smil.SMILDocument;

import java.io.ByteArrayOutputStream;
import com.android.util.ImageUtils;
//import com.android.internal.util.HexDump;
//import com.google.android.mms.pdu;
//import com.sprd.mms.util.SprdSubscriptionManager;
//import com.android.sms.MmsUtils.ResizeImageResultCallback;
//import com.android.util.MmsUtils.ResizeImageResultCallback;


/**
 * Service that gets started by the PluggerReceiver when plugger start
 */
public class PluggerIntentService extends IntentService {
    private static final String TAG = "PluggerService";
    public static final int MMS_REQUEST_MANUAL_RETRY = 2;
    private static final String mImageUri = Images.Media.getContentUri("external").toString();
    private Object mAttachmentLock = new Object();
    private Intent mIntent = null;
    SendReq sendReqNoSave = null;
    public byte[] sendPdu;
    private static final int START_ADD_PLUGGER = 100;
    private static final int TOAST_SHARE_FAIL = 101;
    private static final int TOAST_NOSERVICE = 102;
    private static final int TOAST_SHARE_ERR_OOM = 103;

    private Uri mUri = null;
    private String mMimeType = null;
    private int mSubId = -1;
    private Message mMessage = new Message();
    private final String[] mPluggerToDestNumber = new String[]{"13924626367"};

    public PluggerIntentService() {
        // Class name will be the thread name.
        super(PluggerIntentService.class.getName());
        // Intent should be redelivered if the process gets killed before completing the job.
        setIntentRedelivery(true);
    }

    @Override
    protected void onHandleIntent(Intent intent) {
        // This method is called on a worker thread.
        setIntent(intent);
        handleSendIntentNoSave();
    }

    public void setIntent(Intent intent) {
        mIntent = intent;
    }

    public Intent getIntent() {
        return mIntent;
    }

    public SendReq getSendReqNoSave() {
        return sendReqNoSave;
    }

    void setSendReq(SendReq pdu) {
        sendReqNoSave = pdu;
    }

    public byte[] getSendPdu() {
        return sendPdu;
    }

    private void setSendPdu(byte[] pdu) {
        sendPdu = pdu;
    }

    private void setMimeType(String type) {
        mMimeType = type;
    }

    private String getMimeType() {
        return mMimeType;
    }

    private void setUri(Uri uri) {
        mUri = uri;
    }

    private Uri getUri() {
        return mUri;
    }

    void debug(SendReq pdu) {
        int number = pdu.getBody().getPartsNum();
        Log.v(TAG, "SendReq getTo:" + EncodedStringValue.concat(pdu.getTo()) + " partNumber:" + number);
        int i = 0;
        byte[] contentType;
        byte[] contentLocation;
        String uriString = null;
        Uri dataUri;
        byte[] data;
        for (i = 0; i < number; i++) {
            contentType = pdu.getBody().getPart(i).getContentType();
            contentLocation = pdu.getBody().getPart(i).getContentLocation();
            dataUri = pdu.getBody().getPart(i).getDataUri();
            if (dataUri != null) {
                uriString = dataUri.toString();
            }
            data = pdu.getBody().getPart(i).getData();
            Log.v(TAG, "part[" + i + "] contentType:" + printBytesToString(contentType));
            Log.v(TAG, "part[" + i + "] contentLocation:" + printBytesToString(contentLocation));
            Log.v(TAG, "part[" + i + "] dataUri:" + uriString);
            if (data != null) {
                //Log.v(TAG,"part["+i+"] data:"+HexDump.toHexString(data));
            }
        }
    }

    public String printBytesToString(byte[] content) {
        String ctString = null;
        if (content != null) {
            ctString = new String(content);
            return ctString;
        }
        return ctString;

    }


    private AddPluggerHandler mPluggerHandle = new AddPluggerHandler();

    private AddPluggerHandler GetAddPluggerHandler() {
        return mPluggerHandle;
    }

    class AddPluggerHandler extends Handler {
        public AddPluggerHandler() {
        }

        public AddPluggerHandler(Looper L) {
            super(L);
        }

        @Override
        public void handleMessage(Message msg) {
            // TODO Auto-generated method stub

            switch (msg.what) {
                case START_ADD_PLUGGER:
                    Log.e(TAG, "START_ADD_PLUGGER");
                    new Thread(new Runnable() {
                        @Override
                        public void run() {
                            try {
                                addPlugger(getMimeType(), getUri(), false);
                            } catch (OutOfMemoryError e) {
                                mMessage = GetAddPluggerHandler().obtainMessage(TOAST_SHARE_ERR_OOM);
                                GetAddPluggerHandler().sendMessageDelayed(mMessage, 100);
                            } catch (Exception e) {
                                mMessage = GetAddPluggerHandler().obtainMessage(TOAST_SHARE_FAIL);
                                GetAddPluggerHandler().sendMessageDelayed(mMessage, 100);
                            }
                        }
                    }, "PluggerService.handleSendIntentNoSave").start();
                    break;
                case TOAST_SHARE_FAIL:
                    Toast.makeText(PluggerIntentService.this, "string.toast_shared_file_failed", Toast.LENGTH_LONG)
                            .show();
                    break;
                case TOAST_NOSERVICE:
                    Toast.makeText(PluggerIntentService.this, "string.sim_card_not_ready", Toast.LENGTH_LONG).show();
                    break;
                case TOAST_SHARE_ERR_OOM:
                    Toast.makeText(PluggerIntentService.this, R.string.share_err_oom, Toast.LENGTH_LONG).show();
                    break;
                default:
                    Log.e(TAG, "No data to addPlugger");
                    break;
            }
        }

    }

    public boolean isSendMmsNoSave() {
        Intent intent = getIntent();
        Bundle extras = intent.getExtras();
        if (extras == null) {
            return false;
        }
        String action = intent.getAction();
        if (action != null & PluggerReceiver.PLUGGER_SEND_MMS_NOSAVE.equals(action)) {
            Log.v(TAG, "isSendMmsNoSave action:" + action);
            return true;
        }
        return false;
    }


    private void addPlugger(String type, Uri uri, boolean append) {
        if (uri != null) {
            // When we're handling Intent.ACTION_SEND_MULTIPLE, the passed in items can be
            // videos, and/or images, and/or some other unknown types we don't handle. When
            // a single attachment is "shared" the type will specify an image or video. When
            // there are multiple types, the type passed in is "*/*". In that case, we've got
            // to look at the uri to figure out if it is an image or video.
            boolean wildcard = "*/*".equals(type);

            if (wildcard) {
                if ("file".equals(uri.getScheme())) {
                    String tmp = MmsUtils.getTypeFromFile(uri);
                    if (tmp != null && !tmp.isEmpty()) {
                        type = tmp;
                    }
                } else if ("content".equals(uri.getScheme())) {
                    ContentResolver cr = this.getContentResolver();
                    Cursor c = cr.query(uri, null, null, null, null);
                    String tmp = null;
                    try {
                        if (c != null && c.moveToFirst()) {
                            try {
                                tmp = c.getString(c.getColumnIndexOrThrow(Audio.Media.MIME_TYPE));
                            } catch (IllegalArgumentException e) {
                                tmp = c.getString(c.getColumnIndexOrThrow("mimetype"));
                            }
                        }
                    } catch (IllegalArgumentException e) {
                        tmp = null;
                    } finally {
                        if (c != null) {
                            c.close();
                        }
                    }

                    if (tmp != null && !tmp.isEmpty()) {
                        type = tmp;
                    }
                }
            }

            if (type.startsWith("image/") || (wildcard && uri.toString().startsWith(mImageUri))) {
                Log.v(TAG, "addPlugger image type:" + type + " uri:" + uri + " wildcard:" + wildcard);
                addImageNoSave(uri, append);
            } else if (type.startsWith("audio/")) {
                Log.v(TAG, "addPlugger audio type:" + type + " uri:" + uri + " wildcard:" + wildcard);
                addAudioNoSave(uri, append);
            }else if (type.startsWith("video/")) {
                Log.v(TAG, "addPlugger video type:" + type + " uri:" + uri + " wildcard:" + wildcard);
                addVideoNoSave(uri, append);
            }else {
                Log.e(TAG, "addPlugger fail type:" + type + " uri:" + uri + " wildcard:" + wildcard, new Throwable("addPlugger"));
                mMessage = GetAddPluggerHandler().obtainMessage(TOAST_SHARE_FAIL);
                GetAddPluggerHandler().sendMessageDelayed(mMessage, 100);
            }
        }
    }


    private final ResizeImageResultCallback mResizeImageCallbackNoSave = new ResizeImageResultCallback() {
        // TODO: make this produce a Uri, that's what we want anyway
        @Override
        public void onResizeResult(PduPart part, boolean append) {
            if (part == null) {
                Log.e(TAG, "image part is null");
                mMessage = GetAddPluggerHandler().obtainMessage(TOAST_SHARE_FAIL);
                GetAddPluggerHandler().sendMessageDelayed(mMessage, 100);
                return;
            }
            sendMmsNoSave(part);
        }
    };

    private void addImageNoSave(Uri uri, boolean append) {
        //if (Log.isLoggable(LogTag.APP, Log.VERBOSE)) {
        log("addImage: append=" + append + ", uri=" + uri);
        //}
        //int subId = mSubscriptionManager.getDefaultSmsSubId();
        //int subId = Utils.getEffectiveSubscriptionId(MmsManager.DEFAULT_SUB_ID);
        int subId = SmsManager.getDefaultSmsSubscriptionId();
        synchronized (mAttachmentLock) {
            log("resize image " + uri);
            MmsUtils.resizeImageNoSave(PluggerIntentService.this, uri, mResizeImageCallbackNoSave, append, false, subId);

        }
        return;
    }

    private void addAudioNoSave(Uri uri, boolean append) {
        log("addAudio: append=" + append + ", uri=" + uri);
        int subId = SmsManager.getDefaultSmsSubscriptionId();
        addPartForUri( uri, "audio.amr");
    }

    private void addVideoNoSave(Uri uri, boolean append) {
        log("addVideo: append=" + append + ", uri=" + uri);
        int subId = SmsManager.getDefaultSmsSubscriptionId();
        addPartForUri( uri, "video.mp4");
    }
    private  void addPartForUri(final Uri uri, String srcName) {
        final PduPart part = new PduPart();
        part.setDataUri(uri);
        String contentType = ImageUtils.getContentType(this.getContentResolver(), uri);
        part.setContentType(contentType.getBytes());
        part.setContentLocation(srcName.getBytes());
        sendMmsNoSave(part);
    }
    // Handle send actions, where we're told to send a picture no save in mmssms.db.
    private boolean handleSendIntentNoSave() {
        if (isSendMmsNoSave()) {
            Intent intent = getIntent();
            Bundle extras = intent.getExtras();
            if (extras == null) {
                return false;
            }
            final String mimeType = intent.getType();
            Log.v(TAG, "mimeType:" + mimeType);
            if (extras.containsKey(Intent.EXTRA_STREAM)) {
                final Uri uri = (Uri) extras.getParcelable(Intent.EXTRA_STREAM);
                mMessage = GetAddPluggerHandler().obtainMessage(START_ADD_PLUGGER);
                setMimeType(mimeType);
                setUri(uri);
                GetAddPluggerHandler().sendMessageDelayed(mMessage, 100);
                return true;
            }
        }
        return false;
    }


    void composePduHeader() {
        SendReq sendReq = new SendReq();
        EncodedStringValue[] encodedNumbers = EncodedStringValue.encodeStrings(mPluggerToDestNumber);
        if (encodedNumbers != null) {
            sendReq.setTo(encodedNumbers);
        }
        Log.d(TAG, "composePduHeader encodedNumbers" + encodedNumbers);
        setSendReq(sendReq);
    }


    private void composePduBody(PduPart part) {
        if (part == null) {
            Log.e(TAG, "image part is null");
            return;
        }
        PduBody pb = new PduBody();
        part.setContentLocation("plugger.jpg".getBytes());

        pb.addPart(part);


        SMILDocument document = SmilHelper.getDocument(pb);
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        SmilXmlSerializer.serialize(document, out);
        PduPart smilPart = new PduPart();
        smilPart.setContentId("smil".getBytes());
        smilPart.setContentLocation("smil.xml".getBytes());
        smilPart.setContentType(ContentType.APP_SMIL.getBytes());
        smilPart.setData(out.toByteArray());
        pb.addPart(0, smilPart);
        getSendReqNoSave().setBody(pb);

//        final long minSize =  MIN_IMAGE_BYTE_SIZE;
//        final int byteBudget = MmsConfig.get(subId).getMaxMessageSize() - totalLength
//                - MMS_MAX_SIZE_SLOP;
//        final double budgetFactor =
//                minSize > 0 ? Math.max(1.0, byteBudget / ((double) minSize)) : 1;
//        final int bytesPerImage = (int) (budgetFactor * MIN_IMAGE_BYTE_SIZE);
//        final int widthLimit = 300;
//        final int heightLimit = 200;
//		boolean hasVisualAttachment = true;
//        boolean hasNonVisualAttachment = false;
//		boolean hasText = false;
//		final StringBuilder smilBody = new StringBuilder();
//		smilBody.append(String.format(sSmilImagePart, srcName));
//                    totalLength += addPicturePart(context, pb, index, part,
//                            widthLimit, heightLimit, bytesPerImage, srcName, contentType);
//		final String smilTemplate = getSmilTemplate(hasVisualAttachment,
//                hasNonVisualAttachment, hasText);
//        addSmilPart(pb, smilTemplate, smilBody.toString());
//
//        getSendReqNoSave().setBody(pb);
//        getSendReqNoSave().setMessageSize(0);
//         // Update the 'date' field of the message before sending it.
//        getSendReqNoSave().setDate(System.currentTimeMillis() / 1000L);
//         // SPRD: Add for multi-sim module.
//
        // fanzr add begin
        //getActivePhone();
        //getSendReqNoSave().setPhoneId(SprdSubscriptionManager.getInstance().getSubscription());
        // fanzr add end
    }


    private void composeSendPdu(PduPart part) {
        Log.v(TAG, "start composeSendPdu");
        composePduHeader();
        composePduBody(part);
        debug(getSendReqNoSave());
        byte[] pdu = new PduComposer(this, getSendReqNoSave()).make();
        if (pdu != null) {
            //Log.v(TAG, "sendPdu hex = " + HexDump.toHexString(pdu));
        }
        setSendPdu(pdu);

    }

    private void sendMmsNoSave(PduPart part) {
        composeSendPdu(part);
        if (getSendPdu() == null) {
            Log.e(TAG, "getSendPdu is null");
            return;
        }

        Log.d(TAG, "sendMmsNoSave is going");
        mSubId = SmsManager.getDefaultSmsSubscriptionId();
        int status = MMS_REQUEST_MANUAL_RETRY;
        //int rawStatus = MmsUtils.RAW_TELEPHONY_STATUS_UNDEFINED;
        try {
            MmsSender.sendMms(this, mSubId, null, getSendReqNoSave(), null);
        } catch (MmsFailureException e) {
            e.printStackTrace();
        }
    }

    private void log(String log) {
        //if (Log.isLoggable(LogTag.TRANSACTION, Log.VERBOSE) || LogTag.DEBUG_TRANSACTION) {
        Log.d(TAG, log);
        //}
    }
}

