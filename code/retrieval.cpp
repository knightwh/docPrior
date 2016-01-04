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
};

inline bool comReter(termReter t1,termReter t2) {return t1.grade>t2.grade;}

struct DecisionTreeNode
{
	DecisionTreeNode *parent;
	DecisionTreeNode *left;
	DecisionTreeNode *right;
	Tier *content;
	vector<float> termScores;
	int blockNum;
	float score;
};

inline bool comDecisionTreeNode(DecisionTreeNode *n1,DecisionTreeNode *n2) {return n1->score > n2->score;}

class DecisionTree
{
public:
	DecisionTree(termReter *r,unsigned n,unsigned rn);
	inline void putDoc(unsigned curDoc,termReter *r);
	void getGrade(vector<pair<unsigned,float> > &v);
	inline DecisionTreeNode** getBlockList();
	inline int getBlockNum();
	~DecisionTree();
private:
	unsigned num,retNum;
	int blockNum;
	DecisionTreeNode **blockList;
	unsigned *blockSize;
	DecisionTreeNode *head;

	void deleteNode(DecisionTreeNode *n);
	void deleteLeaf(DecisionTreeNode *n);
};

DecisionTree::DecisionTree(termReter *r,unsigned n,unsigned rn)
{
	num=n;
	retNum=rn;
	vector<DecisionTreeNode*> parents,parentsNew;
	int i;
	vector<DecisionTreeNode*>::iterator it;

	//Generate the tree structure	
	if(num<1) {head=NULL;return;}
	head = new DecisionTreeNode;
	head->parent = NULL;
	head->score = 0;
	parents.push_back(head);
	for(i=0;i<num;i++)
	{
		for(it=parents.begin();it!=parents.end();it++)
		{
			DecisionTreeNode *node = new DecisionTreeNode;
			(*it)->left = node;
			node->parent = *it;
			node->score = (*it)->score + r->grade;
			node->termScores = (*it)->termScores;
			node->termScores.push_back(r->grade);
			parentsNew.push_back(node);
			node = new DecisionTreeNode;
			(*it)->right = node;
			node->parent = *it;
			node->score = (*it)->score;
			node->termScores = (*it)->termScores;
			parentsNew.push_back(node);
			(*it)->content=NULL;
			(*it)->blockNum = -1;
		}
		parents=parentsNew;
		parentsNew.clear();
		r++;
	}
	//Generate the blockList
	blockList = new DecisionTreeNode*[parents.size()];
	blockSize = new unsigned[parents.size()];
	blockNum=0;
	for(it=parents.begin();it!=parents.end();it++)
	{
		(*it)->left=NULL;
		(*it)->right=NULL;
		(*it)->content = new Tier((*it)->termScores.size());
		blockList[blockNum++]=*it;
	}
	sort(blockList,blockList+blockNum,comDecisionTreeNode);
	for(i=0;i<blockNum;i++) {blockList[i]->blockNum=i;blockSize[i]=0;}
}
	
void DecisionTree::deleteNode(DecisionTreeNode *n)
{
	if(n->left!=NULL) deleteNode(n->left);
	if(n->right!=NULL) deleteNode(n->right);
	if(n->content!=NULL) delete n->content;
	delete n;
}

void DecisionTree::deleteLeaf(DecisionTreeNode *n)
{
	if(n->content!=NULL) delete n->content;
	if(n->parent!=NULL)
	{
		DecisionTreeNode *parent = n->parent;
		if(parent->left == n) parent->left = NULL;
		if(parent->right == n) parent ->right = NULL;
		if(parent->left == NULL && parent->right == NULL) deleteLeaf(parent);
	}
	delete n;
}

inline DecisionTreeNode** DecisionTree::getBlockList() {return blockList;}
inline int DecisionTree::getBlockNum() {return blockNum;}

inline void DecisionTree::putDoc(unsigned curDoc,termReter *r)
{
	DecisionTreeNode* curNode = head;
	unsigned tf[num];
	int n=0;
	while(curNode!=NULL && curNode->content==NULL)
	{
		if(curDoc==r->curDocID)
		{
			curNode = curNode->left;
			tf[n++] = r->PR->getTF();
		}
		else
		{
			curNode = curNode->right;
		}
		r++;
	}
	if(curNode!=NULL) 
	{
		curNode->content->addRecord(curDoc,tf);
		n=curNode->blockNum;
		int i;
		for(i=n;i<blockNum;i++) blockSize[i]++;
		while(n<=blockNum-2 && blockSize[blockNum-2]>=retNum)
		{
			blockNum--;
			deleteLeaf(blockList[blockNum]);
		}
	}	
}		

