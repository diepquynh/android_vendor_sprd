/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
precision mediump float;
varying vec2 vTextureCoord;
uniform sampler2D texture;
const float alpha = 0.8;

void main(void){
    vec3 foreground = vec3(30.0,93.0,255.0)/255.0;

    vec4 color = texture2D(texture, vTextureCoord);

    vec3 dstRgb = color.rgb * foreground;

    vec3 finalRgb = color.rgb * alpha + dstRgb*(1.0-alpha);

    gl_FragColor = vec4(finalRgb, color.a);
}