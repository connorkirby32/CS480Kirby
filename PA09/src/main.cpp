// HEADER FILES ///////////////////////////////////////////////////////////////////////////////
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

#include <unistd.h>

#include <assimp/Importer.hpp>    
#include <assimp/scene.h>           
#include <assimp/postprocess.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> //Makes passing matrices to shaders easier

#include <SOIL/SOIL.h>

#include <btBulletDynamicsCommon.h>

using namespace std;

// GLOBAL VARIABLES, CLASSES, FUNCTIONS ///////////////////////////////////////////////////////

// BULLET
btBroadphaseInterface *broadphase;
btDefaultCollisionConfiguration* collisionConfiguration;
btCollisionDispatcher* dispatcher;
btSequentialImpulseConstraintSolver* solver;
btDiscreteDynamicsWorld* dynamicsWorld;

btRigidBody* rigidBodyGround;
btRigidBody* rigidBodyPaddle1;
btRigidBody* rigidBodyPaddle2;
btRigidBody* rigidBodyPuck;

btRigidBody* rigidBodyTableLeftWall;
btRigidBody* rigidBodyTableRightWall;
btRigidBody* rigidBodyTableTopWall;
btRigidBody* rigidBodyTableBottomWall;
btRigidBody* rigidBodyTableLeftTopWall;
btRigidBody* rigidBodyTableRightTopWall;
btRigidBody* rigidBodyTableLeftBottomWall;
btRigidBody* rigidBodyTableRightBottomWall;
btRigidBody* rigidBodyTable;
btRigidBody* rigidBodyUpperLeftWall;
btRigidBody* rigidBodyBottomLeftWall;
btRigidBody* rigidBodyUpperRightWall;
btRigidBody* rigidBodyBottomRightWall;

GLuint program;// The GLSL program handle

// used for rotating viewer camera
class camera
{
	public:
		void setAngleA() // top-down view
		  {
		    x = 0.0;
		    y = 25.0;
		    z = -1.0;
		  }
		void setAngleB() // birdseye view
		  {
		    x = 0.0;
		    y = 15.0;
		    z = -20.0;
		  }
		void setAngleC() // player 1 POV
		  {
		    x = 30.0;
		    y = 15.0;
		    z = 0.0;
		  }
		void setAngleD() // player 2 POV
		  {
		    x = -30.0;
		    y = 15.0;
		    z = 0.0;
		  }
		  
		float x = 0.0, y = 25.0, z = -1.0; // default coordinates
		char angle = 'a'; // default camera angle
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


bool squarePuck = false; 
// used to store data of objects
class model
{	
	public:
	  void loadFromFile(const string fileName); 
    void loadModel(); 
    void renderModel(GLuint program); 
    void loadTexture();
    void bindTexture(GLuint program);
    void clear();
  
    
	  //File Information
	  string fileNameOBJ;
	  string fileNameJPG;
	
	  btTriangleMesh *mTriMesh = new btTriangleMesh();


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

void model::clear()
{
  faces.clear();
  vertices.clear();
  texCoords.clear();
}
		
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
		    
      for (unsigned int i = 0; i < scene->mNumMeshes; i++ )
		    {
		      for( unsigned int j = 0; j < scene->mMeshes[i]->mNumFaces * 3; j +=3 )
		        {
              btVector3 vertex0(vertices[j].x, vertices[j].y, vertices[j].z); 
              btVector3 vertex1(vertices[j+1].x, vertices[j+1].y, vertices[j+1].z);
 				      btVector3 vertex2(vertices[j+2].x, vertices[j+2].y, vertices[j+2].z);
		     
		     	    mTriMesh->addTriangle(vertex0, vertex1, vertex2);
		     	  }
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
model menuItems[3];
int player1Score = 0, player2Score = 0; 

camera viewer;

//Shader
GLint loc_texc;
GLint loc_position;
GLint loc_tex;

//--Evil Global variables
//Just for this example!
int w = 640, h = 480;// Window size

// booleans for options
bool menu = false,
     p1side = true,
     AIon = false;
     
float goal1[3] = { 13.5, 2.0, -2.5 };
float goal2[3] = { -13.5, 2.0, -2.5}; 

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
void initPhysics();
void cleanUp();
void reset();

//--Artificial intelligence
void AI();

//--Random time things
float getDT();
std::chrono::time_point<std::chrono::high_resolution_clock> t1,t2;


void initPhysics()
{
    // initialize variables
    btTransform trans;
    float mass;
    int ran;
    btVector3 inertia(0,0,0);

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

    //Create Top Wall 
    btCollisionShape* tableTopWall = new btStaticPlaneShape (btVector3 (0,0,-1), 1);
    btDefaultMotionState *tableTopWallMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(0, 0, 7.75)));
    btRigidBody::btRigidBodyConstructionInfo rigidBodyTableTopWallCI(0, tableTopWallMotionState, tableTopWall, btVector3(0,0,0));
    rigidBodyTableTopWallCI.m_restitution = 0.5;
    rigidBodyTableTopWall = new btRigidBody(rigidBodyTableTopWallCI);
    dynamicsWorld->addRigidBody(rigidBodyTableTopWall); 
    
