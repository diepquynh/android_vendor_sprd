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

void main(void){
    vec3 rgb = vec3(0.0,255,200.0)/255.0;
    vec4 rgba = texture2D(texture, vTextureCoord);
    float y = getY(rgba);
    if(y>0.5){
        rgba.r = rgb.r;
        rgba.g = rgb.g;
        rgba.b = rgb.b;
    }else{
        rgba.r = 0.0;
        rgba.g = 0.0;
        rgba.b = 0.0;
    }
    gl_FragColor = rgba;
}