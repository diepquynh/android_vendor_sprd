/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
precision mediump float;
varying vec2 vTextureCoord;
uniform sampler2D texture;

void main(void){
    vec3 rgb = texture2D(texture, vTextureCoord).rgb;
    gl_FragColor = vec4(1.0 - rgb,1.0);
}