    //Create Bottom Wall
    btCollisionShape* tableBottomWall = new btStaticPlaneShape (btVector3 (0,0,1), 1);
    btDefaultMotionState *tableBottomWallMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(0, 0, -7.65)));
    btRigidBody::btRigidBodyConstructionInfo rigidBodyTableBottomWallCI(0, tableBottomWallMotionState, tableBottomWall, btVector3(0,0,0));
    rigidBodyTableBottomWallCI.m_restitution = 0.5; 
    rigidBodyTableBottomWall = new btRigidBody(rigidBodyTableBottomWallCI);
    dynamicsWorld->addRigidBody(rigidBodyTableBottomWall);
    
    //Create Left Top Wall;
    btCollisionShape* tableLeftTopWall = new btStaticPlaneShape (btVector3 (-1,0,-1), 1);
    btDefaultMotionState *tableLeftTopWallMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(12.75, 0, 7.00)));
    btRigidBody::btRigidBodyConstructionInfo rigidBodyTableLeftTopWallCI(0, tableLeftTopWallMotionState, tableLeftTopWall, btVector3(0,0,0));
    rigidBodyTableLeftTopWallCI.m_restitution = 0.5;
    rigidBodyTableLeftTopWall = new btRigidBody(rigidBodyTableLeftTopWallCI);
    dynamicsWorld->addRigidBody(rigidBodyTableLeftTopWall);
 
    //Create Right Top Wall
    btCollisionShape* tableRightTopWall = new btStaticPlaneShape (btVector3 (1,0,-1), 1);
    btDefaultMotionState *tableRightTopWallMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(-12.5, 0, 7.00)));
    btRigidBody::btRigidBodyConstructionInfo rigidBodyTableRightTopWallCI(0, tableRightTopWallMotionState, tableRightTopWall, btVector3(0,0,0));
    rigidBodyTableRightTopWallCI.m_restitution = 0.5;
    rigidBodyTableRightTopWall = new btRigidBody(rigidBodyTableRightTopWallCI);
    dynamicsWorld->addRigidBody(rigidBodyTableRightTopWall); 
    
    //Create Left Bottom Wall
    btCollisionShape* tableLeftBottomWall = new btStaticPlaneShape (btVector3 (-1,0,1), 1);
    btDefaultMotionState *tableLeftBottomWallMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(12.5, 0, -7.00)));
    btRigidBody::btRigidBodyConstructionInfo rigidBodyTableLeftBottomWallCI(0, tableLeftBottomWallMotionState, tableLeftBottomWall, btVector3(0,0,0));
    rigidBodyTableLeftBottomWallCI.m_restitution = 0.5;
    rigidBodyTableLeftBottomWall = new btRigidBody(rigidBodyTableLeftBottomWallCI);
    dynamicsWorld->addRigidBody(rigidBodyTableLeftBottomWall);
    
    //Create Right Bottom Wall
    btCollisionShape* tableRightBottomWall = new btStaticPlaneShape (btVector3 (1,0,1), 1);
    btDefaultMotionState *tableRightBottomWallMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(-12.25, 0, -7.00)));
    btRigidBody::btRigidBodyConstructionInfo rigidBodyTableRightBottomWallCI(0, tableRightBottomWallMotionState, tableRightBottomWall, btVector3(0,0,0));
    rigidBodyTableRightBottomWallCI.m_restitution = 0.5;
    rigidBodyTableRightBottomWall = new btRigidBody(rigidBodyTableRightBottomWallCI);
    dynamicsWorld->addRigidBody(rigidBodyTableRightBottomWall);
    
    //Create Upper Left Wall
    btCollisionShape* upperLeftWall = new btBoxShape( btVector3(1.0,1.0,1.0) );
    btDefaultMotionState *upperLeftWallMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(13.25, 0, 4)));
    btRigidBody::btRigidBodyConstructionInfo rigidBodyUpperLeftWallCI(0, upperLeftWallMotionState, upperLeftWall, btVector3(0,0,0));
    rigidBodyUpperLeftWall = new btRigidBody(rigidBodyUpperLeftWallCI);
    dynamicsWorld->addRigidBody(rigidBodyUpperLeftWall);
    
    //Create Bottom Left Wall
    btCollisionShape* bottomLeftWall = new btBoxShape( btVector3(1.0,1.0,1.0) );
    btDefaultMotionState *bottomLeftWallMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(13.25, 0, -4)));
    btRigidBody::btRigidBodyConstructionInfo rigidBodyBottomLeftWallCI(0, bottomLeftWallMotionState, bottomLeftWall, btVector3(0,0,0));
    rigidBodyBottomLeftWall = new btRigidBody(rigidBodyBottomLeftWallCI);
    dynamicsWorld->addRigidBody(rigidBodyBottomLeftWall);
    
    //Create Upper Right Wall
    btCollisionShape* upperRightWall = new btBoxShape( btVector3(1.0,1.0,1.0) );
    btDefaultMotionState *upperRightWallMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(-13.25, 0, 4)));
    btRigidBody::btRigidBodyConstructionInfo rigidBodyUpperRightWallCI(0, upperRightWallMotionState, upperRightWall, btVector3(0,0,0));
    rigidBodyUpperRightWall = new btRigidBody(rigidBodyUpperRightWallCI);
    dynamicsWorld->addRigidBody(rigidBodyUpperRightWall);
    
    //Create Bottom Right Wall
    btCollisionShape* bottomRightWall = new btBoxShape( btVector3(1.0,1.0,1.0) );
    btDefaultMotionState *bottomRightWallMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(-13.25, 0, -4)));
    btRigidBody::btRigidBodyConstructionInfo rigidBodyBottomRightWallCI(0, bottomRightWallMotionState, bottomRightWall, btVector3(0,0,0));
    rigidBodyBottomRightWall = new btRigidBody(rigidBodyBottomRightWallCI);
    dynamicsWorld->addRigidBody(rigidBodyBottomRightWall);                 

    //Create First Paddle
  	btCollisionShape* paddle1 = new btCylinderShape( btVector3(1.0,1.0,1.0) );
    mass = 15;
  	paddle1->calculateLocalInertia(mass,inertia);
  	btDefaultMotionState *paddle1MotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(10, 0, 0)));
  	btRigidBody::btRigidBodyConstructionInfo paddle1Info(mass, paddle1MotionState, paddle1, btVector3(0,0,0)); 
  	paddle1Info.m_restitution = .01;
  	paddle1Info.m_friction = 1.5;
  	rigidBodyPaddle1 = new btRigidBody(paddle1Info);
  	rigidBodyPaddle1->setLinearFactor(btVector3(1,0,1));
  	rigidBodyPaddle1->setAngularFactor(btVector3(0,0,0));
  	dynamicsWorld->addRigidBody(rigidBodyPaddle1);
  	rigidBodyPaddle1->setActivationState(DISABLE_DEACTIVATION);
  	
  	//Create Second Paddle
  	btCollisionShape* paddle2 = new btCylinderShape( btVector3(1.0,1.0,1.0) );
    mass = 15.0f;
  	paddle2->calculateLocalInertia(mass,inertia);
  	btDefaultMotionState *paddle2MotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(-10, 0, 0)));
  	btRigidBody::btRigidBodyConstructionInfo paddle2Info(mass, paddle2MotionState, paddle2, btVector3(0,0,0)); 
  	paddle2Info.m_restitution = .01; 
  	paddle2Info.m_friction = 1.5;
  	rigidBodyPaddle2 = new btRigidBody(paddle2Info);
  	dynamicsWorld->addRigidBody(rigidBodyPaddle2);
  	rigidBodyPaddle2->setLinearFactor(btVector3(1,0,1));
  	rigidBodyPaddle2->setAngularFactor(btVector3(0,0,0));
  	rigidBodyPaddle2->setActivationState(DISABLE_DEACTIVATION);
  	
  	//Determine which player side the puck will start on
  	if( p1side == true )
  	  {
  	    ran = 5;
  	  }
  	else
  	  {
   	  	ran = -5;
  	  }
  	
  	// If we're just using the normal puck (default)  
  	if(!squarePuck)
  	  {	 
   	    //Create Circular Puck
   	    btCollisionShape* puck = new btCylinderShape( btVector3(1.0,.9,1.0) );
        mass = 7.0f;
  	    puck->calculateLocalInertia(mass,inertia);
  	    btDefaultMotionState *puckMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(ran, 0, 0)));
  	    btRigidBody::btRigidBodyConstructionInfo puckInfo(mass, puckMotionState, puck, btVector3(0,0,0)); 
  	    puckInfo.m_restitution = 1.5;
  	    puckInfo.m_friction = 0.05;
  	    rigidBodyPuck = new btRigidBody(puckInfo);
  	    dynamicsWorld->addRigidBody(rigidBodyPuck);
  	    rigidBodyPuck->setLinearFactor(btVector3(1,0,1));
  	    rigidBodyPuck->setAngularFactor(btVector3(0,1,0));
  	    rigidBodyPuck->setActivationState(DISABLE_DEACTIVATION);
  	    models[2].clear();
        models[2].loadFromFile("../bin/modelInfo/puck.txt");
      }
    
    else
      {
        //Create Square Puck
        btCollisionShape* puck = new btBoxShape( btVector3(.1, .1, .1) );
        mass = 7.0f;
  	    puck->calculateLocalInertia(mass,inertia);
  	    btDefaultMotionState *puckMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(ran, 0.5, 0)));
  	    btRigidBody::btRigidBodyConstructionInfo puckInfo(mass, puckMotionState, puck, btVector3(0,0,0)); 
  	    puckInfo.m_restitution = 1.5;
  	    puckInfo.m_friction = 0.05;
  	    rigidBodyPuck = new btRigidBody(puckInfo);
  	    dynamicsWorld->addRigidBody(rigidBodyPuck);
  	    rigidBodyPuck->setLinearFactor(btVector3(1,0,1));
  	    rigidBodyPuck->setAngularFactor(btVector3(1,1,1));
  	    rigidBodyPuck->setActivationState(DISABLE_DEACTIVATION);
  	    models[2].clear();
        models[2].loadFromFile("../bin/modelInfo/cube.txt");
      }
  
  	//Create Table
  	btCollisionShape* table = new btConvexTriangleMeshShape( models[3].mTriMesh );
  	btDefaultMotionState *tableMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(0, 0, 0)));
  	btRigidBody::btRigidBodyConstructionInfo tableInfo(0, tableMotionState, table, btVector3 (0,0,0)); 
  	rigidBodyTable = new btRigidBody(tableInfo);
  	dynamicsWorld->addRigidBody(rigidBodyTable);
  	rigidBodyTable->setActivationState(DISABLE_DEACTIVATION);  	
}

