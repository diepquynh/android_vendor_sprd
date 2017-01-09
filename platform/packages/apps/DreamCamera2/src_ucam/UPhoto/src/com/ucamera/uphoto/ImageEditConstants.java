/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto;

public class ImageEditConstants {
    public static final String ITEM_TITLE = "edit.function.item.title";
    public static final String ITEM_ICON = "edit.function.item.icon";

    public static final String EXTRA_FROM_INNER = "extra_from_inner";
    public static final int ACTION_SAVED_NOT_CROP = -1;
    public static final int ACTION_SAVED_FAILED = -2;
    public static final int ACTION_SAVED_SUCUESS = 2;
    public static final int ACTION_UNLOAD_SUCUESS = 3;
    public static final int ACTION_UNLOAD_FAILED = -3;
    public static final int ACTION_REDO_ICON_NOT_FOCUSED = -10;
    public static final int ACTION_REDO_ICON_FOCUSED = 10;
    public static final int ACTION_UNDO_ICON_NOT_FOCUSED = -11;
    public static final int ACTION_UNDO_ICON_FOCUSED = 11;
    public static final int ACTION_PREVIEW = 0;
    public static final int ADJUST_BITMAP = 12;
    public static final int INVISIBLE_ADJUST_CONTROL = 13;
    public static final int ACTION_SHARE_DIALOG = 14;
    public static final int UPDATE_RES_DECOR = 15;
    public static final int UPDATE_RES_FRAME = 17;
    public static final int UPDATE_RES_TEXTURE = 18;
    public static final int ACTION_TAB_LABEL_UPDATE = 19;
    public static final int LABEL_INPUT_TEXT_MESSAGE = 20;
    public static final int MAKEUP_FACE_DETECT = 21;
    public static final int MAKEUP_PROCESS_RESULT = 22;
    public static final int MAKEUP_PROCESS_RESULT_WARN_1 = 23;
    public static final int MAKEUP_RESET_PARAM = 24;
    public static final int UPDATE_LABEL_BITMAP = 25;
    // finish GIF edit activity message
    public static final int NO_MEMORY_TO_CLOSE_EDIT_ACTIVITY = 16;
    public static final int GIF_CHANGE_POSITION = 25;
    public static final int HANDLER_MSG_SHOW_DLG = 26;
    public static final int MAKEUP_GET_PREVIEW_SIZE = 27;
    public static final int BACKGROUND_DISMISS_DIALOG = 28;
    public static final int BACKGROUND_SHOW_DIALOG = 29;
    public static final int SMART_CUT_VIEW_FINISH = 30;
    public static final int UPDATE_RES_MOSAIC = 31;
    public static final int CHANGE_LIKED_EFFECT = -888;
    public static final int EXCHANGE_LIKED_EFFECT = -889;
    public static final int EFFECT_SELECTED_BACK = -890;

    public static final String ACTION_EXECUTE = "execute";
    public static final String BASIC_EDIT_FUNCTION_ACTION_CROP ="edit.action.function.crop";
    public static final String ACTION_PREVIEW_RECEIVERD = "preview";
    public static String ACTION_DECORVIEW_DELETED_RECEIVERD = "decorview.action.delete";
    public static String ACTION_DECORVIEW_SURE_RECEIVERD = "decorview.action.sure";
    protected static final String ACTION_GRAFFITI_CAN_BE_SAVE_RECEIVERD = "graffitiview.action.save";
    protected static final String ACTION_GRAFFITI_CAN_BE_SAVE_CANCEL_RECEIVERD = "graffitiview.action.save.cancel";
    protected static final String ACTION_GRAFFITI_SELECT_COLOR_RECEIVERD = "graffitiview.action.select_color";
    public static final String ITEM_NO_ACTION = "no action";
    protected static final String EXTRA_SELECT_COLOR = "extra_select_color";
    protected static final String ACTION_FROM_GRAFFITI = "action_from_graffiti";
    protected static final String ACTION_FROM_TEXT_BUBBLE = "action_from_text_bubble";

    public final static int EDIT_VIEW_SIZE_SHORT = 600;
    public final static int EDIT_VIEW_SIZE_LONG = 800;


