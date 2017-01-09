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

float blendSoftLight(float f, float b){
    float result = 0.0;
    if (b < 0.5) {
        result = 2.0 * f * b + f*b - 2.0 * f*f*b;
    } else {
        result = 2.0 * sqrt(f) * b - sqrt(f) + 2.0 * f - 2.0 * f*b;
    }
    return result;
}

vec4 blendSoftLightColor(vec4 fColor, vec4 bColor){
    float r = blendSoftLight(fColor.x, bColor.x);
    float g = blendSoftLight(fColor.y, bColor.y);
    float b = blendSoftLight(fColor.z, bColor.z);
    return vec4(r,g,b,1.0);
}

vec4 blendAlpha(vec4 fColor, vec4 bColor, float alpha) {
    return fColor * alpha + bColor * (1.0-alpha);
}

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
    vec4 color = texture2D(texture, vTextureCoord);
    vec4 dstColor = doColor(resourceTexture1, color);
    dstColor = doColorR(resourceTexture2, dstColor);
    dstColor = doColor(resourceTexture3, dstColor);
    dstColor = blendSoftLightColor(dstColor, color);
    dstColor = blendAlpha(dstColor, color, 0.5);
    dstColor.a = 1.0;
    gl_FragColor = dstColor;
}