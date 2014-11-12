attribute vec3 v_position;
attribute vec2 v_texcoord;
attribute vec3 normal;


varying vec2 texcoord; 
varying vec4 outcolor;  
     
uniform mat4 mvpMatrix;
uniform mat4 mvMatrix; 

void main(void)
{
  vec4 light = vec4(1,1,1,1)* mvMatrix;
  vec3 lightDir = light.xyz;
  
  vec4 color = gl_FrontMaterial.specular;
  vec4 color1 = gl_FrontMaterial.diffuse;
  vec4 color2 = gl_FrontMaterial.ambient;
  
  float cosAngIncidence = dot(normalize(normal), normalize(lightDir));
  cosAngIncidence = clamp(cosAngIncidence, 0, 1);
  outcolor =  (color1+color2+color)*cosAngIncidence + vec4(0.1,0.1,0.1,1.0);
        
  texcoord = v_texcoord;

  gl_Position = mvpMatrix * vec4(v_position, 1.0);
}
        
        



