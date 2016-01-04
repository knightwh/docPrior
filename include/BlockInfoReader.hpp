#pragma once
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <limits.h>

using namespace std;

class BlockInfo
{
public:
	float maxAll;
	float minAll;
	unsigned bn;
	unsigned* docIDs;
	float* maxScores;

	BlockInfo(unsigned* buffer,unsigned n,float mA,float minA);
	~BlockInfo();
};

BlockInfo::BlockInfo(unsigned* buffer,unsigned n,float mA,float minA)
{
	bn=n;
	maxAll=mA;
	minAll=minA;
	docIDs = new unsigned[n+1];
	maxScores = new float[n+1];
	memmove(maxScores,buffer,n*4);
	memmove(docIDs,buffer+n,n*4);
	maxScores[n] = 0;
	docIDs[n] = 0xFFFFFFFF;
}

BlockInfo::~BlockInfo()
{
	delete[] docIDs;
	delete[] maxScores;
}


class BlockInfoReader
{
public:
	BlockInfoReader(char* path);
	BlockInfo* getBlockInfo(unsigned n);
	float getMaxScore(unsigned n);
	~BlockInfoReader();
private:
	unsigned termN;
	unsigned pageSize;
	int FHsummary;
	int FH[256];
	unsigned fileSize[256];
	char thePath[512];

	void openNewFile(unsigned fileNum);
};

BlockInfoReader::BlockInfoReader(char *path)
{
	strcpy(thePath,path);
	pageSize = sysconf(_SC_PAGESIZE)/4;
	//open the summary file
	char filename[512];
	sprintf(filename,"%s-summary",path);
	FHsummary = open(filename,O_RDONLY);
	if(FHsummary==-1) { cerr<<"Could not read the file "<<filename<<endl;return;}
	read(FHsummary,&termN,sizeof(unsigned));
	//initial other things
	unsigned i;
	for(i=0;i<256;i++) FH[i]=-1;
}

void BlockInfoReader::openNewFile(unsigned fileNum)
{
	char filename[512];
	sprintf(filename,"%s-tables/%d",thePath,fileNum);
	FH[fileNum] = open(filename,O_RDONLY);
	if(FH[fileNum] == -1) {cerr<<"Could not read the file "<<filename<<endl;return;}
	struct stat s;
	fstat(FH[fileNum],&s);
	fileSize[fileNum] = s.st_size;
}

BlockInfo* BlockInfoReader::getBlockInfo(unsigned termID)
{
	if(termID==0 || termID>termN) return NULL;
	unsigned *buffer;
	unsigned bufferStart = (termID*4)/pageSize*pageSize;
	unsigned recordPos = termID*4-bufferStart;
	unsigned bufferSize = recordPos + 4;
	buffer = (unsigned*)mmap(0,bufferSize*4,PROT_READ,MAP_SHARED,FHsummary,bufferStart*4);
	unsigned fileNum = buffer[recordPos] & 255;
	unsigned pos = buffer[recordPos+1];
	unsigned bn = buffer[recordPos] >> 8;
	float maxAll = *((float*)buffer+recordPos+2);
	float minAll = *((float*)buffer+recordPos+3);
	munmap((char*)buffer,bufferSize*4);
	// open the posting list tables
	if(FH[fileNum] == -1) openNewFile(fileNum);
	bufferStart = pos/pageSize*pageSize;
	bufferSize = pos - bufferStart + bn*2;
	buffer = (unsigned*)mmap(0,bufferSize*4,PROT_READ,MAP_SHARED,FH[fileNum],bufferStart*4);
	//if(buffer == MAP_FAILED) cerr<<"map failed"<<endl;
	BlockInfo* BI = new BlockInfo(buffer+(pos-bufferStart),bn,maxAll,minAll);
	munmap((char*)buffer,bufferSize*4);
	return BI;
}

float BlockInfoReader::getMaxScore(unsigned termID)
{
	if(termID==0 || termID>termN) return -UINT_MAX;
	float *buffer;
	unsigned bufferStart = (termID*3)/pageSize*pageSize;
	unsigned recordPos = termID*3 - bufferStart;
	unsigned bufferSize = recordPos + 3;
	buffer = (float*)mmap(0,bufferSize*4,PROT_READ,MAP_SHARED,FHsummary,bufferStart*4);
	float maxAll = buffer[recordPos+2];
	munmap((char*)buffer,bufferSize*4);
	return maxAll;
}

BlockInfoReader::~BlockInfoReader()
{
	unsigned i;
	for(i=0;i<256;i++) if(FH[i]!=-1) close(FH[i]);
	close(FHsummary);
}
