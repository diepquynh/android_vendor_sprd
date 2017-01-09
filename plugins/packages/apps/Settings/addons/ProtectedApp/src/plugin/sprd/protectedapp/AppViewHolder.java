package plugin.sprd.protectedapp;

import plugin.sprd.protectedapp.util.ApplicationsState;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.ToggleButton;


// View Holder used when displaying views
public class AppViewHolder {
    public ApplicationsState.AppEntry entry;
    public View rootView;
    public TextView appName;
    public ImageView appIcon;
    public TextView summary;
    public TextView disabled;
    public Switch toggleButton;

    static public AppViewHolder createOrRecycle(LayoutInflater inflater, View convertView) {
        if (convertView == null) {
            convertView = inflater.inflate(R.layout.security_preference_app, null);
            inflater.inflate(R.layout.widget_text_views,
                    (ViewGroup) convertView.findViewById(android.R.id.widget_frame));

            AppViewHolder holder = new AppViewHolder();
            holder.rootView = convertView;
            holder.appName = (TextView) convertView.findViewById(android.R.id.title);
            holder.appIcon = (ImageView) convertView.findViewById(android.R.id.icon);
            holder.summary = (TextView) convertView.findViewById(R.id.widget_text1);
            holder.disabled = (TextView) convertView.findViewById(R.id.widget_text2);
            holder.toggleButton = (Switch) convertView.findViewById(R.id.security_toggle);
            convertView.setTag(holder);
            return holder;
        } else {
            // Get the ViewHolder back to get fast access to the TextView
            // and the ImageView.
            return (AppViewHolder)convertView.getTag();
        }
    }

    void updateSizeText(CharSequence invalidSizeStr, int whichSize) {
        if (entry.sizeStr != null) {
            switch (whichSize) {
                case SecurityManageFragment.SIZE_INTERNAL:
                    summary.setText(entry.internalSizeStr);
                    break;
                case SecurityManageFragment.SIZE_EXTERNAL:
                    summary.setText(entry.externalSizeStr);
                    break;
                default:
                    summary.setText(entry.sizeStr);
                    break;
            }
        } else if (entry.size == ApplicationsState.SIZE_INVALID) {
            summary.setText(invalidSizeStr);
        }
    }
}