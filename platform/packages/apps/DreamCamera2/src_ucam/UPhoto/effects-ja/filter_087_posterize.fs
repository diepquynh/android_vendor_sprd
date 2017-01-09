/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
precision mediump float;
varying vec2 vTextureCoord;
uniform sampler2D texture;

void main(void){
    vec4 baseColor = texture2D(texture,vTextureCoord);
    vec3 rgb = vec3(baseColor.r,baseColor.g,baseColor.b);

    float level = 4.0;
    rgb = (floor(rgb * (level-1.0)+0.5)/(level -1.0)*255.0)/256.0;

    gl_FragColor = vec4(rgb,baseColor.a);
}