DecisionTree::~DecisionTree()
{
	//cout<<"blockSize = "<<blockSize[0]<<endl;
	deleteNode(head);
	delete[] blockSize;
	delete[] blockList;
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
	unsigned retN;
	unsigned curDoc;
	MinHeap* topDocs;

	int evalCounter;

	inline unsigned findNextDoc();
	inline float grade();
};

RetManager::RetManager(IndexReader *IR,query *q)
{
	unsigned i;
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
		}
		else
		{
			r[i].PR = NULL;
			r[i].curDocID = MaxNum;
			r[i].grade = 0;
		}
	}
	sort(r,r+num,comReter);
	//for(i=0;i<num;i++) cout<<r[i].grade<<endl;
	// set other values
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
	delete[] r;
	if(retDocID != NULL) delete[] retDocID;
	if(retDocScore!=NULL) delete[] retDocScore;
}

void RetManager::retrieval(unsigned retNum)
{
	retDocID = new unsigned[retNum];
	retDocScore = new float[retNum];
	topDocs = new MinHeap(retNum);
	DecisionTree *DT = new DecisionTree(r,num,retNum);
	
	while((curDoc = findNextDoc())!=MaxNum)
	{
		DT->putDoc(curDoc,r);
		//float score = grade();
		//if(score > topDocs->smallest) topDocs->push(curDoc,score);
	}
	int i,l,n;
	const float okapiK1=1.2;
	const float okapiB=0.2;
	DecisionTreeNode **blockList = DT->getBlockList();
	vector<pair<unsigned,unsigned*> >::iterator it;
	for(i=0;i<DT->getBlockNum();i++)
	{
		n=blockList[i]->termScores.size();
		for(it=blockList[i]->content->record.begin();it!=blockList[i]->content->record.end();it++)
		{
			unsigned* theTF=it->second;
			float score=0;
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
	}

	retN = topDocs->n;
	for(i=retN-1;i>=0;i--)
	{
		retDocID[i] = topDocs->pop(retDocScore[i]);
	}
	delete(topDocs);
	delete(DT);
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
		if(r[i].curDocID<=curDoc) 
		{
			if(r[i].PR->nextRecord()) r[i].curDocID = r[i].PR->getDocID();
			else r[i].curDocID = MaxNum;
		}
	}
	for(i=0;i<num;i++)
	{
		if(minDoc> r[i].curDocID) minDoc = r[i].curDocID;
	}
	return minDoc;
}

float inline RetManager::grade()
{
	const float okapiB = 0.2;
	const float okapiK1 = 1.2;
	const float  okapiK3 = 7;
	float score = 0;
	float docN = theIndex -> documentN;
	float docLength = theIndex -> getDocLength(curDoc);
	float docLengthAvg = theIndex -> docLengthAvg;
	unsigned i;
	for(i=0;i<num;i++)
	{
		if(curDoc == r[i].curDocID)
		{
			float DF = r[i].PR->docCount;
			float tf = r[i].PR->getTF();
			float idf = log((docN-DF+0.5)/(DF+0.5));
			//float idf = log((1.0+docN)/DF);
			//float weight = (1.0+log(1+log(tf)))/(1.0-okapiB+okapiB*docLength/docLengthAvg);
			float weight = ((okapiK1+1.0)*tf) / (okapiK1*(1.0-okapiB+okapiB*docLength/docLengthAvg)+tf);
			//float tWeight = ((okapiK3+1)*r[i].QF)/(okapiK3+r[i].QF);
			//score+=idf*weight*tWeight;
			score+=idf*weight;
		}
	}
	/*unsigned i;
	float score=0;
	const float mu = 2500;
	float docLength = theIndex -> getDocLength(curDoc);
	for(i=0;i<num;i++)
	{
		if(curDoc == curDocID[i])
		{
			float termPro = PR[i]->totalTF/theIndex->termNTotal;
			float tf = PR[i]->getTF();
			score+=log((tf+mu*termPro)/(docLength+mu));
		}
		else if(PR[i] != NULL)
		{
			float termPro = PR[i]->totalTF/theIndex->termNTotal;
			score+=log((mu*termPro)/(docLength+mu));
		}
	}*/
	//float score = 1;
	evalCounter++;
	return score;
}
	
void retrieval(char *indexPath,queryManager *queries,char *outputFile)
{
	IndexReader *theIndex = new IndexReader(indexPath);
	char blockInfoName[] = "BM25-02";
	//theIndex -> loadBlockInfo(blockInfoName);
	
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
