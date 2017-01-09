package com.android.camera.data;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;

import com.android.camera.data.FilmstripItemAttributes.Attributes;
import com.android.camera.debug.Log;
import com.android.camera2.R;
import com.google.common.base.Optional;

import javax.annotation.Nonnull;

public class GifItem extends PhotoItem {
    private static final FilmstripItemAttributes PHOTO_ITEM_GIF_ATTRIBUTES =
            new FilmstripItemAttributes.Builder()
                    .with(Attributes.CAN_SHARE)
                    .with(Attributes.CAN_DELETE)
                    .with(Attributes.CAN_SWIPE_AWAY)
                    .with(Attributes.CAN_ZOOM_IN_PLACE)
                    .with(Attributes.HAS_DETAILED_CAPTURE_INFO)
                    .with(Attributes.IS_IMAGE)
                    .build();

    public GifItem(Context context, GlideFilmstripManager manager, PhotoItemData data,
                   PhotoItemFactory photoItemFactory) {
        super(context, manager, data, photoItemFactory, PHOTO_ITEM_GIF_ATTRIBUTES);
    }
}