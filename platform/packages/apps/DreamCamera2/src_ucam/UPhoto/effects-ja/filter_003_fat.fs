/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
precision mediump float;
varying vec2 vTextureCoord;
uniform sampler2D texture;
uniform float whratio;
uniform vec2 center;

const float strength = 0.8;

void main(void){
    vec2 coord = vTextureCoord ;
    float radius = max(whratio,1.0/whratio) * 0.5;
    coord -= center;
    float distance = length(coord);
    if (distance < radius) {
        float percent = distance / radius;
        if (strength > 0.0) {
            coord *= mix(1.0, smoothstep(0.0, radius / distance, percent), strength * 0.75);
        } else {
            coord *= mix(1.0, pow(percent, 1.0 + strength * 0.75) * radius / distance, 1.0 - percent);
        }
    }
    coord += center;

    gl_FragColor = texture2D(texture, coord);
    vec2 clampedCoord = clamp(coord, vec2(0.0), vec2(1.0));
    if (coord != clampedCoord) {
        /* fade to transparent if we are outside the image */
        gl_FragColor.a *= max(0.0, 1.0 - length(coord - clampedCoord));

     }
}
