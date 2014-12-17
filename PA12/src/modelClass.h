#define GLM_FORCE_RADIANS
#include <assimp/Importer.hpp>    
#include <assimp/scene.h>           
#include <assimp/postprocess.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> //Makes passing matrices to shaders easier

#include <vector>
#include<string>
#include <fstream>
#include <iostream>

#include <GL/glew.h> // glew must be included before the main gl libs
#include <GL/glut.h> // doing otherwise causes compiler shouting

#include <SOIL/SOIL.h>

#include "../src/shaderLoader.h"
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

