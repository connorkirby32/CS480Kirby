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

#include <btBulletDynamicsCommon.h>

using namespace std;

// BULLET
btBroadphaseInterface *broadphase;
btDefaultCollisionConfiguration* collisionConfiguration;
btCollisionDispatcher* dispatcher;
btSequentialImpulseConstraintSolver* solver;
btDiscreteDynamicsWorld* dynamicsWorld;

btRigidBody* rigidBodySphere;
btRigidBody* rigidBodyGround;
btRigidBody* rigidBodyCylinder;
btRigidBody* rigidBodyTableLeftWall;
btRigidBody* rigidBodyTableRightWall;
btRigidBody* rigidBodyTableTopWall;
btRigidBody* rigidBodyTableBottomWall;
btRigidBody* rigidBodyCube;

GLuint program;// The GLSL program handle

// used for rotating viewer camera
class rotation
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

		float x = 5.0, y = 0.0, z = 3.0; // default coordinates
};

// loads shaders
class shaderLoader
{
    public:
      char* load(const string fileName)
        {
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
      char* buffer;
};

// used to store data of objects
class model
{	
	public:
	  void loadFromFile(const string fileName); 
    void loadModel(); 
    void renderModel(GLuint program); 
    void loadTexture();
    void bindTexture(GLuint program);
    
		//File Information
		string fileNameOBJ;
		string fileNameJPG;
		float distanceFromObject; 
    float objectRotation;
    float axisRotation;	
    int parent; 

	private:	
    //Vertex and UV information
    std::vector<unsigned short> faces;
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texCoords;
	
    GLuint vertexbuffer;
	  GLuint elementbuffer;
    GLuint texbuffer; 
    GLuint TextureID;
    GLuint Texture;
	
    int numElements;	
};
		
void model::loadFromFile(const string fileName)
{
	ifstream fin(fileName.c_str());
	
	fin >> fileNameOBJ;
	fin >> fileNameJPG;

	loadModel();
	loadTexture();

	fin.close();
}

void model::loadModel()
{
	Assimp::Importer read;

	const aiScene* scene = read.ReadFile(fileNameOBJ, aiProcess_Triangulate);
	
  // load vertices
	for( unsigned int x = 0; x < scene->mNumMeshes; x++ )
	  {		
		  const aiMesh* mesh = scene->mMeshes[x]; 
		
		  vertices.reserve(mesh->mNumVertices);
		
		  for(unsigned int i=0; i<mesh->mNumVertices; i++)
		    {
			    aiVector3D pos = mesh->mVertices[i];
			    vertices.push_back(glm::vec3(pos.x, pos.y, pos.z));
		    }

		  texCoords.reserve(mesh->mNumVertices);
		  
		  for(unsigned int i=0; i<mesh->mNumVertices; i++)
		    {
			    aiVector3D UVW = mesh->mTextureCoords[0][i]; 
			    texCoords.push_back(glm::vec2(UVW.x, UVW.y));
		    }

		  // Fill face indices
		  faces.reserve(3*mesh->mNumFaces);
		  
		  for (unsigned int i=0; i<mesh->mNumFaces; i++)
		    {
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

  numElements = faces.size();
  
  glGenBuffers(1, &elementbuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, numElements * sizeof(unsigned short), &faces[0] , GL_STATIC_DRAW);
}

void model:: renderModel(GLuint program)
{
  // bind texture
	this->bindTexture( program);

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

	// Draw the triangles 
	glDrawElements( GL_TRIANGLES, numElements, GL_UNSIGNED_SHORT,(void*)0);
}

void model:: loadTexture()
{
  // load texture from picture
  Texture =  SOIL_load_OGL_texture
        (
        fileNameJPG.c_str(),
        SOIL_LOAD_AUTO,
        SOIL_CREATE_NEW_ID,
        SOIL_FLAG_INVERT_Y
        );
}

void model:: bindTexture(GLuint program)
{
	TextureID  = glGetUniformLocation(program, "sampler");
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, Texture);
	
	glUniform1i(Texture, 0);
}

int numModels = 4;
model models[4];

rotation viewer;

//Shader
GLint loc_texc;
GLint loc_position;
GLint loc_tex;

//--Evil Global variables
//Just for this example!
int w = 640, h = 480;// Window size
bool jump = false; 

//uniform locations
GLint loc_mvpmat;// Location of the modelviewprojection matrix in the shader

//transform matrices
glm::mat4 model[4]; 
glm::mat4 view;//world->eye
glm::mat4 projection;//eye->clip
glm::mat4 mvp[4];//premultiplied modelviewprojection

//--GLUT Callbacks
void render();
void update();
void reshape(int n_w, int n_h);
void keyboard(unsigned char key, int x_pos, int y_pos);

//--Resource management
bool initialize();
void cleanUp();

//--Random time things
float getDT();
std::chrono::time_point<std::chrono::high_resolution_clock> t1,t2;

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
void initPhysics(){

    btTransform trans;

    // initialize BULLET
    broadphase = new btDbvtBroadphase();
    collisionConfiguration = new btDefaultCollisionConfiguration();
    dispatcher = new btCollisionDispatcher(collisionConfiguration);
    solver = new btSequentialImpulseConstraintSolver;
    dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
    
    //Set Gravity
    dynamicsWorld->setGravity(btVector3(0, -9.81, 0));
    
    //Create Floor
    btCollisionShape* ground = new btStaticPlaneShape (btVector3 (0,1,0), 1);
    btDefaultMotionState *groundMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(0, 0, 0)));
    btRigidBody::btRigidBodyConstructionInfo rigidBodyGroundCI(0, groundMotionState, ground, btVector3(0,0,0));
    rigidBodyGround = new btRigidBody(rigidBodyGroundCI);
    dynamicsWorld->addRigidBody(rigidBodyGround);

