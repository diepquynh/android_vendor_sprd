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

package com.android.util;

import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.res.AssetFileDescriptor;
import android.content.res.Resources;
import android.database.Cursor;
import android.database.DatabaseUtils;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteException;
import android.media.MediaMetadataRetriever;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.Uri;
import android.os.Bundle;
import android.provider.Settings;
import android.provider.Telephony;
import android.provider.Telephony.Mms;
import android.provider.Telephony.Sms;
import android.provider.Telephony.Threads;
import android.telephony.SmsManager;
import android.telephony.SmsMessage;
import android.text.TextUtils;
import android.text.util.Rfc822Token;
import android.text.util.Rfc822Tokenizer;
import android.util.Log;
import android.graphics.Rect;

//import com.android.messaging.Factory;
//import com.android.messaging.R;
//import com.android.messaging.datamodel.MediaScratchFileProvider;
//import com.android.messaging.datamodel.action.DownloadMmsAction;
//import com.android.messaging.datamodel.action.SendMessageAction;
//import com.android.messaging.datamodel.data.MessageData;
//import com.android.messaging.datamodel.data.MessagePartData;
//import com.android.messaging.datamodel.data.ParticipantData;
import com.android.mmslib.InvalidHeaderValueException;
import com.android.mmslib.MmsException;
import com.android.mmslib.SqliteWrapper;
import com.google.android.mms.pdu.CharacterSets;
import com.google.android.mms.pdu.EncodedStringValue;
import com.google.android.mms.pdu.GenericPdu;
import com.google.android.mms.pdu.NotificationInd;
import com.google.android.mms.pdu.DeliveryInd;
import com.google.android.mms.pdu.ReadOrigInd;
import com.google.android.mms.pdu.PduBody;
import com.google.android.mms.pdu.PduComposer;
import com.google.android.mms.pdu.PduHeaders;
import com.google.android.mms.pdu.PduParser;
import com.google.android.mms.pdu.PduPart;
import com.google.android.mms.pdu.PduPersister;
import com.google.android.mms.pdu.RetrieveConf;
import com.google.android.mms.pdu.SendConf;
import com.google.android.mms.pdu.SendReq;
import com.android.util.ImageUtils.ImageResizer;

import android.webkit.MimeTypeMap;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.GregorianCalendar;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Set;
import java.util.UUID;

/**
 * Utils for sending sms/mms messages.
 */
public class MmsUtils {
	private static final String TAG = "MmsUtils";

	public static final boolean DEFAULT_DELIVERY_REPORT_MODE = false;
	public static final boolean DEFAULT_READ_REPORT_MODE = false;
	public static final long DEFAULT_EXPIRY_TIME_IN_SECONDS = 7 * 24 * 60 * 60;
	public static final int DEFAULT_PRIORITY = PduHeaders.PRIORITY_NORMAL;

	public static final int MAX_SMS_RETRY = 3;

	// fanzr add begin
	/**
	 * Message overhead that reduces the maximum image byte size. 5000 is a
	 * realistic overhead number that allows for user to also include a small
	 * MIDI file or a couple pages of text along with the picture.
	 */
	public static final int MAX_MESSAGE_SIZE = 300 * 1024;
	public static final int RAW_TELEPHONY_STATUS_MESSAGE_TOO_BIG = 10000;
	public static final int MESSAGE_OVERHEAD = 5000;



	// fanzr add end

	/**
	 * MMS request succeeded
	 */
	public static final int MMS_REQUEST_SUCCEEDED = 0;
	/**
	 * MMS request failed with a transient error and can be retried
	 * automatically
	 */
	public static final int MMS_REQUEST_AUTO_RETRY = 1;
	/**
	 * MMS request failed with an error and can be retried manually
	 */
	public static final int MMS_REQUEST_MANUAL_RETRY = 2;
	/**
	 * MMS request failed with a specific error and should not be retried
	 */
	public static final int MMS_REQUEST_NO_RETRY = 3;

