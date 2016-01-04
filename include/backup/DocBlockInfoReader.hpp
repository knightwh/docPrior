#pragma once
#include <iostream>
#include <string.h>
#include <math.h>
#include "DAMreader.hpp"
#include "IndexReader.hpp"

using namespace std;

struct DocBlockInfo
{
	float maxScore;
	float* blockScores;

	~DocBlockInfo() {delete[] blockScores;}
};

class DocBlockInfoReader
{
public:
	DocBlockInfoReader(char* path,char* theName,IndexReader* IR);
	DocBlockInfo* getDocBlockInfo(unsigned n);
	inline unsigned getBlockNum() {return blockNum;}
	inline unsigned getBlockSize() {return blockSize;}
	~DocBlockInfoReader();
private:
	IndexReader *theIndex;
	unsigned termN;
	unsigned blockNum;
	unsigned blockSize;
	DiskAsMemoryReader *DAMlookup,*DAMcontent;

	DocBlockInfo* getDocBlockOnline(unsigned n);
	inline float grade(unsigned docID,unsigned tf);
        inline float gradeCommon(unsigned DF);
};

DocBlockInfoReader::DocBlockInfoReader(char* path,char* theName,IndexReader* IR)
{
	theIndex=IR;
	char thePath[512];
	sprintf(thePath,"%s/DocBlockInfo/%s",path,theName);
	char theLookup[512];
	char theContent[512];
	sprintf(theLookup,"%s/lookup",thePath);
	sprintf(theContent,"%s/content",thePath);
	DAMlookup = new DiskAsMemoryReader(theLookup);
	DAMcontent = new DiskAsMemoryReader(theContent);
	DiskItemReader *DIlookup = DAMlookup->localItem(0);
	unsigned* lookup = (unsigned*)DIlookup->content;
	termN = lookup[0];
	blockSize = lookup[1];
	delete DIlookup;
	blockNum = DAMcontent->getItemSize();
}

DocBlockInfoReader::~DocBlockInfoReader()
{
	delete DAMlookup;
	delete DAMcontent;
}

DocBlockInfo* DocBlockInfoReader::getDocBlockInfo(unsigned n)
{
	if(n==0 || n>termN) return NULL;
	if(n>50000000) return getDocBlockOnline(n);
	DiskItemReader *DIlookup = DAMlookup->localItem(n);
	unsigned* lookup = (unsigned*)DIlookup->content;
	if(*lookup == 0) return getDocBlockOnline(n); // get it online
	DocBlockInfo* DBI = new DocBlockInfo;
	DBI->maxScore = *(float*)(lookup+1);
	DBI->blockScores = new float[blockNum];
	DiskItemReader *DIcontent = DAMcontent->localItem(*lookup);
	unsigned char* content = (unsigned char*)DIcontent->content;
	unsigned i;
	for(i=0;i<blockNum;i++) DBI->blockScores[i] = DBI->maxScore * content[i]/255.0;
	delete DIcontent;
	delete DIlookup;
	return DBI;
}

inline float DocBlockInfoReader::gradeCommon(unsigned DF)
{
        float docN = theIndex->documentN;
        float idf = log((docN-DF+0.5)/(DF+0.5));
        //float idf = log((1.0+docN)/DF);
        return idf;
        //return 1.0;
}

inline float DocBlockInfoReader::grade(unsigned docID,unsigned tf)
{
        const float b=0.2;
        const float k1=1.2;
        const float pivotS=0.1;
        float docLength = theIndex->getDocLength(docID);
        float docLengthAvg = theIndex->docLengthAvg;
        float weight = ((k1+1.0)*tf) / (k1*(1.0-b+b*docLength/docLengthAvg)+tf);
        return weight;
}
	
DocBlockInfo* DocBlockInfoReader::getDocBlockOnline(unsigned n)
{
	DocBlockInfo* DBI = new DocBlockInfo;
	PostingReader *PR = theIndex->getPosting(n);
	if(PR==NULL) {cerr<<"The posting list of "<<n<<" could not be read"<<endl;return NULL;}
	DBI->maxScore = 0;
	DBI->blockScores = new float[blockNum];
	float IDF = gradeCommon(PR->docCount);
	unsigned curDocID = 0;
	unsigned bn=0;
	float blockMaxScore = 0;
	while(PR->nextRecord())
	{
		while(PR->getDocID() > curDocID + blockSize)
		{
			blockMaxScore*=IDF;
                        if(DBI->maxScore < blockMaxScore) DBI->maxScore = blockMaxScore;
                        DBI->blockScores[bn++] = blockMaxScore;
			blockMaxScore = 0;
			curDocID += blockSize;
               	}
		float score = grade(PR->getDocID(),PR->getTF());
		if(score > blockMaxScore) blockMaxScore = score;
	}
	while(bn<blockNum)
	{
		blockMaxScore*=IDF;
		if(DBI->maxScore < blockMaxScore) DBI->maxScore = blockMaxScore;
		DBI->blockScores[bn++] = blockMaxScore;
		blockMaxScore = 0;
	}
	return DBI;
}

