/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
precision mediump float;
varying vec2 vTextureCoord;
uniform sampler2D texture;

vec4 blur(sampler2D texture,vec2 coord)
{
    vec2 offsets[9];
    offsets[0] = vec2(-0.0011, -0.0011) * 3.0;
    offsets[1] = vec2(0.0, -0.0011)* 2.0;
    offsets[2] = vec2(0.0011, -0.0011)* 3.0;
    offsets[3] = vec2(-0.0011, 0.0)* 2.0;
    offsets[4] = vec2(0.0, 0.0);
    offsets[5] = vec2(0.0011, 0.0)* 2.0;
    offsets[6] = vec2(-0.0011, 0.0011)* 3.0;
    offsets[7] = vec2(0.0, 0.0011)* 2.0;
    offsets[8] = vec2(0.0011, 0.0011)* 3.0;
    vec4 color =
        (texture2D(texture,coord + offsets[0]) + texture2D(texture,coord + offsets[2]) + texture2D(texture,coord + offsets[6]) + texture2D(texture,coord + offsets[8]))* 0.1216 +
        (texture2D(texture,coord + offsets[1]) + texture2D(texture,coord + offsets[3]) + texture2D(texture,coord + offsets[5]) + texture2D(texture,coord + offsets[7]))* 0.1946 +
        texture2D(texture,coord + offsets[0])* 0.2270;
    return color;
}

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

void main() {
   vec4 color = texture2D(texture,vTextureCoord);
   vec4 blurColor = blur(texture,vTextureCoord);
   gl_FragColor = blendOverlay(color, blurColor);
}