    public final static int FLIP_VERTICAL = 1;
    public final static int FLIP_HORIZONTAL = -1;
    public final static int ROTATE_CLOCKWISE = -90;
    public final static int ROTATE_UNCLOCKWISE = 90;
    public final static int NOTHING_ADJUST = -1;
    public final static int CONTRAST_ADJUST = 0;
    public final static int LIGHT_ADJUST = 1;
    public final static int SATURATION_ADJUST = 2;

    public final static String PREF_UPHOTO_PICTURE_SIZE_KEY = "pref_uphoto_picture_size_key";
    public final static String PREF_EVENT_NEW_CHECK_TIME = "pref_event_new_check_time";
    public final static String PREF_EVENT_NEW_UPDATE_TIME = "pref_event_new_update_time";

    public static final int BALLOON_STYLE_OVAL = 0;
    public static final int BALLOON_STYLE_EXPLOSION = 1;
    public static final int BALLOON_STYLE_CLOUND = 2;
    public static final int BALLOON_STYLE_RECT = 3;
    public static final int BALLOON_STYLE_CHAMFER = 4;

    public static final String LABEL_TAG_TITLE = "Title";
    public static final String LABEL_TAG_BALLOON = "Balloon";
    public static final String LABEL_TAG_LABEL = "Label";

    public static final int LABEL_TAG_TITLE_INDEX = 0;
    public static final int LABEL_TAG_BALLOON_INDEX = 1;
    public static final int LABEL_TAG_LABEL_INDEX = 2;

    public static final int LABEL_TITLE_SHAPE = -1;
    public static final int LABEL_LABEL_SHAPE = -2;

    public static final int BRUSH_ITEM_CANVAS_COLOR = 0; //canvas
    public static final int BRUSH_ITEM_CLEAR = 1;  //clear
    public static final int BRUSH_ITEM_ERASER = 2; //eraser
    public static final int BRUSH_ITEM_AUTO = 3;   //auto
    public static final int BRUSH_ITEM_CUSTOM = 4; //custom

    public static final int EFFECT_LOMO          = 0;
    public static final int EFFECT_HDR           = 1;
    public static final int EFFECT_SKIN         = 2;
    public static final int EFFECT_VIVID_LIGHT   = 3;
    public static final int EFFECT_SKETCH        = 4;
    public static final int EFFECT_COLORFULL     = 5;
    public static final int EFFECT_FUNNY         = 6;
    public static final int EFFECT_NOSTALGIA     = 7;
    public static final int EFFECT_BLACKWHITE    = 8;
    public static final int EFFECT_DEFORM        = 9;

  //makeup method
    public static final String MAKEUP_METHOD_SOFTENFACE = "handleSoftenface";
    public static final String MAKEUP_METHOD_WHITEFACE = "handleWhiteface";
    public static final String MAKEUP_METHOD_DEBLEMISH = "handleDeblemish";
    public static final String MAKEUP_METHOD_TRIMFACE = "handleTrimface";
    public static final String MAKEUP_METHOD_BIGEYE = "handleBigeye";

    public static final int MAKEUP_MODE_NONE       = 0x00;
    public static final int MAKEUP_MODE_SOFTENFACE = 0x03;
    public static final int MAKEUP_MODE_WHITENFACE = 0x02;
    public static final int MAKEUP_MODE_DEBLEMISH  = 0x01;
    public static final int MAKEUP_MODE_TRIMFACE   = 0x04;
    public static final int MAKEUP_MODE_BIGEYE     = 0x05;

    public static final String MAKEUP_EXTRA_FACE_NUM = "face_num";
    public static final String MAKEUP_EXTRA_FACE_RECT = "face_rect";
    public static final String MAKEUP_EXTRA_EYE_POINTS = "eye_points";
    public static final String MAKEUP_EXTRA_MOUTH_POINT = "mouth_point";
    public static final String MAKEUP_EXTRA_RESULT = "process_bitmap";

    public static final int MAKEUP_MSG_REDO_ENABLE = 0x01;

    private ImageEditConstants(){}
}
