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
#include <stdlib.h>
#include <sys/time.h>

#include <assimp/Importer.hpp>    
#include <assimp/scene.h>           
#include <assimp/postprocess.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> //Makes passing matrices to shaders easier

#include <SOIL/SOIL.h>
#include <SFML/Audio.hpp>

#include "modelClass.h"

#include "../AntTweakBar/include/AntTweakBar.h"

using namespace std;

// Classes ////////////////////////////////////////////////////////////////////////

//Camera
class camera
{
public:
void setAngleA()
 {
   x = 0.0;
   y = 15.0;
   z = -65.0;
 }
void setAngleB()
 {
   x = 0.0;
   y = 45.0;
   z = -65.0;
 }
 
float x = 0.0, y = 15.0, z = -65.0; // default coordinates
char angle = 'a'; // default camera angle
};
 
// Globals ////////////////////////////////////////////////////////////////////////

camera viewer;

TwBar *bar;
void antInit(); 

// Material
float g_MatAmbient[] = { 1.0f, 1.0f, 1.0f, 1.0f };
float g_MatDiffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
// Light parameter
float g_LightMultiplier = 1.0f;
float g_LightDirection[] = { -0.57735f, -0.57735f, -0.57735f };

// Game Info
bool ballSelect = false;
int score = 0;
int tries = 5; 

//Model Info
model ball;
model level[1];
model pegs[100];
model holder;
model scoreboard;
model medal;
int numModels = 105; 
string medalType;
bool awardedMedal = false;
int highScores[5];

//Bullet
btBroadphaseInterface *broadphase;
btDefaultCollisionConfiguration* collisionConfiguration;
btCollisionDispatcher* dispatcher;
btSequentialImpulseConstraintSolver* solver;
btDiscreteDynamicsWorld* dynamicsWorld;
btRigidBody* rigidBodyBall;
btRigidBody* rigidBodyBall2;
btRigidBody* rigidBodyBall3;
btRigidBody* rigidBodyTable;
btRigidBody* rigidBodyGround;
btRigidBody* rigidBodyPlane;
btRigidBody* rigidBodyCeiling;
btRigidBody* rigidBodyLeftWall;
btRigidBody* rigidBodyRightWall;
btRigidBody* rigidBodyBack;
btRigidBody* rigidBodyHolder;
btRigidBody* ballPointer;
btRigidBody* rigidBodyPegs[100];

void initPhysics();

extern ContactProcessedCallback gContactProcessedCallback;
static bool HandleContacts(btManifoldPoint& point, btCollisionObject* body0, btCollisionObject* body1);

// sound, music variables
sf::SoundBuffer buffer;
sf::Sound sound;
sf::SoundBuffer buffer2;
sf::Sound sound2;
sf::SoundBuffer buffer3;
sf::Sound sound3;

//Shader
GLint loc_texc;
GLint loc_position;
GLint loc_tex;
GLint loc_norm;
GLint loc_lightDir;

//uniform locations
GLint loc_mvpmat;// Location of the modelviewprojection matrix in the shader
GLint loc_mvmat;

//Shader Program
GLuint program;
GLint loc_texc1;
GLint loc_position1;
GLint loc_tex1;
GLint loc_norm1;
GLint loc_lightDir1;

//uniform locations
GLint loc_mvpmat1;// Location of the modelviewprojection matrix in the shader
GLint loc_mvmat1;

//Shader Program
GLuint program1;

// Window size
int w = 1920, h = 1080;

//transform matrices
glm::mat4 model[105];
glm::mat4 view;//world->eye
glm::mat4 projection;//eye->clip
glm::mat4 mvp[105];//premultiplied modelviewprojection
glm::mat4 mv[105];

//--GLUT Callbacks
void render();
void update();
void reshape(int n_w, int n_h);
void keyboard(unsigned char key, int x_pos, int y_pos);
void myMouse(int button, int state, int x, int y);
void myMouseMovement( int x, int y); 

void readInHighScores();
void writeHighScores();
void updateHighScores();

//Resource management
bool initialize();
void cleanUp();

//Random time things
float getDT();
std::chrono::time_point<std::chrono::high_resolution_clock> t1,t2;

void Terminate();

//Bools for checking if balls have scored and if they are in play. 
bool B1 = false, B2 = false, B3 = false;
bool W1 = false, W2 = false, W3 = false;
bool paused = false;
bool won = false;  
void checkGame();

void TW_CALL exit(void *)
{ 
  // exit from menu
  cleanUp();
  exit(0);
}
void TW_CALL reset(void *)
{ 
  // menu reset
	initPhysics();
	score = 0;
	B1 = B2 = B3 = W1 = W2 = W3 = false; 

}

void TW_CALL pause(void *)
{ 
  // menu pause or unpause game
  paused = !paused; 
}

