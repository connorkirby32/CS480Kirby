attribute vec3 v_position;
attribute vec2 v_texcoord;
attribute vec3 normal;


varying vec2 texcoord;  
varying vec4 diffuse,ambient,ecPos,ambientGlobal;
varying vec3 normal1,halfVector;
varying mat4 mvMatrix1; 

uniform mat4 mvpMatrix;
uniform mat4 mvMatrix; 
uniform vec4 direction; 
 
void main(void)
{   
  
     
    /* first transform the normal into eye space and normalize the result */
    normal1 = normalize(gl_NormalMatrix * gl_Normal);
 
    /* compute the vertex position  in camera space. */
      ecPos =  mvMatrix * vec4(1.0,1.0,1.0,1.0);
      
    /* Normalize the halfVector to pass it to the fragment shader */
    halfVector = gl_LightSource[1].halfVector.xyz;
     
    /* Compute the diffuse, ambient and globalAmbient terms */
    diffuse = gl_FrontMaterial.diffuse * gl_LightSource[0].diffuse;
    ambient = gl_FrontMaterial.ambient * gl_LightSource[0].ambient;
    ambientGlobal = gl_LightModel.ambient * gl_FrontMaterial.ambient;
    
 	texcoord = v_texcoord;
  	mvMatrix1 = mvMatrix; 
  	gl_Position = mvpMatrix * vec4(v_position, 1.0);
} 
