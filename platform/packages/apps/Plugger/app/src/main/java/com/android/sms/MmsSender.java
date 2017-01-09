/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.android.sms;

import android.app.Activity;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.support.v7.mms.MmsManager;
import android.telephony.SmsManager;
import android.util.Log;


//import com.android.messaging.datamodel.action.SendMessageAction;
//import com.android.messaging.datamodel.data.MessageData;
import com.sprd.pluggersprd.datamodel.PluggerFileProvider;
import com.android.mmslib.InvalidHeaderValueException;

import com.android.util.Assert;
import com.android.util.MmsUtils;
import com.google.android.mms.pdu.GenericPdu;
import com.google.android.mms.pdu.PduComposer;
import com.google.android.mms.pdu.SendReq;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;

/**
 * Class that sends chat message via MMS.
 *
 * The interface emulates a blocking send similar to making an HTTP request.
 */
public class MmsSender {
    private static final String TAG = "MmsSender";

    /**
     * Send an MMS message.
     *
     * @param context Context
     * @param messageUri The unique URI of the message for identifying it during sending
     * @param sendReq The SendReq PDU of the message
     * @throws MmsFailureException
     */
    public static void sendMms(final Context context, final int subId, final Uri messageUri,
            final SendReq sendReq, final Bundle sentIntentExras) throws MmsFailureException {
        sendMms(context,
                subId,
                messageUri,
                null /* locationUrl */,
                sendReq,
                true /* responseImportant */,
                sentIntentExras);
    }



    /**
     * Send AcknowledgeInd (response to mms manual download). Ignore failures.
     *
     * @param context Context
     * @param subId The SIM's subId we are currently using
     * @param transactionId The transaction id of the MMS message
     * @param contentLocation The url of the MMS message
     * @throws MmsFailureException
     * @throws InvalidHeaderValueException
     */


    /**
     * Send a generic PDU.
     *
     * @param context Context
     * @param messageUri The unique URI of the message for identifying it during sending
     * @param locationUrl The optional URL to send to
     * @param pdu The PDU to send
     * @param responseImportant If the sending response is important. Responses to the
     * Sending of AcknowledgeInd and NotifyRespInd are not important.
     * @throws MmsFailureException
     */
    private static void sendMms(final Context context, final int subId, final Uri messageUri,
            final String locationUrl, final GenericPdu pdu, final boolean responseImportant,
            final Bundle sentIntentExtras) throws MmsFailureException {
        // Write PDU to temporary file to send to platform
        final Uri contentUri = writePduToTempFile(context, pdu, subId);
        Log.d(TAG, "sendMms contentUri:" + contentUri);
        MmsManager.sendMultimediaMessage(subId, context, contentUri, locationUrl, null);
    }

    private static Uri writePduToTempFile(final Context context, final GenericPdu pdu, int subId)
            throws MmsFailureException {
        final Uri contentUri = PluggerFileProvider.buildRawMmsUri(context);
        final File tempFile = PluggerFileProvider.getFile(context,contentUri);
        FileOutputStream writer = null;
        try {
            // Ensure rawmms directory exists
            tempFile.getParentFile().mkdirs();
            writer = new FileOutputStream(tempFile);
            final byte[] pduBytes = new PduComposer(context, pdu).make();
            if (pduBytes == null) {
                throw new MmsFailureException(
                        MmsUtils.MMS_REQUEST_NO_RETRY, "Failed to compose PDU");
            }
            if (pduBytes.length > MmsUtils.MAX_MESSAGE_SIZE) {
                throw new MmsFailureException(
                        MmsUtils.MMS_REQUEST_NO_RETRY,
                        MmsUtils.RAW_TELEPHONY_STATUS_MESSAGE_TOO_BIG);
            }
            writer.write(pduBytes);
        } catch (final IOException e) {
            if (tempFile != null) {
                tempFile.delete();
            }
            Log.e(TAG, "Cannot create temporary file " + tempFile.getAbsolutePath(), e);
            throw new MmsFailureException(
                    MmsUtils.MMS_REQUEST_AUTO_RETRY, "Cannot create raw mms file");
        } catch (final OutOfMemoryError e) {
            if (tempFile != null) {
                tempFile.delete();
            }
            Log.e(TAG, "Out of memory in composing PDU", e);
            throw new MmsFailureException(
                    MmsUtils.MMS_REQUEST_MANUAL_RETRY,
                    MmsUtils.RAW_TELEPHONY_STATUS_MESSAGE_TOO_BIG);
        } finally {
            if (writer != null) {
                try {
                    writer.close();
                } catch (final IOException e) {
                    // no action we can take here
                }
            }
        }
        return contentUri;
    }
/*
    public static SendConf parseSendConf(byte[] response, int subId) {
        if (response != null) {
            final GenericPdu respPdu = new PduParser(
                    response, MmsConfig.get(subId).getSupportMmsContentDisposition()).parse();
            if (respPdu != null) {
                if (respPdu instanceof SendConf) {
                    return (SendConf) respPdu;
                } else {
                    Log.e(TAG, "MmsSender: send response not SendConf");
                }
            } else {
                // Invalid PDU
                Log.e(TAG, "MmsSender: send invalid response");
            }
        }
        // Empty or invalid response
        return null;
    }
*/

    /*
    public static RetrieveConf parseRetrieveConf(byte[] data, int subId) {
        if (data != null) {
            final GenericPdu pdu = new PduParser(
                    data, MmsConfig.get(subId).getSupportMmsContentDisposition()).parse();
            if (pdu != null) {
                if (pdu instanceof RetrieveConf) {
                    return (RetrieveConf) pdu;
                } else {
                    Log.e(TAG, "MmsSender: downloaded pdu not RetrieveConf: "
                            + pdu.getClass().getName());
                }
            } else {
                Log.e(TAG, "MmsSender: downloaded pdu could not be parsed (invalid)");
            }
        }
        Log.e(TAG, "MmsSender: downloaded pdu is empty");
        return null;
    }
    */
    // Process different result code from platform MMS service
    public static int getErrorResultStatus(int resultCode, int httpStatusCode) {
        Assert.isFalse(resultCode == Activity.RESULT_OK);
        switch (resultCode) {
            case SmsManager.MMS_ERROR_UNABLE_CONNECT_MMS:
            case SmsManager.MMS_ERROR_IO_ERROR:
                return MmsUtils.MMS_REQUEST_AUTO_RETRY;
            case SmsManager.MMS_ERROR_INVALID_APN:
            case SmsManager.MMS_ERROR_CONFIGURATION_ERROR:
            case SmsManager.MMS_ERROR_NO_DATA_NETWORK:
            case SmsManager.MMS_ERROR_UNSPECIFIED:
                return MmsUtils.MMS_REQUEST_MANUAL_RETRY;
            case SmsManager.MMS_ERROR_HTTP_FAILURE:
                if (httpStatusCode == 404) {
                    return MmsUtils.MMS_REQUEST_NO_RETRY;
                } else {
                    return MmsUtils.MMS_REQUEST_AUTO_RETRY;
                }
            default:
                return MmsUtils.MMS_REQUEST_MANUAL_RETRY;
        }
    }
}
