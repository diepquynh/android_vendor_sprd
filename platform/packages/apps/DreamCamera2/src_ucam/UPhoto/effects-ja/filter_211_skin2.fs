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

float blendColorDodge(float f, float b){
    float result = f / (1.0 - b);
    return result;
}

float blendColorBurn(float f, float b){
    float result = 1.0 - (1.0 - f) / b;
    return result;
}

vec4 blendVividLight(vec4 fColor,vec4 bColor){
    vec4 r = vec4(1.0);
   if(bColor.x<0.5){
       r.x = blendColorBurn(fColor.x, bColor.x*2.0);
   }else{
       r.x = blendColorDodge(fColor.x, (bColor.x - 0.5)*2.0);
   }

   if(bColor.y<0.5){
       r.y = blendColorBurn(fColor.y, bColor.y*2.0);
   }else{
       r.y = blendColorDodge(fColor.y, (bColor.y - 0.5)*2.0);
   }

   if(bColor.z<0.5){
       r.z = blendColorBurn(fColor.z, bColor.z*2.0);
   }else{
       r.z = blendColorDodge(fColor.z, (bColor.z - 0.5)*2.0);
   }
   return r;
}

vec4 blendHardMix(vec4 fColor, vec4 bColor){
    vec4 r = vec4(1.0);
    vec4 vColor = blendVividLight(fColor, bColor);
    if(vColor.x<0.5){
        r.x = 0.0;
    }
    if(vColor.y<0.5){
        r.y = 0.0;
    }
    if(vColor.z<0.5){
        r.z = 0.0;
    }
    return r;
}

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

vec4 psHighPassFilter(vec4 srcColor,vec4 dstColor) {
    vec4 r = vec4(1.0);
    r.xyz = (srcColor.xyz + vec3(1.0) - dstColor.xyz) * 0.5;
    return r;
}

vec4 doColor(sampler2D resourceTexture, vec4 orig) {
    vec4 r=texture2D (resourceTexture,vec2(orig.r,0.0));
    vec4 g=texture2D (resourceTexture,vec2(orig.g,0.0));
    vec4 b=texture2D (resourceTexture,vec2(orig.b,0.0));
    return vec4 (r.r,g.r,b.r,1.0);
}

void main() {
   /*vec4 color = texture2D(texture, vTextureCoord);
   vec4 blurColor = blur(texture, vTextureCoord);
   color = psHighPassFilter(color, blurColor);
   color = blendHardMix(color, color);
   color = doColor(resourceTexture1, color);
   color = doColor(resourceTexture2, color);
   color = doColor(resourceTexture3, color);
   gl_FragColor = color;
   */
   vec4 color = texture2D(texture, vTextureCoord);
   color = doColor(resourceTexture1, color);
   color = doColor(resourceTexture2, color);
   color = doColor(resourceTexture3, color);
   gl_FragColor = color;
}