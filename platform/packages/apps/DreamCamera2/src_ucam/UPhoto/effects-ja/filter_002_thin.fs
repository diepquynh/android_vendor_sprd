/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
precision mediump float;
varying vec2 vTextureCoord;
uniform sampler2D texture;
uniform vec2 texSize;

const float strength = -0.4;
const float texCoordScaledFactor = 0.4;

void main() {
    vec2 texCoordCenter = vec2(0.5);
    float radius = max(texSize.x,texSize.y)*0.5;
    vec2 scaledTextureCoord=vTextureCoord-texCoordCenter;
    scaledTextureCoord*=texCoordScaledFactor;
    scaledTextureCoord+=texCoordCenter;
    vec2 coord = scaledTextureCoord * texSize;
    vec2 center = texCoordCenter * texSize;
    coord -= center;
    float distance = length(coord);
    if (distance < radius) {
        float percent = distance / radius;
        coord *= mix(1.0, pow(percent, 1.0 + strength * 0.75) * radius / distance,  1.0-percent);
    }

    coord += center;
    vec2 clampedCoord = clamp(coord, vec2(0.0), texSize);

    vec2 uv = clampedCoord / texSize;

    gl_FragColor=texture2D(texture, uv);
}