void TW_CALL cheat(void *)
{ 
  // menu option to cheat
  if(score < 5000)
  {	
  		score+= 1000; 
  }
  if(tries > 0)
  {
  		tries -= 1;
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
    glutCreateWindow("Planko");

    // Now that the window is created the GL context is fully set up
    // Because of that we can now initialize GLEW to prepare work with shaders
    GLenum status = glewInit();
    if( status != GLEW_OK)
    {
        std::cerr << "[F] GLEW NOT INITIALIZED: ";
        std::cerr << glewGetErrorString(status) << std::endl;
        return -1;
    }
    
    // Initialize AntTweakBar
    TwInit(TW_OPENGL, NULL);

    // read in high scores for game
    readInHighScores();
    
    // stream and play theme music
    sf::Music music;
    music.openFromFile("../bin/Audio/theme.ogg");
    music.play();
    
    // load sound effects for calling
    buffer.loadFromFile("../bin/Audio/applause.ogg");
    sound.setBuffer(buffer);
    sound.play();
    buffer2.loadFromFile("../bin/Audio/smallApplause.ogg");
    sound2.setBuffer(buffer2);
    buffer3.loadFromFile("../bin/Audio/boo.ogg");
    sound3.setBuffer(buffer3);

    // Set all of the callbacks to GLUT that we need
    glutDisplayFunc(render);// Called when its time to display
    glutReshapeFunc(reshape);// Called if the window is resized
    glutIdleFunc(update);// Called if there is nothing else to do
    glutKeyboardFunc(keyboard);// Called if there is keyboard input
    glutPassiveMotionFunc(myMouseMovement);
    glutMouseFunc(myMouse);
    //glutSpecialFunc((GLUTspecialfun)TwEventSpecialGLUT);
    
    // Initialize all of our resources(shaders, geometry)
    bool init = initialize();
    
    //Create Ant Tweak Bar
    antInit();

    if(init)
    {
        t1 = std::chrono::high_resolution_clock::now();
        glutMainLoop();
    }

    // Clean up after ourselves
    cleanUp();
    return 0;
}

void readInHighScores()
{
  // initialize and read in file
  ifstream fin;
  fin.open( "../bin/highScores.txt" );
  
  // if no high scores create blank scores
  if( !fin.good() )
  {
    for( int i = 0; i < 5; i++ )
    {
      highScores[i] = 0;
    }
  }
  
  // store scores
  else
  {
    int num;
    char dummy;
    for( int i = 0; i < 5; i++ )
    {
      fin >> num >> dummy;
      highScores[i] = num;
    }
  }
  
  fin.close();
}

void writeHighScores()
{
  // read updated scores into file
  ofstream fout("highScores.txt");  
  for( int i = 0; i < 5; i++ )
  {
    if( i == 4 )
    {
      fout << highScores[ i ] << ";" << endl;
    }
    else
    {
      fout << highScores[ i ] << "," << endl;
    }
  }
  fout.close();
}

void updateHighScores()
{
  // update high scores at end of every game
  
  // bubble sort in descending order
  for( int i = 0; i < 4; i++ )
  {
    for( int j = 0; j < (5 - i - 1); j++ )
    {
      if( highScores[ j ] < highScores[ j + 1 ] )
      {
        int swap = highScores[ j ];
        highScores[ j ] = highScores[ j + 1 ];
        highScores[ j + 1 ] = swap;
      }
    }
  }

  // check if current score is higher than any in the list
  for( int i = 0; i < 5; i++ )
  {
    if( score > highScores[i] )
    {    
      for( int j = 4; j > i; j-- )
      {
        highScores[j] = highScores[j-1];       
      }
      
      highScores[i] = score; 
      return;
    }
  }
}

//--Function Implementations
void printText(float x, float y, const char * text)
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

	//Used to set light parameters
	GLfloat v[4]; 
	
  //clear the screen
  glClearColor(0.0, 0.0,0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
  glDisable(GL_CULL_FACE);
  glEnable(GL_NORMALIZE);
  
  //Enable Shader
  glUseProgram(program);
 
  for(int i = 0; i < numModels; i++)
  {  
   		 mvp[i] = projection * view * model[i];
   		 mv[i] = view * model[i];
  }
  
  // Set light
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
    
  // Set material
  glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, g_MatAmbient);
  glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, g_MatDiffuse);

  //upload the matrix to the shader  
  if(!awardedMedal)
  {   
    v[0] = v[1] = v[2] = g_LightMultiplier*0.4f; v[3] = 1.0f;
    glLightfv(GL_LIGHT0, GL_AMBIENT, v);
  }
    
  else
  {
    float r = rand() % 15;      
	  v[0] = r;
	  v[1] = r;
	  v[2] = r; 
	  v[3] = 1.0f;
    glLightfv(GL_LIGHT0, GL_AMBIENT, v);
  }
  
  v[0] = v[1] = v[2] = g_LightMultiplier*0.8f; v[3] = 1.0f;
  glLightfv(GL_LIGHT0, GL_DIFFUSE, v);
  v[0] = -g_LightDirection[0]; v[1] = -g_LightDirection[2]; v[2] = -g_LightDirection[1]; v[3] = 0.0f;
  glLightfv(GL_LIGHT0, GL_POSITION, v); 

	glUniformMatrix4fv(loc_mvpmat, 1, GL_FALSE, glm::value_ptr(mvp[0]));
	glUniformMatrix4fv(loc_mvmat, 1, GL_FALSE, glm::value_ptr(mv[0]));

  // render board
	level[0].renderModel(program);

  // render pegs on board	
  for( int i = 0; i < 100; i++ )
  {	
    glUniformMatrix4fv(loc_mvpmat, 1, GL_FALSE, glm::value_ptr(mvp[i+2]));
	  glUniformMatrix4fv(loc_mvmat, 1, GL_FALSE, glm::value_ptr(mv[i+2]));
	
	  pegs[i].renderModel(program);
	}
		
	// render ball		 
	glUniformMatrix4fv(loc_mvpmat, 1, GL_FALSE, glm::value_ptr(mvp[1]));
	glUniformMatrix4fv(loc_mvmat, 1, GL_FALSE, glm::value_ptr(mv[1]));
	
	ball.renderModel(program);

  // render ball holder
  glUniformMatrix4fv(loc_mvpmat, 1, GL_FALSE, glm::value_ptr(mvp[102]));
	glUniformMatrix4fv(loc_mvmat, 1, GL_FALSE, glm::value_ptr(mv[102]));
	
	holder.renderModel(program);
	
	// render scoreboard
	glUniformMatrix4fv(loc_mvpmat, 1, GL_FALSE, glm::value_ptr(mvp[103]));
	glUniformMatrix4fv(loc_mvmat, 1, GL_FALSE, glm::value_ptr(mv[103]));
	
	scoreboard.renderModel(program);
	
	// render medal is awarded
	if( tries == 0 && medalType != "None" )
	{
	  glUseProgram(program1);
	  glUniformMatrix4fv(loc_mvpmat1, 1, GL_FALSE, glm::value_ptr(mvp[104]));
	  glUniformMatrix4fv(loc_mvmat, 1, GL_FALSE, glm::value_ptr(mv[104]));
	  
	  medal.renderModel(program1);	  
	}
	
	// print HUD
 	char c[20];
  sprintf(c, "%d", tries);
  char d[20];
  sprintf(d, "%d", score);
  char mes[] = {"Tries"};
  char mes2[] = {"Score"}; 
  char e[20];
  sprintf(e, "%d", 1000);
  char f[20];
  sprintf(f, "%d", 0); 
  char g[20];
  sprintf(g, "%d", 500);
  char h[20];
  sprintf(h, "%d", 100);
	
	// print values on screen
  if( viewer.angle == 'a' )
  {
      printText(0.675f,0.5f, mes);
      printText(0.69f,0.6f,c);
      printText(0.56f,0.5f, mes2);
      printText(0.57f,0.6f,d);
      printText(-0.02f,-0.74f,e);
      printText(-0.073f,-0.74f,f);
      printText(0.0655f,-0.74f,f);
      printText(-0.15f,-0.74f,g);
      printText(0.125f,-0.74f,g);
      printText(-0.2175f,-0.74f,h);
      printText(0.19f,-0.74f,h);
  }
  else
  {
      printText(0.62f,0.375f, mes);
      printText(0.635f,0.47f,c);
      printText(0.505f,0.37f, mes2);
      printText(0.525f,0.47f,d);
      printText(-0.02f,-0.52f,e);
      printText(-0.058f,-0.52f,f);
      printText(0.0505f,-0.52f,f);
      printText(-0.119f,-0.52f,g);
      printText(0.09f,-0.52f,g);
      printText(-0.1685f,-0.52f,h);
      printText(0.14f,-0.52f,h);    
  }
   
  // print end game results if there 
  if( tries == 0 && awardedMedal )
  { 
    char mes5[] = {"Pick up the ball to play again"};
    printText(0.54f,-0.65f, mes5);
    if( medalType != "None" )
    {
      char mes3[] = {"Congratulations!"};  
      char mes4[] = {"medal"};  
      
      const char* i = medalType.c_str();
      printText(0.59f,-0.57f, mes3);
      printText(0.5975f,-0.61f,i);
      printText(0.6775f,-0.61f, mes4);
      
    }
  }
	
	// draw the menu bar
	TwDraw();

  //swap the buffers
  glutSwapBuffers();   
}