void checkPaddles(int centerOfBoard)
{
  // initialize variables
	btTransform trans;
	
	// find player 1 paddle
	rigidBodyPaddle1->getMotionState()->getWorldTransform(trans);
	trans.getOpenGLMatrix(glm::value_ptr(model[0]));

  // if player 1 paddle getting to close to middle	
	if(trans.getOrigin().getX() < centerOfBoard + 1)
	  {
	    rigidBodyPaddle1->translate( btVector3( 1.0f, 0.0f, 0.0f )); 
	  }
	// if player 1 paddle too far into goal
	else if( trans.getOrigin().getX() > 12 )
	  {
	    rigidBodyPaddle1->translate( btVector3( -1.0f, 0.0f, 0.0f )); 
	  }
	
	// find player 2 paddle
	rigidBodyPaddle2->getMotionState()->getWorldTransform(trans);
	trans.getOpenGLMatrix(glm::value_ptr(model[1]));
	
	// if player 2 paddle getting to close to middle	 
	if(trans.getOrigin().getX() > centerOfBoard - 1)
	  {
			rigidBodyPaddle2->translate(btVector3( -1.0f, 0.0f, 0.0f )); 
	  }
	// if player 2 paddle too far into goal  
	else if( trans.getOrigin().getX() < -11.5 )
	  {
	    rigidBodyPaddle2->translate( btVector3( 1.0f, 0.0f, 0.0f )); 
	  }
}

