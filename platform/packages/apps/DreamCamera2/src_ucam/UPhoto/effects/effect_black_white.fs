/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
precision mediump float;
varying vec2 vTextureCoord;
uniform sampler2D texture;

void main(void){

    gl_FragColor = vec4(vec3(texture2D(texture, vTextureCoord).r),1.0);

}