void checkBall()
{
  // initialize variables
  btTransform trans;

  // find where ball is
  rigidBodyBall->getMotionState()->getWorldTransform(trans);
  trans.getOpenGLMatrix(glm::value_ptr(model[1]));

  // check where ball scored
  if( trans.getOrigin().getX() < 17 && trans.getOrigin().getX() > -17 )
  {
    if( trans.getOrigin().getY() < -24.25 )
    {
      // if in 100 points goals
      if( trans.getOrigin().getX() > 10.65 || trans.getOrigin().getX() < -10.65 )
      {
        score += 100;
        if( tries > 1 )
        {
          sound2.play();
        }
      }
      // if in right 500 points goal
      else if( trans.getOrigin().getX() < 10.65 && trans.getOrigin().getX() > 7.969 )
      {
        score += 500;
        if( tries > 1 )
        {
          sound2.play();
        }
      }
      // if in left 500 points goal
      else if( trans.getOrigin().getX() > -10.65 && trans.getOrigin().getX() < -7.969 )
      {
        score += 500;
        if( tries > 1 )
        {
          sound2.play();
        }
      }
      // if in right 0 points goal
      else if( trans.getOrigin().getX() < 6 && trans.getOrigin().getX() > 3.3 )
      {
        score += 0;
        if( tries > 1 )
        {
          sound3.play();
        }
      }
      // if in left 0 points goal
      else if( trans.getOrigin().getX() > -6 && trans.getOrigin().getX() < -3.3 )
      {
        score += 0;
        if( tries > 1 )
        {
          sound3.play();
        }
      }
      // if in 1000 point goal
      else if( trans.getOrigin().getX() < 1.33  && trans.getOrigin().getX() > -1.33 )
      {
        score += 1000;
        if( tries > 1 )
        {
          sound2.play();
        }
      }
       
      // reset board  
      tries--;
      ballSelect = false;        
      initPhysics(); // reinitialize physics
    }
  }
}

