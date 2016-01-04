#include "query.hpp"
#include <iostream>
#include <math.h>
#include "IndexReader.hpp"
#include "MinHeap.hpp"
#include <string>
#include <fstream>
#include "TimeCounter.hpp"
#include <stdlib.h>
#include <algorithm>
#include "Tier.hpp"
#include <vector>

using namespace std;

#define MaxNum 0xFFFFFFFF

int RetNum=1000;

struct termReter
{
        unsigned curDocID;
        PostingReader *PR;
        unsigned termID;
        float QF;
        float grade;
	float threshold;
        unsigned blockNum;
        unsigned curBlock;
        unsigned* blockDocID;
        float* blockMaxScore;
        BlockInfo *BI;
};

inline bool comReter(termReter t1,termReter t2) {return t1.grade>t2.grade;}

struct BlockNode
{
	Tier *content;
	vector<float> termScores;
	//int blockNum;
	unsigned code;
	float score;
};

class BlockManager
{
public:
	BlockManager(termReter *r, unsigned n,unsigned rn);
	inline void putDoc(unsigned curDoc,termReter *r);
	void getGrade(vector<pair<unsigned,float> > &v);
	inline BlockNode** getBlockList() {return blockList;}
	inline int getBlockNum() {return blockNum;}
	//inline bool decideCut(unsigned n) {return (cutNum[n]<=0);}
	inline float getTh() {return threshold;}
	~BlockManager();
private:
	unsigned num,retNum;
	int blockNum;
	BlockNode **blockList;
	unsigned *blockSize;
	float threshold;
	//int *cutNum;

	int findScore(float score);
};

BlockManager::BlockManager(termReter *r,unsigned n,unsigned rn)
{
	num=n;
	retNum=rn;
	int i,l,j;
	if(num<1) {return;}
	vector<float> termScores;
	blockNum = pow(2,num)-1;
	blockList = new BlockNode*[blockNum];
	blockSize = new unsigned[blockNum];
	//cutNum =new int[num];
	threshold = -10000;
	for(i=0;i<blockNum;i++)
	{
		termScores.clear();
		unsigned code = i+1;
		float score = 0;
		for(l=0;l<num;l++)
		{
			if(code%2) 
			{
				score += (r+l)->grade;
				termScores.push_back((r+l)->grade);
			}
			code/=2;
		}
		for(l=0;l<i;l++) if(score > blockList[l]->score) break;
		for(j=i;j>l;j--) blockList[j] = blockList[j-1];
		blockList[l] = new BlockNode;
		blockList[l]->score = score;
		blockList[l]->termScores = termScores;
		blockList[l]->content = new Tier(termScores.size());
		blockList[l]->code = code;
	}
	//Generate the lookupTable
	for(i=0;i<blockNum;i++)
	{
		blockSize[i]=0;
	}
	//generate the cutNum
	/*int temp = 1;
	for(i=num-1;i>=0;i--)
	{
		temp*=2;
		cutNum[i]=temp-1;
	}*/
}

inline void BlockManager::putDoc(unsigned curDoc,termReter *r)
{
	float score = 0;
	unsigned tf[num];
	int i;
	int n=0;
	for(i=0;i<num;i++)
	{
		if(curDoc == r->curDocID)
		{
			score += r->grade;
			tf[n++] = r->PR->getTF();
		}
		r++;
	}
	n = findScore(score);
	if(n>=0)
	{
		blockList[n]->content->addRecord(curDoc,tf);
		for(i=n;i<blockNum;i++) blockSize[i]++;
		while(n<=blockNum-2 && blockSize[blockNum-2]>=retNum)
		{
			/*unsigned code = blockList[--blockNum]->code;
			for(i=0;i<num-1;i++)
			{
				if((code & (1<<i)) == 0) cutNum[i+1]--;
				else break;
			}*/
			threshold = blockList[blockNum-1]->score;
			delete blockList[blockNum-1]->content;
			delete blockList[blockNum-1];
			blockNum--;
			//deleteNode(--blockNum);
		}
	}
	/*else
	{
		cerr<<"Strange! Could not find the score!\n"<<endl;
	}*/
}

