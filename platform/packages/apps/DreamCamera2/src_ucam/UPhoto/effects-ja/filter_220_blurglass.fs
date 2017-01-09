/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
precision mediump float;
varying vec2 vTextureCoord;
uniform sampler2D texture;

const float radius = 8.0;
const float resolution = 1024.0;
const vec2 dir = vec2(1.0,0.0);

void main() {
vec4 sum = vec4(0.0);

vec2 tc = vTextureCoord;

float blur = radius/resolution;

float hstep = dir.x;
float vstep = dir.y;

sum += texture2D(texture, vec2(tc.x - 4.0*blur*hstep, tc.y - 4.0*blur*vstep)) * 0.0162162162;
sum += texture2D(texture, vec2(tc.x - 3.0*blur*hstep, tc.y - 3.0*blur*vstep)) * 0.0540540541;
sum += texture2D(texture, vec2(tc.x - 2.0*blur*hstep, tc.y - 2.0*blur*vstep)) * 0.1216216216;
sum += texture2D(texture, vec2(tc.x - 1.0*blur*hstep, tc.y - 1.0*blur*vstep)) * 0.1945945946;

sum += texture2D(texture, vec2(tc.x, tc.y)) * 0.2270270270;

sum += texture2D(texture, vec2(tc.x + 1.0*blur*hstep, tc.y + 1.0*blur*vstep)) * 0.1945945946;
sum += texture2D(texture, vec2(tc.x + 2.0*blur*hstep, tc.y + 2.0*blur*vstep)) * 0.1216216216;
sum += texture2D(texture, vec2(tc.x + 3.0*blur*hstep, tc.y + 3.0*blur*vstep)) * 0.0540540541;
sum += texture2D(texture, vec2(tc.x + 4.0*blur*hstep, tc.y + 4.0*blur*vstep)) * 0.0162162162;

gl_FragColor = vec4(sum.rgb, 1.0);
}
