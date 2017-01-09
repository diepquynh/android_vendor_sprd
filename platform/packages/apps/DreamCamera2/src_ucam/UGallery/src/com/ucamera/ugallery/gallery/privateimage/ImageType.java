package com.ucamera.ugallery.gallery.privateimage;
/**
 * Copyright (C) 2010,2013 Thundersoft Corporation
 * All rights Reserved
 */
public enum ImageType {

    PNG(0, "image/png", "png", "pngucam", "FFD8FF", "96AAA2"),
    JPG(1, "image/jpeg", "jpg", "jpgucam", "FFD8FF", "96AAA2"),
    JPEG(2, "image/jpeg", "jpeg", "jpgucam", "FFD8FF", "96AAA2"),
    GIF(3, "image/gif", "gif", "gifucam", "47494638", "6D33649D"),
    UNKNOWN(4, "application/octet-stream", "unknown", "unknown", "000000", "000000"),
    PNG_LOCKED(10, "image/png", "pngucam", "pngucam", "834DAEBB", "834DAEBB"),
    JPG_LOCKED(20, "image/jpeg", "jpgucam", "jpgucam", "96AAA2", "96AAA2"),
    GIF_LOCKED(30, "image/gif", "gifucam", "gifucam", "6D33649D", "6D33649D");

    public final String extension;
    public final String extensionLocked;
    public final String header;
    public final String headerLocked;
    public final String mimetype;
    public final int value;

    private ImageType(int paramString2, String paramString3,
            String paramString4, String paramString5, String arg7, String arg8) {
        this.value = paramString2;
        this.extension = paramString4;
        this.extensionLocked = paramString5;
        this.mimetype = paramString3;
        this.header = arg7;
        this.headerLocked = arg8;
    }
}
