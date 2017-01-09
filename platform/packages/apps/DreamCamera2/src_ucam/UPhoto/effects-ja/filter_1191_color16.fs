/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
precision mediump float;
varying vec2 vTextureCoord;
uniform sampler2D texture;

float getY(vec4 color){
   float y = 0.299*color.x+0.587*color.y+0.114*color.z;
   return y;
}

vec3 rgb2hsv(vec3 color) {
     float r = color.r;
     float g = color.g;
     float b = color.b;
     float h = 0.0;
     float s = 0.0;
     float v = 0.0;

     float min = min( min(r, g), b );
     float max = max( max(r, g), b );
     v = max;

     float delta = max - min;

     if( max != 0.0 )
         s = delta / max;
     else {
         s = 0.0;
         h = -1.0;
         return vec3(h, s, v);
     }
     if( r == max )
         h = ( g - b ) / delta;
     else if( g == max )
         h = 2.0 + ( b - r ) / delta;
     else
         h = 4.0 + ( r - g ) / delta;

     h = h * 60.0;

     if( h < 0.0 )
         h += 360.0;

     return vec3(h, s, v);
 }

void main(void){
    float hueStart = 120.0;
    float hueEnd = 140.0;
    bool b = false;
    vec4 color = texture2D(texture, vTextureCoord);
    vec3 hsv = rgb2hsv(color.xyz);
    if(hueStart > hueEnd){
        if(hsv.x > hueStart || hsv.x < hueEnd){
            b = true;
        }
    }else{
       if(hsv.x > hueStart && hsv.x < hueEnd){
            b = true;
        }
    }
    if(b==false){
        float y = getY(color);
        color.x = y;
        color.y = y;
        color.z = y;
    }
    gl_FragColor = color;
}