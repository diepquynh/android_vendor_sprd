/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
precision mediump float;
varying vec2 vTextureCoord;
uniform sampler2D texture;

uniform vec2 texSize;
/*const float NumOfMosaic=16.0;*/
const float NumOfMosaic=8.0;

vec4 doMosaic (vec2 i) {
    vec2  coord = i * texSize;
    vec2 mosaicCoord   = floor(coord/NumOfMosaic) * NumOfMosaic;
    vec2 mosaicCoordInTexture= mosaicCoord/texSize;
    vec4 r = texture2D(texture, mosaicCoordInTexture);
    return r;
}

void main() {
    gl_FragColor=doMosaic (vTextureCoord);
}
