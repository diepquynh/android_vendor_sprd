
package com.spreadst.validator;

import com.spreadst.validator.R;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnCancelListener;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.ListView;
import android.widget.TextView;
import android.util.Log;
import android.content.SharedPreferences;

public class UIDialog extends Activity {

    private static final String TAG = "Validator_UIDialog";

    public static final String ACTION_ID = "id";

    public static final String RE_TEST = "re_test";

    public static final String CURRENT_APP = "current_app";

    public static final int DIALOG_BEGIN = 0;

    public static final int DIALOG_CONFIRM = 1;

    public static final int DIALOG_END = 2;

    public static final int DIALOG_FINISH = 3;

    private SelectAdapter mArrayAdapter;

    private View mTestSlectView = null;

    private CheckBox mSelectALL = null;

    private ListView mListViewItem = null;

    private AlertDialog mBeginDialog = null;

    private AlertDialog mConfirmDialog = null;

    private AlertDialog mSelectTestDialog = null;

    public static final int VIDEO_TEST = 0;

    public static final int CAM_DC = 1;

    public static final int CAM_DV = 2;

    public static final int BENCHMARK = 3;

    public static final int VOICE_CYCLE = 4;

    public static final int TEL_GSM = 5;

    public static final int TEL_TD = 6;

    public static final int TEL_LTE = 7;

    public static final int BT = 8;

    public static final int WIFI = 9;

    private boolean mSelectTestCase[] = new boolean[ValidateService.APP_END - 2];

    public static int mTestResult[] = new int[ValidateService.APP_END - 2];

    public static final int SUCCESS = 1;

    public static final int FAIL = 2;

    public static final String BOOT_COMPLETE_START = "boot_complete_start";

    public static final String SLT_VALUE = "slt_value";

    public boolean finishActivity;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        finishActivity = false;

        int id = getIntent().getIntExtra(ACTION_ID, -1);

