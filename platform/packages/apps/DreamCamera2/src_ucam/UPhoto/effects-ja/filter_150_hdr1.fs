/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
precision mediump float;
varying vec2 vTextureCoord;
uniform sampler2D texture;

float blendColorDodge(float f, float b){
    float result = f / (1.0 - b);
    return result;
}

float blendColorBurn(float f, float b){
    float result = 1.0 - (1.0 - f) / b;
    return result;
}


void main() {
   float r =0.0;
   float g =0.0;
   float b =0.0;
   vec4 color = texture2D(texture,vTextureCoord);
   if(color.x<0.5){
       r = blendColorBurn(color.x, color.x*2.0);
   }else{
       r = blendColorDodge(color.x, (color.x - 0.5)*2.0);
   }

   if(color.y<0.5){
       g = blendColorBurn(color.y, color.y*2.0);
   }else{
       g = blendColorDodge(color.y, (color.y - 0.5)*2.0);
   }

   if(color.z<0.5){
       b = blendColorBurn(color.z, color.z*2.0);
   }else{
       b = blendColorDodge(color.z, (color.z - 0.5)*2.0);
   }
   gl_FragColor = vec4(r,g,b,1.0);
}