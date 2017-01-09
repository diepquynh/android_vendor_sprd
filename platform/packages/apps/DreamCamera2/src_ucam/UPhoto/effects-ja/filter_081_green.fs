/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
precision mediump float;
varying vec2 vTextureCoord;
uniform sampler2D texture;

void main(void){

    float Rc = 130.0;
    float Gc = 220.0;
    float Bc = 81.0;
    float U = (Rc * -.168736 + Gc * -.331264 + Bc *  .500000 + 128.0)/256.0;
    float V = (Rc *  .500000 + Gc * -.418688 + Bc * -.081312 + 128.0)/256.0;

    float y = (texture2D(texture, vTextureCoord).r -0.0627)* 1.164;
    vec2 uv = vec2(U,V)- 0.5;
    vec3 rgb = vec3( 1.596 * uv.x, - 0.813 * uv.x - 0.391 * uv.y, 2.018 * uv.y) + y;

    gl_FragColor = vec4(rgb,1.0);

}
