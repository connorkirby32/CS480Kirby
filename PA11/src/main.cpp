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

class model
{
	public:
	  // object functions
		void loadFromFile(const string fileName); 
		void loadModel(); 
		void renderModel(GLuint program); 
		void loadTexture();
		void bindTexture(GLuint program);
		
		// object members
		string fileNameOBJ;
		string fileNameJPG;
	  btTriangleMesh *trimesh;
		
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
		
void model::loadFromFile(const string fileName)
{
  // open file
	ifstream fin(fileName.c_str());

  // extract obj and texture locations
	fin >> fileNameOBJ;
	fin >> fileNameJPG;
	
	loadModel();

	loadTexture();

  // close file
	fin.close();
}

void model::loadModel()
{
  // initialize assimp
	Assimp::Importer read;

  // create scene
	const aiScene* scene = read.ReadFile(fileNameOBJ, aiProcess_Triangulate);
	
	// create triangle mesh for object
	trimesh = new btTriangleMesh();
	
  // create each mesh 
	for( unsigned int x = 0; x < scene->mNumMeshes; x++ )
	{	
		const aiMesh* mesh = scene->mMeshes[x]; 
		
		vertices.reserve(mesh->mNumVertices);
		
		for(unsigned int i=0; i<mesh->mNumVertices; i++)
		{
		  aiVector3D pos = mesh->mVertices[i];
		  vertices.push_back(glm::vec3(pos.x, pos.y, pos.z));
		}
		
		normals.reserve(mesh->mNumVertices);
		
		texCoords.reserve(mesh->mNumVertices);
		
		for(unsigned int i=0; i<mesh->mNumVertices; i++)
		{
		  aiVector3D UVW = mesh->mTextureCoords[0][i];
		  aiVector3D norms = mesh->mNormals[i]; 
			 
		  texCoords.push_back(glm::vec2(UVW.x, UVW.y));
		  normals.push_back(glm::vec3(norms.x, norms.y , norms.z));	
		}

		// Fill face indices
		faces.reserve(3*mesh->mNumFaces);
		
		for (unsigned int i=0; i<mesh->mNumFaces; i++)
		{
			faces.push_back(mesh->mFaces[i].mIndices[0]);
			faces.push_back(mesh->mFaces[i].mIndices[1]);
			faces.push_back(mesh->mFaces[i].mIndices[2]);
		}

	  // populate triangle mesh
	  for (unsigned int i=0; i<mesh->mNumVertices; i++)
	  {
		  aiVector3D pos = mesh->mVertices[i];
	    btVector3 btVertexA(pos.x, pos.y, pos.z);
	  
	    pos = mesh->mVertices[++i];
	    btVector3 btVertexB(pos.x, pos.y, pos.z);
	  
	    pos = mesh->mVertices[++i];
	    btVector3 btVertexC(pos.x, pos.y, pos.z);
	  
	    trimesh->addTriangle(btVertexA, btVertexB, btVertexC);  
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

void model:: renderModel(GLuint program)
{	
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

void model:: loadTexture()
{
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

class shaderLoader
{
    public:
        char * load(const string fileName)
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
    char * buffer;
};

class camera
{
	public:
		void setAngleA()
		  {
		    x = 0.0;
		    y = 45.0;
		    z = -45.0;
		    direction = 1;
		  }
		void setAngleB()
		  {
		    x = 0.0;
		    y = 65.0;
		    z = -1.0;
		    direction = 1;
		  }
		void setAngleC()
		  {
		    x = 0.0;
		    y = 65.0;
		    z = 1.0;
		    direction = -1;
		  }
		void setAngleD()
		  {
		    x = 0.0;
		    y = 45.0;
		    z = 45.0;
		    direction = -1;
		  }
		  
		float x = 0.0, y = 45.0, z = -45.0; // default coordinates
		char angle = 'a'; // default camera angle
		int direction = 1;
};

class rotation
{
	public:
		void rotateX( float num )
			{
				x = num;
			}
		float getX()
			{
				return x;
			}
		void rotateY( float num )
			{
				y = num;
			}
		float getY()
			{
				return y;
			}
		void rotateZ( float num )
			{
				z = num;
			}
		float getZ()
			{
				return z;
			}
		void reverse()
			{
				direction *= -1;
			}
		int getDirection()
			{
				return direction;
			}

	private:
		float x = -1.0, y = -1.0, z = 0.0; // default coordinates
		int direction = 1; // default direction
};

// GLOBAL VARIABLES
GLuint program;// The GLSL program handle
int numModels = 5;
int numberOfBoardPieces = 10;
int playerLives = 3;
float score = 201,
      totalScore = 0;
model ball;
model ball2;
model knob1;
model knob2;
model level[2];
camera viewer;
rotation rotator;
bool menu = false,
     twoBalls = false;

//Table Tilt Info
float tableX = 0.0,tableY = 0.0, tableZ = 0.0;
float tableTiltScale = .01; 

// BULLET
btBroadphaseInterface *broadphase;
btDefaultCollisionConfiguration* collisionConfiguration;
btCollisionDispatcher* dispatcher;
btSequentialImpulseConstraintSolver* solver;
btDiscreteDynamicsWorld* dynamicsWorld;
btRigidBody* rigidBodyBall;
btRigidBody* rigidBodyBall2;
btRigidBody* rigidBodyTable;
btRigidBody* rigidBodyGround;

//Shader
GLint loc_texc;
GLint loc_position;
GLint loc_tex;
GLint loc_norm;

int w = 1920, h = 1080;// Window size

//uniform locations
GLint loc_mvpmat;// Location of the modelviewprojection matrix in the shader
GLint loc_mvmat;
//transform matrices
glm::mat4 model[5];
glm::mat4 view;//world->eye
glm::mat4 projection;//eye->clip
glm::mat4 mvp[5];//premultiplied modelviewprojection
glm::mat4 mv[5];

//--GLUT Callbacks
void render();
void update();
void reshape(int n_w, int n_h);
void keyboard(unsigned char key, int x_pos, int y_pos);

//--Resource management
bool initialize();
void cleanUp();
void reset();
void initBall2();

//--Random time things
float getDT();
std::chrono::time_point<std::chrono::high_resolution_clock> t1,t2;

void processMenuEvents(int option)
{
  // VOID
}

void createGLUTMenus() 
{
  // VOID
}

void myMouse(int x, int y) 
{
  // initialize variables
  GLint viewport[4];
  GLdouble modelview[16];
  GLdouble projection[16];
  GLfloat winX, winY, winZ;
  GLdouble posX, posY, posZ;
  btTransform tr;
  tr.setIdentity();
  btQuaternion quat;
  quat.setEuler(tableX,0.0,tableZ); //or quat.setEulerZYX depending on the ordering you want
  tr.setRotation(quat);
 
  // get model, view, projection
  glGetDoublev( GL_MODELVIEW_MATRIX, modelview );
  glGetDoublev( GL_PROJECTION_MATRIX, projection );
  glGetIntegerv( GL_VIEWPORT, viewport );
 
  winX = (float)x;
  winY = (float)viewport[3] - (float)y;
  glReadPixels( x, int(winY), 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ );
 
  gluUnProject( winX, winY, winZ, modelview, projection, viewport, &posX, &posY, &posZ);
  
  if( !menu )
  {  
    // move mouse based on camera angle and cursor position   
    if( viewer.angle == 'a' || viewer.angle == 'b' )
    {
      tableZ -= tableTiltScale * -2 * posX;
      tableY -= tableTiltScale * -2 * posY;
      quat.setEuler(tableX,tableY,tableZ); 
      tr.setRotation(quat);
      rigidBodyTable->setCenterOfMassTransform(tr);
    }
    else
    {
      tableZ -= tableTiltScale * posX;
      tableY -= tableTiltScale * posY;
      quat.setEuler(tableX,tableY,tableZ); 
      tr.setRotation(quat);
      rigidBodyTable->setCenterOfMassTransform(tr);
    }
  }
}

void initPhysics()
{
  // initialize variables
  btTransform trans;
  float mass;
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
  btDefaultMotionState *groundMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(0, -40, 0)));
  btRigidBody::btRigidBodyConstructionInfo rigidBodyGroundCI(0, groundMotionState, ground, btVector3(0,0,0));
  rigidBodyGround = new btRigidBody(rigidBodyGroundCI);
  dynamicsWorld->addRigidBody(rigidBodyGround);

  //Create Table Base
  btCollisionShape* table = new btBvhTriangleMeshShape( level[0].trimesh, true );
  btDefaultMotionState *tableMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(0, 0, 0)));
  btRigidBody::btRigidBodyConstructionInfo tableInfo(100, tableMotionState, table, btVector3(0,0,0));
  tableInfo.m_restitution = 0.0;
  tableInfo.m_friction = 0.5; 
  rigidBodyTable = new btRigidBody(tableInfo);
  dynamicsWorld->addRigidBody(rigidBodyTable);        
  rigidBodyTable->setLinearFactor(btVector3(0,0,0));
  rigidBodyTable->setAngularFactor(btVector3(0,0,0));
  rigidBodyTable->setActivationState(DISABLE_DEACTIVATION);   

  //Create Ball
  btCollisionShape* ball = new btSphereShape( 1.0 );
  mass = 10.0f;
  ball->calculateLocalInertia(mass,inertia);
  btDefaultMotionState *ballMotionState =
              new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(25.5, 5, 23)));
  btRigidBody::btRigidBodyConstructionInfo ballInfo(100, ballMotionState, ball, inertia); 
  ballInfo.m_restitution = .01; 
  ballInfo.m_friction = 1.7;
  rigidBodyBall = new btRigidBody(ballInfo);
  dynamicsWorld->addRigidBody(rigidBodyBall);
  rigidBodyBall->setLinearFactor(btVector3(1,1,1));
  rigidBodyBall->setAngularFactor(btVector3(1,1,1));;
  rigidBodyBall->setActivationState(DISABLE_DEACTIVATION);
}

