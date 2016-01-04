#pragma once
#include <vector>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "PForDelta.hpp"

#ifndef MaxFileSize
#define MaxFileSize (1024*1024*512)
#endif

using namespace std;

class PostingListBuilder
{
public:
	PostingListBuilder(char *inputPath);
	void insertPosting(PForDeltaCompressor* PF);
	void finish();
	~PostingListBuilder();
private:
	char outputPath[256];
	unsigned termNum;
	int fileNum;
	FILE* fileHandle,*fileHandleSummary;
	unsigned fileUsage;
	bool finishMark;

	void openNewFile();
};

PostingListBuilder::PostingListBuilder(char *path)
{
	strcpy(outputPath,path);
	//initial summary file
	char filename[512];
	sprintf(filename,"%s-summary",outputPath);
	if(!(fileHandleSummary = fopen(filename,"wb"))) {cerr<<"Could not write to the file "<<filename<<endl;return;}
	unsigned dump=0;
	fwrite(&dump,sizeof(unsigned),1,fileHandleSummary);
	fwrite(&dump,sizeof(unsigned),1,fileHandleSummary);
	//initial others
	sprintf(filename,"%s-tables",outputPath);
	mkdir(filename,S_IRWXU);
	fileUsage = MaxFileSize;
	fileNum = -1;
	termNum = 0;
	fileHandle = NULL;
	finishMark=false;
}

void PostingListBuilder::openNewFile()
{
	if(fileHandle!=NULL) fclose(fileHandle);
	fileNum++;
	char filename[512];
	sprintf(filename,"%s-tables/%d",outputPath,fileNum);
	if(!(fileHandle = fopen(filename,"wb"))) {cerr<<"Could not write to the file "<<filename<<endl;return;}
	fileUsage = 0;
}

void PostingListBuilder::insertPosting(PForDeltaCompressor* PF)
{
	termNum++;
	unsigned i;
	unsigned theSize = PF->cDatas.size();
	unsigned* buffer = (unsigned*) malloc (sizeof(unsigned)*(theSize*(PostingPerSet+2)+3));
	unsigned n=0;
	buffer[n++] = PF->docNum;
	buffer[n++] = PF->totalTF;
	for(i=0;i<theSize;i+=2)
	{
		unsigned temp=PF->As[i]<<13;
		temp |= PF->Bs[i]<<8;
		temp |= PF->cDatas[i].size();
		//if(i+1<theSize)
		{
			temp |= PF->As[i+1]<<29;
			temp |= PF->Bs[i+1]<<24;
			temp |= PF->cDatas[i+1].size()<<16;
		}
		buffer[n++] = temp;
	}
	for(i=0;i<theSize;i++)
	{
		vector<unsigned>::iterator it;
		for(it=PF->cDatas[i].begin();it!=PF->cDatas[i].end();it++) buffer[n++] = *it;
	}
	if(fileUsage+n > MaxFileSize) 
	{
		//if(fileNum>=0) fwrite(&fileUsage,sizeof(unsigned),1,fileHandleSummary);
		openNewFile();
	}
	fwrite(buffer,sizeof(unsigned),n,fileHandle);
	unsigned summary[2];
	summary[0]=fileUsage;
	summary[1]=fileNum;
	fwrite(summary,sizeof(unsigned),2,fileHandleSummary);
	fileUsage+=n;
	free(buffer);
}

void PostingListBuilder::finish()
{
	if(fileHandle!=NULL) fclose(fileHandle);
	if(fileNum>=0) fwrite(&fileUsage,sizeof(unsigned),1,fileHandleSummary);
	fseek(fileHandleSummary,0,SEEK_SET);
	fwrite(&termNum,sizeof(unsigned),1,fileHandleSummary);
	fclose(fileHandleSummary);
	finishMark = true;
}

PostingListBuilder::~PostingListBuilder()
{
	if(!finishMark)
	{
		cerr<<"Have not called finish function. The Posting list building may not be finished"<<endl;
		if(fileHandle!=NULL) fclose(fileHandle);
		fclose(fileHandleSummary);
	}
}
