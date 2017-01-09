/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
precision mediump float;
varying vec2 vTextureCoord;
uniform sampler2D texture;
uniform sampler2D resourceTexture1;
uniform sampler2D resourceTexture2;
uniform sampler2D resourceTexture3;

vec4 doColorR(sampler2D resourceTexture, vec4 orig) {
    vec4 r=texture2D (resourceTexture,vec2(orig.r,0.0));
    vec4 g=texture2D (resourceTexture,vec2(orig.g,0.0));
    vec4 b=texture2D (resourceTexture,vec2(orig.b,0.0));
    return vec4 (r.r,g.r,b.r,1.0);
}

vec4 doColor(sampler2D resourceTexture, vec4 orig) {
    vec4 r=texture2D (resourceTexture,vec2(orig.r,0.0));
    vec4 g=texture2D (resourceTexture,vec2(orig.g,0.0));
    vec4 b=texture2D (resourceTexture,vec2(orig.b,0.0));
    return vec4 (r.r,g.g,b.b,1.0);
}

void main() {
   vec4 color = texture2D(texture,vTextureCoord);
   color = doColorR(resourceTexture1, color);
   color = doColorR(resourceTexture2, color);
   color = doColor(resourceTexture3, color);
   gl_FragColor = color;
}