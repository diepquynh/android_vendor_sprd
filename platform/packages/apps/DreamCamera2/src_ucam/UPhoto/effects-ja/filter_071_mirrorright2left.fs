/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
precision mediump float;
varying vec2 vTextureCoord;
uniform sampler2D texture;

void main() {
    vec2 coord = vTextureCoord;
    if(coord.x < 0.5){
        coord.x = 1.0 - vTextureCoord.x;
    }
    gl_FragColor=texture2D(texture, coord);
}
