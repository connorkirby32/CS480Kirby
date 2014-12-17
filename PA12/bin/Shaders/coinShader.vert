attribute vec3 v_position;
attribute vec2 v_texcoord;
attribute vec3 normal;


varying vec2 texcoord; 
varying vec4 outcolor;  
     
uniform mat4 mvpMatrix;
uniform mat4 mvMatrix; 

void main(void)
{


  texcoord = v_texcoord;

  gl_Position = mvpMatrix * vec4(v_position, 1.0);
}
   

        
        



