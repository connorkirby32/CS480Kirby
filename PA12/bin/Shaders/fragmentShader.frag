varying vec2 texcoord;
varying vec4 outcolor;  


uniform sampler2D sampler; 

void main(void)
{
  vec4 color = texture2D(sampler, texcoord.xy);
  gl_FragColor = outcolor * color;
}

