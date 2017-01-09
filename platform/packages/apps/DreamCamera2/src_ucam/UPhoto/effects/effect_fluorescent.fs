/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
 
precision mediump float;
/*
varying vec2 vTextureCoord;
uniform sampler2D texture;
*/
varying vec2 vTextureCoord;
uniform sampler2D texture;

float kernel[9];
vec2 offset[9];
float edgeDetection(){
    vec2 offsets[9];
    offsets[0] = vec2(-0.0011, -0.0011);
    offsets[1] = vec2(0.0, -0.0011);
    offsets[2] = vec2(0.0011, -0.0011);
    offsets[3] = vec2(-0.0011, 0.0);
    offsets[4] = vec2(0.0, 0.0);
    offsets[5] = vec2(0.0011, 0.0);
    offsets[6] = vec2(-0.0011, 0.0011);
    offsets[7] = vec2(0.0, 0.0011);
    offsets[8] = vec2(0.0011, 0.0011);
    float wx[9];
    wx[0] = -1.0;wx[1] = -2.0;wx[2] = -1.0;
    wx[3] =  0.0;wx[4] =  0.0;wx[5] =  0.0;
    wx[6] =  1.0;wx[7] =  2.0;wx[8] =  1.0;
    float wy[9];
    wy[0] = -1.0;wy[1] =  0.0;wy[2] =  1.0;
    wy[3] = -2.0;wy[4] =  0.0;wy[5] =  2.0;
    wy[6] = -1.0;wy[7] =  0.0;wy[8] =  1.0;
    vec4 sumx = vec4(0.0);
    vec4 sumy = vec4(0.0);
    vec4 texColor;
    texColor = texture2D(texture, vTextureCoord + offsets[0]);
   sumx += wx[0] * texColor;
   sumy += wy[0] * texColor;
   texColor = texture2D(texture, vTextureCoord + offsets[1]);
   sumx += wx[1] * texColor;
   sumy += wy[1] * texColor;
   texColor = texture2D(texture, vTextureCoord + offsets[2]);
   sumx += wx[2] * texColor;
   sumy += wy[2] * texColor;
   texColor = texture2D(texture, vTextureCoord + offsets[3]);
   sumx += wx[3] * texColor;
   sumy += wy[3] * texColor;
   texColor = texture2D(texture, vTextureCoord + offsets[4]);
   sumx += wx[4] * texColor;
   sumy += wy[4] * texColor;
   texColor = texture2D(texture, vTextureCoord + offsets[5]);
   sumx += wx[5] * texColor;
   sumy += wy[5] * texColor;
   texColor = texture2D(texture, vTextureCoord + offsets[6]);
   sumx += wx[6] * texColor;
   sumy += wy[6] * texColor;
   texColor = texture2D(texture, vTextureCoord + offsets[7]);
   sumx += wx[7] * texColor;
   sumy += wy[7] * texColor;
   texColor = texture2D(texture, vTextureCoord + offsets[8]);
   sumx += wx[8] * texColor;
   sumy += wy[8] * texColor;
    return clamp((abs(sumx.r) + abs(sumy.r )), 0.0, 1.0);
}                         void main(void){   vec4 sum = vec4(0.0);
   vec3 color00 = vec3(1,0,0);
   vec3 color01 = vec3(0,1,0);
   vec3 color10 = vec3(1,0,0);
   vec3 color11 = vec3(0,1,0);
   vec3 colorBg = vec3(0,0,0);
   vec3 colorup = (color01- color00)*vTextureCoord.x + color00;
   vec3 colordown = (color11 - color10) * vTextureCoord.x + color10;
   vec3 color = (colordown - colorup) * vTextureCoord.y + colorup;
      float grad = edgeDetection();
    sum.a = 1.0;
    sum.rgb = abs(grad)*(color-colorBg) + colorBg;
   gl_FragColor = sum;
}
