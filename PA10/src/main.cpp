#define GLM_FORCE_RADIANS

#include <GL/glew.h> // glew must be included before the main gl libs
#include <GL/glut.h> // doing otherwise causes compiler shouting
#include <iostream>
#include <chrono>

#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <chrono>
#include <string>
#include <streambuf>

#include <assimp/Importer.hpp>    
#include <assimp/scene.h>           
#include <assimp/postprocess.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> //Makes passing matrices to shaders easier

#include <SOIL/SOIL.h>

using namespace std;

GLuint program;// The GLSL program handle

int numModels = 1;

const char *vs;
const char *fs;
bool firstLoad = false;
int shaderInput = 1;

//Shader
GLint loc_texc;
GLint loc_position;
GLint loc_tex;
GLint loc_norm;

class model{
	
	public:
	
		void loadFromFile(const string fileName); 

      

		void loadModel(); 
		void renderModel(GLuint program); 
		
		void loadTexture();
		void bindTexture(GLuint program);
		
		//File Information
		string fileNameOBJ;
		string fileNameJPG;

		
		
		
		
	private:
	
	//Vertex and UV information
	std::vector<unsigned short> faces;
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> texCoords;
	std::vector<glm::vec3> normals; 
	
	GLuint vertexbuffer;
	GLuint elementbuffer;
	GLuint texbuffer; 
	GLuint normalbuffer; 
	GLuint TextureID;
	GLuint Texture;
	
	int numElements;
	


	

	
	
};
		
void model::loadFromFile(const string fileName){


	ifstream fin(fileName.c_str());

	fin>>fileNameOBJ;
	fin>>fileNameJPG;
	
	loadModel();

	loadTexture();


	fin.close();
}
void model::loadModel(){

 
	Assimp::Importer read;

	const aiScene* scene = read.ReadFile(fileNameOBJ, aiProcess_Triangulate);
	

	for( unsigned int x = 0; x < scene->mNumMeshes; x++ ){
		
		const aiMesh* mesh = scene->mMeshes[x]; 
		
		vertices.reserve(mesh->mNumVertices);
		
		for(unsigned int i=0; i<mesh->mNumVertices; i++){
			aiVector3D pos = mesh->mVertices[i];
			vertices.push_back(glm::vec3(pos.x, pos.y, pos.z));
		}
		normals.reserve(mesh->mNumVertices);
		texCoords.reserve(mesh->mNumVertices);
		for(unsigned int i=0; i<mesh->mNumVertices; i++){
			aiVector3D UVW = mesh->mTextureCoords[0][i];
			aiVector3D norms = mesh->mNormals[i]; 
			 
			texCoords.push_back(glm::vec2(UVW.x, UVW.y));
			normals.push_back(glm::vec3(norms.x, norms.y , norms.z));		
			
		}

		// Fill face indices
		faces.reserve(3*mesh->mNumFaces);
		for (unsigned int i=0; i<mesh->mNumFaces; i++){
		
			faces.push_back(mesh->mFaces[i].mIndices[0]);
			faces.push_back(mesh->mFaces[i].mIndices[1]);
			faces.push_back(mesh->mFaces[i].mIndices[2]);
		}

	}

	// Load it into a VBO

    glGenBuffers(1, &vertexbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), & vertices[0], GL_STATIC_DRAW);

    
    glGenBuffers(1, &texbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, texbuffer);
    glBufferData(GL_ARRAY_BUFFER, texCoords.size() * sizeof(glm::vec2), &texCoords[0], GL_STATIC_DRAW);

    glGenBuffers(1, &normalbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);

    numElements = faces.size();
    glGenBuffers(1, &elementbuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, numElements * sizeof(unsigned short), &faces[0] , GL_STATIC_DRAW);
	 
}

void model:: renderModel(GLuint program){

	
	this->bindTexture(program);

	//Vertexes
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0 , (void*)0);

	
	//Faces
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);

	//Texture Coordinates
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, texbuffer);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
	
	//Normal Coordinates
	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);


	// Draw the triangles 
	glDrawElements( GL_TRIANGLES, numElements, GL_UNSIGNED_SHORT,(void*)0);
   

}

void model:: loadTexture(){


  Texture =  SOIL_load_OGL_texture
        (
        fileNameJPG.c_str(),
        SOIL_LOAD_AUTO,
        SOIL_CREATE_NEW_ID,
        SOIL_FLAG_INVERT_Y
        );

}

void model:: bindTexture(GLuint program){


	TextureID  = glGetUniformLocation(program, "sampler");
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, Texture);
	
	glUniform1i(Texture, 0);

}

//--Evil Global variables
//Just for this example!
int w = 640, h = 480;// Window size
model models;

//uniform locations
GLint loc_mvpmat;// Location of the modelviewprojection matrix in the shader
GLint loc_mvmat;
//transform matrices
glm::mat4 model, sunModel;//obj->world each object should have its own model matrix 
glm::mat4 view;//world->eye
glm::mat4 projection;//eye->clip
glm::mat4 mvp;//premultiplied modelviewprojection
glm::mat4 mv;

