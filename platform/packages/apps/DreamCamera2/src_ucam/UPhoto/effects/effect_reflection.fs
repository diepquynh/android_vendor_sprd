/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
precision mediump float;
varying vec2 vTextureCoord;
uniform sampler2D texture;

void main(void)
{
   vec2 normCoord = vTextureCoord;
   float u,v;
   if(normCoord.x < 0.5 && normCoord.y < 0.5){
      normCoord = vTextureCoord * 2.0;
      u = 0.4246;
      v=0.5753;
   }else if(normCoord.x > 0.5 && normCoord.y < 0.5){
      normCoord.x = (vTextureCoord.x -0.5) * 2.0 ;
      normCoord.y = vTextureCoord.y  * 2.0 ;
      u = 0.2870;
      v = 0.3678;
   }else if(normCoord.x < 0.5 && normCoord.y > 0.5){
      normCoord.x = vTextureCoord.x * 2.0 ;
      normCoord.y = (vTextureCoord.y - 0.5) * 2.0 ;
      u = 0.5671;
      v = 0.8282;
   }else {
      normCoord = (vTextureCoord - 0.5) * 2.0;
      u = 0.5845;
      v = 0.4362;
   }
   
   vec4 color = texture2D(texture,normCoord);
   float gray = 0.3*color.r + 0.59*color.g + 0.11*color.b;
   
   if(vTextureCoord.x < 0.5 && vTextureCoord.y > 0.5){
   }else {
      v -= 0.5;
      u -= 0.5;
      color.r = gray + 1.4075 * v;
      color.g = gray - 0.3455 * u - 0.7169* v;
      color.b = gray + 1.779*u;
   }
   gl_FragColor = color;
}
