/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
precision mediump float;

varying highp vec2 vTextureCoord;
uniform sampler2D texture;
uniform sampler2D texture2;
uniform sampler2D resourceTexture;
uniform float type;

const vec2 center = vec2(0.5);
vec4 doVignette (vec4 orig) {
    vec3 rgb=orig.xyz;
    float d=distance(vTextureCoord, center);
    float total = distance(vec2(0.0),center);
    rgb*=smoothstep(0.0,0.5,(total - d)/total);

    return vec4(vec3(rgb),1.0);

}

vec4 doColor (vec4 orig) {
    vec4 r=texture2D (resourceTexture,vec2(orig.r,type));
    vec4 g=texture2D (resourceTexture,vec2(orig.g,type));
    vec4 b=texture2D (resourceTexture,vec2(orig.b,type));
    return vec4 (r.r,g.g,b.b,1.0);
}
void main() {

    vec4 orig=texture2D(texture, vTextureCoord);
    orig=doVignette(orig);
    gl_FragColor=doColor (orig);

}