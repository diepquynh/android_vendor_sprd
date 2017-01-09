/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
precision mediump float;
varying vec2 vTextureCoord;
uniform sampler2D texture;
uniform vec2 texSize;

/*const float pixelsize = 0.0011;*/

vec3 emboss(vec2 coord)
{
vec2 pixelsize = vec2(1.0/texSize.x, 1.0/texSize.y);
    float color = texture2D(texture,coord).r;
    float colorup = texture2D(texture,coord - pixelsize).r;
    float colordown = texture2D(texture,coord + pixelsize).r;
    return vec3(clamp(colorup * 2.0 - color - colordown + 0.5,0.0,1.0));
}
void main() {
    gl_FragColor=vec4(emboss(vTextureCoord),1.0);
}
