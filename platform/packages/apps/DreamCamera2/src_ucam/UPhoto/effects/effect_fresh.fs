precision mediump float;
uniform sampler2D texture;
varying vec2 vTextureCoord;
uniform sampler2D resourceTexture;

const float type = 0.05;

const vec2 center = vec2(0.5);
vec4 doVignette (vec4 orig) {
    vec3 rgb=orig.xyz;
    float d=distance(vTextureCoord, center);
    float total = distance(vec2(0.0),center);
    rgb*=smoothstep(0.0,0.5,(total - d)/total);

    return vec4(rgb,1.0);

}

vec4 doColor (vec4 orig) {
    vec3 c = orig.rgb * 200.0/256.0 + 28.0/256.0;
    vec4 r=texture2D (resourceTexture,vec2(c.r,type));
    vec4 g=texture2D (resourceTexture,vec2(c.g,type));
    vec4 b=texture2D (resourceTexture,vec2(c.b,type));
    return vec4 (r.r,g.g,b.b,1.0);
}
void main() {

    vec4 orig= texture2D(texture,vTextureCoord);
    orig=doVignette(orig);
    gl_FragColor=doColor (orig);

}
