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
using namespace std;

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
		
void model::loadFromFile(const string fileName){


	ifstream fin(fileName.c_str());
	
	fin>> distanceFromObject;
	cout<<distanceFromObject<<endl;
	fin>>parent;
	cout<<parent<<endl;
	fin>>objectRotation;
	cout<<objectRotation<<endl;
	fin>>axisRotation;
	cout<<axisRotation<<endl;
	
	fin>>fileNameOBJ;
	cout<<fileNameOBJ<<endl;
	fin>>fileNameJPG;
	cout<<fileNameJPG<<endl;
	
	
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

		texCoords.reserve(mesh->mNumVertices);
		for(unsigned int i=0; i<mesh->mNumVertices; i++){
			aiVector3D UVW = mesh->mTextureCoords[0][i]; 
			texCoords.push_back(glm::vec2(UVW.x, UVW.y));
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

    numElements = faces.size();
    glGenBuffers(1, &elementbuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, numElements * sizeof(unsigned short), &faces[0] , GL_STATIC_DRAW);
	
}

void model:: renderModel(GLuint program){

	
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
