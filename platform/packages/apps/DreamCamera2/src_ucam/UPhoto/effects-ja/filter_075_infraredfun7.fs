/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
precision mediump float;
varying vec2 vTextureCoord;
uniform sampler2D texture;

float getY(vec4 color){
   float y = 0.299*color.x+0.587*color.y+0.114*color.z;
   return y;
}

vec3 infrared(float gray)
{
    vec3 rgb;
    if(gray < 0.5){
        rgb.r = gray * 2.0;
    }else {
        rgb.r = 1.0 -  (gray - 0.5) *2.0;
    }
    if(gray < 0.5){
        rgb.g = 0.0;
    }else {
        rgb.g = 1.0 -  gray *2.0;
    }
    if(gray < 0.5){
        rgb.b = 1.0 - (gray - 0.5) * 2.0;
    }else {
        rgb.b = 0.0;
    }
    return rgb;
}
void main() {
    vec4 color = texture2D(texture, vTextureCoord);
    float y = getY(color);
    gl_FragColor=vec4(infrared(y),1.0);
}
