precision mediump float;
uniform sampler2D texture;
varying vec2 vTextureCoord;
uniform sampler2D resourceTexture;
uniform sampler2D impressionistTexture2;
void main(void)
{
	vec3 s  =  texture2D(texture, vTextureCoord).rgb;
	float alpha = texture2D(resourceTexture,vTextureCoord).r;
	vec3 t  =  vec3( texture2D(impressionistTexture2, vec2(s.r, 0.0)).r,
		texture2D(impressionistTexture2, vec2(s.g, 0.0)).g,
		texture2D(impressionistTexture2, vec2(s.b, 0.0)).b
		);
	s = vec3( texture2D(impressionistTexture2, vec2(t.r, 1.0)).r,
		texture2D(impressionistTexture2, vec2(t.g, 1.0)).g,
		texture2D(impressionistTexture2, vec2(t.b, 1.0)).b
	);
	gl_FragColor = vec4(t*alpha,1.0);
}
