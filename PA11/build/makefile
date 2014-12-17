# Linux
CC=g++
LIBS= -lglut -lGLEW -lGL -lassimp -lGLU -lSOIL -I/usr/include/bullet -lBulletDynamics -lBulletSoftBody -lBulletCollision -lLinearMath

# For Macs uncomment the next line and comment out the previous one
#CC=clang++
#LIBS=  -framework OpenGL -framework GLUT -framework Cocoa -lGLEW -stdlib=libc++
# Assuming you want to use a recent compiler



# Compiler flags
CXXFLAGS= -g -Wall -std=c++0x


all: ../bin/Matrix

../bin/Matrix: ../src/main.cpp ;
	$(CC) $(CXXFLAGS) ../src/main.cpp -o ../bin/Matrix $(LIBS)
	

clean:

	
	
    


