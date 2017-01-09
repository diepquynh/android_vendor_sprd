precision mediump float;
uniform sampler2D texture;
varying vec2 vTextureCoord;
uniform sampler2D resourceTexture;
uniform vec2 imgSize;
const vec2 impressionistTextureSize = vec2(720.0,960.0);
 void main(void)
 {
vec2 textureOffset;
vec2 textureSize;
float imgWHRatio = imgSize.x/imgSize.y;
if( imgWHRatio < impressionistTextureSize.x/impressionistTextureSize.y) {
    textureOffset.x = (impressionistTextureSize.x - imgWHRatio * impressionistTextureSize.y)/(2.0 * impressionistTextureSize.x);
    textureOffset.y = 0.0;
    textureSize.x = 1.0 - textureOffset.x * 2.0;
    textureSize.y =1.0;
}else {
    textureOffset.y = (impressionistTextureSize.y - impressionistTextureSize.x/imgWHRatio)/(2.0 * impressionistTextureSize.y);
    textureOffset.x = 0.0;
    textureSize.y = 1.0 - textureOffset.y * 2.0;
    textureSize.x =1.0;
}

     vec4 overlay = texture2D(texture, vTextureCoord);
     vec4 base = texture2D(resourceTexture, vTextureCoord* textureSize + textureOffset);

    float ra;
    if (2.0 * base.r < base.a)
     {
   ra = 2.0 * overlay.r * base.r + overlay.r * (1.0 - base.a) + base.r * (1.0 - overlay.a);
      } else {
         
 ra = overlay.a * base.a - 2.0 * (base.a - base.r) * (overlay.a - overlay.r) + overlay.r * (1.0 - base.a) + base.r * (1.0 - overlay.a);

      }
 
 
     float ga;

     if (2.0 * base.g < base.a) {

             ga = 2.0 * overlay.g * base.g + overlay.g * (1.0 - base.a) + base.g * (1.0 - overlay.a);
    
 } else {
 
            ga = overlay.a * base.a - 2.0 * (base.a - base.g) * (overlay.a - overlay.g) + overlay.g * (1.0 - base.a) + base.g * (1.0 - overlay.a);
 
      }
 
 
      float ba;
 
      if (2.0 * base.b < base.a) {
 
            ba = 2.0 * overlay.b * base.b + overlay.b * (1.0 - base.a) + base.b * (1.0 - overlay.a);
 
      } else {
 
            ba = overlay.a * base.a - 2.0 * (base.a - base.b) * (overlay.a - overlay.b) + overlay.b * (1.0 - base.a) + base.b * (1.0 - overlay.a);
 
      }
 

 gl_FragColor = vec4(ra, ga, ba, 1.0);
 }
