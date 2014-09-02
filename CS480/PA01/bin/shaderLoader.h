#include <fstream>
#include <iostream>
#include<string>
#include<vector>
using namespace std; 

char * load(const string fileName); 


char *  load(const string fileName){
		
	ifstream fin(fileName.c_str()); 
	
	//Find File Length		
	fin.seekg(0,std::ios::end);
	streampos length = fin.tellg();
	fin.seekg(0,std::ios::beg);

	//Allocate and copy to char pointer
	char * buffer = new char [length];
	fin.read(&buffer[0], length); 
		
	return buffer;

}



