#include <GL/glew.h> // glew must be included before the main gl libs
#include <GL/glut.h> // doing otherwise causes compiler shouting
#include <iostream>
#include <chrono>
#include <fstream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> //Makes passing matrices to shaders easier


#include "../src/shaderLoader.h"


//Global Flag for cube spinning.
bool spin = true; 
int orbit= -1;
int orbit_axis = -1;  



//--Data types
//This object will define the attributes of a vertex(position, color, etc...)



struct Vertex
{
    GLfloat position[3];
    GLfloat color[3];
};

//--Evil Global variables
//Just for this example!
int w = 640, h = 480;// Window size
GLuint program;// The GLSL program handle
GLuint vbo_geometry;// VBO handle for our geometry
GLuint vbo_geometry2; 

//uniform locations
GLint loc_mvpmat;// Location of the modelviewprojection matrix in the shader

//attribute locations
GLint loc_position;
GLint loc_color;


//transform matrices
glm::mat4 model;//obj->world each object should have its own model matrix
glm::mat4 modelMoon; 
glm::mat4 view;//world->eye
glm::mat4 projection;//eye->clip
glm::mat4 mvp;//premultiplied modelviewprojection
glm::mat4 mvpTest;
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
		    spin = true;
		 break;
		case 2 :
		   spin = false;          
		break;
		case 3 :
	        exit(0); 
		 break;
	}
}

void createGLUTMenus() {

   

	// create the menu and 
	// tell glut that "processMenuEvents" will
	// handle the events
	glutCreateMenu(processMenuEvents);

	//add entries to our menu
	glutAddMenuEntry("Start",1);
	glutAddMenuEntry("Stop",2);
	glutAddMenuEntry("Exit",3);


	// attach the menu to the right button
	glutAttachMenu(GLUT_RIGHT_BUTTON);
}

void myMouse(int button, int state, int x, int y) {

    if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) orbit *= -1; 
        
}

void SpecialInput(int key, int x, int y)
{
switch(key)
{
case GLUT_KEY_UP:
orbit_axis *= -1; 
break;	
case GLUT_KEY_DOWN:
orbit_axis *= -1; 
break;
case GLUT_KEY_LEFT:
orbit *= -1; 
break;
case GLUT_KEY_RIGHT:
orbit *= -1; 
break;
}

glutPostRedisplay();
}


//--Main
int main(int argc, char **argv)
{
    // Initialize glut
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(w, h);

    // Name and create the Window
    glutCreateWindow("PA03");
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
    glutSpecialFunc(SpecialInput);



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
void render()
{
    //--Render the scene

    //clear the screen
    glClearColor(0.0, 0.0, 0.2, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //premultiply the matrix for this example
    mvp = projection * view * model;
    mvpTest = projection * view *  modelMoon;

    //enable the shader program
    glUseProgram(program);

    //upload the matrix to the shader
    glUniformMatrix4fv(loc_mvpmat, 1, GL_FALSE, glm::value_ptr(mvp));
    //set up the Vertex Buffer Object so it can be drawn
    glEnableVertexAttribArray(loc_position);
    glEnableVertexAttribArray(loc_color);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_geometry);
    //set pointers into the vbo for each of the attributes(position and color)
    glVertexAttribPointer( loc_position,//location of attribute
                           3,//number of elements
                           GL_FLOAT,//type
                           GL_FALSE,//normalized?
                           sizeof(Vertex),//stride
                           0);//offset

    glVertexAttribPointer( loc_color,
                           3,
                           GL_FLOAT,
                           GL_FALSE,
                           sizeof(Vertex),
                           (void*)offsetof(Vertex,color));

    glDrawArrays(GL_TRIANGLES, 0, 36);//mode, starting index, count
   
   	glUniformMatrix4fv(loc_mvpmat, 1, GL_FALSE, glm::value_ptr(mvpTest));
    //set up the Vertex Buffer Object so it can be drawn
    glEnableVertexAttribArray(loc_position  );
    glEnableVertexAttribArray(loc_color);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_geometry2);
    //set pointers into the vbo for each of the attributes(position and color)
    glVertexAttribPointer( loc_position,//location of attribute
                           3,//number of elements
                           GL_FLOAT,//type
                           GL_FALSE,//normalized?
                           sizeof(Vertex),//stride
                           0);//offset

    glVertexAttribPointer( loc_color,
                           3,
                           GL_FLOAT,
                           GL_FALSE,
                           sizeof(Vertex),
                           (void*)offsetof(Vertex,color));

    glDrawArrays(GL_TRIANGLES,0 , 36);//mode, starting index, count
 
    //clean up
    glDisableVertexAttribArray(loc_position);
    glDisableVertexAttribArray(loc_color);
                           
    //swap the buffers
    glutSwapBuffers();
}

