
package com.sprd.note.utils;

import android.content.Context;
import android.content.Intent;

import com.sprd.note.R;
import com.sprd.note.data.NoteItem;

public class IntentUtils {

    public static final void sendSharedIntent(Context context, NoteItem item) {
        Intent intent = new Intent(Intent.ACTION_SEND);
        // SPRD: Modify for bug441261. @{
        intent.setType("text/plain");

        String titleKey = context.getString(R.string.export_title);
        String contentKey = context.getString(R.string.export_content);

        StringBuilder sb = new StringBuilder();
        sb.append(titleKey).append(item.getShareTitle(context)).append(',');
        sb.append(contentKey).append(item.getShareContent(context));

        intent.putExtra(Intent.EXTRA_TEXT, sb.toString());
        intent.putExtra(Intent.EXTRA_SUBJECT, context.getString(R.string.share));
        context.startActivity(Intent.createChooser(intent, context.getString(R.string.share) + ":"
                + item.getShortTitle()));
    }
}