void calculateWin()
{
	    if( score >= 500 && score <= 2000 )
	    {
	      // BRONZE MEDAL
	      medalType = "Bronze";
	      medal.loadFromFile("../bin/Level1/bronzeMedal.txt");
	      sound.play();	      
	    }
	    else if( score > 2000 && score < 4000 )
	    {
	      // SILVER MEDAL
	      medalType = "Silver";
	      medal.loadFromFile("../bin/Level1/silverMedal.txt");
	      sound.play();
	    }
	    else if( score >= 4000 && score < 5000 )
	    {
	      // GOLD MEDAL
	      medalType = "Gold";
	      medal.loadFromFile("../bin/Level1/goldMedal.txt");
	      sound.play();
	    }
	    else if( score == 5000 )
	    {
	      // PLATINUM MEDAL

	      medalType = "Platinum";
	      medal.loadFromFile("../bin/Level1/platinumMedal.txt");
	      sound.play();
	    }
	    else
	    {
	      // NO MEDAL
	      medalType = "None";
	    }
	    
	    updateHighScores();
	    writeHighScores();
}

void update()
{   
	//Update Camera
	view = glm::lookAt( glm::vec3(0.0, viewer.y, viewer.z), //Eye Position
      	glm::vec3(0.0, 0.0, 0.0), //Focus point
		glm::vec3(0.0, 1.0, 0.0)); //Positive Y is up   
		
	if(!paused)
	{
	  // if game ended
	  if( tries == 0 && !awardedMedal )
	  {
	    awardedMedal = true;
      calculateWin();
      	ballSelect = false;
	  }
	  
	  // if player is starting new game
	  if( tries == 0 && ballSelect && awardedMedal )
	  {
	    tries = 5;
	    score = 0;
	    awardedMedal = false;
	    medalType = "None";
	  }
	  
		float dt = getDT();
		dynamicsWorld->stepSimulation(dt, 10);
		
		// angle used to rotate medal
		static float angle = 0.0;
		angle+= dt * M_PI/2; //move through 90 degrees a second
		
		// move scoreboard to upper right corner
		model[103] = glm::translate( glm::mat4(1.0f), glm::vec3(-40.0 , 20.0, 0.0));
		
		// show medal if at end of game and are rewarded one
		if( tries == 0 && medalType != "None" )
		{
		  model[104] = glm::translate( glm::mat4(1.0f), glm::vec3(-45.0 , -10.0, 0.0));
		  model[104] = glm::rotate( model[104], angle, glm::vec3(0.0, 1.0, 0.0) );
		}

		// Update the state of the scene
		glutPostRedisplay();

    // update objects in motion
		btTransform trans;
		
		rigidBodyTable->getMotionState()->getWorldTransform(trans);
		trans.getOpenGLMatrix(glm::value_ptr(model[0]));

		rigidBodyBall->getMotionState()->getWorldTransform(trans);
		trans.getOpenGLMatrix(glm::value_ptr(model[1]));
		
		// disable ball select if already in board
		if( trans.getOrigin().getX() > -17 && trans.getOrigin().getX() < 17
		    && trans.getOrigin().getY() < 26 )
		{
		  ballSelect = false;
		}
		
		// used for debugging purposes
		//cout << trans.getOrigin().getX() << " " << trans.getOrigin().getY() << endl;
		
		// update more objects in motion
		for( int i = 2; i < 102; i++ )
		{
		  rigidBodyPegs[i-2]->getMotionState()->getWorldTransform(trans);
		  trans.getOpenGLMatrix(glm::value_ptr(model[i]));
    }
    
    rigidBodyHolder->getMotionState()->getWorldTransform(trans);
		trans.getOpenGLMatrix(glm::value_ptr(model[102]));
 	}
 	
 	  
 	/*
 	if(won){
 	//Do some cool shit then exit
 		//displayFireworks();
 		int x;
 		cin>>x;
 		cleanUp();
 		exit(0); 		
 	}
 	*/
 	
 	// check if ball is in goals
 	checkBall();  
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
       
    //Update Tweak Bar
    TwWindowSize(w, h);
}

void keyboard(unsigned char key, int x_pos, int y_pos)
{
  // initialize variables
  btTransform tr;
  tr.setIdentity();

  btQuaternion quat;
           
  if( !TwEventKeyboardGLUT(key, x_pos, y_pos))  // send event to AntTweakBar
  {     
  }
         
        // Handle keyboard input
        if(key == 27)//ESC
        {
        		cleanUp();
                exit(0);
        }
        
        // cheat points
        if( key == 'n')
        {
          score += 1000;
        }
        
        // end game
        if( key == 'm')
        {
          tries = 0;
        }
        
        // jump ball up if stuck
        if( key == 'q')
        {
          btTransform trans;
          rigidBodyBall->getMotionState()->getWorldTransform(trans);
          trans.getOpenGLMatrix(glm::value_ptr(model[1]));
          if( trans.getOrigin().getX() > -17 && trans.getOrigin().getX() < 17
            && trans.getOrigin().getY() < 26 )
          { 
            ballPointer->setLinearVelocity( btVector3(0.0 , 1.0, 0.0));
          }
        }
        
        // change camera angles
        if( key == ' ' )
        {
          if( viewer.angle == 'b' )
          {
            viewer.setAngleA();
            viewer.angle = 'a';
          }
          else
          {
            viewer.setAngleB();
            viewer.angle = 'b';
          }
        }   
}

