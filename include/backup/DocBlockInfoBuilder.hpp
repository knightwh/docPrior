#pragma once
#include "IndexReader.hpp"
#include <string.h>
#include <math.h>
#include <limits.h>
#include <iostream>
#include "DAM.hpp"
#include <vector>

using namespace std;

#ifndef DocBlockThreshold
#define DocBlockThreshold (1<<15)
#endif

#ifndef DocBlockSize
#define DocBlockSize (1<<10)
#endif

class DocBlockInfoBuilder
{
public:
	DocBlockInfoBuilder(char* path,char* name);
	void buildDocBlockInfo();
	~DocBlockInfoBuilder();
private:
	IndexReader *IR;
	DiskAsMemory *DAMlookup,*DAMcontent;
	unsigned blockNum,contentSize;
	
	inline float grade(unsigned docID,unsigned tf);
	inline float gradeCommon(unsigned DF);
	inline void showProcess(unsigned c,unsigned e);
};

DocBlockInfoBuilder::DocBlockInfoBuilder(char* path,char* name)
{
	IR = new IndexReader(path);
	char thePath[512];
	sprintf(thePath,"%s/DocBlockInfo",path);
	mkdir(thePath,S_IRWXU);
	sprintf(thePath,"%s/DocBlockInfo/%s",path,name);
	mkdir(thePath,S_IRWXU);
	char theLookup[512];
	char theContent[512];
	sprintf(theLookup,"%s/lookup",thePath);
	sprintf(theContent,"%s/content",thePath);
	DAMlookup = new DiskAsMemory(theLookup,sizeof(unsigned)*2,1024);
	blockNum = (IR->documentN+DocBlockSize-1)/DocBlockSize;
	contentSize = (blockNum+3)/4*4;
	cout<<"contentSize="<<contentSize<<endl;
	DAMcontent = new DiskAsMemory(theContent,sizeof(unsigned char)*contentSize,1024);
}

DocBlockInfoBuilder::~DocBlockInfoBuilder()
{
	delete DAMlookup;
	delete DAMcontent;
	delete IR;
}

void DocBlockInfoBuilder::buildDocBlockInfo()
{
	DiskItem* DIlookup = DAMlookup->addNewItem();
	unsigned* lookup = (unsigned*)DIlookup->content;
	lookup[0] = IR->termN;
	lookup[1] = DocBlockSize;
	delete DIlookup;
	DiskItem* DIcontent = DAMcontent->addNewItem();
	delete DIcontent;
	
	unsigned i,l;
	unsigned curPos = 1;
	for(i=1;i<=IR->termN && i<=50000000;i++)
	{
		showProcess(i,IR->termN);
		PostingReader *PR = IR->getPosting(i);
		if(PR==NULL) {cerr<<"The posting list of "<<i<<" could not be read"<<endl;continue;}
		if(PR->docCount < DocBlockThreshold) //short posting list
		{
			DIlookup = DAMlookup->addNewItem();
			lookup = (unsigned*)DIlookup->content;
			lookup[0] = 0;lookup[1] = 0;
			delete DIlookup;
			delete PR;
			continue;
		}
		// long posting list
		DIlookup = DAMlookup->addNewItem();
		lookup = (unsigned*)DIlookup->content;
		lookup[0] = curPos;
		float blockScores[contentSize];
		float maxScore = 0;
		unsigned bn=0;
		unsigned curDocID = 0;
		float blockMaxScore = 0;
		while(PR->nextRecord())
		{
			while(PR->getDocID() > curDocID + DocBlockSize)
			{
				if(maxScore < blockMaxScore) maxScore = blockMaxScore;
				blockScores[bn++] = blockMaxScore;
				blockMaxScore = 0;
				curDocID += DocBlockSize;
			}
			float score = grade(PR->getDocID(),PR->getTF());
			if(score > blockMaxScore) blockMaxScore = score;
		}
		while(IR->documentN > curDocID + DocBlockSize)
		{
			if(maxScore < blockMaxScore) maxScore = blockMaxScore;
			blockScores[bn++] = blockMaxScore;
			blockMaxScore = 0;
			curDocID += DocBlockSize;
		}
		while(bn<contentSize) blockScores[bn++] = 0;
		DiskItem* DIcontent = DAMcontent->addNewItem();
		curPos++;
		char* content = DIcontent->content;
		for(l=0;l<contentSize;l++)
		{
			content[l] = (unsigned char)(blockScores[l]*255/maxScore);
			//content[l/4] = (unsigned(blockScores[l]*255/maxScore)) + ((unsigned(blockScores[l+1]*255/maxScore))<<8) + ((unsigned(blockScores[l+2]*255/maxScore))<<16) + ((unsigned(blockScores[l+3]*255/maxScore))<<24);
		}
		delete DIcontent;
		maxScore *= gradeCommon(PR->docCount);
		memmove(lookup+1,&maxScore,sizeof(float));
		delete DIlookup;
		delete PR;
	}
	cout<<endl;
	cout<<"curPos="<<curPos<<endl;
}
			

inline float DocBlockInfoBuilder::gradeCommon(unsigned DF)
{
        float docN = IR->documentN;
        float idf = log((docN-DF+0.5)/(DF+0.5));
        //float idf = log((1.0+docN)/DF);
        return idf;
        //return 1.0;
}

inline float DocBlockInfoBuilder::grade(unsigned docID,unsigned tf)
{
        const float b=0.2;
        const float k1=1.2;
        const float pivotS=0.1;
        float docLength = IR->getDocLength(docID);
        float docLengthAvg = IR->docLengthAvg;
        float weight = ((k1+1.0)*tf) / (k1*(1.0-b+b*docLength/docLengthAvg)+tf);
        return weight;
}

inline void DocBlockInfoBuilder::showProcess(unsigned c,unsigned e)
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
