/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
precision mediump float;
varying vec2 vTextureCoord;
uniform sampler2D texture;

void main(void){
    float a = 1.0/4.0;
    float b = a/10.0;
    float pi = 3.1416;
    vec2 newCoord = vTextureCoord;
    newCoord.x +=sin(vTextureCoord.y*2.0*pi/a)*b ;
    newCoord.y +=sin(vTextureCoord.x*2.0*pi/a)*b ;
    newCoord *= (1.0 -2.0 * b);
    newCoord += b;
    gl_FragColor = texture2D(texture,newCoord);
}
