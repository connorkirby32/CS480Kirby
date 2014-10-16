varying vec2 texcoord;
uniform sampler2D sampler; 

void main(void){
	gl_FragColor = texture2D(sampler, texcoord);
}
