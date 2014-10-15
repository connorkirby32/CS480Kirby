attribute vec3 v_position;
attribute vec2 _texcoord;
varying vec2 texcoord;        
uniform mat4 mvpMatrix;

void main(void){
 	gl_Position = mvpMatrix * vec4(v_position, 1.0);
	texcoord = _texcoord;
}
        
        