void initBall2()
{       
  // initialize variables
  float mass;
  btVector3 inertia(0,0,0);
          
  //Create second ball
  btCollisionShape* ball2 = new btSphereShape( 1.0 );
  mass = 10.0f;
  ball2->calculateLocalInertia(mass,inertia);
  btDefaultMotionState *ball2MotionState =
            new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(-24, 5, 2)));
  btRigidBody::btRigidBodyConstructionInfo ball2Info(100, ball2MotionState, ball2, inertia); 
  ball2Info.m_restitution = .01; 
  ball2Info.m_friction = 1.7;
  rigidBodyBall2 = new btRigidBody(ball2Info);
  dynamicsWorld->addRigidBody(rigidBodyBall2);
  rigidBodyBall2->setLinearFactor(btVector3(1,1,1));
  rigidBodyBall2->setAngularFactor(btVector3(1,1,1));;
  rigidBodyBall2->setActivationState(DISABLE_DEACTIVATION);
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

  // get window x and y coordinates
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
      	
      		if(posX > -0.541779 && posX < -0.469542 ) // resume game
      		  {
      		    if( playerLives == 0 )
      		    {
      		      playerLives = 3;
      		      reset();
      		    }
      		      
      			  menu = false;
      			  glutSetCursor(GLUT_CURSOR_NONE);	
      		  }
      		
      	   else if(posX > -0.0867925 && posX < -0.0188679 ) // restart game
      	    {
      	      playerLives = 3;		  
       		    reset();
      	      menu = false;
      			  glutSetCursor(GLUT_CURSOR_NONE); 
      		  }
      		 else if(posX > 0.36496 && posX < 0.408086 ) // quit game
      		  {
      		    exit(0);
      		  }
        }	
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
    glutCreateWindow("Labrynth");
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

