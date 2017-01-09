package com.sprd.APCT;

import com.sprd.APCT.R;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.app.Activity;
import android.text.method.ScrollingMovementMethod;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.GestureDetector;

import android.view.GestureDetector.OnGestureListener;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnTouchListener;

import android.view.Window;
import android.view.WindowManager;
import android.widget.LinearLayout;
import android.widget.TextView;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.List;
import java.util.StringTokenizer;

public class APCTPsinfo extends Activity implements OnTouchListener, OnGestureListener {
	final String TAG = "APCT";
	private GestureDetector mDetector;
	private DisplayMetrics metrics;
	private TextView mTextView;
	private int mScreenWidth, mScreenHeight;
	private int mCurX = 0, mCurY = 0;

	final int COL_PID		= 0;
	final int COL_PPID		= 1;
	final int COL_VMPEAK	= 2;
	final int COL_VSS		= 3;
	final int COL_RSS		= 4;
	final int COL_PSS		= 5;
	final int COL_USS		= 6;
	final int COL_USER		= 7;
	final int COL_SWITCHS	= 8;	
	final int COL_STATE		= 9;
	final int COL_NAME 		= 10;
	final int COL_OOM_ADJ	= 11;	
	final int COL_NUMBER	= 12;
	String[] display_line = new String[COL_NUMBER];

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		requestWindowFeature(Window.FEATURE_NO_TITLE);
		getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
		setContentView(R.layout.psinfo);

		mTextView = (TextView)findViewById(R.id.psinfoText);
		mTextView.setText("Geting process info, please wait...");
		metrics = new DisplayMetrics();
		getWindowManager().getDefaultDisplay().getMetrics(metrics);
		mScreenWidth = metrics.widthPixels;
		mScreenHeight = metrics.heightPixels;

		//mTextView.setMovementMethod(ScrollingMovementMethod.getInstance());
		//mTextView.setGravity(Gravity.TOP);
		LinearLayout.LayoutParams mParams = (LinearLayout.LayoutParams) mTextView.getLayoutParams();
		mParams.width = mScreenWidth + 1500; //expand TextView width
		mParams.height = mScreenHeight;
		mTextView.setLayoutParams(mParams);

		mDetector = new GestureDetector(this);
		mTextView.setOnTouchListener(APCTPsinfo.this);
		mTextView.setLongClickable(true);

