package com.android.insertdata.addmms;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.net.Uri;
import android.provider.Telephony.Mms.Draft;
import android.provider.Telephony.Mms.Inbox;
import android.provider.Telephony.Mms.Sent;
import android.text.TextUtils;
import android.util.Log;

import com.google.android.mms.ContentType;
import com.google.android.mms.pdu.CharacterSets;
import com.google.android.mms.pdu.EncodedStringValue;
import com.google.android.mms.pdu.GenericPdu;
import com.google.android.mms.pdu.PduBody;
import com.google.android.mms.pdu.PduHeaders;
import com.google.android.mms.pdu.PduPart;
import com.google.android.mms.pdu.PduPersister;
import com.google.android.mms.pdu.RetrieveConf;
import com.google.android.mms.pdu.SendReq;

public class MmsDataGenrat {
	static final String TAG = "MmsDataGenrat";
	private MmsConfig mmsCongfig;

	static final String KEY_READ = "read";
	static final String KEY_DATE = "date";
	static final String KEY_TEXT = "text"; 
	static final String KEY_MSG_BOX = "msg_box"; 
	static final String KEY_ADDRESS = "address";
	static final String KEY_SUBJECT = "sub";
	static final String KEY_THREAD_ID = "thread_id";

	private int currentCount;
	ContentResolver contentResolver;
	Context context;

	public MmsDataGenrat(MmsConfig mmsCongfig, ContentResolver resolver,
			Context context) {
		this.mmsCongfig = mmsCongfig;
		this.contentResolver = resolver;
		this.context = context;
		currentCount = 0;
		buildReadContentValues();
	}

	public void setCurrent(int currentCount) {
		this.currentCount = currentCount;
	}

	private Uri getMmsUri(int boxId) {
		Uri typeUri;

		switch (mmsCongfig.boxId) {
		case MmsConstant.INBOX:
			typeUri = Inbox.CONTENT_URI;
			break;
		case MmsConstant.SENTBOX:
			typeUri = Sent.CONTENT_URI;
			break;
		case MmsConstant.DRAFTBOX:
			typeUri = Draft.CONTENT_URI;
			break;
		default:
			throw new IllegalArgumentException(String.valueOf(mmsCongfig.boxId));
		}
		return typeUri;

	}

	// get mms subject
	private String getMmsSubject() {
		int count = currentCount + 1;
		String retValue = "Subject:" + count;
		return retValue;
	}

	/******************************************************************************************************************/
	private class MmsException extends Exception {

		public MmsException() {
			super();
		}

		public MmsException(String message) {
			super(message);
		}

		public MmsException(Throwable cause) {
			super(cause);
		}

		public MmsException(String message, Throwable cause) {
			super(message, cause);
		}
	}

	private static PduPersister sPduPersister;

	private PduPersister getPduPersister() throws Exception {
		if (null == sPduPersister && null != context) {
			sPduPersister = PduPersister.getPduPersister(context);
		}
		if (null == sPduPersister) {
			throw new MmsException("Count not create pduPersister.");
		}
		return sPduPersister;
	}

	private static ContentValues mReadContentValues;
	private static ContentValues mUnReadContentValues;

	private void buildReadContentValues() {
		if (mReadContentValues == null) {
			mReadContentValues = new ContentValues(2);
			mReadContentValues.put("read", 1);
			mReadContentValues.put("seen", 1);
		}
		if (mReadContentValues == null) {
			mReadContentValues = new ContentValues(2);
			mReadContentValues.put("read", 0);
			mReadContentValues.put("seen", 0);
		}
	}

	private PduBody composePduBody() throws Exception {
		PduBody body = new PduBody();
		/*********************** text part **************************/
		PduPart textPart = new PduPart();
		/* set content-type */
		textPart.setContentType(ContentType.TEXT_PLAIN
				.getBytes(CharacterSets.MIMENAME_ISO_8859_1));
		textPart.setName("text".getBytes(CharacterSets.MIMENAME_ISO_8859_1));
		/* set content location */
		textPart.setContentLocation(Long.toOctalString(
				System.currentTimeMillis()).getBytes());
		/* set charset */
		textPart.setCharset(CharacterSets.UTF_8);
		/* set text data */
		StringBuilder build = new StringBuilder();
		build.append("测试彩信(").append(mmsCongfig.mmsSize).append("K)");
		textPart.setData(build.toString().getBytes("UTF-8"));

		body.addPart(textPart);
		/*********************** file part **************************/
		/* if file size <= 0 ,just return */
		if (mmsCongfig.mmsSize <= 0) {
			return body;
		}
		PduPart filePart = new PduPart();
		/* set content type */
		if (getMsgBoxId() == MmsConstant.DRAFTBOX) {
			filePart.setContentType(ContentType.IMAGE_UNSPECIFIED
					.getBytes(CharacterSets.MIMENAME_ISO_8859_1));
		} else {
			filePart.setContentType(ContentType.IMAGE_UNSPECIFIED
					.getBytes(CharacterSets.MIMENAME_ISO_8859_1));
		}
		filePart.setName("file".getBytes(CharacterSets.MIMENAME_ISO_8859_1));
		/* set contentLocation */
		filePart.setContentLocation(Long.toOctalString(
				System.currentTimeMillis()).getBytes());
		/* set file data */
		int k = mmsCongfig.mmsSize;
		filePart.setData(new byte[1024 * k]);
		/* add part */
		body.addPart(filePart);
		return body;
	}

