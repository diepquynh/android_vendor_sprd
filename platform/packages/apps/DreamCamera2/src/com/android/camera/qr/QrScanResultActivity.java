
package com.android.camera.qr;

import android.app.ActionBar;
import android.app.Activity;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.os.Bundle;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;
import android.media.MediaFile;

import com.android.camera.debug.Log;
import com.android.camera.util.ToastUtil;
import com.android.camera2.R;

public class QrScanResultActivity extends Activity {
    public static final Log.Tag TAG = new Log.Tag("QrScanResultActivity");
    private TextView mTextView;
    private ImageView mImageView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_qrcode_main);

        ActionBar actionBar = getActionBar();
        actionBar.setDisplayUseLogoEnabled(false);
        actionBar.setDisplayShowHomeEnabled(false);
        actionBar.setDisplayShowTitleEnabled(true);
        actionBar.setDisplayHomeAsUpEnabled(true);
        actionBar.setTitle(R.string.scan_result);

        mTextView = (TextView) findViewById(R.id.result);
        mImageView = (ImageView) findViewById(R.id.qrcode_bitmap);
        Button mOnlyScanButton = (Button) findViewById(R.id.only_cancel_button);

        mOnlyScanButton.setOnClickListener(new OnClickListener() {

            @Override
            public void onClick(View view) {
                QrScanResultActivity.this.finish();
                // Update flash parameters.
            }
        });

        Button mScanButton = (Button) findViewById(R.id.cancel_button);
        mScanButton.setOnClickListener(new OnClickListener() {

            @Override
            public void onClick(View view) {
                QrScanResultActivity.this.finish();
            }
        });

        Button mAccessLinkButton = (Button) findViewById(R.id.next_button);
        mAccessLinkButton.setOnClickListener(new OnClickListener() {

            @Override
            public void onClick(View view) {
                Intent intent = getIntent();
                Bundle bundle = intent.getExtras();
                try {
                    // for test case
                    /*String https = "https://www.baidu.com";
                    String http = "http://www.baidu.com";
                    String geo = "geo:38.899533,-77.036476";
                    String file = "file:///sdcard/二零一二.mp3";
                    String mailto = "mailto:yangjg@spreadst.com";
                    String tel = "tel:10086";
                    String sms = "smsto://10086";
                    Uri uri = Uri.parse(file);*/

                    /**
                     * NOTE: we should better DO NOT LOG THE RESULT OF SCAN, because it maybe contains
                     * use's private information, such as pay link, telephone number, and so on.
                     * if we log it , it maybe reported by other application to it's service for debugging
                     */
                    final Uri uri = Uri.parse(bundle.getString("result"));
                    String scheme = uri.getScheme();
                    Log.d(TAG, "onClick: scheme = " + scheme);
                    switch (scheme) {
                        case "https":
                        case "http":
                        case "geo":
                            final Intent it = new Intent(Intent.ACTION_VIEW, uri);
                            startActivity(it);
                            break;
                        case "file":
                            final Intent fileIntent = new Intent(Intent.ACTION_VIEW);
                            fileIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                            // unfortunately, the MediaFile is a hide interface in media at present
                            MediaFile.MediaFileType fileType = MediaFile.getFileType(uri.getPath());
                            String mimeType = fileType == null ? null : fileType.mimeType;
                            fileIntent.setDataAndType(uri, mimeType);
                            startActivity(fileIntent);
                            break;
                        case "mailto":
                            final Intent mail = new Intent(Intent.ACTION_SENDTO, uri);
                            startActivity(mail);
                            break;
                        case "tel":
                            final Intent telIntent = new Intent(Intent.ACTION_DIAL, uri);
                            startActivity(telIntent);
                            break;
                        case "smsto":
                            final Intent smsIntent = new Intent(Intent.ACTION_SENDTO, uri);
                            startActivity(smsIntent);
                            break;
                        case "package":
                            // TODO: add app installer and unInstaller
                            throw new Exception("Package scheme Uri can't resolve exception, URI ="
                                    + uri.toString());
                        default:
                            if (!uri.isAbsolute()) {
                                /*
                                 TODO : relative uri dose't has scheme, if the path contains "www"
                                 or ".com" or other suffix, we should better try give it a http scheme.
                                 */
                                throw new Exception("Can not resolve relative uri, URI = " + uri.toString());
                            } else {
                                // TODO: we should disable link button and tell user it can not resolve
                                throw new Exception("Uri can't resolve exception, URI = " + uri.toString());
                            }
                    }
                } catch (Exception e) {
                    ToastUtil.showToast(QrScanResultActivity.this, R.string.qrcode_can_not_resolve,
                            ToastUtil.LENGTH_LONG);
                    Log.e(TAG, "can't handle the intent: " + e.getMessage());
                }
            }
        });

        Intent intent = getIntent();
        Bundle bundle = intent.getExtras();
        String resultString = bundle.getString("result");

        Bitmap bitmapSafe = BitmapFactory.decodeResource(getApplicationContext().getResources(),
                R.drawable.ic_scan_result_safe);
        Bitmap bitmapFail = BitmapFactory.decodeResource(getApplicationContext().getResources(),
                R.drawable.ic_scan_result_fail);

        if (resultString.equals("defaulturi")) {
            mTextView.setText(getApplicationContext().getResources().getString(
                    R.string.qrcode_result));
            mTextView.setTextColor(getApplicationContext().getResources().getColor(
                    R.color.qrcode_url));
            mImageView.setImageBitmap(bitmapFail);
            mAccessLinkButton.setVisibility(View.GONE);
            mScanButton.setVisibility(View.GONE);
            mOnlyScanButton.setVisibility(View.VISIBLE);
        } else {
            mTextView.setText(bundle.getString("result"));
            mImageView.setImageBitmap(bitmapSafe);
            mOnlyScanButton.setVisibility(View.GONE);
        }
    }

    @Override
    public boolean onMenuItemSelected(int featureId, MenuItem item) {
        int itemId = item.getItemId();
        if (itemId == android.R.id.home) {
            finish();
            return true;
        }
        return true;
    }

}
