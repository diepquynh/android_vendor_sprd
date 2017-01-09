/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
precision mediump float;
varying vec2 vTextureCoord;
uniform sampler2D texture;

const vec3 one = vec3(1.0);
void main() {
   vec4 color = texture2D(texture, vTextureCoord);
   gl_FragColor = vec4(one-color.rgb,color.a);
}