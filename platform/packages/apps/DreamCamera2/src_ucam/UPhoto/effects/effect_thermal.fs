/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
precision mediump float;
varying vec2 vTextureCoord;
uniform sampler2D texture;

void main(void){
    vec3 rgb = texture2D(texture, vTextureCoord).rgb;
    if(rgb.r > 0.5) {
        rgb.r = 1.0 - rgb.r;
    }
    if(rgb.g > 0.5) {
        rgb.g = 1.0 - rgb.g;
    }
    if(rgb.b > 0.5) {
        rgb.b = 1.0 - rgb.b;
    }

    gl_FragColor = vec4(rgb,1.0);
}
