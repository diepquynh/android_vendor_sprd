precision mediump float;
uniform sampler2D texture;
varying vec2 vTextureCoord;
uniform sampler2D resourceTexture;

float vignetteStart = 0.1;
float vignetteEnd = 0.3;
vec2 center = vec2(0.5);

const vec2 u =vec2( 1.0/1000.0);

vec3 boxblur0(vec2 pos)
{
vec3 sumx = 0.0;
for (float i = -7.0; i < 8.0; ++i)
{
sumx += texture2D(texture, pos+i*vec2(u.x,0.0)).rgb;
}
sumx = sumx/15.0;
sumx = clamp(sumx,0.0,1.0);
return sumx;
}

vec3 boxblur1(vec2 pos)
{
vec3 sumx = 0.0;
for (float i = -7.0; i < 8.0; ++i)
{
sumx += texture2D(texture, pos+i*vec2(0.0, u.y)).rgb;
}
sumx = sumx/15.0;
sumx = clamp(sumx,0.0,1.0);
return sumx;
}

void main(void)
{
vec3 blur = 0.0;
for(int i =-7; i <=7 ; i++) {
   vec3 blurvalue = boxblur0(vec2( vTextureCoord.x , vTextureCoord.y + i * u.y ));
   blur += blurvalue;
}
blur = blur/15.0;
blur = clamp(blur,0.0,1.0);

vec4 c  = texture2D(texture, vTextureCoord);
vec4 b  = vec4(vec3(blur), 1.0);
float           d = distance(vTextureCoord, center);
d = 1.0 - smoothstep(vignetteStart, vignetteEnd, d);

vec3 s = c.rgb*d + b.rgb*(1.0-d);
vec3 texel   = vec3( texture2D(resourceTexture, vec2(s.r, 0.0)).r,
texture2D(resourceTexture, vec2(s.g, 0.0)).g,
texture2D(resourceTexture, vec2(s.b, 0.0)).b
);
gl_FragColor = vec4(texel,1.0);
}