int BlockManager::findScore(float s)
{
	unsigned begin = 0,end = blockNum;
	while(end-1>begin)
	{
		unsigned mid=(begin+end)/2;
		if(blockList[mid]->score ==s) return mid;
		else if(blockList[mid]->score < s) end = mid;
		else begin = mid;
	}
	if(blockList[begin]->score == s) return begin;
	else return -1;
}

		
BlockManager::~BlockManager()
{
	int i;
	for(i=0;i<blockNum;i++) {delete blockList[i]->content;delete blockList[i];}
	delete[] blockSize;
	delete[] blockList;
	//delete[] cutNum;
}

class RetManager
{
public:
	RetManager(IndexReader *IR,query *q);
	void retrieval(unsigned retNum);
	void show(ostream& FO);
        inline int getEvalCounter() {return evalCounter;}
        ~RetManager();
private:
        IndexReader *theIndex;
        unsigned num;
        string topicNum;

        termReter *r;

        unsigned* retDocID;
        float* retDocScore;
        int retN;
        unsigned curDoc;
        MinHeap* topDocs;
        int evalCounter;

	BlockManager *BM;

        inline unsigned moveToDoc(unsigned n,unsigned docID);
        inline unsigned findNextDoc();
        inline float grade();
};

RetManager::RetManager(IndexReader *IR,query *q)
{
        int i;
        theIndex = IR;
        num = q->term.size();
        topicNum = q->topicNum;
        // Initial arrays
        r = new termReter[num];
        // set initial values
        for(i=0;i<num;i++)
        {
                r[i].QF = q->tf[i];
                r[i].termID = theIndex->termLookup(theIndex->stem(q->term[i]));
                if(r[i].termID!=0)
                {
                        r[i].PR = theIndex->getPosting(r[i].termID);
                        if(r[i].PR!=NULL && r[i].PR->nextRecord()) r[i].curDocID = r[i].PR->getDocID();
                        else {r[i].curDocID = MaxNum;cout<<"Could not find the term "<<q->term[i]<<endl;}
                        //r[i].maxScore = theIndex->getMaxScore(r[i].termID);
                        float docN = theIndex->documentN;
                        float DF = r[i].PR->docCount;
                        r[i].grade = log((docN-DF+0.5)/(DF+0.5));
			//cout<<r[i].grade<<endl;
                        BlockInfo* BI = theIndex->getBlockInfo(r[i].termID);
                        r[i].blockDocID = BI->docIDs;
                        r[i].blockMaxScore = BI->maxScores;
                        r[i].blockNum = BI->bn;
                        r[i].curBlock = 0;
                        r[i].BI = BI;
                }
                else
                {
                        r[i].PR = NULL;
                        r[i].BI = NULL;
                        r[i].curDocID = MaxNum;
                        r[i].grade = 0;
                }
        }
        sort(r,r+num,comReter);
	if(num>0) r[num-1].threshold = r[num-1].grade;
	for(i=num-2;i>=0;i--) r[i].threshold = r[i+1].threshold+r[i].grade;
	retDocID = NULL;
        retDocScore = NULL;
        retN = 0;
        curDoc = 0;
        topDocs = NULL;
        evalCounter=0;
}

RetManager::~RetManager()
{
        unsigned i;
        for(i=0;i<num;i++) if(r[i].PR!=NULL) delete r[i].PR;
        for(i=0;i<num;i++) if(r[i].BI!=NULL) delete r[i].BI;
        delete[] r;
        if(retDocID != NULL) delete[] retDocID;
        if(retDocScore!=NULL) delete[] retDocScore;
}

