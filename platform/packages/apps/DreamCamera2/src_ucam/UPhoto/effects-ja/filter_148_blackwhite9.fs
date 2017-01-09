/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
precision mediump float;
varying vec2 vTextureCoord;
uniform sampler2D texture;
uniform sampler2D resourceTexture;

float getY(vec4 color){
   float y = 0.25*color.x+0.5*color.y+0.25*color.z;
   return y;
}

vec4 doColor (vec4 orig) {
    vec4 r=texture2D (resourceTexture,vec2(orig.r,0.0));
    vec4 g=texture2D (resourceTexture,vec2(orig.g,0.0));
    vec4 b=texture2D (resourceTexture,vec2(orig.b,0.0));
    return vec4 (r.r,g.r,b.r,1.0);
}

void main() {
   vec4 color = texture2D(texture, vTextureCoord);
   float y = getY(color);
   gl_FragColor = doColor(vec4(y));
}