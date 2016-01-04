#pragma once
#include <stdio.h>
#include <string.h>
#include <iostream>

using namespace std;

class DocInfoBuilder
{
public:
	DocInfoBuilder(char *filename);
	void insertDoc(unsigned l);
	void finish();
	~DocInfoBuilder();
private:
	unsigned docNum;
	FILE *fileHandle;
	bool finishMark;
};

DocInfoBuilder::DocInfoBuilder(char *filename)
{
	fileHandle = fopen(filename,"wb");
	if(!fileHandle) { cerr<<"Could not wrtie the file "<<filename<<endl;return;}
	docNum=0;
	fwrite(&docNum,sizeof(unsigned),1,fileHandle);
	finishMark=false;
}

void DocInfoBuilder::insertDoc(unsigned l)
{
	fwrite(&l,sizeof(unsigned),1,fileHandle);
	docNum++;
}

void DocInfoBuilder::finish()
{
	fseek(fileHandle,0,SEEK_SET);
	fwrite(&docNum,sizeof(unsigned),1,fileHandle);
	finishMark=true;
}

DocInfoBuilder::~DocInfoBuilder()
{
	fclose(fileHandle);
	if(!finishMark) {cerr<<"Did not call finish function, the record may be unfinished!"<<endl;}
}

///////////////////////////////////////////////////////////////////////////////

class DocInfoReader
{
public:
	DocInfoReader(char* filename);
	inline unsigned getDocLength(unsigned n);
	~DocInfoReader();
private:
	unsigned docNum;
	unsigned* lengths;
};

DocInfoReader::DocInfoReader(char* filename)
{
	FILE *fileHandle = fopen(filename,"rb");
	if(!fileHandle) {cerr<<"Could not read the file "<<filename<<endl;return;}
	fread(&docNum,sizeof(unsigned),1,fileHandle);
	lengths = new unsigned[docNum+1];
	fread(lengths+1,sizeof(unsigned),docNum,fileHandle);
	fclose(fileHandle);
}

inline unsigned DocInfoReader::getDocLength(unsigned n)
{
	if(n<0 || n>docNum) return 0;
	return lengths[n];
}

DocInfoReader::~DocInfoReader()
{
	delete[] lengths;
}
