/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
precision highp float;
varying vec2 vTextureCoord;
uniform sampler2D texture;
uniform vec2 imgSize;

void main(void){

    float NumMosaic=80.0;
    float size = 1.0/NumMosaic;
    vec2 Pbase = vTextureCoord - mod(vTextureCoord, vec2(size));
    vec2 PCenter;
    PCenter = Pbase + vec2(size)/2.0;
    float delL = length(vTextureCoord - PCenter);

    /*if(delL < 0.4*size) {*/
        gl_FragColor = texture2D(texture,vTextureCoord);
    /*}*/
    if(delL > 0.4*size) {
        gl_FragColor = texture2D(texture,PCenter);
    }

}
