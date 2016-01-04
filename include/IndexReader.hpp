#pragma once
#include <iostream>
#include <string.h>
#include "PostingListReader.hpp"
#include "QuickTermLookup.hpp"
#include "PrefixTreeReader.hpp"
#include "DocIDLookup.hpp"
#include "DocInfo.hpp"
#include "porter.hpp"
#include "BlockInfoReader.hpp"

using namespace std;

class IndexReader
{
public:
	unsigned documentN;
	unsigned termN;
	double termNTotal;
	double docLengthAvg;
	unsigned maxDF;

	IndexReader(char* path);
	~IndexReader();

	inline PostingReader* getPosting(unsigned n);
	inline unsigned termLookup(char* term);
	inline unsigned termLookup(string s);
	inline char* findDoc(unsigned n);
	inline unsigned getDocLength(unsigned n);
	inline char* stem(char *t);
	inline string stem(string s);

	void loadBlockInfo(char *name);
	inline BlockInfo* getBlockInfo(unsigned n);
	inline float getMaxScore(unsigned n);
private:
	char thePath[256]; 
	PostingListReader *PLR;
	QuickTermLookup *QTL;
	PrefixTreeReader *PTR;
	DocIDLookupReader *DLR;
	DocInfoReader *DR;
	StemTerm *stemmer;
	BlockInfoReader* BIR;
};

IndexReader::IndexReader(char* path)
{
	strcpy(thePath,path);
	//Get the summary information
	char filename[256];
	FILE* fileHandle;
	sprintf(filename,"%s/summary",path);
	fileHandle = fopen(filename,"rb");
	if(!fileHandle) {cerr<<"Could not read the file "<<filename<<endl;return;}
	fread(&termN,sizeof(unsigned),1,fileHandle);
	fread(&documentN,sizeof(unsigned),1,fileHandle);
	fread(&maxDF,sizeof(unsigned),1,fileHandle);
	fread(&docLengthAvg,sizeof(double),1,fileHandle);
	fread(&termNTotal,sizeof(double),1,fileHandle);
	fclose(fileHandle);
	PLR = NULL;QTL = NULL;PTR = NULL;DLR = NULL; DR = NULL;BIR=NULL;
	//Initial the PostingList Reader
	sprintf(filename,"%s/postingList",path);
	PLR = new PostingListReader(filename);
	//Initial the QuickTermLookup
	sprintf(filename,"%s/quickTermLookup",path);
	QTL = new QuickTermLookup();
	QTL -> readFromFile(filename);
	//Initial the term lookup
	sprintf(filename,"%s/termLookup",path);
	PTR = new PrefixTreeReader(filename);
	//Initial the docID lookup
	sprintf(filename,"%s/DocIDLookup",path);
	DLR = new DocIDLookupReader(filename);
	//Initial the doc info reader
	sprintf(filename,"%s/documentInfo",path);
	DR = new DocInfoReader(filename);
	//Initial stemmer
	stemmer = new StemTerm();
}

inline PostingReader* IndexReader::getPosting(unsigned n)
{
	return PLR->getPosting(n);
}

inline unsigned IndexReader::termLookup(char *t)
{
	unsigned termID;
	if((termID=QTL->termLookup(t))==0) return PTR->lookupTerm(t);
	else return termID;
}

inline unsigned IndexReader::termLookup(string s)
{
	unsigned termID;
	if((termID=QTL->termLookup(s))==0) return PTR->lookupTerm(s);
	else return termID;
}

inline char* IndexReader::findDoc(unsigned n)
{
	return DLR->findDoc(n);
}

inline unsigned IndexReader::getDocLength(unsigned n)
{
	return DR->getDocLength(n);
}

inline char* IndexReader::stem(char* t)
{
	return stemmer->stem(t);
}

inline string IndexReader::stem(string s)
{
	return stemmer->stem(s);
}

void IndexReader::loadBlockInfo(char *n)
{
	char filename[512];
	sprintf(filename,"%s/blockInfo/%s",thePath,n);
	if(BIR!=NULL) delete(BIR);
	BIR = new BlockInfoReader(filename);
}

inline BlockInfo* IndexReader::getBlockInfo(unsigned n)
{
	return BIR->getBlockInfo(n);
}

inline float IndexReader::getMaxScore(unsigned n)
{
	return BIR->getMaxScore(n);
}

IndexReader::~IndexReader()
{
	if(PLR!=NULL) delete(PLR);
	if(QTL!=NULL) delete(QTL);
	if(PTR!=NULL) delete(PTR);
	if(DLR!=NULL) delete(DLR);
	if(DR!=NULL) delete(DR);
	if(stemmer!=NULL) delete(stemmer);
	if(BIR!=NULL) delete(BIR);
}