    //Create Left Wall    
    btCollisionShape* tableLeftWall = new btStaticPlaneShape (btVector3 (-1,0,0), 1);
    btDefaultMotionState *tableLeftWallMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(11, 0, 0)));
    btRigidBody::btRigidBodyConstructionInfo rigidBodyTableLeftWallCI(0, tableLeftWallMotionState, tableLeftWall, btVector3(0,0,0));
    rigidBodyTableLeftWall = new btRigidBody(rigidBodyTableLeftWallCI);
    dynamicsWorld->addRigidBody(rigidBodyTableLeftWall);

    //Create Right Wall    
    btCollisionShape* tableRightWall = new btStaticPlaneShape (btVector3 (1,0,0), 1);
    btDefaultMotionState *tableRightWallMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(-9.5, 0, 0)));
    btRigidBody::btRigidBodyConstructionInfo rigidBodyTableRightWallCI(0, tableRightWallMotionState, tableRightWall, btVector3(0,0,0));
    rigidBodyTableRightWall = new btRigidBody(rigidBodyTableRightWallCI);
    dynamicsWorld->addRigidBody(rigidBodyTableRightWall);    
    
    //Create Top Wall
    btCollisionShape* tableTopWall = new btStaticPlaneShape (btVector3 (0,0,-1), 1);
    btDefaultMotionState *tableTopWallMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(0, 0, 11)));
    btRigidBody::btRigidBodyConstructionInfo rigidBodyTableTopWallCI(0, tableTopWallMotionState, tableTopWall, btVector3(0,0,0));
    rigidBodyTableTopWall = new btRigidBody(rigidBodyTableTopWallCI);
    dynamicsWorld->addRigidBody(rigidBodyTableTopWall); 
    
    //Create Bottom Wall
    btCollisionShape* tableBottomWall = new btStaticPlaneShape (btVector3 (0,0,1), 1);
    btDefaultMotionState *tableBottomWallMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(0, 0, -5.65)));
    btRigidBody::btRigidBodyConstructionInfo rigidBodyTableBottomWallCI(0, tableBottomWallMotionState, tableBottomWall, btVector3(0,0,0));
    rigidBodyTableBottomWall = new btRigidBody(rigidBodyTableBottomWallCI);
    dynamicsWorld->addRigidBody(rigidBodyTableBottomWall);       
   
   	//Create Sphere 
    btCollisionShape* sphere = new btSphereShape( 1.0 );
    btDefaultMotionState *sphereMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(6, 6, 0)));
    btRigidBody::btRigidBodyConstructionInfo rigidBodySphereCI(8230, sphereMotionState, sphere, btVector3(0,0,0));
    rigidBodySphere = new btRigidBody(rigidBodySphereCI);
    dynamicsWorld->addRigidBody(rigidBodySphere);
    rigidBodySphere->setActivationState(DISABLE_DEACTIVATION);

    //Create Cylinder
    btCollisionShape* cylinder = new btSphereShape( 1.0 );
    btDefaultMotionState *cylinderMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(0, 6, 3)));
    btRigidBody::btRigidBodyConstructionInfo rigidBodyCylinderCI(4000, cylinderMotionState, cylinder, btVector3(0,0,0));
    rigidBodyCylinder = new btRigidBody(rigidBodyCylinderCI);
    dynamicsWorld->addRigidBody(rigidBodyCylinder);   
   	rigidBodyCylinder->setActivationState(DISABLE_DEACTIVATION);
   	
   	//Create Static Cube
   	btCollisionShape* cube = new btBoxShape( btVector3(1.0,1.0,1.0) );
    btDefaultMotionState *cubeMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(-3, 2, 0)));
    btRigidBody::btRigidBodyConstructionInfo rigidBodyCubeCI(0, cubeMotionState, cube, btVector3(0,0,0));
    rigidBodyCube = new btRigidBody(rigidBodyCubeCI);
    dynamicsWorld->addRigidBody(rigidBodyCube);   
        	     
    
  	rigidBodyCube ->getMotionState()->getWorldTransform(trans);
  	trans.getOpenGLMatrix(glm::value_ptr(model[2]));
}


