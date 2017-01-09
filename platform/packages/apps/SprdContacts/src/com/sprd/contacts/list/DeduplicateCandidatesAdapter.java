
package com.sprd.contacts.list;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import com.android.contacts.common.ContactPhotoManager;
import com.sprd.contacts.DeduplicationCandidate;
import com.android.contacts.R;

public class DeduplicateCandidatesAdapter extends
        GroupCheckAdapter<DeduplicationCandidate> {

    ContactPhotoManager mPhotoManager;

    public DeduplicateCandidatesAdapter(Context context) {
        super(context);
        mPhotoManager = ContactPhotoManager.getInstance(context);
    }

    @Override
    protected View newGroupItem(ViewGroup parent, DeduplicationCandidate item,
            LayoutInflater inflater) {
        View candidateView = inflater.inflate(R.layout.candidate_item_layout,
                parent, false);
        TextView nameText = (TextView) candidateView
                .findViewById(R.id.candidate_name);
        TextView numberText = (TextView) candidateView
                .findViewById(R.id.candidate_number);
        ImageView photo = (ImageView) candidateView
                .findViewById(R.id.candidate_photo);
        nameText.setText(item.mName);
        numberText.setText(item.mNumber);
        // accountNameText.setVisibility(isFirst ? View.VISIBLE : View.GONE);
        if (item.mPhotoId <= 0) {
            mPhotoManager.loadPhoto(photo, null, -1, false, true, null);
        } else {
            mPhotoManager.loadThumbnail(photo, item.mPhotoId, false, true, null);
        }

        return candidateView;
    }

    public class RawContactWitAccount {
        public long id;
        public long accountId;

        public RawContactWitAccount(long id, long accountId) {
            this.id = id;
            this.accountId = accountId;
        }

    }

    @Override
    protected RawContactWitAccount getSubItem(DeduplicationCandidate t) {
        return new RawContactWitAccount(t.mRawContactId, t.mAccountId);
    }

    @Override
    protected void setUpHeaderText(TextView text, DeduplicationCandidate item) {
        text.setText(item.mAccountName);
        text.setBackgroundResource(R.drawable.list_section_divider_holo_light_sprd);
    }

}