//--GLUT Callbacks
void render();
void update();
void reshape(int n_w, int n_h);
void keyboard(unsigned char key, int x_pos, int y_pos);

//--Resource management
bool initialize();
void cleanUp();
void loadNewShader( int shaderInput );

//--Random time things
float getDT();
std::chrono::time_point<std::chrono::high_resolution_clock> t1,t2;

class shaderLoader{
    public:
        char * load(const string fileName){
        
        ifstream fin(fileName.c_str());
        
        //Find File Length
        fin.seekg(0,std::ios::end);
        streampos length = fin.tellg();
        fin.seekg(0,std::ios::beg);
        
        //Allocate and copy to char pointer
        buffer = new char [length];
        fin.read(&buffer[0], length);
        return buffer;
   }
    
    private:
    char * buffer;
};

class camera
{
	public:
		float rotateXNeg()
			{
				return --x;
			}
		float rotateXPos()
			{
				return ++x;
			}
		float rotateYNeg()
			{
				return --y;
			}
	  float rotateYPos()
			{
				return ++y;
			}
		float rotateZNeg()
			{
				return --z;
			}
		float rotateZPos()
			{
				return ++z; 
			}
		  
		float x = 0.0, y = 0.0, z = -5.0; // default coordinates
};

camera viewer;

void processMenuEvents(int option) {

	switch (option) {
		case 1 :
		  
		 break;
		case 2 :
		           
		break;
		case 3 :
	        exit(0); 
		 break;
	}
}

void createGLUTMenus() {

	glutCreateMenu(processMenuEvents);

	//add entries to our menu
	glutAddMenuEntry("Start",1);
	glutAddMenuEntry("Stop",2);
	glutAddMenuEntry("Exit",3);


	// attach the menu to the right button
	glutAttachMenu(GLUT_RIGHT_BUTTON);
}

void myMouse(int button, int state, int x, int y) {

   

    if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN){
    
        view = glm::lookAt( glm::vec3(0.0, 0.0, 0.0), //Eye Position
                        glm::vec3(0.0, 0.0, 0.0), //Focus point
                        glm::vec3(0.0, 1.0, 0.0)); //Positive Y is up
                        
    } 
        
}





//--Main
int main(int argc, char **argv)
{
    // Initialize glut
    
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(w, h);
	
    // Name and create the Window
    glutCreateWindow("Lighting");
    createGLUTMenus();  
    // Now that the window is created the GL context is fully set up
    // Because of that we can now initialize GLEW to prepare work with shaders
    GLenum status = glewInit();
    if( status != GLEW_OK)
    {
        std::cerr << "[F] GLEW NOT INITIALIZED: ";
        std::cerr << glewGetErrorString(status) << std::endl;
        return -1;
    }

    // Set all of the callbacks to GLUT that we need
    glutDisplayFunc(render);// Called when its time to display
    glutReshapeFunc(reshape);// Called if the window is resized
    glutIdleFunc(update);// Called if there is nothing else to do
    glutKeyboardFunc(keyboard);// Called if there is keyboard input
    glutMouseFunc(myMouse);



    // Initialize all of our resources(shaders, geometry)
    bool init = initialize();

    if(init)
    {
        t1 = std::chrono::high_resolution_clock::now();
        glutMainLoop();
    }

    // Clean up after ourselves
    cleanUp();
    return 0;
}

