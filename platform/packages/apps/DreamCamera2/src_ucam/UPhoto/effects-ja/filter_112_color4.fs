/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
precision mediump float;
varying vec2 vTextureCoord;
uniform sampler2D texture;

void main(void){
    vec4 rgba = texture2D(texture, vTextureCoord);
    gl_FragColor = vec4(rgba.r,0.0,0.0,rgba.a);
}