void checkPuck(float goalInfo[], float goalInfo2[])
{
  // initialize variables
	btTransform trans;
	
	// find where puck is
	rigidBodyPuck->getMotionState()->getWorldTransform(trans);
	trans.getOpenGLMatrix(glm::value_ptr(model[2]));
	
	// check if player 2 scored
	if(trans.getOrigin().getX() > goalInfo[0])
	  {
		  if(trans.getOrigin().getZ() < goalInfo[1] && trans.getOrigin().getZ() > goalInfo[2])
		    {  
			    player2Score++; //Increment Score
			    p1side = true; // player 1 starts next round with puck
			    initPhysics(); 		
		    }
	  }
		
	// check if player 1 scored
	if(trans.getOrigin().getX() < goalInfo2[0])
	  {
		  if(trans.getOrigin().getZ() < goalInfo2[1] && trans.getOrigin().getZ() > goalInfo2[2])
		    {
			    player1Score++; //Increment Score
			    p1side = false;	//player 2 starts next round with puck
			    initPhysics(); 		
		    }
	  }
}

void printText(float x, float y, char * text)
{
  // initialize
  glUseProgram(0);
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_LIGHTING);
  bool blending = false;

  if(glIsEnabled(GL_BLEND))
      blending = true;
      
     glEnable(GL_BLEND);
     glColor3f(0.0, 1.0, 0.0);
     glRasterPos2f(x,y);
     
   while(*text)
    {
     glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *text);
     text++; 
    }
   
   if(!blending)
   glDisable(GL_BLEND); 
}



