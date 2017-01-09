/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
precision mediump float;

uniform vec2 rangeMax;
varying vec2 vTextureCoord;
uniform sampler2D texture;

void main() {
    vec2 coord = vTextureCoord;
    if(coord.x > rangeMax.x/2.0){
        coord.x = rangeMax.x - vTextureCoord.x;
    }
    float board = 0.01;
    coord = coord * (rangeMax - 2.0 * board)/rangeMax + board;
    gl_FragColor=texture2D(texture,coord);
}
