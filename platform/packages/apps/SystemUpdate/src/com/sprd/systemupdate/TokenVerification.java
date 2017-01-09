package com.sprd.systemupdate;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;
import android.os.SystemProperties;

import org.apache.http.HttpHost;
import org.apache.http.HttpResponse;
import org.apache.http.HttpStatus;
import org.apache.http.NameValuePair;
import org.apache.http.client.entity.UrlEncodedFormEntity;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.message.BasicNameValuePair;
import org.json.JSONObject;

import android.content.Context;
import android.os.Build;
import android.util.Log;

public class TokenVerification {

	private Context mContext;
	private Storage mStorage;
	public String mDeviceId;
	public String mProduct;
	public String mVersion;
	
	private static final String TOKEN_CONSTANT = "453278";
	private static final String FILENAME_MSV = "/sys/board_properties/soc/msv";
	public static final String NO_SEED = "no_seed";
	public static final String SEED_EXPRIED = "seed_expired";
	public static final String CONNECT_ERROR = "connect_error";
	public static final String FIRST_REGISTER = "first_register";
	public static final String GET_TOKEN_NO_INFO = "get_token_no_info";
	
	private static final String TAG = "TokenVerfication";
	
	public TokenVerification(Context context) {
		mContext = context;
		mStorage = Storage.get(context);
		mDeviceId = getDeviceId();
		mProduct = Build.MODEL + getMsvSuffix();
		//mVersion = Build.DISPLAY;
		mVersion = SystemProperties.get("ro.build.description");
	}
	
	public static String getMD5(String seed) throws NoSuchAlgorithmException {
		if(seed==null){
			return null;
		}
		MessageDigest md5 = MessageDigest.getInstance("MD5");
		md5.update(seed.getBytes());
		byte[] b = md5.digest();
		StringBuffer sb = new StringBuffer();
		for (int i = 0; i < b.length; i++) {
			int temp = 0xFF & b[i];
			String s = Integer.toHexString(temp);
			if (temp <= 0x0F) {
				s = "0" + s;
			}
			sb.append(s);
		}
		return sb.toString();
	}

	private String getradom() {
		String radom=null;
		int status;
		
		List<NameValuePair> pairs = new ArrayList<NameValuePair>();
		pairs.add(new BasicNameValuePair("version", mVersion));
		pairs.add(new BasicNameValuePair("product", mProduct));
		pairs.add(new BasicNameValuePair("jid", mDeviceId));

		DefaultHttpClient client = new DefaultHttpClient();
		try {
			HttpPost post = new HttpPost("/request/request_token");
			post.setEntity(new UrlEncodedFormEntity(pairs));
			HttpResponse response = client.execute(new HttpHost(PushService.SERVER_ADDR, 3000), post);
			if (response.getStatusLine().getStatusCode() != HttpStatus.SC_OK) {
				Log.e(TAG, "StatusCode" + response.getStatusLine().getStatusCode());
				return CONNECT_ERROR;
			}
			BufferedReader reader = new BufferedReader(new InputStreamReader(response.getEntity().getContent()));
			String json = reader.readLine();
			JSONObject result = new JSONObject(json);
			status = result.getInt("status");
			if(result.length()>1){
				radom=mDeviceId + TOKEN_CONSTANT + result.getString("seed");
			}
			int deal_result = ErrorStatus.DealStatus(mContext, status, radom);		
			if(deal_result!=0){
				return GET_TOKEN_NO_INFO;
			}		

		} catch (Exception e) {
			Log.e(TAG, "getradom()--Exception:" + e.getMessage());
			return CONNECT_ERROR;
		}
		return radom;
	}

	public String getToken(String seed) throws NoSuchAlgorithmException {
		String val = null;
		String storetoken = mStorage.getToken();
		
		if (seed.equals(NO_SEED)|| seed.equals(SEED_EXPRIED) || seed.equals(FIRST_REGISTER)) {
			if (seed.equals(NO_SEED)) {
				if (storetoken != null) {
					return storetoken;
				}
			}
			val = getradom();
		} else {
			val = seed;
		}
		if (val.equals(GET_TOKEN_NO_INFO) || val.equals(CONNECT_ERROR)) {
			return val;
		}
		String token = getMD5(val);
		mStorage.setToken(token);
		clearToken();
		return token;
	}

	private void clearToken() {
		ScheduledExecutorService executor = Executors.newSingleThreadScheduledExecutor();
		Runnable runner = new Runnable() {
			@Override
			public void run() {
				mStorage.setToken(null);
			}
		};
		executor.schedule(runner, 1, TimeUnit.HOURS);
	}

	private String getDeviceId() {
		String storedId = mStorage.getDeviceId();
		if (storedId != null) {
			return storedId;
		}

  	    String deviceId = UUID.randomUUID().toString();
		mStorage.setDeviceId(deviceId);

		return deviceId;
	}

	private String getMsvSuffix() {
		try {
			String msv = readLine(FILENAME_MSV);
			if (Long.parseLong(msv, 16) == 0) {
				return " (ENGINEERING)";
			}
		} catch (IOException ioe) {
		} catch (NumberFormatException nfe) {
		}
		return "";
	}

	private String readLine(String filename) throws IOException {
		BufferedReader reader = new BufferedReader(new FileReader(filename), 256);
		try {
			return reader.readLine();
		} finally {
			reader.close();
		}
	}

}