	public static final String getRequestStatusDescription(final int status) {
		switch (status) {
		case MMS_REQUEST_SUCCEEDED:
			return "SUCCEEDED";
		case MMS_REQUEST_AUTO_RETRY:
			return "AUTO_RETRY";
		case MMS_REQUEST_MANUAL_RETRY:
			return "MANUAL_RETRY";
		case MMS_REQUEST_NO_RETRY:
			return "NO_RETRY";
		default:
			return String.valueOf(status) + " (check MmsUtils)";
		}
	}

	public static final int PDU_HEADER_VALUE_UNDEFINED = 0;

	private static final int DEFAULT_DURATION = 5000; // ms

	// amount of space to leave in a MMS for text and overhead.
	private static final int MMS_MAX_SIZE_SLOP = 1024;
	public static final long INVALID_TIMESTAMP = 0L;
	private static String[] sNoSubjectStrings;

	public static class MmsInfo {
		public Uri mUri;
		public int mMessageSize;
		public PduBody mPduBody;
	}

	// Sync all remote messages apart from drafts
	private static final String REMOTE_SMS_SELECTION = String.format(Locale.US,
			"(%s IN (%d, %d, %d, %d, %d))", Sms.TYPE, Sms.MESSAGE_TYPE_INBOX,
			Sms.MESSAGE_TYPE_OUTBOX, Sms.MESSAGE_TYPE_QUEUED,
			Sms.MESSAGE_TYPE_FAILED, Sms.MESSAGE_TYPE_SENT);

	private static final String REMOTE_MMS_SELECTION = String.format(Locale.US,
			"((%s IN (%d, %d, %d, %d)) AND (%s IN (%d, %d, %d)))",
			Mms.MESSAGE_BOX, Mms.MESSAGE_BOX_INBOX, Mms.MESSAGE_BOX_OUTBOX,
			Mms.MESSAGE_BOX_SENT, Mms.MESSAGE_BOX_FAILED, Mms.MESSAGE_TYPE,
			PduHeaders.MESSAGE_TYPE_SEND_REQ,
			PduHeaders.MESSAGE_TYPE_NOTIFICATION_IND,
			PduHeaders.MESSAGE_TYPE_RETRIEVE_CONF);

	/**
	 * Type selection for importing sms messages.
	 * 
	 * @return The SQL selection for importing sms messages
	 */
	public static String getSmsTypeSelectionSql() {
		return REMOTE_SMS_SELECTION;
	}

	/**
	 * Type selection for importing mms messages.
	 * 
	 * @return The SQL selection for importing mms messages. This selects the
	 *         message type, not including the selection on timestamp.
	 */
	public static String getMmsTypeSelectionSql() {
		return REMOTE_MMS_SELECTION;
	}

	// SMIL spec: http://www.w3.org/TR/SMIL3

	private static final String sSmilImagePart = "<par dur=\""
			+ DEFAULT_DURATION + "ms\">"
			+ "<img src=\"%s\" region=\"Image\" />" + "</par>";

	private static final String sSmilVideoPart = "<par dur=\"%2$dms\">"
			+ "<video src=\"%1$s\" dur=\"%2$dms\" region=\"Image\" />"
			+ "</par>";

	private static final String sSmilAudioPart = "<par dur=\"%2$dms\">"
			+ "<audio src=\"%1$s\" dur=\"%2$dms\" />" + "</par>";

	private static final String sSmilTextPart = "<par dur=\""
			+ DEFAULT_DURATION + "ms\">"
			+ "<text src=\"%s\" region=\"Text\" />" + "</par>";

	private static final String sSmilPart = "<par dur=\"" + DEFAULT_DURATION
			+ "ms\">" + "<ref src=\"%s\" />" + "</par>";

	private static final String sSmilTextOnly = "<smil>" + "<head>"
			+ "<layout>" + "<root-layout/>"
			+ "<region id=\"Text\" top=\"0\" left=\"0\" "
			+ "height=\"100%%\" width=\"100%%\"/>" + "</layout>" + "</head>"
			+ "<body>" + "%s" + // constructed body goes here
			"</body>" + "</smil>";