//--Implementations
void render(){
    //--Render the scene

    //clear the screen
    glClearColor(0.0, 0.0,0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    //Enable Shader
    glUseProgram(program);
    
	
    for(int i = 0; i < numModels; i++){
    
   		 mvp = projection * view * model;
   		 mv = view * model;
   		 
    }

    //upload the matrix to the shader
    
    for(int i = 0; i < numModels; i++){

    	glUniformMatrix4fv(loc_mvpmat, 1, GL_FALSE, glm::value_ptr(mvp));
    	glUniformMatrix4fv(loc_mvmat, 1, GL_FALSE, glm::value_ptr(mv));

	models.renderModel(program);
	
	}

    //swap the buffers
    glutSwapBuffers();
	
   
}

void update()
{      
	  view = glm::lookAt( glm::vec3( viewer.x, viewer.y, viewer.z ), //Eye Position
		                    glm::vec3(0.0, 0.0, 0.0), //Focus point
		                    glm::vec3(0.0, 1.0, 0.0)); //Positive Y is up

    // Update the state of the scene
     glutPostRedisplay();//call the display callback
}
 

void reshape(int n_w, int n_h)
{
    w = n_w;
    h = n_h;
     //Change the viewport to be correct
    glViewport( 0, 0, w, h);
    //Update the projection matrix as well
    //See the init function for an explaination
    projection = glm::perspective(45.0f, float(w)/float(h), 0.01f, 100.0f);

}

void keyboard(unsigned char key, int x_pos, int y_pos)
{
	switch(key)
	{
		case ('a' | 'A'):
		  if( viewer.x == -50 || viewer.x != 50 )
		    {  
			    viewer.rotateXPos();
			  }					
		break;
		
		case ('s' | 'S'):
		  if( viewer.z == 75 || viewer.z != -75 )
		    {  	
          viewer.rotateZNeg();
		    }		      
		break;
		
		case ('d' | 'D'):			
		  if( viewer.x == 50 || viewer.x != -50 )
		    {		
          viewer.rotateXNeg();
	      }      
		break;
		
		case ('w' | 'W'):		
			if( viewer.z == -75 || viewer.z != 75 )
		    {
          viewer.rotateZPos();
			  }
		break;
		
		case ('r' | 'R'):	
			if( viewer.y == -75 || viewer.y != 75 )
		    {
					viewer.rotateYPos();
			  }
		break;
		
		case ('f' | 'F'):		
			if( viewer.y == 75 || viewer.y != -75 )
		    {
					viewer.rotateYNeg();
			  }
		break;
		
		case ('1'):
		  loadNewShader( 1 );
		break;
		
		case ('2'):
		  loadNewShader( 2 );
		break;
		
		case ('3'):
		  loadNewShader( 3 );
		break;

		case ('4'):
		  loadNewShader( 4 );
		break;
	}

    // Handle keyboard input
    if(key == 27)//ESC
    {
        exit(0);
    }
    
    
}

bool initialize()
{

    // Initialize basic geometry and shaders for this example

   
    // Read our .obj file
	
	  models.loadFromFile("../bin/object.txt");

	
    //--Geometry done

    
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

    
    //Set shader char pointers
    shaderLoader x;
    
    if( !firstLoad )
      { 
        vs = x.load("../bin/shaders/distantShader.vert");
        fs = x.load("../bin/shaders/fragmentShader.frag"); 
        firstLoad = true;
      }

    //compile the shaders
    GLint shader_status;
 
    // Vertex shader first
    glShaderSource(vertex_shader, 1, &vs, NULL);
    glCompileShader(vertex_shader);
    //check the compile status
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &shader_status);
    if(!shader_status)
    {
        std::cerr << "[F] FAILED TO COMPILE VERTEX SHADER!" << std::endl;
        
      
    }

    // Now the Fragment shader
    glShaderSource(fragment_shader, 1, &fs, NULL);
    glCompileShader(fragment_shader);
    //check the compile status
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &shader_status);
    if(!shader_status)
    {
        std::cerr << "[F] FAILED TO COMPILE FRAGMENT SHADER!" << std::endl;
        
    }

    //Now we link the 2 shader objects into a program
    //This program is what is run on the GPU
    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
   
    glLinkProgram(program);
    //check if everything linked ok
    glGetProgramiv(program, GL_LINK_STATUS, &shader_status);
    //this defines a cube, this is why a model loadfer is nice
    
    loc_position = glGetAttribLocation(program,
                    const_cast<const char*>("v_position"));

    loc_tex = glGetAttribLocation(program,
                    const_cast<const char*>("v_texture"));


    loc_mvpmat = glGetUniformLocation(program,
                    const_cast<const char*>("mvpMatrix"));
    loc_mvmat = glGetUniformLocation(program,
                    const_cast<const char*>("mvMatrix"));

    loc_texc = glGetUniformLocation(program,
                    const_cast<const char*>("gSampler"));

    loc_norm = glGetUniformLocation(program,
                    const_cast<const char*>("v_normal"));
   

    //--Init the view and projection matrices
    //  if you will be having a moving camera the view matrix will need to more dynamic
    //  ...Like you should update it before you render more dynamic 
    //  for this project having them static will be fine
			view = glm::lookAt( glm::vec3( viewer.x, viewer.y, viewer.z ), //Eye Position
		                    glm::vec3(0.0, 0.0, 0.0), //Focus point
		                    glm::vec3(0.0, 1.0, 0.0)); //Positive Y is up
		                    
    projection = glm::perspective( 45.0f, //the FoV typically 90 degrees is good which is what this is set to
                                   float(w)/float(h), //Aspect Ratio, so Circles stay Circular
                                   0.01f, //Distance to the near plane, normally a small value like this
                                   100.0f); //Distance to the far plane, 
                                                             
    
    //enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    //and its done
    return true;
}

void cleanUp()
{
    // Clean up, Clean up
    glDeleteProgram(program);
	
}

//returns the time delta
float getDT()
{
    float ret;
    t2 = std::chrono::high_resolution_clock::now();
    ret = std::chrono::duration_cast< std::chrono::duration<float> >(t2-t1).count();
    t1 = std::chrono::high_resolution_clock::now();
    return ret;
}

void loadNewShader( int shaderInput )
{
  shaderLoader x;
  cleanUp();
  
  if ( shaderInput == 1 )
    {
      vs = x.load("../bin/shaders/ambientShader.vert");
    }
  else if ( shaderInput == 2 )
    {      
      vs = x.load("../bin/shaders/distantShader.vert");
    }
  else if ( shaderInput == 3 )
    {
      vs = x.load("../bin/shaders/pointShader.vert");
    }      
  else if ( shaderInput == 4 )
    {
      vs = x.load("../bin/shaders/spotShader.vert");
    }  
    
  initialize();
}
