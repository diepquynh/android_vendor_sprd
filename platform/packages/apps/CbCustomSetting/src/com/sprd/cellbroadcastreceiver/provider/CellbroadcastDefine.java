package com.sprd.cellbroadcastreceiver.provider;

public interface CellbroadcastDefine {

	public static final String TABLE_NAME = "broadcasts";
	
	public static final String _ID = "_id";
	public static final String GEOGRAPHICAL_SCOPE = "geo_scope";
	public static final String PLMN = "plmn";
	public static final String LAC = "lac";
	public static final String CID = "cid";
	public static final String SERIAL_NUMBER = "serial_number";
	public static final String SERVICE_CATEGORY = "service_category";
	public static final String LANGUAGE_CODE = "language";
	public static final String MESSAGE_BODY = "body";
	public static final String DELIVERY_TIME = "date";
	public static final String MESSAGE_READ = "read";
	public static final String MESSAGE_FORMAT = "format";
	public static final String MESSAGE_PRIORITY = "priority";
	public static final String ETWS_WARNING_TYPE = "etws_warning_type";
	public static final String CMAS_MESSAGE_CLASS = "cmas_message_class";
	public static final String CMAS_CATEGORY = "cmas_category";
	public static final String CMAS_RESPONSE_TYPE = "cmas_response_type";
	public static final String CMAS_SEVERITY = "cmas_severity";
	public static final String CMAS_URGENCY = "cmas_urgency";
	public static final String CMAS_CERTAINTY = "cmas_certainty";
	public static final String SUB_ID = "sub_id";
	
	
	public static final String CREATE_BROADCAST =
			"CREATE TABLE " + TABLE_NAME + " ("
	                + _ID + " INTEGER PRIMARY KEY AUTOINCREMENT,"
	                + GEOGRAPHICAL_SCOPE + " INTEGER,"
	                + PLMN + " TEXT,"
	                + LAC + " INTEGER,"
	                + CID + " INTEGER,"
	                + SERIAL_NUMBER + " INTEGER,"
	                + SERVICE_CATEGORY + " INTEGER,"
	                + LANGUAGE_CODE + " TEXT,"
	                + MESSAGE_BODY + " TEXT,"
	                + DELIVERY_TIME + " INTEGER,"
	                + MESSAGE_READ + " INTEGER,"
	                + MESSAGE_FORMAT + " INTEGER,"
	                + MESSAGE_PRIORITY + " INTEGER,"
	                + ETWS_WARNING_TYPE + " INTEGER,"
	                + CMAS_MESSAGE_CLASS + " INTEGER,"
	                + CMAS_CATEGORY + " INTEGER,"
	                + CMAS_RESPONSE_TYPE + " INTEGER,"
	                + CMAS_SEVERITY + " INTEGER,"
	                + CMAS_URGENCY + " INTEGER,"
	                + CMAS_CERTAINTY + " INTEGER,"
	                + SUB_ID + " INTEGER);";
	
}
