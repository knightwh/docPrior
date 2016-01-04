#pragma once
#include "PostingListBuilder.hpp"
#include "QuickTermLookup.hpp"
#include "PrefixTree.hpp"
#include "DocIDLookup.hpp"
#include "DocInfo.hpp"
#include <iostream>
#include <string.h>
#include <stdio.h>

#define QuickTermLookupSize 65535

using namespace std;

class IndexBuilder
{
public:
	IndexBuilder(char *path,char* indriPath);
	void buildIndex();
	void buildPostingList();
	void buildQuickTermLookup();
	void buildTermLookup();
	void buildDocIDLookup();
	void buildDocInfo();
	void buildSummary();
	~IndexBuilder();
private:
	char thePath[256];
	IndriIndexReader *II;

	inline void showProcess(unsigned c,unsigned e);
};

IndexBuilder::IndexBuilder(char *indriPath,char *path)
{
	strcpy(thePath,path);
	II = new IndriIndexReader(indriPath);
	mkdir(path,S_IRWXU);
}

void IndexBuilder::buildIndex()
{
	buildPostingList();
	buildQuickTermLookup();
	buildTermLookup();
	buildDocIDLookup();
	buildDocInfo();
	buildSummary();
}

inline void IndexBuilder::showProcess(unsigned c,unsigned e)
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

void IndexBuilder::buildPostingList()
{
	cout<<"Begin build posting list"<<endl;
	char filename[256];
	sprintf(filename,"%s/postingList",thePath);
	PostingListBuilder *PLB = new PostingListBuilder(filename);
	unsigned i;
	for(i=1;i<=II->termN;i++)
	{
		IndriPostingList* IPL=II->getPostingList(i);
		PForDeltaCompressor* PF = new PForDeltaCompressor(IPL);
		PF->compressData();
		PLB->insertPosting(PF);
		showProcess(i,II->termN);
		delete(IPL);
		delete(PF);
	}
	PLB->finish();
	cout<<"\tOK"<<endl;
	delete(PLB);
}

void IndexBuilder::buildQuickTermLookup()
{
	cout<<"Begin build the quick term lookup"<<endl;
	QuickTermLookup *QTL = new QuickTermLookup(QuickTermLookupSize*4);
	char filename[256];
	sprintf(filename,"%s/quickTermLookup",thePath);
	unsigned i;
	unsigned tn = (II->termN < QuickTermLookupSize)?II->termN:QuickTermLookupSize;
	for(i=1;i<tn;i++)
	{
		if(II->checkTerm(i)) QTL->insertTerm(II->getTerm(i),i);
		showProcess(i,tn);
	}
	QTL->writeToFile(filename);
	cout<<"\tOK"<<endl;
	delete(QTL);
}

void IndexBuilder::buildTermLookup()
{
	cout<<"Begin build the term look up"<<endl;
	char filename[256];
	sprintf(filename,"%s/termLookup",thePath);
	unsigned i;
	PrefixTree *PT = new PrefixTree();
	for(i=1;i<=II->termN;i++)
	{
		if(II->checkTerm(i)) PT->insertTerm(II->getTerm(i),i);
		showProcess(i,II->termN);
	}
	PT->writeToFile(filename);
	cout<<"\tOK"<<endl;
	delete(PT);
}

void IndexBuilder::buildDocIDLookup()
{
	cout<<"Begin build the docID look up"<<endl;
	char filename[256];
	sprintf(filename,"%s/DocIDLookup",thePath);
	unsigned i;
	DocIDLookupBuilder *DLB = new DocIDLookupBuilder(filename);
	for(i=1;i<=II->documentN;i++)
	{
		DLB->addDoc(II->getDoc(i));
		showProcess(i,II->documentN);
	}
	cout<<"\tOK"<<endl;
	DLB->finish();
	delete(DLB);
}

void IndexBuilder::buildDocInfo()
{
	cout<<"Begin build the document info index"<<endl;
	unsigned i;
	char filename[256];
	sprintf(filename,"%s/documentInfo",thePath);
	DocInfoBuilder *DIB = new DocInfoBuilder(filename);
	for(i=1;i<=II->documentN;i++)
	{
		DIB->insertDoc(II->getDocumentLength(i));
		showProcess(i,II->documentN);
	}
	DIB->finish();
	delete(DIB);
	cout<<"\tOK"<<endl;
}

void IndexBuilder::buildSummary()
{
	cout<<"Begin build the index summary"<<endl;
	char filename[256];
	sprintf(filename,"%s/summary",thePath);
	FILE *fileHandle = fopen(filename,"wb");
	if(!fileHandle) { cerr<<"Could not write the file "<<filename<<endl;return;}
	unsigned theStat;
	theStat = II->termN;
	fwrite(&theStat,sizeof(unsigned),1,fileHandle);
	theStat = II->documentN;
	fwrite(&theStat,sizeof(unsigned),1,fileHandle);
	theStat = II->getMaxDF();
	fwrite(&theStat,sizeof(unsigned),1,fileHandle);
	double theStatD;
	theStatD = II->docLengthAvg;
	fwrite(&theStatD,sizeof(double),1,fileHandle);
	theStatD = II->collectionN;
	fwrite(&theStatD,sizeof(double),1,fileHandle);
	fclose(fileHandle);
	cout<<"OK"<<endl;
}
	

IndexBuilder::~IndexBuilder()
{
	delete(II);
}
