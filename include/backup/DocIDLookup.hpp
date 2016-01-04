#pragma once
#include <string>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/mman.h>
//#include <vector>

using namespace std;

#define DocIDLookupBufferSize 1024
#define MaxDocNameLength 256

#ifndef MaxFileSize
#define MaxFileSize (1024*1024*512)
#endif

class DocIDLookupBuilder
{
public:
	DocIDLookupBuilder(char *path);
	void addDoc(char *docName);
	void addDoc(string docName);
	void finish();
	~DocIDLookupBuilder();

private:
	unsigned docID;
	unsigned bufferUsage;
	unsigned pos;
	unsigned posArray[DocIDLookupBufferSize];
	unsigned lengthArray[DocIDLookupBufferSize];
	char stringArray[DocIDLookupBufferSize][MaxDocNameLength];

	char outputPath[256];
	int fileNum;
	unsigned fileUsage;
	FILE *fileHandle;
	FILE *fileSummaryHandle;

	void openNewFile();
	void writeBuffer();
};

DocIDLookupBuilder::DocIDLookupBuilder(char *path)
{
	strcpy(outputPath,path);
	docID = 0;
	fileNum=-1;
	fileUsage = MaxFileSize;
	fileHandle = NULL;

	char filename[512];
	sprintf(filename,"%s-tables",outputPath);
	mkdir(filename,S_IRWXU);
	sprintf(filename,"%s-summary",outputPath);
	fileSummaryHandle = fopen(filename,"wb");
	if(!fileSummaryHandle) {cerr<<"Could not write to the file "<<filename<<endl;return;}
	unsigned fake[2];
	fake[0]=0;fake[1]=0;
	fwrite(fake,4,2,fileSummaryHandle);
	openNewFile();
}

void DocIDLookupBuilder::addDoc(char *docName)
{
	if(strlen(docName)>255) docName[255]='\0';
	posArray[bufferUsage] = pos;
	unsigned length = strlen(docName);
	lengthArray[bufferUsage] = length;
	memmove(stringArray[bufferUsage],docName,length);
	pos+=length;
	bufferUsage++;
	docID++;
	if(bufferUsage>=DocIDLookupBufferSize) writeBuffer();// buffer full
}

void DocIDLookupBuilder::addDoc(string s)
{
	if(s.length()>255) s=s.substr(0,255);
	char docName[256];
	strcpy(docName,s.c_str());
	addDoc(docName);
}

void DocIDLookupBuilder::finish()
{
	writeBuffer();
	// add fix summary code here
	fseek(fileSummaryHandle,0,SEEK_SET);
	fwrite(&docID,sizeof(unsigned),1,fileSummaryHandle);
	fwrite(&fileNum,sizeof(unsigned),1,fileSummaryHandle);
}

void DocIDLookupBuilder::openNewFile()
{
	if(fileHandle!=NULL) fclose(fileHandle);
	fileNum++;
	char filename[512];
	sprintf(filename,"%s-tables/%d",outputPath,fileNum);
	fileHandle = fopen(filename,"wb");
	if(!fileHandle) {cerr<<"Could not write to file "<<filename<<endl;return;}
	fileUsage = 0;
	pos = 0;
}

void DocIDLookupBuilder::writeBuffer()
{
	unsigned i;
	unsigned summary[2*bufferUsage];
	for(i=0;i<bufferUsage;i++) summary[i*2] = lengthArray[i] | (fileNum<<8);
	for(i=0;i<bufferUsage;i++) summary[i*2+1] = posArray[i];
	fwrite(summary,sizeof(unsigned),bufferUsage*2,fileSummaryHandle);
	for(i=0;i<bufferUsage;i++)
	{
		fwrite(stringArray[i],sizeof(char),lengthArray[i],fileHandle);
	}
	bufferUsage = 0;
	fileUsage = pos;
	if(fileUsage > MaxFileSize) openNewFile();
}

DocIDLookupBuilder::~DocIDLookupBuilder()
{
	fclose(fileHandle);
	fclose(fileSummaryHandle);
}


////////////////////////////////////////////////////////////////////////////////////////////////


class DocIDLookupReader
{
public:
	DocIDLookupReader(char* path);
	char *findDoc(unsigned n);
	~DocIDLookupReader();
private:
	unsigned pageSize;
	unsigned docNum;
	char docName[256];
	char thePath[256];
	unsigned bufferStart,bufferEnd,bufferSize;
	unsigned curFileNum;
	int fileHandle;
	char *buffer;
	unsigned *bufferSummary;
};

DocIDLookupReader::DocIDLookupReader(char* path)
{
	strcpy(thePath,path);
	buffer = NULL;
	bufferSummary = NULL;
	// initial the buffer summary
	char filename[512];
	sprintf(filename,"%s-summary",path);
	FILE *fileHandleSummary = fopen(filename,"rb");
	if(!fileHandleSummary) {cerr<<"Could not read the file "<<filename<<endl;return;}
	unsigned dummy;
	fread(&docNum,sizeof(unsigned),1,fileHandleSummary);
	fread(&dummy,sizeof(unsigned),1,fileHandleSummary);
	bufferSummary = new unsigned[docNum*2+2];
	fread(bufferSummary+2,sizeof(unsigned),docNum*2,fileHandleSummary);
	fclose(fileHandleSummary);
	// intitial others
	bufferStart = 0;bufferEnd = 0;bufferSize = 0;
	curFileNum = 0xFFFFFFFF;
	fileHandle = -1;
	pageSize = sysconf(_SC_PAGESIZE);
}

char* DocIDLookupReader::findDoc(unsigned docID)
{
	if(docID==0 || docID>docNum)
	{
		docName[0]='\0';
		return docName;
	}
	// read the summary
	unsigned record = bufferSummary[docID*2];
	unsigned pos = bufferSummary[docID*2+1];
	unsigned length = record & 255;
	unsigned fileNum = record >> 8;
	// locate the record
	if(curFileNum!=fileNum) //different files
	{
		if(buffer!=NULL) munmap(buffer,bufferSize);
		close(fileHandle);
		char filename[512];
		sprintf(filename,"%s-tables/%d",thePath,fileNum);
		fileHandle = open(filename,O_RDONLY);
		if(fileHandle==-1) {cerr<<"could not read the file "<<filename<<endl;return NULL;}
		bufferStart = pos/pageSize*pageSize;
		bufferEnd = bufferStart + pageSize;
		if(pos+length > bufferEnd) bufferEnd += pageSize;
		bufferSize = bufferEnd - bufferStart;
		buffer = (char*)mmap(0,bufferSize,PROT_READ,MAP_SHARED,fileHandle,bufferStart);
	}
	else if(pos<bufferStart || pos+length > bufferEnd) //out of range
	{
		if(buffer!=NULL) munmap(buffer,bufferSize);
		bufferStart = pos/pageSize*pageSize;
		bufferEnd = bufferStart + pageSize;
		if(pos+length > bufferEnd) bufferEnd += pageSize;
		bufferSize = bufferEnd - bufferStart;
		buffer = (char*)mmap(0,bufferSize,PROT_READ,MAP_SHARED,fileHandle,bufferStart);
	}
	memmove(docName,buffer+(pos-bufferStart),length);
	docName[length]='\0';
	return docName;
}

DocIDLookupReader::~DocIDLookupReader()
{
	if(buffer!=NULL) munmap(buffer,bufferSize);
	if(fileHandle!=-1) close(fileHandle);
	delete[] bufferSummary;
}