void RetManager::retrieval(unsigned retNum)
{
	retDocID = new unsigned[retNum];
	retDocScore = new float[retNum];

	BM = new BlockManager(r,num,retNum);

	int i,l,n;
	while((curDoc = findNextDoc())!=MaxNum)
	{
		for(i=0;i<num;i++) if(r[i].curDocID<curDoc) r[i].curDocID = moveToDoc(i,curDoc);
		BM->putDoc(curDoc,r);
	}
	const float okapiK1=1.2;
	const float okapiB=0.2;
	BlockNode ** blockList = BM->getBlockList();
	vector<pair<unsigned,unsigned*> >::iterator it;
	for(i=0;i<BM->getBlockNum();i++)
	{
		n=blockList[i]->termScores.size();
		int theSize = blockList[i]->content->record.size();
		if(theSize+retN>retNum) theSize = retNum-retN;
		if(theSize==0) continue;
		topDocs = new MinHeap(theSize);
		for(it=blockList[i]->content->record.begin();it!=blockList[i]->content->record.end();it++)
		{
			unsigned* theTF = it->second;
			float score = 0;
			float docLength = theIndex -> getDocLength(it->first);
			for(l=0;l<n;l++)
			{
				float tf = theTF[l];
				float weight = ((okapiK1+1.0)*tf) / (okapiK1*(1.0-okapiB+okapiB*docLength/theIndex->docLengthAvg)+tf);
				score+=weight*blockList[i]->termScores[l];
			}
			if(score > topDocs->smallest) topDocs->push(it->first,score);
			evalCounter++;
		}
		for(l=retN+theSize-1;l>=retN;l--)
		{
			retDocID[l] = topDocs->pop(retDocScore[l]);
		}
		retN+=theSize;
		delete(topDocs);
	}
	delete(BM);
}

void RetManager::show(ostream &OF)
{
        unsigned i;
        for(i=0;i<retN;i++)
        {
                OF<<topicNum<<"\t";
                OF<<theIndex->findDoc(retDocID[i])<<"\t";
                OF<<retDocScore[i]<<endl;
        }
}

unsigned inline RetManager::findNextDoc()
{
        unsigned minDoc = MaxNum;
        unsigned i,l;
        float s=0.0f;

        for(i=0;i<num;i++)
        {
		//if(BM->decideCut(i)) break;
		if(BM->getTh() >= r[i].threshold) break;
                if(r[i].curDocID<=curDoc)
                {
                        if(r[i].PR->nextRecord()) r[i].curDocID = r[i].PR->getDocID();
                        else r[i].curDocID = MaxNum;
                }
                if(minDoc>r[i].curDocID) minDoc = r[i].curDocID;
        }
        return minDoc;
}

inline unsigned RetManager::moveToDoc(unsigned n,unsigned docID)
{
        if(r[n].blockDocID[r[n].curBlock] < docID)
        {
        for(;r[n].blockDocID[r[n].curBlock]<docID;r[n].curBlock++);
        }
        if(r[n].curBlock>0) r[n].PR->jumpToBlock(r[n].curBlock,r[n].blockDocID[r[n].curBlock-1]);
        unsigned temp;
        while((temp=r[n].PR->getDocID()) < docID)
        {
                r[n].PR->nextRecord();
        }
        return temp;
}

void retrieval(char *indexPath,queryManager *queries,char *outputFile)
{
        IndexReader *theIndex = new IndexReader(indexPath);
        char blockInfoName[] = "BM25-02";
        theIndex -> loadBlockInfo(blockInfoName);

        ofstream F1;
        F1.open(outputFile);
        if(!F1) {cerr << "Error: file "<<outputFile<<" Could not be writ to"<<endl; return;}

        double totalEval=0;
        TimeCounter *timer = new TimeCounter();
        unsigned i;
        for(i=0;i<queries->num();i++)
        {
                timer->start();
                query* qry = queries -> getQuery(i);
                RetManager *reter = new RetManager(theIndex,qry);
                reter->retrieval(RetNum);
                timer->stop();
                cout<<qry->topicNum<<'\t'<<timer->getTrackTime()<<'\t'<<reter->getEvalCounter()<<endl;
                totalEval+=reter->getEvalCounter();
                reter->show(F1);
                //cout<<"\r retrieval topic "<<qry->topicNum<<flush;
                delete reter;
        }
        cout<<endl;
        cout<<"Used total time "<<timer->getTotalTime()<<"ms, average "<<timer->getAverageTime()<<"ms"<<" with "<<totalEval/queries->num()<<" evaluateions"<<endl;
        F1.close();
        delete(theIndex);
        delete(timer);
}

int main(int argc,char **argv)
{
        if(argc!=4 && argc!=5) { cout<<"Usage: "<<argv[0]<<" indexPath qry outputFile (retNum)"<<endl; return 0;}
        if(argc==5) RetNum=atoi(argv[4]);
        queryManager *queries = new queryManager(argv[2]);
        //queries->show();
        retrieval(argv[1],queries,argv[3]);
        delete queries;
        return 0;
}