void myMouse(int x, int y)
{
  // initialize variables
  GLint viewport[4];
  GLdouble modelview[16];
  GLdouble projection[16];
  GLfloat winX, winY, winZ;
  GLdouble posX, posY, posZ;
 
  // get model, view, projection
  glGetDoublev( GL_MODELVIEW_MATRIX, modelview );
  glGetDoublev( GL_PROJECTION_MATRIX, projection );
  glGetIntegerv( GL_VIEWPORT, viewport );
 
  winX = (float)x;
  winY = (float)viewport[3] - (float)y;
  glReadPixels( x, int(winY), 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ );
 
  gluUnProject( winX, winY, winZ, modelview, projection, viewport, &posX, &posY, &posZ);
    
  // move mouse based on camera angle and cursor position   
  if( viewer.angle == 'c' )
    {
      rigidBodyPaddle1->setLinearVelocity( btVector3(-20 * posY , 0.0 , -20*  posX));
    }
  else if( viewer.angle == 'd' )
    {
      rigidBodyPaddle1->setLinearVelocity( btVector3(20 * posY , 0.0 , 20*  posX));
    }
  else
    {
      rigidBodyPaddle1->setLinearVelocity( btVector3(-20 * posX , 0.0 , 20*  posY));
    }
}

void myMouseMenu(int button, int state, int x, int y)
{
  // initialize variables
	GLint viewport[4];
	GLdouble modelview[16];
	GLdouble projection[16];
	GLfloat winX, winY, winZ;
	GLdouble posX, posY, posZ;

  // get model, view, projection
	glGetDoublev( GL_MODELVIEW_MATRIX, modelview );
	glGetDoublev( GL_PROJECTION_MATRIX, projection );
	glGetIntegerv( GL_VIEWPORT, viewport );

	winX = (float)x;
	winY = (float)viewport[3] - (float)y;
	glReadPixels( x, int(winY), 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ );

  // if menu is enabled
  if(menu)
    {
	    gluUnProject( winX, winY, winZ, modelview, projection, viewport, &posX, &posY, &posZ);
	    
	    // menu options
      if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
        {
          cout<< posX <<endl;
      	
      		if(posX > -.3 && posX < -.22 ) // resume game
      		  {
      			  menu = false;
      			  glutSetCursor(GLUT_CURSOR_NONE);	
      		  }
      		
      	   else if(posX > -.99 && posX < .036) // restart game
      	    {		  
       		    reset();
      	      menu = false;
      			  glutSetCursor(GLUT_CURSOR_NONE); 
      		  }
      	      		
      	   else if(posX > .07 && posY < .38) // restart game with new puck
      	    {
      			  squarePuck = !squarePuck;
      			  reset();
      	      menu = false;
      			  glutSetCursor(GLUT_CURSOR_NONE); 
      		  }
        }	
    }       
}

// MAIN FUNCTION //////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
    // Initialize glut
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(w, h);
    
    // Name and create the Window
    glutCreateWindow("Air Hockey");  
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
    glutPassiveMotionFunc(myMouse);
    glutMouseFunc(myMouseMenu);
    glutWarpPointer(w / 2, h / 2);
	  glutSetCursor(GLUT_CURSOR_NONE);


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

