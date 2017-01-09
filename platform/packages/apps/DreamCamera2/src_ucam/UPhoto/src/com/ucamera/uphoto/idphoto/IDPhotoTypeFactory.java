/**
 * Copyright (C) 2014,2015 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto.idphoto;

public class IDPhotoTypeFactory {
    public static IDPhotoType getIdPhotoType(int type) {
        switch (type) {
        case IDPhotoTypeConst.TYPE1:
            return new IDPhotoType1();
        case IDPhotoTypeConst.TYPE2:
            return new IDPhotoType2();
        case IDPhotoTypeConst.TYPE3:
            return new IDPhotoType3();
        case IDPhotoTypeConst.TYPE4:
            return new IDPhotoType4();
        case IDPhotoTypeConst.TYPE5:
            return new IDPhotoType5();
        case IDPhotoTypeConst.TYPE6:
            return new IDPhotoType6();
        default:
            break;
        }
        return null;
    }
}