	private GenericPdu composePdu() throws Exception {
		PduBody body = null;
		int boxId = getMsgBoxId();
		int messageType = -1;
		Log.d(TAG, "composeHeaders -> boxId:" + boxId);
		switch (boxId) {
		case MmsConstant.INBOX:
			messageType = PduHeaders.MESSAGE_TYPE_RETRIEVE_CONF;
			break;
		/* send & draft use the same transaction */
		case MmsConstant.SENTBOX:
		case MmsConstant.DRAFTBOX:
			messageType = PduHeaders.MESSAGE_TYPE_SEND_REQ;
			break;
		default:
			break;
		}
		if ((PduHeaders.MESSAGE_TYPE_SEND_REQ == messageType)
				|| (PduHeaders.MESSAGE_TYPE_RETRIEVE_CONF == messageType)) {
			/* need to compose the parts */
			body = composePduBody();
			if (null == body) {
				Log.d("lu", "null == body");
				return null;
			}
		}
		switch (messageType) {
		case PduHeaders.MESSAGE_TYPE_SEND_REQ: {
			/* make send-req transaction */
			SendReq sendReq = new SendReq();

			sendReq.setMessageType(messageType);

			EncodedStringValue[] encodedNumbers = EncodedStringValue
					.encodeStrings(new String[] { getMsgAddress() });
			if (encodedNumbers != null) {
				sendReq.setTo(encodedNumbers);
			} else {
				Log.d(TAG, "[makeSendReq] encodedNumbers is null, setTo fail");
			}

			if (!TextUtils.isEmpty(getMmsSubject())) {
				sendReq.setSubject(new EncodedStringValue(getMmsSubject()
						.toString()));
			}

			sendReq.setDate(System.currentTimeMillis() / 1000L);
			sendReq.setSubject(new EncodedStringValue(getMmsSubject()));
			sendReq.setBody(body);
			return sendReq;
		}
		case PduHeaders.MESSAGE_TYPE_RETRIEVE_CONF: {
			/* make retrieve-conf transaction */
			RetrieveConf retrieveConf = new RetrieveConf();
			retrieveConf.setMessageType(messageType);
			retrieveConf.setContentType(ContentType.MULTIPART_MIXED
					.getBytes(CharacterSets.MIMENAME_ISO_8859_1));
			retrieveConf.setFrom(new EncodedStringValue(getMsgAddress()));
			retrieveConf.setDate(System.currentTimeMillis() / 1000L);
			retrieveConf.setSubject(new EncodedStringValue(getMmsSubject()));
			retrieveConf.setBody(body);

			return retrieveConf;

		}
		}
		return null;
	}

	private int getMsgBoxId() {
		return mmsCongfig.boxId;
	}

	private String getMsgAddress() throws Exception {
		String address = mmsCongfig.phoneNo;
		if (TextUtils.isEmpty(address)) {
			throw new MmsException("Can not get KEY_ADDRESS.");
		}
		return address;
	}

	private int getMsgRead() {
		if (true == mmsCongfig.isRead) {
			return 1;
		} else {
			return 0;
		}
	}

	private void savePdu(GenericPdu pdu) throws Exception {
		Log.d(TAG, "---box uri:" + getMmsUri(getMsgBoxId()) + " "
				+ currentCount);
		PduPersister persister = getPduPersister();
//		Uri uri = persister.persist(pdu, getMmsUri(getMsgBoxId()));
        Uri uri = persister.persist(pdu, getMmsUri(getMsgBoxId()), true , false, null);
		updatePdu(uri);
	}

	private void updatePdu(Uri uri) throws Exception {
		int boxId = getMsgBoxId();
		switch (boxId) {
		case MmsConstant.INBOX:
			if (getMsgRead() == 1) {
				context.getContentResolver().update(uri, mReadContentValues,
						null, null);
			} else {
				context.getContentResolver().update(uri, mUnReadContentValues,
						null, null);
			}
			break;
		case MmsConstant.SENTBOX:

			break;
		case MmsConstant.DRAFTBOX:

			break;
		default:
			break;
		}
	}

	public void insert() {
		// Log.d(TAG, "===== begin insert! =====");
		try {
			// Log.d(TAG, " --- save pdu ---");
            if(composePdu() != null){
                savePdu(composePdu());
                Thread.sleep(100);
            }
		} catch (Exception e) {
			e.printStackTrace();
		}
		// Log.d(TAG, "===== end ! =====");
	}
}
