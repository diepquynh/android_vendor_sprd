/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
precision mediump float;
varying vec2 vTextureCoord;
uniform sampler2D texture;
uniform float angle;
uniform vec2 center;
uniform float whratio;

void main() {
        vec2 coord = vTextureCoord;
        coord -= center;
        float distance = length(coord);
        float radius = max(whratio,1.0/whratio) * 0.5;
        if (distance < radius) {
            float percent = (radius - distance) / radius;
            float theta = percent * percent * angle;
            float s = sin(theta);
            float c = cos(theta);
            coord = vec2(
                coord.x * c - coord.y * s,
                coord.x * s + coord.y * c
            );
        }
        coord += center;
        gl_FragColor = texture2D(texture, coord);
        vec2 clampedCoord = clamp(coord, vec2(0.0), vec2(1.0));
        if (coord != clampedCoord) {
            /* fade to transparent if we are outside the image */
            gl_FragColor.a *= max(0.0, 1.0 - length(coord - clampedCoord));
        }}