void update()
{
    //total time
    static float angle = 0.0, axisAngle = 0.0;
    float dt = getDT();// if you have anything moving, use dt.
    if(orbit ==  -1){
    	angle += (dt * M_PI/2);
  }
   else{
   		angle -= (dt * M_PI/2);
   } 
   
   if(orbit_axis ==  -1){
    	axisAngle -= (dt * M_PI/2);
  }
   else{
   		axisAngle += (dt * M_PI/2);
   }   
    
        
 	if(spin){       
        
        model = glm::translate( glm::mat4(1.0f), glm::vec3(6.0 * sin(angle), 0.0, 6.0 * cos(angle)));
        model = glm::rotate( model, axisAngle*40 , glm::vec3(0.0, 1.0, 0.0));
               
        modelMoon = glm::translate( model, glm::vec3(8.0 * sin(angle), 0.0, 8.0 * cos(angle)));
        modelMoon = glm::rotate( modelMoon, axisAngle*40 , glm::vec3(0.0,  1.0, 0.0));
        
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
    // Handle keyboard input
    if(key == 27)//ESC
    {
        exit(0);
    }
    
    if(key == ('a' | 'A')){
    
        orbit_axis *= -1; 
    
    }
}

bool initialize()
{
    // Initialize basic geometry and shaders for this example

    //this defines a cube, this is why a model loadfer is nice
    //you can also do this with a draw elements and indices, try to get that working
          Vertex geometry[] = { {{-4.0, -4.0, -4.0}, {0.0, 0.0, 0.0}},
                          {{-4.0, -4.0, 4.0}, {0.0, 0.0, 4.0}},
                          {{-4.0, 4.0, 4.0}, {0.0, 4.0, 4.0}},

                          {{4.0, 4.0, -4.0}, {4.0, 4.0, 0.0}},
                          {{-4.0, -4.0, -4.0}, {0.0, 0.0, 0.0}},
                          {{-4.0, 4.0, -4.0}, {0.0, 4.0, 0.0}},
                          
                          {{4.0, -4.0, 4.0}, {4.0, 0.0, 4.0}},
                          {{-4.0, -4.0, -4.0}, {0.0, 0.0, 0.0}},
                          {{4.0, -4.0, -4.0}, {4.0, 0.0, 0.0}},
                          
                          {{4.0, 4.0, -4.0}, {4.0, 4.0, 0.0}},
                          {{4.0, -4.0, -4.0}, {4.0, 0.0, 0.0}},
                          {{-4.0, -4.0, -4.0}, {0.0, 0.0, 0.0}},

                          {{-4.0, -4.0, -4.0}, {0.0, 0.0, 0.0}},
                          {{-4.0, 4.0, 4.0}, {0.0, 4.0, 4.0}},
                          {{-4.0, 4.0, -4.0}, {0.0, 4.0, 0.0}},

                          {{4.0, -4.0, 4.0}, {4.0, 0.0, 4.0}},
                          {{-4.0, -4.0, 4.0}, {0.0, 0.0, 4.0}},
                          {{-4.0, -4.0, -4.0}, {0.0, 0.0, 0.0}},

                          {{-4.0, 4.0, 4.0}, {0.0, 4.0, 4.0}},
                          {{-4.0, -4.0, 4.0}, {0.0, 0.0, 4.0}},
                          {{4.0, -4.0, 4.0}, {4.0, 0.0, 4.0}},
                          
                          {{4.0, 4.0, 4.0}, {4.0, 4.0, 4.0}},
                          {{4.0, -4.0, -4.0}, {4.0, 0.0, 0.0}},
                          {{4.0, 4.0, -4.0}, {4.0, 4.0, 0.0}},

                          {{4.0, -4.0, -4.0}, {4.0, 0.0, 0.0}},
                          {{4.0, 4.0, 4.0}, {4.0, 4.0, 4.0}},
                          {{4.0, -4.0, 4.0}, {4.0, 0.0, 4.0}},

                          {{4.0, 4.0, 4.0}, {4.0, 4.0, 4.0}},
                          {{4.0, 4.0, -4.0}, {4.0, 4.0, 0.0}},
                          {{-4.0, 4.0, -4.0}, {0.0, 4.0, 0.0}},

                          {{4.0, 4.0, 4.0}, {4.0, 4.0, 4.0}},
                          {{-4.0, 4.0, -4.0}, {0.0, 4.0, 0.0}},
                          {{-4.0, 4.0, 4.0}, {0.0, 4.0, 4.0}},

                          {{4.0, 4.0, 4.0}, {4.0, 4.0, 4.0}},
                          {{-4.0, 4.0, 4.0}, {0.0, 4.0, 4.0}},
                          {{4.0, -4.0, 4.0}, {4.0, 0.0, 4.0}}
                        };
                        
                        
                        
      Vertex geometryTest[] = { {{-1.0, -1.0, -1.0}, {0.0, 0.0, 0.0}},
                          {{-1.0, -1.0, 1.0}, {0.0, 0.0, 1.0}},
                          {{-1.0, 1.0, 1.0}, {0.0, 1.0, 1.0}},

                          {{1.0, 1.0, -1.0}, {1.0, 1.0, 0.0}},
                          {{-1.0, -1.0, -1.0}, {0.0, 0.0, 0.0}},
                          {{-1.0, 1.0, -1.0}, {0.0, 1.0, 0.0}},
                          
                          {{1.0, -1.0, 1.0}, {1.0, 0.0, 1.0}},
                          {{-1.0, -1.0, -1.0}, {0.0, 0.0, 0.0}},
                          {{1.0, -1.0, -1.0}, {1.0, 0.0, 0.0}},
                          
                          {{1.0, 1.0, -1.0}, {1.0, 1.0, 0.0}},
                          {{1.0, -1.0, -1.0}, {1.0, 0.0, 0.0}},
                          {{-1.0, -1.0, -1.0}, {0.0, 0.0, 0.0}},

                          {{-1.0, -1.0, -1.0}, {0.0, 0.0, 0.0}},
                          {{-1.0, 1.0, 1.0}, {0.0, 1.0, 1.0}},
                          {{-1.0, 1.0, -1.0}, {0.0, 1.0, 0.0}},

                          {{1.0, -1.0, 1.0}, {1.0, 0.0, 1.0}},
                          {{-1.0, -1.0, 1.0}, {0.0, 0.0, 1.0}},
                          {{-1.0, -1.0, -1.0}, {0.0, 0.0, 0.0}},

                          {{-1.0, 1.0, 1.0}, {0.0, 1.0, 1.0}},
                          {{-1.0, -1.0, 1.0}, {0.0, 0.0, 1.0}},
                          {{1.0, -1.0, 1.0}, {1.0, 0.0, 1.0}},
                          
                          {{1.0, 1.0, 1.0}, {1.0, 1.0, 1.0}},
                          {{1.0, -1.0, -1.0}, {1.0, 0.0, 0.0}},
                          {{1.0, 1.0, -1.0}, {1.0, 1.0, 0.0}},

                          {{1.0, -1.0, -1.0}, {1.0, 0.0, 0.0}},
                          {{1.0, 1.0, 1.0}, {1.0, 1.0, 1.0}},
                          {{1.0, -1.0, 1.0}, {1.0, 0.0, 1.0}},

                          {{1.0, 1.0, 1.0}, {1.0, 1.0, 1.0}},
                          {{1.0, 1.0, -1.0}, {1.0, 1.0, 0.0}},
                          {{-1.0, 1.0, -1.0}, {0.0, 1.0, 0.0}},

                          {{1.0, 1.0, 1.0}, {1.0, 1.0, 1.0}},
                          {{-1.0, 1.0, -1.0}, {0.0, 1.0, 0.0}},
                          {{-1.0, 1.0, 1.0}, {0.0, 1.0, 1.0}},

                          {{1.0, 1.0, 1.0}, {1.0, 1.0, 1.0}},
                          {{-1.0, 1.0, 1.0}, {0.0, 1.0, 1.0}},
                          {{1.0, -1.0, 1.0}, {1.0, 0.0, 1.0}}
                        };
                        
                        
    // Create a Vertex Buffer object to store this vertex info on the GPU
    glGenBuffers(1, &vbo_geometry);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_geometry);
    glBufferData(GL_ARRAY_BUFFER, sizeof(geometry), geometry, GL_STATIC_DRAW);
	
    //--Geometry done
    
    glGenBuffers(1, &vbo_geometry2);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_geometry2);
    glBufferData(GL_ARRAY_BUFFER, sizeof(geometryTest), geometryTest, GL_STATIC_DRAW);


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
    if(!shader_status)
    {
        std::cerr << "[F] THE SHADER PROGRAM FAILED TO LINK" << std::endl;
        return false;
    }

    //Now we set the locations of the attributes and uniforms
    //this allows us to access them easily while rendering
    loc_position = glGetAttribLocation(program,
                    const_cast<const char*>("v_position"));
    if(loc_position == -1)
    {
        std::cerr << "[F] POSITION NOT FOUND" << std::endl;
        return false;
    }

    loc_color = glGetAttribLocation(program,
                    const_cast<const char*>("v_color"));
    if(loc_color == -1)
    {
        std::cerr << "[F] V_COLOR NOT FOUND" << std::endl;
        return false;
    }

    loc_mvpmat = glGetUniformLocation(program,
                    const_cast<const char*>("mvpMatrix"));
    if(loc_mvpmat == -1)
    {
        std::cerr << "[F] MVPMATRIX NOT FOUND" << std::endl;
        return false;
    }
    
    //--Init the view and projection matrices
    //  if you will be having a moving camera the view matrix will need to more dynamic
    //  ...Like you should update it before you render more dynamic 
    //  for this project having them static will be fine
    view = glm::lookAt( glm::vec3(0.0, 16.0, -64.0), //Eye Position
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
    glDeleteBuffers(1, &vbo_geometry);
    glDeleteBuffers(1, &vbo_geometry2);
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