//--Implementations
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
  glColor3f(1.0, 1.0, 1.0);
  glRasterPos2f(x,y);
  
  while(*text)
  {
    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *text);
    text++;
  }
  
  if(!blending)
    glDisable(GL_BLEND);
}


void render()
{
  //--Render the scene

  //clear the screen
  glClearColor(0.0, 0.0,0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
  //Enable Shader
  glUseProgram(program);
  
  // if we are not at the menu  
  if(!menu)
  {
    for(int i = 0; i < numModels; i++)
    {
   		 mvp[i] = projection * view * model[i];
   		 mv[i] = view * model[i]; 
    }

    //upload the matrix to the shader
    glUniformMatrix4fv(loc_mvpmat, 1, GL_FALSE, glm::value_ptr(mvp[0]));
    glUniformMatrix4fv(loc_mvmat, 1, GL_FALSE, glm::value_ptr(mv[0]));
	
	  level[0].renderModel(program);
	
	  // render the second ball if it's enabled
	  if( twoBalls )
	  {
	    glUniformMatrix4fv(loc_mvpmat, 1, GL_FALSE, glm::value_ptr(mvp[1]));
    	glUniformMatrix4fv(loc_mvmat, 1, GL_FALSE, glm::value_ptr(mv[1]));
	
	    ball2.renderModel(program);
	  }
	  
    glUniformMatrix4fv(loc_mvpmat, 1, GL_FALSE, glm::value_ptr(mvp[2]));
    glUniformMatrix4fv(loc_mvmat, 1, GL_FALSE, glm::value_ptr(mv[2]));
	
	  ball.renderModel(program);
	  
	  glUniformMatrix4fv(loc_mvpmat, 1, GL_FALSE, glm::value_ptr(mvp[3]));
    glUniformMatrix4fv(loc_mvmat, 1, GL_FALSE, glm::value_ptr(mv[3]));
	
	  knob1.renderModel(program);
	  
	  glUniformMatrix4fv(loc_mvpmat, 1, GL_FALSE, glm::value_ptr(mvp[4]));
    glUniformMatrix4fv(loc_mvmat, 1, GL_FALSE, glm::value_ptr(mv[4]));
	
	  knob2.renderModel(program);
	
	  // print HUD
	  char c[20];
    sprintf(c, "%d", playerLives);
  
    char d[20];
    sprintf(d, "%d", int(score));
  
    char e[20];
    sprintf(e, "%d", int(totalScore));

    char mes[] = {"Player Lives: "};
    printText(0.4f,0.75f, mes);
    printText(0.6f,0.75f,c);
  
    char mes2[] = {"Clock Score: "};
    printText(-0.75f,0.75f, mes2);
    printText(-0.6f,0.75f,d);
  
    char mes3[] = {"Total Score: "};
    printText(-0.2f,0.75f, mes3);
    printText(0.0f,0.75f,e);
  
    char mes4[] = {"Goal"};
    char mes5[] = {"|"};
  
    // display where the goal is based on the camera angle
    if( viewer.angle == 'a'|| viewer.angle == 'b' )
    {
      printText(0.8f,-0.3f, mes4);
      printText(0.825f,-0.35f,mes5);
    }
  
    else if( viewer.angle == 'c' || viewer.angle == 'd' )
    {
      printText(-0.85f,0.3f, mes4);
      printText(-0.825f,0.35f,mes5);
    }
  
    char mes6[] = {"Camera View: "};
  
    if( viewer.angle == 'a' )
    {
      char mes7[] = {"Normal"};
      printText(0.65f,0.9f, mes6);
      printText(0.8f,0.9f,mes7);
    }
  
    else if( viewer.angle == 'b' )
    {
      char mes7[] = {"Bird's Eye"};
      printText(0.65f,0.9f, mes6);
      printText(0.8f,0.9f,mes7);
    }
    else if( viewer.angle == 'c' )
    {
      char mes7[] = {"Reverse Bird's Eye"};
      printText(0.65f,0.9f, mes6);
      printText(0.8f,0.9f,mes7);
    }
    else if( viewer.angle == 'd' )
    {
      char mes7[] = {"Reverse"};
      printText(0.65f,0.9f, mes6);
      printText(0.8f,0.9f,mes7);
    } 
   
  }
  
  // we are at the menu
  else
	{
	  // print menu text
		for(int i = 0; i < numModels; i++)
		{
	    glUniformMatrix4fv(loc_mvpmat, 1, GL_FALSE, glm::value_ptr(mvp[i]));
		}
		
    glutSetCursor(1);
    
    // if we are at the menu because game is over  
    if( playerLives == 0 )
  	{
  	  char mes2[] = {"GAME OVER"};
      printText(0.0525f,0.3f, mes2);
      char mes[] = {"                                                                                                                              Restart                                                                        Quit"};
  	  printText(-0.8f,0.0f, mes);
    }
    
    // menu was actually toggled
    else
    {
      char mes[] = {"                                                Resume                                                                        Restart                                                                        Quit"};
  	  printText(-0.8f,0.0f, mes);
  	}
	}
	
  //swap the buffers
  glutSwapBuffers();  
}

void checkBall()
{
  // initialize variables
  btTransform trans;
  btTransform trans2;

  // if only playing with one ball
  if( !twoBalls )
  {
    // find where ball is
    rigidBodyBall->getMotionState()->getWorldTransform(trans);
    trans.getOpenGLMatrix(glm::value_ptr(model[2]));

    // check if ball fell off board
    if(trans.getOrigin().getY() < -37)
    {
      playerLives--; // decrement lives
      tableX = tableY = tableZ = 0.0; // reset table
      initPhysics(); // reinitialize physics
    }
    
    // if ball is actually in goal
    else if( trans.getOrigin().getX() < -32 )
    {
      totalScore += int(score); // add current score to total
      score = 201; // reset current score
      tableX = tableY = tableZ = 0.0; // reset table
      reset(); // reset objects
      initPhysics(); // reinitialize object physics
    }
  }
  
  // two balls are playing
  else
  {
    // find where first ball is
    rigidBodyBall->getMotionState()->getWorldTransform(trans);
    trans.getOpenGLMatrix(glm::value_ptr(model[2]));

    // check if ball fell off board
    if(trans.getOrigin().getY() < -37)
    {
      playerLives--; // decrement lives
      tableX = tableY = tableZ = 0.0; // reset board
      initPhysics(); // reinitialize physics
      initBall2(); // reinitialize second ball
    }
    
    // if first ball is in goal
    else if( trans.getOrigin().getX() < -32 )
    {
      // find where second ball is
      rigidBodyBall2->getMotionState()->getWorldTransform(trans2);
      trans2.getOpenGLMatrix(glm::value_ptr(model[2]));
      
      // if second ball is in goal as well
      if( trans2.getOrigin().getX() < -32 )
      {
        totalScore += int(score); // add current score to total
        score = 201; // reset current score
        tableX = tableY = tableZ = 0.0; // reset table
        reset(); // reset objects
        initPhysics(); // reinitialize object physics
        initBall2(); // reinitialize second ball
      }
    } 
    
    // find where second ball is
    rigidBodyBall2->getMotionState()->getWorldTransform(trans2);
    trans2.getOpenGLMatrix(glm::value_ptr(model[1]));

    // check if ball fell off board
    if(trans2.getOrigin().getY() < -37)
    {
      playerLives--; // decrement lives
      tableX = tableY = tableZ = 0.0;
      initPhysics();
      initBall2();
    }
    
    // if second ball is in goal
    else if( trans2.getOrigin().getX() < -32 )
    {  
      // if first ball is in goal as well
      if( trans.getOrigin().getX() < -32 )
      {
        totalScore += int(score); // add to total score
        score = 201; // reset current score
        tableX = tableY = tableZ = 0.0; // reset table
        reset(); // reset objects
        initPhysics(); // reinitialize object physics
        initBall2(); // initialize second ball
      }
    }
  }
  
  
}

void update()
{
  if( !menu )
  {
    glutWarpPointer(w / 2, h / 2);
    glutSetCursor(GLUT_CURSOR_NONE);	
    // if out of lives
    if( playerLives == 0 )
    {
      menu = !menu; // prompt up menu
    }   
      
    float dt = getDT();
    dynamicsWorld->stepSimulation(dt, 100);
        
    score -= dt; // decrement current score
    
    model[3] = glm::translate( glm::mat4(1.0f), glm::vec3(50.0 , 0.0, 4.0));
    model[3] = glm::rotate( model[3], tableZ, glm::vec3(0.0, rotator.getY(), 0.0) );
    
    model[4] = glm::translate( glm::mat4(1.0f), glm::vec3(-45.0 , 0.0, 4.0));
    model[4] = glm::rotate( model[4], tableY, glm::vec3(0.0, rotator.getX(), 0.0) );

    // Update the state of the scene
    glutPostRedisplay();//call the display callback
        
    btTransform trans;

    // update board
	  rigidBodyTable->getMotionState()->getWorldTransform(trans);
	  trans.getOpenGLMatrix(glm::value_ptr(model[0]));
	  
	  // if two ball are playing, update second one
	  if(twoBalls)
	  {
	    rigidBodyBall2->getMotionState()->getWorldTransform(trans);
	    trans.getOpenGLMatrix(glm::value_ptr(model[1]));
	  }
	  
	  // update ball
	  rigidBodyBall->getMotionState()->getWorldTransform(trans);
	  trans.getOpenGLMatrix(glm::value_ptr(model[2]));
	  
	  // check if ball is out of bounds or at goal
	  checkBall();
	  
	  // update camera angle
	  view = glm::lookAt( glm::vec3( viewer.x, viewer.y, viewer.z ), //Eye Position
		                    glm::vec3(0.0, 0.0, 0.0), //Focus point
		                    glm::vec3(0.0, 1.0, 0.0)); //Positive Y is up
  } 
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
  // initialize
  btTransform tr;
  tr.setIdentity();
  btQuaternion quat;
  quat.setEuler(tableX,0.0,tableZ); //or quat.setEulerZYX depending on the ordering you want
  tr.setRotation(quat);
  
  // Handle keyboard input
  if(key == 27)//ESC
  {
   exit(0);
  }
  
  if(key == 'a') // tilt board left
  {
    if( viewer.direction == 1 ) // if normal direction
    { 
      tableZ -= tableTiltScale;
      quat.setEuler(tableX,tableY,tableZ); 
      tr.setRotation(quat);
      rigidBodyTable->setCenterOfMassTransform(tr);
    }
    else // if reverse direction
    {
      tableZ += tableTiltScale;
      quat.setEuler(tableX,tableY,tableZ); 
      tr.setRotation(quat);
      rigidBodyTable->setCenterOfMassTransform(tr);               
    }
    
    rotator.rotateY( rotator.getY() + 10.0 );
  }
  
  if(key == 'd') // tilt board right
  {
    if( viewer.direction == 1 )
    {
      tableZ += tableTiltScale;
      quat.setEuler(tableX,tableY,tableZ); 
      tr.setRotation(quat);
      rigidBodyTable->setCenterOfMassTransform(tr);
    }
    else
    { 
      tableZ -= tableTiltScale;
      quat.setEuler(tableX,tableY,tableZ); 
      tr.setRotation(quat);
      rigidBodyTable->setCenterOfMassTransform(tr);
    } 
  }
  
  if(key == 'w') // tilt board up
  {
    if( viewer.direction == 1 )
    {
      tableY += tableTiltScale;
      quat.setEuler(tableX,tableY,tableZ);
      tr.setRotation(quat);
      rigidBodyTable->setCenterOfMassTransform(tr);
    }  
    else
    {
      tableY -= tableTiltScale;
      quat.setEuler(tableX,tableY,tableZ);
      tr.setRotation(quat);
      rigidBodyTable->setCenterOfMassTransform(tr);               
    }
    rotator.rotateX( rotator.getX() + 25.0 );   
  }
  
  if(key == 's') // tilt board downward
  {
    if( viewer.direction == 1 ) 
    {
      tableY -= tableTiltScale;
      quat.setEuler(tableX,tableY,tableZ); 
      tr.setRotation(quat);
      rigidBodyTable->setCenterOfMassTransform(tr);
    }
    else
    {
      tableY += tableTiltScale;
      quat.setEuler(tableX,tableY,tableZ); 
      tr.setRotation(quat);
      rigidBodyTable->setCenterOfMassTransform(tr);               
    }
  }
    
  // play with two balls    
  if( key == 'n' )
  {
    if( !twoBalls )
    {
      twoBalls = true;
      playerLives = 3;
      reset();
      initPhysics();
      initBall2();
    }
  }
  
  // toggle menu      
  if( key == 'm' )
  {		
 	  menu = !menu; 
  }
  
  // cheange camera angle  
  if (key == ' ')
  {
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
  }
}

bool initialize()
{
  // Read in .obj file
	level[0].loadFromFile("../bin/Level1/table.txt");	
	ball2.loadFromFile("../bin/Ball/ball.txt");
	ball.loadFromFile("../bin/Ball/ball.txt");
	knob1.loadFromFile("../bin/Level1/knob.txt");
	knob2.loadFromFile("../bin/Level1/knob.txt");
	
  // create shaders  
  GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

    
  //Set shader char pointers
  shaderLoader x; 
  const char *vs = x.load("../bin/Shaders/vertexShader.vert");
  const char *fs = x.load("../bin/Shaders/fragmentShader.frag"); 

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
    exit(0);
  }

  // Now the Fragment shader
  glShaderSource(fragment_shader, 1, &fs, NULL);
  glCompileShader(fragment_shader);
  //check the compile status
  glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &shader_status);
  if(!shader_status)
  {
    std::cerr << "[F] FAILED TO COMPILE FRAGMENT SHADER!" << std::endl;
    exit(0); 
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

  // initialize object physics
  initPhysics();  
    
  //and its done
  return true;
}

void cleanUp()
{
  // Clean up, Clean up
  glDeleteProgram(program);
    
  delete rigidBodyTable;
  delete rigidBodyBall;
  delete rigidBodyBall2;
	delete rigidBodyGround;
	
}

void reset()
{
  // delete objects	
  delete rigidBodyTable;
  delete rigidBodyBall;
  delete rigidBodyBall2;
	delete rigidBodyGround;

	// reset scores and physics
	score = 201;
	totalScore = 0;
	tableX = tableY = tableZ = 0.0;
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

