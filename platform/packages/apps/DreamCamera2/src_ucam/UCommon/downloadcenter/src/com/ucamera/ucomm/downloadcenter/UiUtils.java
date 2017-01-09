/**
 * Copyright (C) 2010,2013 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucomm.downloadcenter;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.AlertDialog.Builder;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.Uri;
import android.os.Handler;
import android.util.DisplayMetrics;
import android.view.View;
import android.view.WindowManager;
import android.widget.HorizontalScrollView;
import android.widget.ListView;
import android.widget.SimpleAdapter;
import android.widget.TextView;
import android.widget.Toast;
import com.ucamera.ucomm.downloadcenter.R;

public abstract class UiUtils {
    public static final String KEY_CONFIRM_NETWORK_PERMISSION = "key_confirm_network_permission";
    public static final String KEY_FEATURE_NAME = "name";
    public static final int CONFIRM_NETWORK_RESULTS = 5;
    public static final int CONFIRM_NETWORK_RESULT = 6;
    public static final int NETWORK_FEATURE_TYPE_DOWNLOAD = 1;
    public static final int NETWORK_FEATURE_TYPE_SHARE = 2;
    public static final int NETWORK_FEATURE_TYPE_FEEDBACK = 3;
    public static final int NETWORK_FEATURE_TYPE_ABOUTUS = 4;
    public static final int NETWORK_FEATURE_TYPE_RATE_ME = 5;
    public static final int NETWORK_FEATURE_TYPE_HOT_APP = 6;
    private static int mScreenWidth;

    public static int dpToPixel(Context context, int dp) {
        DisplayMetrics metrics = new DisplayMetrics();
        WindowManager wm = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
        wm.getDefaultDisplay().getMetrics(metrics);
        float pixelDensity = metrics.density;
        return Math.round(pixelDensity * dp);
    }

    public static void confirmNetworkPermission(final Context context, final String[] features, final SharedPreferences sp, final Handler handler, final int type, final String flag) {
        List<Map<String, String>> featureDateList = new ArrayList<Map<String, String>>();
        Map<String, String> map = null;
        for(int i = 0;i < features.length;i++) {
            map = new HashMap<String, String>();
            map.put(KEY_FEATURE_NAME, features[i]);
            featureDateList.add(map);
        }

        Builder build = new AlertDialog.Builder(context);
        String title = ((Activity)context).getString(R.string.confirm_network_permission_title);
        build.setTitle(title);

        build.setNegativeButton(((Activity)context).getString(R.string.confirm_network_permission_disagree),new Dialog.OnClickListener() {
           @Override
            public void onClick(DialogInterface dialog, int which) {
               if(features != null && features.length > 1 && sp != null) {
                   SharedPreferences.Editor editor = sp.edit();
                   editor.putString(KEY_CONFIRM_NETWORK_PERMISSION, "off");
                   editor.commit();
               }
            }
        });
        build.setPositiveButton(((Activity)context).getString(R.string.confirm_network_permission_agree), new Dialog.OnClickListener() {
           @Override
            public void onClick(DialogInterface dialog, int which) {
               if(features != null && features.length == 1) {
                   if(type == NETWORK_FEATURE_TYPE_DOWNLOAD) {
                       startDownloadActivity(context,flag);
                   }else if(type == NETWORK_FEATURE_TYPE_SHARE) {
                       startSnsShare(context, Uri.parse(flag));
                   }else if(type == NETWORK_FEATURE_TYPE_FEEDBACK) {
                       startFeedback(context);
                   }else if(type == NETWORK_FEATURE_TYPE_ABOUTUS) {
                       startAboutUs(context);
                   }else if(type == NETWORK_FEATURE_TYPE_RATE_ME) {
                       startRateMe(context);
                   }else if(type == NETWORK_FEATURE_TYPE_HOT_APP) {
                       startHotApp(context);
                   }
                   if(handler != null) {
                       handler.sendEmptyMessage(CONFIRM_NETWORK_RESULT);
                   }
               }else if(features != null && features.length > 1 && sp != null) {
                   SharedPreferences.Editor editor = sp.edit();
                   editor.putString(KEY_CONFIRM_NETWORK_PERMISSION, "on");
                   editor.commit();
                   if(handler != null) {
                       handler.sendEmptyMessage(CONFIRM_NETWORK_RESULTS);
                   }
               }
            }
        });

        View contentView = ((Activity)context).getLayoutInflater().inflate(R.layout.dialog_view, null);
        TextView textView = (TextView) contentView.findViewById(R.id.confirm_content);
        String content = ((Activity)context).getString(R.string.confirm_network_permission_content);
        textView.setText(content);
        ListView listView = (ListView) contentView.findViewById(R.id.feature_list);
        SimpleAdapter adapter = new SimpleAdapter(context, featureDateList, R.layout.dialog_view_item, new String[]{KEY_FEATURE_NAME}, new int[]{R.id.item_name});
        listView.setAdapter(adapter);
        build.setView(contentView);
        build.setCancelable(false);
        build.show();
    }

    private static void startDownloadActivity(Context activity,String flag) {
        String density = Constants.EXTRA_DENSITY_HDPI;
        Intent intent = new Intent();
        intent.setClass(activity.getApplicationContext(), DownloadTabActivity.class);
        intent.putExtra(Constants.ACTION_DOWNLOAD_TYPE, flag);
//        intent.putExtra(Constants.ACTION_SCREEN_DENSITY, density);
        activity.startActivity(intent);
    }

    private static void startSnsShare(Context context, Uri uri) {
        Intent intent = new Intent();
        intent.setClassName(context, "com.ucamera.ucomm.sns.ShareActivity");
        intent.setAction("android.intent.action.UGALLERY_SHARE");
        intent.setDataAndType(uri, "image/*");
        try {
            context.startActivity(intent);
        } catch (ActivityNotFoundException e) {
            //Toast.makeText(context, R.string.text_activity_is_not_found,Toast.LENGTH_LONG).show();
        }
    }

    private static void startFeedback(Context context) {
        Intent intent = null;
        try {
            intent = new Intent(context, Class.forName(context.getPackageName()+".launcher.settings.MyFullDialogActivity"));
        } catch (ClassNotFoundException e) {
            e.printStackTrace();
        }
        intent.putExtra("DIALOG_STUB",1);
        context.startActivity(intent);
    }

    private static void startAboutUs(Context context) {
        Intent intent = null;
        try {
            intent = new Intent(context, Class.forName(context.getPackageName()+".launcher.settings.MyFullDialogActivity"));
        } catch (ClassNotFoundException e) {
            e.printStackTrace();
        }
        intent.putExtra("DIALOG_STUB",2);
        context.startActivity(intent);
    }

    private static void startRateMe(Context context) {
        if (isNetworkAvailable(context)) {
            try {
                Uri uri = null;
                uri = Uri.parse("market://details?id=" + context.getPackageName());
                context.startActivity(new Intent(Intent.ACTION_VIEW, uri));
            } catch (ActivityNotFoundException e) {
                Toast.makeText(context,
                        R.string.text_not_installed_market_app,
                        Toast.LENGTH_SHORT).show();
            }
        }
    }

    public static boolean isNetworkAvailable(Context context) {
        ConnectivityManager connManager =
                (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo networkInfo = connManager.getActiveNetworkInfo();
        if (networkInfo != null) {
            return networkInfo.isAvailable();
        } else {
            return false;
        }
    }

    public static void startHotApp(Context context) {
        try {
            context.startActivity(new Intent(context,Class.forName(context.getPackageName()+".hot.application.HotAppActivity.class")));
        } catch (ClassNotFoundException e) {
            e.printStackTrace();
        }
    }

    public static void scrollToCurrentPosition(HorizontalScrollView scroller, int itemWidth, int currentPosition){

        if(itemWidth > 0 && scroller != null ) {
            if(mScreenWidth == 0) {
                DisplayMetrics metrics = new DisplayMetrics();
                WindowManager wm = (WindowManager) scroller.getContext().getSystemService(Context.WINDOW_SERVICE);
                wm.getDefaultDisplay().getMetrics(metrics);
                mScreenWidth = metrics.widthPixels;
            }
           if(currentPosition > mScreenWidth/itemWidth) {
               scroller.scrollTo(itemWidth * currentPosition, scroller.getScrollY());
           } else {
               scroller.scrollTo(0, scroller.getScrollY());
           }
        }
    }
}
