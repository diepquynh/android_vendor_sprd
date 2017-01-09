
package com.sprd.note;

import android.app.Activity;
import android.content.Context;
import android.widget.Toast;

import com.sprd.note.data.NoteDataManager;

public class BaseActivity extends Activity {

    /* SPRD: modify/add 20131122 Spreadtrum of 241526 maxLength toast display when it is not end 1000 @{ */
    private static int TOAST_SHOW_MAX_TIME = 3000;
    /* @} */
    public NoteDataManager getDataManager(Context context) {
        return ((NoteApplication) getApplication()).getNoteDataManager(context);
    }

    /* SPRD: modify/add 20131122 Spreadtrum of 241526 maxLength toast display when it is not end 1000  @{ */
    public Toast showMessage(Toast textLenghtToast, int maxLength) {
        if (textLenghtToast == null) {
            String toastMessage = BaseActivity.this.getString(
                    R.string.tilte_input_more, maxLength);
            textLenghtToast = Toast.makeText(BaseActivity.this, toastMessage,
                    Toast.LENGTH_SHORT);
        } else {
            textLenghtToast.setDuration(TOAST_SHOW_MAX_TIME);
        }
        textLenghtToast.show();
        return textLenghtToast;
    }
    /* @} */
}