	private static final String sSmilVisualAttachmentsOnly = "<smil>"
			+ "<head>" + "<layout>" + "<root-layout/>"
			+ "<region id=\"Image\" fit=\"meet\" top=\"0\" left=\"0\" "
			+ "height=\"100%%\" width=\"100%%\"/>" + "</layout>" + "</head>"
			+ "<body>" + "%s" + // constructed body goes here
			"</body>" + "</smil>";

	private static final String sSmilVisualAttachmentsWithText = "<smil>"
			+ "<head>" + "<layout>" + "<root-layout/>"
			+ "<region id=\"Image\" fit=\"meet\" top=\"0\" left=\"0\" "
			+ "height=\"80%%\" width=\"100%%\"/>"
			+ "<region id=\"Text\" top=\"80%%\" left=\"0\" height=\"20%%\" "
			+ "width=\"100%%\"/>" + "</layout>" + "</head>" + "<body>" + "%s" + // constructed
																				// body
																				// goes
																				// here
			"</body>" + "</smil>";

	private static final String sSmilNonVisualAttachmentsOnly = "<smil>"
			+ "<head>" + "<layout>" + "<root-layout/>" + "</layout>"
			+ "</head>" + "<body>" + "%s" + // constructed body goes here
			"</body>" + "</smil>";

	private static final String sSmilNonVisualAttachmentsWithText = sSmilTextOnly;

	public static final String MMS_DUMP_PREFIX = "mmsdump-";
	public static final String SMS_DUMP_PREFIX = "smsdump-";

	public static final int MIN_VIDEO_BYTES_PER_SECOND = 4 * 1024;
	public static final int MIN_IMAGE_BYTE_SIZE = 16 * 1024;
	public static final int MAX_VIDEO_ATTACHMENT_COUNT = 1;

	// fanzr add begin
	public static void makePluggerPduBody(final Context context, PduPart part) {

	}

	// fanzr add end

	private static void setPartContentLocationAndId(final PduPart part,
			final String srcName) {
		// Set Content-Location.
		part.setContentLocation(srcName.getBytes());

		// Set Content-Id.
		final int index = srcName.lastIndexOf(".");
		final String contentId = (index == -1) ? srcName : srcName.substring(0,
				index);
		part.setContentId(contentId.getBytes());
	}

	private static int addTextPart(final Context context, final PduBody pb,
			final String text, final String srcName) {
		final PduPart part = new PduPart();

		// Set Charset if it's a text media.
		part.setCharset(CharacterSets.UTF_8);

		// Set Content-Type.
		part.setContentType(ContentType.TEXT_PLAIN.getBytes());

		// Set Content-Location.
		setPartContentLocationAndId(part, srcName);

		part.setData(text.getBytes());

		pb.addPart(part);

		return part.getData().length;
	}

	private static void addPartForUri(final Context context, final PduBody pb,
			final String srcName, final Uri uri, final String contentType) {
		final PduPart part = new PduPart();
		part.setDataUri(uri);
		part.setContentType(contentType.getBytes());

		setPartContentLocationAndId(part, srcName);

		pb.addPart(part);
	}

	/**
	 * Add video part recompressing video if necessary. If recompression fails,
	 * part is not added.
	 */

	private static String getSmilTemplate(final boolean hasVisualAttachments,
			final boolean hasNonVisualAttachments, final boolean hasText) {
		if (hasVisualAttachments) {
			return hasText ? sSmilVisualAttachmentsWithText
					: sSmilVisualAttachmentsOnly;
		}
		if (hasNonVisualAttachments) {
			return hasText ? sSmilNonVisualAttachmentsWithText
					: sSmilNonVisualAttachmentsOnly;
		}
		return sSmilTextOnly;
	}

	/**
	 * Returns {@code true} if group mms is turned on, {@code false} otherwise.
	 * 
	 * For the group mms feature to be enabled, the following must be true: 1.
	 * the feature is enabled in mms_config.xml (currently on by default) 2. the
	 * feature is enabled in the SMS settings page
	 * 
	 * @return true if group mms is supported
	 */