bool initialize()
{ 
  // Read our .obj files

	level[0].loadFromFile("../bin/Level1/table.txt");	
	ball.loadFromFile("../bin/Ball/ball.txt");
	
	for( int i = 0; i < 100; i++ )
	{
	  pegs[i].loadFromFile("../bin/Level1/peg.txt");	
	}
  holder.loadFromFile("../bin/Level1/holder.txt");
  scoreboard.loadFromFile("../bin/Level1/scoreboard.txt");
	
    //--Geometry done
   
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
        std::cerr << "INCONSISTENCY WITH VIRTUAL MACHINE, please run again" << std::endl;   
        return 1;   
    }

    // Now the Fragment shader
    glShaderSource(fragment_shader, 1, &fs, NULL);
    glCompileShader(fragment_shader);
    
    //check the compile status
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &shader_status);
    if(!shader_status)
    {
        std::cerr << "INCONSISTENCY WITH VIRTUAL MACHINE, please run again" << std::endl;
        return 1;  
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
                    
                    
    //Set shader char pointers for coin
    shaderLoader xC; 
    const char *vsC = xC.load("../bin/Shaders/coinShader.vert");
    const char *fsC = xC.load("../bin/Shaders/coinShader.frag"); 

    // Vertex shader first
    glShaderSource(vertex_shader, 1, &vsC, NULL);
    glCompileShader(vertex_shader);
    //check the compile status
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &shader_status);
    if(!shader_status)
    {
        std::cerr << "INCONSISTENCY WITH VIRTUAL MACHINE, please run again" << std::endl; 
        return 1;      
    }

    // Now the Fragment shader
    glShaderSource(fragment_shader, 1, &fsC, NULL);
    glCompileShader(fragment_shader);
    //check the compile status
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &shader_status);
    if(!shader_status)
    {
        std::cerr << "INCONSISTENCY WITH VIRTUAL MACHINE, please run again" << std::endl;    
        return 1;     
    }
  
    //Now we link the 2 shader objects into a program
    //This program is what is run on the GPU
    program1 = glCreateProgram();
    glAttachShader(program1, vertex_shader);
    glAttachShader(program1, fragment_shader);
   
    glLinkProgram(program1);
    //check if everything linked ok
    glGetProgramiv(program, GL_LINK_STATUS, &shader_status);
    //this defines a cube, this is why a model loadfer is nice
    
    loc_position1 = glGetAttribLocation(program1,
                    const_cast<const char*>("v_position"));

    loc_tex1 = glGetAttribLocation(program1,
                    const_cast<const char*>("v_texture"));

    loc_mvpmat1 = glGetUniformLocation(program1,
                    const_cast<const char*>("mvpMatrix"));
    loc_mvmat1 = glGetUniformLocation(program1,
                    const_cast<const char*>("mvMatrix"));

    loc_texc1 = glGetUniformLocation(program1,
                    const_cast<const char*>("gSampler"));

    loc_norm1 = glGetUniformLocation(program,
                    const_cast<const char*>("v_normal"));
                                 
    //--Init the view and projection matrices
    //  if you will be having a moving camera the view matrix will need to more dynamic
    //  ...Like you should update it before you render more dynamic 
    //  for this project having them static will be fine
       view = glm::lookAt( glm::vec3(0.0, viewer.y, viewer.z), //Eye Position
                        glm::vec3(0.0, 0.0, 0.0), //Focus point
                        glm::vec3(0.0, 1.0, 0.0)); //Positive Y is up

    projection = glm::perspective( 45.0f, //the FoV typically 90 degrees is good which is what this is set to
                                   float(w)/float(h), //Aspect Ratio, so Circles stay Circular
                                   0.01f, //Distance to the near plane, normally a small value like this
                                   100.0f); //Distance to the far plane, 
                                                                
    //enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    initPhysics();    
    //and its done
    return true;
}