        Log.d(TAG, "onCreat getIntent: id = " + id);
        switch (id) {
            case DIALOG_END:
                Log.d(TAG, "onCreate DIALOG_END");
                translateUserChoose(ValidateService.BUTTON_NO);
                finish();
                break;
            case DIALOG_FINISH:
                Log.d(TAG, "onCreate DIALOG_FINISH");
                translateUserChoose(ValidateService.BUTTON_CANCEL);
                finish();
                break;
            case -1:
                translateUserChoose(ValidateService.USER_START_APK);
                finish();
                break;
            default:
                showDialog(id);
                break;
        }
    }

    @Override
    protected Dialog onCreateDialog(int id) {
        Log.d(TAG, "Dialog type: id = " + id);
        switch (id) {
            case DIALOG_BEGIN:
                return createBeginDialog();
            case DIALOG_CONFIRM:
                return createConfirmDialog();
            default:
                this.finish();
                break;
        }
        return null;
    }

    private AlertDialog createBeginDialog() {
        if (mBeginDialog == null) {
            mBeginDialog = new AlertDialog.Builder(UIDialog.this)
            .setIcon(R.drawable.alert_dialog_icon)
            .setTitle(R.string.chroose_running_dialog_title)
            .setPositiveButton(R.string.alert_dialog_yes,
                    new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int whichButton) {
                            /*
                             * Start service to do YES action , start the
                             * first app.
                             */
                            mBeginDialog.dismiss();
                            SharedPreferences sltValue = getSharedPreferences(SLT_VALUE, Context.MODE_PRIVATE);
                            boolean bootStart = sltValue.getBoolean(BOOT_COMPLETE_START, false);
                            Log.d(TAG, "createBeginDialog BUTTON_OK, bootStart = " + bootStart);
                            if (bootStart) {
                                selectTestCase();//add for select test case
                            } else {
                                showBootCompleteStartDialog();
                            }
                        }
                    })
            .setNegativeButton(R.string.alert_dialog_no, new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int whichButton) {
                    translateUserChoose(ValidateService.BUTTON_CANCEL);
                    Log.d(TAG, "createBeginDialog BUTTON_CANCEL");
                    mBeginDialog.dismiss();
                    finish();
                }
            }).create();
        }
        mBeginDialog.show();

        mBeginDialog.setCanceledOnTouchOutside(false);
        mBeginDialog.setOnCancelListener(new OnCancelListener() {
            @Override
            public void onCancel(DialogInterface dialog) {
                Log.d(TAG, "createBeginDialog BUTTON_CANCEL");
                mBeginDialog.dismiss();
                translateUserChoose(ValidateService.BUTTON_CANCEL);
                finish();
            }
        });

        return mBeginDialog;
    }

    private AlertDialog createConfirmDialog() {
        View confirmView = getLayoutInflater().inflate(R.layout.confirm_dialog_view, null);
        Button cancelButton = (Button) confirmView.findViewById(R.id.confirm_cancel);
        cancelButton.setOnClickListener(new View.OnClickListener() {

            @Override
            public void onClick(View v) {
                Log.d(TAG, "createConfirmDialog cancelButton onClick");
                mConfirmDialog.dismiss();
                startTestResultActivity();
                finish();
            }
        });

        Button retryButton = (Button) confirmView.findViewById(R.id.confirm_retry);
        retryButton.setOnClickListener(new View.OnClickListener() {

            @Override
            public void onClick(View v) {
                /*
                 * Start service to do RETRY action for current
                 * app test and restart current app
                 */
                mConfirmDialog.dismiss();
                translateUserChoose(ValidateService.BUTTON_RETRY);
                finish();
                Log.d(TAG, "createConfirmDialog BUTTON_RETRY");
            }
        });

        Button failButton = (Button) confirmView.findViewById(R.id.confirm_fail);
        failButton.setOnClickListener(new View.OnClickListener() {

            @Override
            public void onClick(View v) {
                /*
                 * Start service to do fail action for current
                 * app test and start next app
                 */
                mConfirmDialog.dismiss();
                int currentApp = getIntent().getIntExtra(CURRENT_APP, 0);
                mTestResult[currentApp - 1] = FAIL;
                translateUserChoose(ValidateService.BUTTON_NEXT);
                finish();
                Log.d(TAG, "createConfirmDialog BUTTON_NEXT");
            }
        });

        Button passButton = (Button) confirmView.findViewById(R.id.confirm_pass);
        passButton.setOnClickListener(new View.OnClickListener() {

            @Override
            public void onClick(View v) {
                /*
                 * Start service to do PASS action for current
                 * app test and start next app
                 */
                mConfirmDialog.dismiss();
                int currentApp = getIntent().getIntExtra(CURRENT_APP, 0);
                mTestResult[currentApp - 1] = SUCCESS;
                translateUserChoose(ValidateService.BUTTON_NEXT);
                finish();
                Log.d(TAG, "createConfirmDialog BUTTON_NEXT");
            }
        });

        mConfirmDialog = new AlertDialog.Builder(UIDialog.this)
                .setIcon(R.drawable.alert_dialog_icon)
                .setTitle(R.string.chroose_test_result_dialog_title)
                .setMessage(R.string.chroose_test_result_dialog_msg)
                .setView(confirmView)
                .create();
                /*.setPositiveButton(R.string.alert_dialog_pass,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int whichButton) {
                                *
                                 * Start service to do PASS action for current
                                 * app test and start next app
                                 *
                                translateUserChoose(ValidateService.BUTTON_NEXT);
                                finish();
                                Log.i(TAG, "createConfirmDialog BUTTON_NEXT");
                            }
                        })
                .setNeutralButton(R.string.alert_dialog_retry,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int whichButton) {
                                *
                                 * Start service to do RETRY action for current
                                 * app test and restart current app
                                 *
                                translateUserChoose(ValidateService.BUTTON_RETRY);
                                finish();
                                Log.i(TAG, "createConfirmDialog BUTTON_RETRY");
                            }
                        })
                .setNegativeButton(R.string.alert_dialog_cancel,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int whichButton) {
                                *
                                 * Start service to do CANCEL action for current
                                 * app test and cancel test app.
                                 *
                                translateUserChoose(ValidateService.BUTTON_CANCEL);
                                finish();
                                Log.i(TAG, "createConfirmDialog BUTTON_NO");
                            }
                        }).create();*/
        mConfirmDialog.setCanceledOnTouchOutside(false);
        mConfirmDialog.setOnCancelListener(new OnCancelListener() {
            @Override
            public void onCancel(DialogInterface dialog) {
                mConfirmDialog.dismiss();
                translateUserChoose(ValidateService.STOP_RECEIVER);
                Log.d(TAG, "createConfirmDialog STOP_RECEIVER");
                finish();
            }
        });
        mConfirmDialog.show();

        return mConfirmDialog;
    }

    private void translateUserChoose(int choose) {
        Intent intent = new Intent(UIDialog.this, ValidateService.class);
        intent.putExtra(ValidateService.ID, choose);
        Log.d(TAG,"Sylar.wang ValidateService.ID"+ ValidateService.ID);
        startService(intent);
    }

    private void translateUserChooseCase(int choose) {
        boolean reTest = getIntent().getBooleanExtra(RE_TEST, false);
        Intent intent = new Intent(UIDialog.this, ValidateService.class);
        intent.putExtra(ValidateService.ID, choose);
        Log.d(TAG,"Sylar.wang ValidateService.ID"+ ValidateService.ID);
        intent.putExtra(ValidateService.TEST_CASE, mSelectTestCase);
        if (reTest) {
            intent.putExtra(ValidateService.RE_TEST, true);
        }
        for (int i = 0; i < mTestResult.length; i++) {
            mTestResult[i] = 0;
        }
        startService(intent);
    }

    public static final int mListItem[] = {
        R.string.video_test,
        R.string.cam_dc,
        R.string.cam_dv,
        R.string.bench_mark,
        R.string.voice_cycle,
        R.string.tel_gsm,
        R.string.tel_td,
        R.string.tel_lte,
        R.string.bt,
        R.string.wifi
    };

    private void selectTestCase() {
        Log.d(TAG, "selectTestCase");

        mTestSlectView = getLayoutInflater().inflate(R.layout.test_select_view, null);
        mListViewItem = (ListView) mTestSlectView.findViewById(R.id.test_select);
        mSelectALL = (CheckBox) mTestSlectView.findViewById(R.id.check_all);
        mSelectALL.setOnClickListener(new View.OnClickListener() {

            @Override
            public void onClick(View v) {
                if (mSelectALL.isChecked()) {
                    mSelectALL.setChecked(true);
                    for (int i = 0; i < mArrayAdapter.getCount(); i++) {
                        mSelectTestCase[i] = true;
                    }
                    mArrayAdapter.notifyDataSetChanged();
                } else {
                    mSelectALL.setChecked(false);
                    for (int i = 0; i < mArrayAdapter.getCount(); i++) {
                        mSelectTestCase[i] = false;
                    }
                    mArrayAdapter.notifyDataSetChanged();
                }
            }
        });

        mArrayAdapter = new SelectAdapter(mListItem);

        mListViewItem.setAdapter(mArrayAdapter);
        mListViewItem.setChoiceMode(ListView.CHOICE_MODE_MULTIPLE);
        mListViewItem.setOnItemClickListener(mOnItemClickListener);

        mSelectTestDialog = new AlertDialog.Builder(UIDialog.this)
        .setTitle(R.string.select_test_case)
        .setView(mTestSlectView)
        .setPositiveButton(R.string.alert_dialog_confirm,
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                        mSelectTestDialog.dismiss();
                        translateUserChooseCase(ValidateService.BUTTON_OK);
                        finish();
            }
        })
        .setNegativeButton(R.string.alert_dialog_cancel, new DialogInterface.OnClickListener() {

            @Override
            public void onClick(DialogInterface dialog, int which) {
                Log.d(TAG, "mSelectTestDialog cancelButton onClick ");
                mSelectTestDialog.dismiss();
                translateUserChoose(ValidateService.BUTTON_CANCEL);
                finish();
            }
        })
        .create();
        mSelectTestDialog.setCanceledOnTouchOutside(false);
        mSelectTestDialog.setOnCancelListener(new OnCancelListener() {
            @Override
            public void onCancel(DialogInterface dialog) {
                Log.d(TAG, "mSelectTestDialog BUTTON_CANCEL");
                mSelectTestDialog.dismiss();
                translateUserChoose(ValidateService.BUTTON_CANCEL);
                finish();
            }
        });
        mSelectTestDialog.show();
    }

    private class SelectAdapter extends BaseAdapter {
        private int[] mListString;

        public SelectAdapter(int[] listStrings) {
            mListString = listStrings;
        }

        @Override
        public int getCount() {
            return mListString.length;
        }

        @Override
        public Object getItem(int position) {
            return position;
        }

        @Override
        public long getItemId(int position) {
            return position;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            ViewHolder viewHolder = null;
            if (convertView == null) {
                convertView = getLayoutInflater().inflate(R.layout.item_select_view, null);
                viewHolder = new ViewHolder();
                viewHolder.mTextView = (TextView) convertView.findViewById(R.id.test_title);
                viewHolder.mCheckBox = (CheckBox) convertView.findViewById(R.id.check_item);
                convertView.setTag(viewHolder);
            } else {
                viewHolder = (ViewHolder) convertView.getTag();
            }
            viewHolder.mTextView.setText(getString(mListString[position]));
            if (mSelectTestCase[position]) {
                viewHolder.mCheckBox.setChecked(true);
                viewHolder.mTextView.setTextColor(getResources().getColor(android.R.color.white));
            } else {
                viewHolder.mCheckBox.setChecked(false);
                viewHolder.mTextView.setTextColor(getResources().getColor(android.R.color.darker_gray));
            }
            return convertView;
        }
    }

    private final class ViewHolder {
            CheckBox mCheckBox;
            TextView mTextView;
    }

    private OnItemClickListener mOnItemClickListener = new OnItemClickListener() {
        @Override
        public void onItemClick(AdapterView l, View v, int position, long id) {
            Log.d(TAG, "onItemClick, position ="+position);
            ViewHolder viewHolder = (ViewHolder) v.getTag();
            viewHolder.mCheckBox.toggle();
            if (mSelectTestCase[position]) {
                mSelectTestCase[position] = false;
                viewHolder.mTextView.setTextColor(getResources().getColor(android.R.color.darker_gray));
            } else {
                mSelectTestCase[position] = true;
                viewHolder.mTextView.setTextColor(getResources().getColor(android.R.color.white));
            }

            boolean foudFalse = false;
            for (int i = 0; i < mSelectTestCase.length; i++) {
                if (mSelectTestCase[i] == false) {
                    mSelectALL.setChecked(false);
                    foudFalse = true;
                    break;
                }
            }
            if (!foudFalse) {
                mSelectALL.setChecked(true);
            }
        }
    };

    private void startTestResultActivity() {
        Log.d(TAG,"startTestResultActivity");
        Intent i = new Intent();
        i.setClass(UIDialog.this, TestResultActivity.class);
        startActivity(i);
    }

    private void showBootCompleteStartDialog() {
        Log.d(TAG, "showBootCompleteStartDialog");
        AlertDialog bootCompleteDialog = new AlertDialog.Builder(UIDialog.this)
        .setIcon(R.drawable.alert_dialog_icon)
        .setTitle(R.string.chroose_boot_dialog_title)
        .setPositiveButton(R.string.alert_dialog_yes,
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                        SharedPreferences sltValue = getSharedPreferences(SLT_VALUE, Context.MODE_PRIVATE);
                        sltValue.edit().putBoolean(BOOT_COMPLETE_START, true).commit();
                        selectTestCase();
                        Log.d(TAG, "showBootCompleteStartDialog BUTTON_OK");
                    }
                })
        .setNegativeButton(R.string.alert_dialog_no, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int whichButton) {
                SharedPreferences sltValue = getSharedPreferences(SLT_VALUE, Context.MODE_PRIVATE);
                sltValue.edit().putBoolean(BOOT_COMPLETE_START, false).commit();
                selectTestCase();
                Log.d(TAG, "showBootCompleteStartDialog BUTTON_CANCEL");
            }
        }).create();

        bootCompleteDialog.show();

        bootCompleteDialog.setCanceledOnTouchOutside(false);
        bootCompleteDialog.setOnCancelListener(new OnCancelListener() {
        @Override
        public void onCancel(DialogInterface dialog) {
            Log.d(TAG, "createBeginDialog BUTTON_CANCEL");
            translateUserChoose(ValidateService.BUTTON_CANCEL);
            finish();
        }
    });
    }
}