	/**
	 * Get a version of this image resized to fit the given dimension and
	 * byte-size limits. Note that the content type of the resulting PduPart may
	 * not be the same as the content type of this UriImage; always call
	 * {@link PduPart#getContentType()} to get the new content type.
	 * 
	 * @param widthLimit
	 *            The width limit, in pixels
	 * @param heightLimit
	 *            The height limit, in pixels
	 * @param byteLimit
	 *            The binary size limit, in bytes
	 * @param width
	 *            The image width, in pixels
	 * @param height
	 *            The image height, in pixels
	 * @param orientation
	 *            Orientation constant from ExifInterface for rotating or
	 *            flipping the image
	 * @param imageUri
	 *            Uri to the image data
	 * @param context
	 *            Needed to open the image
	 * @return A new PduPart containing the resized image data
	 */
	private static PduPart getResizedImageAsPart(final int widthLimit,
			final int heightLimit, final int byteLimit, final int width,
			final int height, final int orientation, final Uri imageUri,
			final Context context, final String contentType) {
		final PduPart part = new PduPart();

		final byte[] data = ImageResizer.getResizedImageData(width, height,
				orientation, widthLimit, heightLimit, byteLimit, imageUri,
				context, contentType);
		if (data == null) {
			Log.v(TAG, "Resize image failed.");
			return null;
		}

		part.setData(data);
		// Any static images will be compressed into a jpeg
		final String contentTypeOfResizedImage = ImageUtils.isGif(context,contentType,
				imageUri) ? ContentType.IMAGE_GIF : ContentType.IMAGE_JPEG;
		part.setContentType(contentTypeOfResizedImage.getBytes());

		return part;
	}

	public static void resizeImageNoSave(final Context context, Uri imageUri,
			ResizeImageResultCallback cb, boolean append, boolean showToast,
			final int subId) {
		final PduPart part;
		try {
			// UriImage image = new UriImage(context, imageUri);
			// int widthLimit = MmsConfig.get(subId).getMaxImageWidth();
			// int heightLimit = MmsConfig.get(subId).getMaxImageHeight();
			int widthLimit = 640;
			int heightLimit = 480;
			// In mms_config.xml, the max width has always been declared larger
			// than the max
			// height. Swap the width and height limits if necessary so we scale
			// the picture
			// as little as possible.

			final Rect imageSize = ImageUtils.decodeImageBounds(context, imageUri);
			if (imageSize.height() > imageSize.width()) {
				int temp = widthLimit;
				widthLimit = heightLimit;
				heightLimit = temp;
			}
			int orientation = ImageUtils.getOrientation(context, imageUri);
			part = getResizedImageAsPart(widthLimit,
					heightLimit,
					// MmsConfig.get(subId).getMaxMessageSize() -
					// MmsUtils.MESSAGE_OVERHEAD,
					(MmsUtils.MAX_MESSAGE_SIZE - MmsUtils.MESSAGE_OVERHEAD),
					imageSize.height(), imageSize.width(), orientation,
					imageUri, context, ImageUtils.getContentType(
							context.getContentResolver(), imageUri));
		} finally {
		}
		cb.onResizeResult(part, append);
	}

	public static String getTypeFromFile(Uri uri) {
		String path = uri.getPath();
		String src = path.substring(path.lastIndexOf('/') + 1);
		MimeTypeMap mimeTypeMap = MimeTypeMap.getSingleton();
		String extension = MimeTypeMap.getFileExtensionFromUrl(src);
		String type = "*/*";
		if (TextUtils.isEmpty(extension)) {
			// getMimeTypeFromExtension() doesn't handle spaces in filenames nor
			// can it handle
			// urlEncoded strings. Let's try one last time at finding the
			// extension.
			int dotPos = src.lastIndexOf('.');
			if (0 <= dotPos) {
				extension = src.substring(dotPos + 1);
			}
		}
		type = mimeTypeMap.getMimeTypeFromExtension(extension.toLowerCase());
		return type;
	}
}