void cleanUp()
{
  // Clean up, Clean up       
	delete rigidBodyTable;
	delete rigidBodyBall;
	delete rigidBodyGround;
	delete rigidBodyPlane;
	delete rigidBodyCeiling;
  delete rigidBodyLeftWall;
	delete rigidBodyRightWall;
	delete rigidBodyBack;
	delete rigidBodyHolder;
	for( int i = 0; i < 100; i++ )
	{
	  delete rigidBodyPegs[i];
  }
    //TwTerminate();
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

void myMouseMovement( int x, int y) 
{
  // initialize variables
  GLint viewport[4];
  GLdouble modelview[16];
  GLdouble projection[16];
  GLfloat winX, winY, winZ;
  GLdouble posX, posY, posZ;
  
  
  if( !TwEventMouseMotionGLUT(x, y) )  // send event to AntTweakBar
    {   	
    }
  
 
  // get model, view, projection
  glGetDoublev( GL_MODELVIEW_MATRIX, modelview );
  glGetDoublev( GL_PROJECTION_MATRIX, projection );
  glGetIntegerv( GL_VIEWPORT, viewport );
 
  winX = (float)x;
  winY = (float)viewport[3] - (float)y;
  glReadPixels( x, int(winY), 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ );
 
  gluUnProject( winX, winY, winZ, modelview, projection, viewport, &posX, &posY, &posZ);

  // move ball around if selected
  if(ballSelect)
  {
   	ballPointer->setLinearVelocity( btVector3(-20 * posX , 20 *  posY, 0.0));
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
        btDefaultMotionState *groundMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(0, -30, 0)));
        btRigidBody::btRigidBodyConstructionInfo rigidBodyGroundCI(0, groundMotionState, ground, btVector3(0,0,0));
        rigidBodyGround = new btRigidBody(rigidBodyGroundCI);
        dynamicsWorld->addRigidBody(rigidBodyGround);

        //Create Table
        btCollisionShape* table = new btBvhTriangleMeshShape( level[0].trimesh, true );
        btDefaultMotionState *tableMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(0, 0, 0)));
        btRigidBody::btRigidBodyConstructionInfo tableInfo(100, tableMotionState, table, btVector3(0,0,0));
        tableInfo.m_restitution = 0.2;
        tableInfo.m_friction = 0.5; 
        rigidBodyTable = new btRigidBody(tableInfo);
        dynamicsWorld->addRigidBody(rigidBodyTable);        
        rigidBodyTable->setLinearFactor(btVector3(0,0,0));
        rigidBodyTable->setAngularFactor(btVector3(0,0,0));
        rigidBodyTable->setActivationState(DISABLE_DEACTIVATION);   

        rigidBodyTable->setCollisionFlags(rigidBodyTable->getCollisionFlags() |
                         btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);
                         
        //Create Plane to set against front of board
        btCollisionShape* plane = new btStaticPlaneShape (btVector3 (0,0,1), 1);
        btDefaultMotionState *planeMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(0, 0, -5)));
        btRigidBody::btRigidBodyConstructionInfo rigidBodyPlaneCI(0, planeMotionState, plane, btVector3(0,0,0));
        rigidBodyPlane = new btRigidBody(rigidBodyPlaneCI);
        dynamicsWorld->addRigidBody(rigidBodyPlane);
        
        //Create ceiling to contain ball within screen
        btCollisionShape* ceiling = new btStaticPlaneShape (btVector3 (0,-1,0), 1);
        btDefaultMotionState *ceilingMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(0, 35, 0)));
        btRigidBody::btRigidBodyConstructionInfo rigidBodyCeilingCI(0, ceilingMotionState, ceiling, btVector3(0,0,0));
        rigidBodyCeiling = new btRigidBody(rigidBodyCeilingCI);
        dynamicsWorld->addRigidBody(rigidBodyCeiling);
        
        //Create left wall to contain ball within screen
        btCollisionShape* leftWall = new btStaticPlaneShape (btVector3 (-1,0,0), 1);
        btDefaultMotionState *leftWallMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(60, 0, 0)));
        btRigidBody::btRigidBodyConstructionInfo rigidBodyLeftWallCI(0, leftWallMotionState, leftWall, btVector3(0,0,0));
        rigidBodyLeftWall = new btRigidBody(rigidBodyLeftWallCI);
        dynamicsWorld->addRigidBody(rigidBodyLeftWall);
        
        //Create right wall to contain ball within screen
        btCollisionShape* rightWall = new btStaticPlaneShape (btVector3 (1,0,0), 1);
        btDefaultMotionState *rightWallMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(-21, 0, 0)));
        btRigidBody::btRigidBodyConstructionInfo rigidBodyRightWallCI(0, rightWallMotionState, rightWall, btVector3(0,0,0));
        rigidBodyRightWall = new btRigidBody(rigidBodyRightWallCI);
        dynamicsWorld->addRigidBody(rigidBodyRightWall);
        
        //Create back wall to contain ball within screen
        btCollisionShape* back = new btStaticPlaneShape (btVector3 (0,0,-1), 1);
        btDefaultMotionState *backMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(0, 0, -1)));
        btRigidBody::btRigidBodyConstructionInfo rigidBodyBackCI(0, backMotionState, back, btVector3(0,0,0));
        rigidBodyBack = new btRigidBody(rigidBodyBackCI);
        dynamicsWorld->addRigidBody(rigidBodyBack);

        //Create Ball
        btCollisionShape* ball = new btSphereShape( 1.0 );
        mass = 10.0f;
        ball->calculateLocalInertia(mass,inertia);
        btDefaultMotionState *ballMotionState =
        new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(35, 5, 0)));
        btRigidBody::btRigidBodyConstructionInfo ballInfo(100, ballMotionState, ball, inertia); 
        ballInfo.m_restitution = 0.5; 
        ballInfo.m_friction = 1.7;
        rigidBodyBall = new btRigidBody(ballInfo);
        dynamicsWorld->addRigidBody(rigidBodyBall);
        rigidBodyBall->setLinearFactor(btVector3(1,1,1));
        rigidBodyBall->setAngularFactor(btVector3(1,1,1));;
        rigidBodyBall->setActivationState(DISABLE_DEACTIVATION);
        rigidBodyBall->setCollisionFlags(rigidBodyBall->getCollisionFlags() |
                         btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);
                         
                         
        gContactProcessedCallback = (ContactProcessedCallback)HandleContacts;
        
        
        //Create ball holder
        btCollisionShape* ballHolder = new btBvhTriangleMeshShape( holder.trimesh, true );
        btDefaultMotionState *holderMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(35, 0, 0)));
        btRigidBody::btRigidBodyConstructionInfo holderInfo(0, holderMotionState, ballHolder, btVector3(0,0,0));
        holderInfo.m_restitution = 0.0;
        holderInfo.m_friction = 0.5; 
        rigidBodyHolder = new btRigidBody(holderInfo);
        dynamicsWorld->addRigidBody(rigidBodyHolder);        
        rigidBodyTable->setActivationState(DISABLE_DEACTIVATION);   
        
        btCollisionShape* boardPegs[100];
        btDefaultMotionState *pegMotionStates[100];
         
        //Create Pegs
        srand (time(NULL));
        
          //Prime the loop
          int sign = rand() % 2;
          
          // determine if peg goes on left or right side of board
          if( sign == 0 )
          {
            sign = 1;
          }
          else
          {
            sign = -1;  
          } 
          
          // randomly place pegs on board 
          int ran = ( ( ( rand() %  15 / (3 + ( 0 % 2 ) ) ) * ( 3 + ( 0 % 2 ) ) )  ) * sign;

          boardPegs[0] = new btBvhTriangleMeshShape( pegs[0].trimesh, true );
          pegMotionStates[0] = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(ran, 22, -2)));
          btRigidBody::btRigidBodyConstructionInfo pegInfo(100, pegMotionStates[0], boardPegs[0], btVector3(0,0,0)); 
          pegInfo.m_restitution = 1.5; 
          pegInfo.m_friction = 0.2;
          rigidBodyPegs[0] = new btRigidBody(pegInfo);
          dynamicsWorld->addRigidBody(rigidBodyPegs[0]);
          rigidBodyPegs[0]->setLinearFactor(btVector3(0,0,0));
          rigidBodyPegs[0]->setAngularFactor(btVector3(0,0,0));
          rigidBodyPegs[0]->setActivationState(DISABLE_DEACTIVATION);
          rigidBodyPegs[0]->setCollisionFlags(rigidBodyPegs[0]->getCollisionFlags() |
              btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);
        
        // now place rest of pegs on board
        for( int n = 0; n < 12; n++ )
        {
        for( int i = 1; i < 9; i++ )
        {
          // determine if peg goes on left or right side of board
          int sign = rand() % 2;
          if( sign == 0 )
          {
            sign = 1;
          }
          else
          {
            sign = -1;  
          }  
          
          // randomly place pegs on board 
          int ran = ( ( ( rand() %  15 / (3 + ( n % 2 ) ) ) * ( 3 + ( n % 2 ) ) )  ) * sign;
          
          // go down board based of loop index
          int row = 22 - (n*3);
        
          boardPegs[i+(n*8)] = new btBvhTriangleMeshShape( pegs[i+(n*8)].trimesh, true );
          pegMotionStates[i+(n*8)] = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(ran, row, -2)));
          btRigidBody::btRigidBodyConstructionInfo pegInfo(100, pegMotionStates[i+(n*8)], boardPegs[i+(n*8)], btVector3(0,0,0)); 
          pegInfo.m_restitution = 1.5; 
          pegInfo.m_friction = 0.2;
          rigidBodyPegs[i+(n*8)] = new btRigidBody(pegInfo);
          dynamicsWorld->addRigidBody(rigidBodyPegs[i+(n*8)]);
          rigidBodyPegs[i+(n*8)]->setLinearFactor(btVector3(0,0,0));
          rigidBodyPegs[i+(n*8)]->setAngularFactor(btVector3(0,0,0));
          rigidBodyPegs[i+(n*8)]->setActivationState(DISABLE_DEACTIVATION);
          rigidBodyPegs[i+(n*8)]->setCollisionFlags(rigidBodyPegs[i+(n*8)]->getCollisionFlags() |
              btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);
        }
      }
        
        // place four more pegs
        for( int i = 97; i < 100; i++ )
        {
          int sign = rand() % 2;
          if( sign == 0 )
          {
            sign = 1;
          }
          else
          {
            sign = -1;  
          }  
          
          int ran = ( ( ( rand() %  15 / (3 + ( 0 % 2 ) ) ) * ( 3 + ( 0 % 2 ) ) )  ) * sign;
        
          boardPegs[i] = new btBvhTriangleMeshShape( pegs[i].trimesh, true );
          pegMotionStates[i] = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(ran, -14, -2)));
          btRigidBody::btRigidBodyConstructionInfo pegInfo(100, pegMotionStates[i], boardPegs[i], btVector3(0,0,0)); 
          pegInfo.m_restitution = 1.5; 
          pegInfo.m_friction = 0.2;
          rigidBodyPegs[i] = new btRigidBody(pegInfo);
          dynamicsWorld->addRigidBody(rigidBodyPegs[i]);
          rigidBodyPegs[i]->setLinearFactor(btVector3(0,0,0));
          rigidBodyPegs[i]->setAngularFactor(btVector3(0,0,0));
          rigidBodyPegs[i]->setActivationState(DISABLE_DEACTIVATION);
          rigidBodyPegs[i]->setCollisionFlags(rigidBodyPegs[i]->getCollisionFlags() |
              btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);
        } 
}