// FUNCTION IMPLEMENTATION ////////////////////////////////////////////////////////////////////
void render()
{
  //--Render the scene

  //clear the screen
  glClearColor(0.0, 0.0, 	0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glUseProgram(program);

	if(!menu)
	  {
		  // mvp for all objects
		  for(int i = 0; i < numModels; i++)
		    {
	   		  mvp[i] = projection * view * model[i];
		    }

		  //upload the matrix to the shader
		  for(int i = 0; i < numModels; i++)
		    {
			    glUniformMatrix4fv(loc_mvpmat, 1, GL_FALSE, glm::value_ptr(mvp[i]));
			    models[i].renderModel(program);	
		    }
		
      // determine if player 1 won
      if(player1Score >= 5 && (player1Score - player2Score) > 1)
        {
          char mes[] = {"Player 1 Wins"};
	        printText(-0.2f,0.7f, mes);
          cout << "Player 1 Wins" << endl;	 
	      }
	    
	    // determine if player 1 won
      if(player2Score >= 5 && (player2Score - player1Score) > 1)
        {
          char mes[] = {"Player 2 Wins"};  
          printText(0.0f,0.7f, mes);
          cout << "Player 2 Wins" << endl;
        }
        
      // print player scores  
      char c[20];
      sprintf(c, "%d", player2Score);
      char d[20];
      sprintf(d, "%d", player1Score);
		
		  char mes[] = {"Player 2 Score: "}; 
		  printText(0.2f,0.7f, mes);
		  printText(0.63f,0.7f,c);
		
		  char mes2[] = {"Player 1 Score: "}; 
		  printText(-0.8f,0.7f, mes2);
	    printText(-0.37f,0.7f, d);
	  }
	
	else
	  {
	    // print menu text
		  for(int i = 0; i < numModels; i++)
		    {
			    glUniformMatrix4fv(loc_mvpmat, 1, GL_FALSE, glm::value_ptr(mvp[i]));
		    }
      glutSetCursor(1);
      char mes[] = {"                                                                                             Resume                        Restart                     Change Shape of Puck(Resets Game)"};
  	  printText(-0.8f,0.0f, mes);
	  }
	
		//swap the buffers
		glutSwapBuffers(); 
}

void AI()
{
  // initialize difficulty
  int difficulty = 1.0;
  
  // find out where puck and player 2 paddle is
	btTransform trans, trans2;
	
	rigidBodyPuck->getMotionState()->getWorldTransform(trans);
	trans.getOpenGLMatrix(glm::value_ptr(model[2]));
	rigidBodyPaddle2->getMotionState()->getWorldTransform(trans2);
	trans2.getOpenGLMatrix(glm::value_ptr(model[1]));
	
	float x, y, z, a;
	
	// get coordinates of puck
	x = trans.getOrigin().getX();
	y = trans.getOrigin().getY();
	z = trans.getOrigin().getZ();
	
	// get coordinate of player 2 paddle
  a = trans2.getOrigin().getX();
	
	// if puck is on player 1's side, chill out
  if( x >= 0)
    {
      rigidBodyPaddle2->applyCentralImpulse(btVector3( 0,difficulty * y, difficulty * z ) );
    }

  else
    {
      // push puck in front of paddle
      if( (a - x) > .5)
        {
          rigidBodyPaddle2->applyCentralImpulse(btVector3( difficulty * (goal2[0]),difficulty * goal2[1], difficulty * goal2[2] ) );
        }
      else
        {
          rigidBodyPaddle2->applyCentralImpulse(btVector3( -difficulty * x/2,difficulty * y/2, difficulty * z/2 ) );
        }
    }
}

void update()
{
  // if menu is not toggled
	if(!menu)
	  {
	
	    float dt = getDT();// if you have anything moving, use dt. 
		
		  dynamicsWorld->stepSimulation(dt, 10);
	
		  btTransform trans;

      // find out where player 1 paddle is
		  rigidBodyPaddle1->getMotionState()->getWorldTransform(trans);
		  trans.getOpenGLMatrix(glm::value_ptr(model[0]));
		
      // find out where player 2 paddle is
		  rigidBodyPaddle2->getMotionState()->getWorldTransform(trans);
		  trans.getOpenGLMatrix(glm::value_ptr(model[1]));

      // find out where puck is	
	  	rigidBodyPuck->getMotionState()->getWorldTransform(trans);
		  trans.getOpenGLMatrix(glm::value_ptr(model[2])); 
	
	    // find out where table is
		  rigidBodyTable->getMotionState()->getWorldTransform(trans);
		  trans.getOpenGLMatrix(glm::value_ptr(model[3])); 

      // update camera angle if needed	
			view = glm::lookAt( glm::vec3( viewer.x, viewer.y, viewer.z ), //Eye Position
		                    glm::vec3(0.0, 0.0, 0.0), //Focus point
		                    glm::vec3(0.0, 1.0, 0.0)); //Positive Y is up
	  
    // check if puck is in any of the goals
	  checkPuck(goal1, goal2);
	  
	  // check if paddles are not where they're supposed to be
	  checkPaddles(0);
	  
	  // toggle AI on
	  if( AIon )
	    {
	      AI();
	    }
	  
	}
	
	// Party if Player 1 has won
  if(player1Score >= 5 && (player1Score - player2Score) > 1)
   {
     usleep(500000);
	   reset(); 
	 }
	
	// Party if Player 2 has won
  if(player2Score >= 5 && (player2Score - player1Score) > 1)
    {    
      usleep(500000);
	    reset();
    } 

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
  // initialize variables
	double scaler = 5.0f; 
	
	switch(key)
	  {
	    // move player 1 left
		  case ('a' | 'A'):
		    if( viewer.angle == 'c' )
		      {
		        rigidBodyPaddle1->applyCentralImpulse(btVector3( 0.0f, 0.0f, scaler * 5.0f ) );
	        }
	      else if( viewer.angle == 'd' )
	        {
	          rigidBodyPaddle1->applyCentralImpulse(btVector3( 0.0f, 0.0f, scaler * -5.0f ) );
	        }      
	      else
	        {  	
				    rigidBodyPaddle1->applyCentralImpulse(btVector3( scaler * 5.0f, 0.0f, 0.0f ) );
				  }
		  break;
		
		  // move player 1 down
		  case ('s' | 'S'):
				if( viewer.angle == 'c' )
		      {
		        rigidBodyPaddle1->applyCentralImpulse(btVector3( scaler * 5.0f, 0.0f, 0.0f ) );
	        }
	      else if( viewer.angle == 'd' )
	        {
	          rigidBodyPaddle1->applyCentralImpulse(btVector3( scaler * -5.0f , 0.0f, 0.0f ) );
	        }      
	      else
	        {  		
				    rigidBodyPaddle1->applyCentralImpulse(btVector3( 0.0f, 0.0f, scaler * -5.0f ) );
				  }
		  break;
		  	
		  // move player 1 right
		  case ('d' | 'D'):
		    if( viewer.angle == 'c' )
		      {
		        rigidBodyPaddle1->applyCentralImpulse(btVector3( 0.0f, 0.0f, scaler * -5.0f ) );
	        }
	      else if( viewer.angle == 'd' )
	        {
	          rigidBodyPaddle1->applyCentralImpulse(btVector3( 0.0f, 0.0f, scaler * 5.0f ) );
	        }      
	      else
	        {  	
				    rigidBodyPaddle1->applyCentralImpulse(btVector3( scaler * -5.0f, 0.0f, 0.0f ) );
				  }
		  break;
		
		  // move player 1 forward
		  case ('w' | 'W'):	
		    if( viewer.angle == 'c' )
		      {
		        rigidBodyPaddle1->applyCentralImpulse(btVector3( scaler * -5.0f, 0.0f, 0.0f ) );
	        }
	      else if( viewer.angle == 'd' )
	        {
	          rigidBodyPaddle1->applyCentralImpulse(btVector3( scaler * 5.0f, 0.0f, 0.0f ) );
	        }      
	      else
	        {  
  			    rigidBodyPaddle1->applyCentralImpulse( btVector3( 0.0f, 0.0f, scaler * 5.0f ) ); 
  			  }
		  break;
 			
 			// move player 2 left	
 		  case ('j' | 'J'):		
				if( viewer.angle == 'c' )
		      {
		        rigidBodyPaddle2->applyCentralImpulse(btVector3( 0.0f, 0.0f, scaler * 5.0f ) );
	        }
	      else if( viewer.angle == 'd' )
	        {
	          rigidBodyPaddle2->applyCentralImpulse(btVector3( 0.0f, 0.0f, scaler * -5.0f ) );
	        }      
	      else
	        {  	
				    rigidBodyPaddle2->applyCentralImpulse(btVector3( scaler * 5.0f, 0.0f, 0.0f ) );
				  }	
		  break;
		
		  // move player 2 backward	
		  case ('k' | 'K'):		
				if( viewer.angle == 'c' )
		      {
		        rigidBodyPaddle2->applyCentralImpulse(btVector3( scaler * 5.0f, 0.0f, 0.0f ) );
	        }
	      else if( viewer.angle == 'd' )
	        {
	          rigidBodyPaddle2->applyCentralImpulse(btVector3( scaler * -5.0f , 0.0f, 0.0f ) );
	        }      
	      else
	        {  		
				    rigidBodyPaddle2->applyCentralImpulse(btVector3( 0.0f, 0.0f, scaler * -5.0f ) );
				  }
		  break;
		
		  // move player 2 right
		  case ('l' | 'L'):				
		    if( viewer.angle == 'c' )
		      {
		        rigidBodyPaddle2->applyCentralImpulse(btVector3( 0.0f, 0.0f, scaler * -5.0f ) );
	        }
	      else if( viewer.angle == 'd' )
	        {
	          rigidBodyPaddle2->applyCentralImpulse(btVector3( 0.0f, 0.0f, scaler * 5.0f ) );
	        }      
	      else
	        {  	
				    rigidBodyPaddle2->applyCentralImpulse(btVector3( scaler * -5.0f, 0.0f, 0.0f ) );
				  }
		  break;
		
		  // move player 2 forward
		  case ('i' | 'I'):		
		    if( viewer.angle == 'c' )
		      {
		        rigidBodyPaddle2->applyCentralImpulse(btVector3( scaler * -5.0f, 0.0f, 0.0f ) );
	        }
	      else if( viewer.angle == 'd' )
	        {
	          rigidBodyPaddle2->applyCentralImpulse(btVector3( scaler * 5.0f, 0.0f, 0.0f ) );
	        }      
	      else
	        {  
  			    rigidBodyPaddle2->applyCentralImpulse( btVector3( 0.0f, 0.0f, scaler * 5.0f ) ); 
  			  }
		  break;

      // open menu		
		  case ('m' | 'M'):		
 			  menu = !menu; 
		  break;
		
		  // toggle AI on
		  case ('n' | 'N'):
		    AIon = !AIon;
		  break;

      // change camera angle
      case (' '):
        if( viewer.angle == 'a' )
          {
            viewer.setAngleB();
            viewer.angle = 'b';
          }
        else if ( viewer.angle == 'b' )
          {
            viewer.setAngleC();
            viewer.angle = 'c';
          }
        else if ( viewer.angle == 'c' )
          {
            viewer.setAngleD();
            viewer.angle = 'd';
          }
        else
          {
            viewer.setAngleA();
            viewer.angle = 'a';          
          }
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
  // initialize game physics
  initPhysics();
  
  // Initialize basic geometry and shaders for this example
	std::vector<unsigned short> faces;
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> texCoords;
	
	// read in all objects
	models[0].loadFromFile("../bin/modelInfo/paddle1.txt");
	models[1].loadFromFile("../bin/modelInfo/paddle2.txt");
	models[3].loadFromFile("../bin/modelInfo/table.txt");

    
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
    view = glm::lookAt( glm::vec3( viewer.x, viewer.y, viewer.z ), //Eye Position
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

    // delete objects	  
    delete rigidBodyPaddle1;
    delete rigidBodyPaddle2;
    delete rigidBodyPuck;
	  delete rigidBodyGround;

    // delete walls
	  delete rigidBodyTableLeftWall;
	  delete rigidBodyTableRightWall;
	  delete rigidBodyTableTopWall;
	  delete rigidBodyTableBottomWall;
	  delete rigidBodyTableLeftTopWall;
    delete rigidBodyTableRightTopWall;
    delete rigidBodyTableLeftBottomWall;
    delete rigidBodyTableRightBottomWall;  
    delete rigidBodyUpperLeftWall;
    delete rigidBodyBottomLeftWall;
    delete rigidBodyUpperRightWall;
    delete rigidBodyBottomRightWall;
}

void reset()
{
  // delete objects	
  delete rigidBodyPaddle1;
  delete rigidBodyPaddle2;
  delete rigidBodyPuck;
	delete rigidBodyGround;
	
	// delete walls
	delete rigidBodyTableLeftWall;
	delete rigidBodyTableRightWall;
	delete rigidBodyTableTopWall;
	delete rigidBodyTableBottomWall;
	delete rigidBodyTableLeftTopWall;
  delete rigidBodyTableRightTopWall;
  delete rigidBodyTableLeftBottomWall;
  delete rigidBodyTableRightBottomWall;
  delete rigidBodyUpperLeftWall;
  delete rigidBodyBottomLeftWall;
  delete rigidBodyUpperRightWall;
  delete rigidBodyBottomRightWall;
	
	// reset scores and physics
	player1Score = player2Score = 0;
	initPhysics();   
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