		thread.start();
	}
	private boolean ReadOomAdjAndUser() {
		FileReader fr = null;
		BufferedReader reader = null;
		String filename = "/proc/" + display_line[COL_PID] + "/oom_adj";
		Process p = null;
		String line = null;
		StringTokenizer stoken;
		try {
			fr = new FileReader(filename);
			reader = new BufferedReader(fr);
			display_line[COL_OOM_ADJ] = " (" + reader.readLine() + ")"; 
			fr.close();
			reader.close();
			p = Runtime.getRuntime().exec("ls -l " + filename);
			InputStreamReader is = new InputStreamReader(p.getInputStream());
			reader = new BufferedReader(is);
			line = reader.readLine();
			reader.close();
			is.close();
			//p.waitFor();
			if(line!=null) {
				stoken = new StringTokenizer(line);
				line = stoken.nextToken();
				line = stoken.nextToken();
				display_line[COL_USER] = line;
			}
		} catch (IOException e) {
			Log.d(TAG, "cannot open " + filename + e);
			return false;
		}
		return true;
	}
	private boolean ReadProcrankLine(StringTokenizer stoken) {
		String tokens;
		int count = stoken.countTokens();
		for( int j=0; j < count; j++) {
			tokens = stoken.nextToken().trim();
			//PID      Vss      Rss      Pss      Uss  cmdline
			switch (j) {
			case 0:
				display_line[COL_PID] = tokens;
				break;
			case 1: //
				display_line[COL_VSS] = tokens;
				break;
			case 2:
				display_line[COL_RSS] = tokens;
				break;
			case 3:
				display_line[COL_PSS] = tokens;
				break;
			case 4:
				display_line[COL_USS] = tokens;
				break;
			case 5:
				display_line[COL_NAME] = tokens;
				return true;
			}
		}
		return false;
	}
	private boolean ReadPidStatus() {
		String line = null;
		FileReader fr = null;
		BufferedReader reader = null;
		String filename = "/proc/" + display_line[COL_PID] + "/status";
		String str = "";
		try {
			fr = new FileReader(filename);
			reader = new BufferedReader(fr);

			line = "";
			while( (str=reader.readLine()) != null) {
				line += str + "\n";
			}
			reader.close();
			fr.close();
		} catch (IOException e) {
			Log.d(TAG, "cannot open " + filename + e);
			return false;
		}		
		StringTokenizer stoken = new StringTokenizer(line,"\n");
		line = null;
		int count = stoken.countTokens();
		long switches = 0, len = 0;

		for(int i=0; i < count; i++ ) {
			line = stoken.nextToken();
			if (line.startsWith("State")) {
				display_line[COL_STATE] = line.substring(7);
			} else if (line.startsWith("PPid")) {
				display_line[COL_PPID] = line.substring(6);
			} else if (line.startsWith("VmPeak")) {
				display_line[COL_VMPEAK] = line.substring(9).replace(" ", "");
			} else if (line.startsWith("voluntary")) {
				len = Long.parseLong(line.substring(25));
				switches  += len;
			} else if (line.startsWith("nonv")) {
				len = Long.parseLong(line.substring(28));
				switches  += len;
				display_line[COL_SWITCHS] = Long.toString(switches);
				break;
			}
		}
		return true;
	}

	private void PrepareTextView(StringBuffer viewtext, boolean isHead) {
		if (isHead) {
			viewtext.append("PID===PPID==VMPEAK======VSS=====RSS=====PSS=====USS=====USER=====SWITCHES====STATE==========NAME (oom_adj)\n");
			return;
		}
		int []col_width =   {6,    6,    12,         8,      8,      8,      10,      10,       12,       15,            30 , 0,0,0};
		int num=0, len = 0, width = 0;

		for (int i=0; i < COL_NUMBER; i++) {
			viewtext.append(display_line[i]);
			if(display_line[i]!=null) {
				len += display_line[i].length();
			} else {
				len += 4; //null
			}
			if(i==COL_NUMBER-1){
				break;
			}
			width += col_width[i];
			num = width - len;
			for (int j=0; j<num; j++) {
				viewtext.append("=");
				len++;
			}
		}
	}
	private Thread thread = new Thread() {
		@Override
		public void run() {
			Process p = null;
			List<String> row = new ArrayList<String>();
			String line = "";
			StringBuffer viewtext = new StringBuffer();

			try {
				p = Runtime.getRuntime().exec("procrank");

				//procrank
				//warning: could not create process interface for 14035
				//  PID      Vss      Rss      Pss      Uss  cmdline
				//  401   74444K   65072K   34294K   31760K  system_server
				InputStreamReader is = new InputStreamReader(p.getInputStream());
				BufferedReader in = new BufferedReader(is);
				while ((line = in.readLine()) != null) {
					row.add(line);
				}
				in.close();
				is.close();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}

			int size = row.size();
			for(int d=0; d < size;d++)
				Log.e(TAG, row.get(d));

			row.remove(size-1);
			row.remove(size-2);
			row.remove(size-3);
			row.remove(size-4);
			String str = null;
			size = row.size();
			for(int d=0; d < size;d++)
				Log.e(TAG, row.get(d));

			while( (str = row.get(0)) != null ) {
				if(str.indexOf("PID") >= 0) {
					row.remove(0);
					break;
				} else {
					row.remove(0);
				}
			}

			size = row.size();
			for(int d=0; d < size;d++)
				Log.e(TAG, row.get(d));

			for(int i=0; i< size; i++) {
				if(i%10==0) {
					viewtext.append("\n");
					PrepareTextView(viewtext, true);
					viewtext.append("\n");
				}
				if(i==size-1) {
					Log.e(TAG, row.get(i));
				}
				ReadProcrankLine(new StringTokenizer(row.get(i)));
				if (!ReadOomAdjAndUser() || !ReadPidStatus()){
					continue;
				}

				PrepareTextView(viewtext, false);
				viewtext.append("\n");
			}

			viewtext.append("_______________________________________________________________________________________________________________________\n \n");
			Message msg = handler.obtainMessage(0, viewtext.toString());
			viewtext.delete(0, viewtext.length()-1);
			handler.sendMessage(msg);
			Log.d(TAG, "----ps exit");
		}
	};

	private Handler handler = new Handler() {
		@Override
		public void handleMessage(Message msg) {
			String str;
			switch (msg.what) {
			case 0:
				str = (String)msg.obj;
				mTextView.setText(str);
				break;
			default:
				break;
			}
		}
	};

	@Override
	public boolean onTouch(View v, MotionEvent event) {
		// TODO Auto-generated method stub
		mDetector.onTouchEvent(event);
		return false;
	}

	@Override
	public boolean onDown(MotionEvent e) {
		// TODO Auto-generated method stub
		return false;
	}

	@Override
	public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX,
			float velocityY) {
		// TODO Auto-generated method stub
		return false;
	}

	@Override
	public void onLongPress(MotionEvent e) {
		// TODO Auto-generated method stub
	}

	@Override
	public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX,
			float distanceY) {
		// TODO Auto-generated method stub
		int mLayoutWidth	= mTextView.getLayoutParams().width; //TextView height
		int mLayoutHeight	= mTextView.getLineCount() * mTextView.getLineHeight(); //TextView content height

        if (mCurX + distanceX >= 0) {
            if (mCurX + distanceX > mLayoutWidth - mScreenWidth) {
                mCurX = mLayoutWidth - mScreenWidth;
            } else {
                mCurX = (int) (mCurX + distanceX);
            }
        } else {
            mCurX = 0;
        }

        if ((mCurY + distanceY >= 0) && (mLayoutHeight >= mScreenHeight) ) {
            if (mCurY + distanceY > mLayoutHeight - mScreenHeight) {
                mCurY = mLayoutHeight - mScreenHeight;
            } else {
                mCurY = (int) (mCurY + distanceY);
            }
        } else {
            mCurY = 0;
        }
        mTextView.scrollTo(mCurX, mCurY);
        return false;
	}

	@Override
	public void onShowPress(MotionEvent e) {
		// TODO Auto-generated method stub
	}

	@Override
	public boolean onSingleTapUp(MotionEvent e) {
		// TODO Auto-generated method stub
		return false;
	}
}