void antInit()
{
    // Create a tweak bar
    bar = TwNewBar("TweakBar");
    TwDefine(" GLOBAL help='Planko Help Menu: ' "); // Message added to the help bar.
    TwDefine(" TweakBar size='200 400' color='96 216 224' "); // change default tweak bar size and color

    // Add 'g_LightMultiplier' to 'bar': this is a variable of type TW_TYPE_FLOAT. Its key shortcuts are [+] and [-].
    TwAddVarRW(bar, "Multiplier", TW_TYPE_FLOAT, &g_LightMultiplier, 
               " label='Light booster' min=0.1 max=4 step=0.02 keyIncr='+' keyDecr='-' help='Increase/decrease the light power.' ");

    // Add 'g_LightDirection' to 'bar': this is a variable of type TW_TYPE_DIR3F which defines the light direction
    TwAddVarRW(bar, "LightDir", TW_TYPE_DIR3F, &g_LightDirection, 
               " label='Light direction' opened=true help='Change the light direction.' ");

    // Add 'g_MatAmbient' to 'bar': this is a variable of type TW_TYPE_COLOR3F (3 floats color, alpha is ignored)
    // and is inserted into a group named 'Material'.
    TwAddVarRW(bar, "Ambient", TW_TYPE_COLOR3F, &g_MatAmbient, " group='Material' ");

    // Add 'g_MatDiffuse' to 'bar': this is a variable of type TW_TYPE_COLOR3F (3 floats color, alpha is ignored)
    // and is inserted into group 'Material'.
     TwAddVarRW(bar, "Diffuse", TW_TYPE_COLOR3F, &g_MatDiffuse, " group='Material' ");
     
    string x = "Top 5";  
   
	TwAddVarRO(bar, "1. ", TW_TYPE_UINT32, &highScores[0], " group='Scores' ");	
	TwAddVarRO(bar, "2. ", TW_TYPE_UINT32, &highScores[1], " group='Scores' ");
	TwAddVarRO(bar, "3. ", TW_TYPE_UINT32, &highScores[2], " group='Scores' ");
	TwAddVarRO(bar, "4. ", TW_TYPE_UINT32, &highScores[3], " group='Scores' ");	
	TwAddVarRO(bar, "5. ", TW_TYPE_UINT32, &highScores[4], " group='Scores' ");

	TwAddButton(bar, "Reset", reset, NULL, " label='Reset' ");
  TwAddButton(bar, "Cheat", cheat, NULL, " label='Cheat' "); 
  TwAddButton(bar, "Pause", pause, NULL, " label='Pause' ");
	TwAddButton(bar, "Exit", exit, NULL, " label='Exit' ");
}

