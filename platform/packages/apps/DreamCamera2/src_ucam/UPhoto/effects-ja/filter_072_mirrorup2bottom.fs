/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
precision mediump float;
varying vec2 vTextureCoord;
uniform sampler2D texture;

void main() {
    vec2 coord = vTextureCoord;
    if(coord.y > 0.5){
        coord.y = 1.0 - vTextureCoord.y;
    }
    gl_FragColor=texture2D(texture, coord);
}
