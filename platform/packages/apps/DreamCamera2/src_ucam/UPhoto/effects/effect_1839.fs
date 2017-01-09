precision mediump float;
uniform sampler2D texture;
varying vec2 vTextureCoord;


const vec2 imgSize = vec2(1000.0);
void main(void)
{
    vec4 sample[9];
    vec4 greyMat = vec4(0.299,0.587,0.114,0.0);
    float disy=1./imgSize.x;
    float disx=1./imgSize.y;
    sample[0] = texture2D(texture, vTextureCoord + vec2(-disx,disy));
    sample[1] = texture2D(texture, vTextureCoord + vec2(0.0,disy));
    sample[2] = texture2D(texture, vTextureCoord + vec2(disx,disy));
    sample[3] = texture2D(texture, vTextureCoord + vec2(-disx,0.0));
    sample[4] = texture2D(texture, vTextureCoord + vec2(0.0,0.0));
    sample[5] = texture2D(texture, vTextureCoord + vec2(disx,0.0));
    sample[6] = texture2D(texture, vTextureCoord + vec2(-disx,-disy));
    sample[7] = texture2D(texture, vTextureCoord + vec2(0.0,-disy));
    sample[8] = texture2D(texture, vTextureCoord + vec2(disx,-disy));

    vec4 edge = sample[4] * 2.5 -
    (sample[0] + sample[1] + sample[2] + sample[3]  +
    sample[5] + sample[6] + sample[7] + sample[8]) * 0.125;
    float g = dot(greyMat, edge);
    float f_r = (g - 0.7) / 0.6 + 0.5;
    gl_FragColor = vec4(f_r,f_r,f_r, 1.0);
}
