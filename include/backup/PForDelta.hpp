#pragma once
#include "IndriIndex.hpp"
#include <vector>

#define BThreshold (0.9)
#define PostingPerSet (64)

using namespace std;

class PForDeltaCompressor
{
public:
	unsigned docNum;
	unsigned totalTF;
	unsigned b;
	unsigned a;
	IndriPostingList *PL;
	vector< vector<unsigned> > cDatas;
	vector<unsigned> Bs;
	vector<unsigned> As;
	
	PForDeltaCompressor(IndriPostingList *P);
	void getAB(vector<unsigned> &);
	void PForCompress(vector<unsigned> &);
	void compressData();
private:
	unsigned bsets[9];
};

PForDeltaCompressor::PForDeltaCompressor(IndriPostingList *P)
{
	PL=P;
	totalTF=PL->total;
	docNum=PL->DF;
	bsets[0]=1;bsets[1]=2;bsets[2]=3;bsets[3]=4;bsets[4]=5;bsets[5]=6;
	bsets[6]=8;bsets[7]=10;bsets[8]=16;
}

void PForDeltaCompressor::getAB(vector<unsigned> &v)
{
	vector<unsigned> theV=v;
	sort(theV.begin(),theV.end());

	unsigned threshold = theV[((unsigned)(double)theV.size()*BThreshold-1)];
	unsigned maxNumber = theV[theV.size()-1];
	unsigned bn=0;
	while((threshold>>bsets[bn])>0 && bn<8) bn++;
	b=bsets[bn];
	maxNumber=maxNumber>>b;
	a=1;
	while(a<4 && (maxNumber>= (1<<(a*8)))) a++;
	As.push_back(a);
	Bs.push_back(b);
}

void PForDeltaCompressor::compressData()
{
	vector<unsigned>::iterator it;
	
	vector<unsigned> docIDs;
	vector<unsigned> TFs;
	unsigned end=0;
	unsigned i;
	for(i=0;i<PL->docIDs.size();i++)
	{
		if(i%PostingPerSet==0 && i!=0)
		{
			getAB(docIDs);
			PForCompress(docIDs);
			docIDs.clear();
			getAB(TFs);
			PForCompress(TFs);
			TFs.clear();
		}
		docIDs.push_back(PL->docIDs[i] - end -1);
		end = PL->docIDs[i];
		TFs.push_back(PL->TFs[i] -1);
	}
	getAB(docIDs);
	PForCompress(docIDs);
	getAB(TFs);
	PForCompress(TFs);
}
	
void PForDeltaCompressor::PForCompress(vector<unsigned> &v)
{
	unsigned n=cDatas.size();
	unsigned threshold=1<<b;
	cDatas.resize(n+1);
	
	vector<unsigned> tail;
	vector<unsigned>::iterator it;

	unsigned theSize=(v.size()+32/b-1)/(32/b);
	for(unsigned i=0;i<theSize;i++) cDatas[n].push_back(0);
	for(unsigned i=0;i<v.size();i++)
	{
		unsigned low=v[i] & (threshold-1);
		cDatas[n][i/(32/b)] |= low << i%(32/b)*b;
		if(v[i] >=threshold)
		{
			tail.push_back(i);
			unsigned high=v[i]>>b;
			for(unsigned l=0;l<a;l++)
			{
				tail.push_back((high>>(l*8)) & 255);
			}
			//tail.push_back(i);
		}
	}
	unsigned temp=0;
	unsigned i;
	for(i=0;i<tail.size();i++)
	{
		temp |= tail[i]<<(i*8%32);
		if(i%4==3) { cDatas[n].push_back(temp);temp=0;}
	}
	if(i%4!=0) cDatas[n].push_back(temp);
}

//#################################################################################################



