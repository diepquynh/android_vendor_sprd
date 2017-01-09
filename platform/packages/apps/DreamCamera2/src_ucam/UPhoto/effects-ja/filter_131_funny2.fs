/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
precision mediump float;
varying vec2 vTextureCoord;
uniform sampler2D texture;
uniform sampler2D resourceTexture;

float blendChannel(float b, float f){
   float r = 0.0;
   if(b<0.5){
      r = 2.0*f*b;
   }else{
      r = 1.0-2.0*(1.0-f)*(1.0-b);
   }
   return r;
}

vec4 blendOverlay(vec4 bColor, vec4 fColor) {
   vec4 color = vec4(1.0);
   color.r = blendChannel(bColor.r, fColor.r);
   color.g = blendChannel(bColor.g, fColor.g);
   color.b = blendChannel(bColor.b, fColor.b);
   return color;
}

float getY(vec4 color){
   float y = 0.299*color.x+0.587*color.y+0.114*color.z;
   return y;
}

void main() {
   vec4 bColor = texture2D(texture, vTextureCoord);
   float y = getY(bColor);
   vec4 yColor = vec4(y,y,y,bColor.a);
   vec4 fColor = texture2D(resourceTexture, vTextureCoord);
    gl_FragColor = blendOverlay(yColor, fColor);
}