void myMouse( int button, int state, int x, int y)
{
    // move ball around
    if(button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN)
    {
    	   glutSetCursor(GLUT_CURSOR_NONE);
           if(!B1)
           {      
           		ballSelect = !ballSelect;
           		ballPointer = rigidBodyBall;            
           }
           
           else if(!B2)
           {
           		ballSelect = !ballSelect;
           		ballPointer = rigidBodyBall2;                      
           }
           
           else if(!B3)
           {           
           		ballSelect = !ballSelect;
           		ballPointer = rigidBodyBall3;   
           }
           
           else
           {
           		ballSelect = false; 
                glutSetCursor(1);  
           }
    } 
    
    
    if( !TwEventMouseButtonGLUT(button, state,  x, y))  // send event to AntTweakBar
    { 
    }
    
}

static bool HandleContacts(btManifoldPoint& point, btCollisionObject* body0, btCollisionObject* body1)
{
   // initialize variables for handling contact of objects
   btRigidBody* rigidbody0 = dynamic_cast<btRigidBody*>(body0);
   btRigidBody* rigidbody1 = dynamic_cast<btRigidBody*>(body1);
   static struct timeval start, end;
   static bool veryFirst = true;
   static bool first = true;
   
   float timeThreshold = 1000;
   
   // handle object contacting for sound effects
   float passedTime = 0;
   if ( first)
   {
      first = false;
      if (veryFirst)
      {
         veryFirst = false;
         passedTime = timeThreshold;
      }
      gettimeofday(&start, NULL);
   }
   else
   {
      long mtime, seconds, useconds;  
      gettimeofday(&end, NULL);
      seconds  = end.tv_sec  - start.tv_sec;
      useconds = end.tv_usec - start.tv_usec;
      
      mtime = ((seconds) * 1000 + useconds/1000.0) + 0.5;
      passedTime = mtime;
      
      if (passedTime >= timeThreshold)
      {
        	 first = true;
        	 
       }
   }
   // ignore calls until enough time has passed. 
   if (passedTime >= timeThreshold)
   {
      if(rigidbody0 && rigidbody1)
      {
		   		return true;   
      }   
   }
   return false;
}


