#pragma once
#include "IndexReader.hpp"
#include <string.h>
#include <math.h>
#include <limits.h>
#include <iostream>

#ifndef MaxFileSize
#define MaxFileSize (1024*1024*512)
#endif

using namespace std;

class BlockInfoBuilder
{
public:
	BlockInfoBuilder(char* path,char* name);
	void buildBlockInfo();
	~BlockInfoBuilder();
private:
	IndexReader *IR;
	char thePath[512];
	char theName[512];
	FILE *FH,*FHsummary;
	PostingReader* PR;

	int fileNum;
	unsigned fileUsage;
	float* buffer;
	unsigned* buffer2;

	float getBlockInfoTerm();
	float inline grade(unsigned docID,unsigned tf);
	float inline gradeCommon();
	void openNewFile();
	inline void showProcess(unsigned c,unsigned e);
};

BlockInfoBuilder::BlockInfoBuilder(char* path,char* name)
{
	strcpy(thePath,path);
	char filename[512];
	sprintf(filename,"%s/blockInfo",path);
	mkdir(filename,S_IRWXU);
	sprintf(theName,"%s/blockInfo/%s",path,name);
}

void BlockInfoBuilder::buildBlockInfo()
{
	char filename[512];
	fileNum = -1;
	fileUsage = MaxFileSize;
	FH=NULL;

	IR = new IndexReader(thePath);
	sprintf(filename,"%s-summary",theName);
	FHsummary = fopen(filename,"wb");
	if(!FHsummary) {cerr<<"Could not write to the file "<<filename<<endl;return;}
	sprintf(filename,"%s-tables",theName);
	mkdir(filename,S_IRWXU);
	unsigned dump[3] = {0,0,0};
	dump[0] = IR->termN;
	fwrite(dump,sizeof(unsigned),3,FHsummary);
	
	unsigned i;
	for(i=1;i<=IR->termN;i++)
	{
		PR = IR->getPosting(i);
		if(PR==NULL) cerr<<"Strange the posting list of "<<i<<" could not be read"<<endl;
		unsigned bn = (PR->docCount+PostingPerSet) / PostingPerSet;
		buffer = new float[bn];
		buffer2 = new unsigned[bn];
		float theMaxScore = getBlockInfoTerm();
		if(fileUsage + bn > MaxFileSize) openNewFile();
		unsigned temp = fileNum | (bn<<8);
		fwrite(&temp,sizeof(int),1,FHsummary);
		fwrite(&fileUsage,sizeof(unsigned),1,FHsummary);
		fwrite(&theMaxScore, sizeof(float),1,FHsummary);
		fwrite(buffer,sizeof(float),bn,FH);
		fwrite(buffer2,sizeof(unsigned),bn,FH);
		fileUsage += bn*2;
		delete[] buffer;
		delete[] buffer2;
		delete PR;
		showProcess(i,IR->termN);
	}
	cout<<endl;
	if(FH!=NULL) fclose(FH);
	fclose(FHsummary);
	delete(IR);
}

void BlockInfoBuilder::openNewFile()
{
	if(FH!=NULL) fclose(FH);
	fileNum++;
	char filename[512];
	sprintf(filename,"%s-tables/%d",theName,fileNum);
	if(!(FH = fopen(filename,"wb"))) {cerr<<"Could not write to the file "<<filename<<endl;return;}
	fileUsage = 0;
}

float BlockInfoBuilder::getBlockInfoTerm()
{
	unsigned docIDs[PostingPerSet*2];
	unsigned TFs[PostingPerSet*2];
	float maxAll = -UINT_MAX;
	unsigned n=0;
	while(PR->nextBlock())
	{
		unsigned length = PR->getBlock(docIDs,TFs);
		float maxBlock = -UINT_MAX;
		unsigned i;
		float score;
		for(i=0;i<length;i++)
		{
			if((score = grade(docIDs[i],TFs[i]))>maxBlock) maxBlock = score;
		}
		maxBlock*=gradeCommon();
		buffer[n] = maxBlock;
		buffer2[n] = docIDs[length-1];
		n++;
		if(maxAll<maxBlock) maxAll = maxBlock;
	}
	return maxAll;
}

inline float BlockInfoBuilder::gradeCommon()
{
	float docN = IR->documentN;
	float DF = PR->docCount;
	float idf = log((docN-DF+0.5)/(DF+0.5));
	return idf;
}

inline float BlockInfoBuilder::grade(unsigned docID,unsigned tf)
{
	const float b=0.2;
	const float k1=1.2;
	float docLength = IR->getDocLength(docID);
	float docLengthAvg = IR->docLengthAvg;
	float weight = ((k1+1.0)*tf) / (k1*(1.0-b+b*docLength/docLengthAvg)+tf);
	return weight;
}

inline void BlockInfoBuilder::showProcess(unsigned c,unsigned e)
{
        if(c<100 || c%500==0)
        {
                unsigned i;
                unsigned length = (double)c/e*21;
                cout<<"\r"<<c<<":[";
                for(i=0;i<length;i++) cout<<"-";
                for(;i<20;i++) cout<<" ";
                cout<<"]:"<<e<<flush;
        }
}


BlockInfoBuilder::~BlockInfoBuilder() {}