void createGLUTMenus() {

	glutCreateMenu(processMenuEvents);

	//add entries to our menu
	glutAddMenuEntry("Exit",3);


	// attach the menu to the right button
	glutAttachMenu(GLUT_RIGHT_BUTTON);
}

void myMouse(int button, int state, int x, int y)
{
    if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
      {

      }       
}

//--Main
int main(int argc, char **argv)
{
    // Initialize glut
     initPhysics();
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(w, h);
	
    // Name and create the Window
    glutCreateWindow("Bullet");
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
    glClearColor(0.0, 0.0, 	0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    //Enable Shader
    glUseProgram(program);
    
	  // mvp for all planets
    for(int i = 0; i < numModels; i++){
    
   		 mvp[i] = projection * view * model[i];
   		
   		 
    }

    //upload the matrix to the shader
    for(int i = 0; i < numModels; i++){

    	glUniformMatrix4fv(loc_mvpmat, 1, GL_FALSE, glm::value_ptr(mvp[i]));

		models[i].renderModel(program);
		
	}
	

    //swap the buffers
    glutSwapBuffers();
	
   
}

void update(){
	 
 
   	float dt = getDT();// if you have anything moving, use dt. 
	 
		
	dynamicsWorld->stepSimulation(dt, 10);
	
	btTransform trans;

	rigidBodySphere ->getMotionState()->getWorldTransform(trans);
	trans.getOpenGLMatrix(glm::value_ptr(model[0]));
		
	rigidBodyCylinder ->getMotionState()->getWorldTransform(trans);
	trans.getOpenGLMatrix(glm::value_ptr(model[1]));   
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
	
	
	switch(key){
	
		case ('a' | 'A'):
		
				rigidBodySphere->translate( btVector3( 1.0f, 0.0f, 0.0f ) );	
		break;
		
		case ('s' | 'S'):
		
				rigidBodySphere->translate( btVector3( 0.0f, 0.0f, -1.0f ) );
		break;
		
		case ('d' | 'D'):
				
				rigidBodySphere->translate( btVector3( -1.0f, 0.0f, 0.0f ) );
		break;
		
		case ('w' | 'W'):
		
 				rigidBodySphere->translate( btVector3( 0.0f, 0.0f, 1.0f ) );
  
		break;
		
		case ('c' | 'C'):
		
 				rigidBodySphere->translate( btVector3( 0.0f, 2.0f, 0.0f ) );
 		break;
 				
 		case ('j' | 'J'):
		
				rigidBodyCylinder->translate( btVector3( 1.0f, 0.0f, 0.0f ) );	
		break;
		
		case ('k' | 'K'):
		
				rigidBodyCylinder->translate( btVector3( 0.0f, 0.0f, -1.0f ) );
		break;
		
		case ('l' | 'L'):
				
				rigidBodyCylinder->translate( btVector3( -1.0f, 0.0f, 0.0f ) );
		break;
		
		case ('i' | 'I'):
		
 				rigidBodyCylinder->translate( btVector3( 0.0f, 0.0f, 1.0f ) );
  
		break;
		
		case ('n' | 'N'):
		
 				rigidBodyCylinder->translate( btVector3( 0.0f, 2.0f, 0.0f ) );
  
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
	std::vector<unsigned short> faces;
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> texCoords;
   
    // Read our .obj files
    
	// read in sun 
	
	// read in all planets
	models[0].loadFromFile("../bin/modelInfo/sphere.txt");
	models[1].loadFromFile("../bin/modelInfo/cylinder.txt");
	models[2].loadFromFile("../bin/modelInfo/cube.txt");
	models[3].loadFromFile("../bin/modelInfo/table.txt");
	
	
    //--Geometry done

    
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

    
    //Set shader char pointers
    shaderLoader x; 
    const char *vs = x.load("../bin/vertexShader.vert");
    const char *fs = x.load("../bin/fragmentShader.frag"); 


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
        return false;
    }

    // Now the Fragment shader
    glShaderSource(fragment_shader, 1, &fs, NULL);
    glCompileShader(fragment_shader);
    //check the compile status
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &shader_status);
    if(!shader_status)
    {
        std::cerr << "[F] FAILED TO COMPILE FRAGMENT SHADER!" << std::endl;
        return false;
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
    if(loc_position == -1)
    {
        std::cerr << "[F] POSITION NOT FOUND" << std::endl;
        return false;
    }

    loc_tex = glGetAttribLocation(program,
                    const_cast<const char*>("_texcoord"));
    if(loc_tex == -1)
    {
        std::cerr << "[F] TEXTURE CORD. NOT FOUND" << std::endl;
        return false;
    }

    loc_mvpmat = glGetUniformLocation(program,
                    const_cast<const char*>("mvpMatrix"));
    if(loc_mvpmat == -1)
    {
        std::cerr << "[F] MVPMATRIX NOT FOUND" << std::endl;
        return false;
    } 
    loc_texc = glGetUniformLocation(program,
                    const_cast<const char*>("sampler"));
   
    if(loc_tex == -1)
    {
        std::cerr << "[F] TEXTURE SAMPLER NOT FOUND" << std::endl;
        return false;
    }
    //--Init the view and projection matrices
    //  if you will be having a moving camera the view matrix will need to more dynamic
    //  ...Like you should update it before you render more dynamic 
    //  for this project having them static will be fine
    view = glm::lookAt( glm::vec3(0.0, 15.0, -15.0), //Eye Position
                        glm::vec3(0.0, 0.0, 0.0), //Focus point
                        glm::vec3(0.0, 1.0, 0.0)); //Positive Y is up

    projection = glm::perspective( 90.0f, //the FoV typically 90 degrees is good which is what this is set to
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
    
    // delete BULLET material
    delete dynamicsWorld;
	  delete solver;
	  delete dispatcher;
	  delete collisionConfiguration;
	  delete broadphase;
	  
	  delete rigidBodySphere;
	  delete rigidBodyCylinder;
	  delete rigidBodyCube;
	  delete rigidBodyGround;
	  delete rigidBodyTableLeftWall;
	  delete rigidBodyTableRightWall;
	  delete rigidBodyTableTopWall;
	  delete rigidBodyTableBottomWall;
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




