#pragma once
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <vector>
#include <iostream>
#include <stack>

using namespace std;

class PrefixTreeNode
{
public:
	unsigned value;

	PrefixTreeNode();
	unsigned lookupTerm(char *term);
	void insertTerm(char *term,unsigned v);
	PrefixTreeNode* simplifyPath(char *term,int &length);
	~PrefixTreeNode();

	vector<char> keys;
	vector<class PrefixTreeNode*> children;
};

PrefixTreeNode::PrefixTreeNode()
{
	value = 0;
}

unsigned PrefixTreeNode::lookupTerm(char *term)
{
	char ch = *term;
	if(ch == '\0') return value;

	unsigned i;
	for(i=0;i<keys.size();i++)
	{
		if(ch == keys[i]) return children[i]->lookupTerm(term+1);
	}
	return 0;
}

PrefixTreeNode* PrefixTreeNode::simplifyPath(char *term,int &length)
{
	if(value !=0 || keys.size()!=1 || length>=15) return this;
	term[length++]=keys[0];
	return children[0]->simplifyPath(term,length);
}

void PrefixTreeNode::insertTerm(char *term,unsigned v)
{
	if(strlen(term)>255) term[255]='\0';
	char ch = *term;
	if(ch == '\0') {value = v;return;}
	unsigned i;
	for(i=0;i<keys.size();i++)
	{
		if(ch == keys[i]) {children[i]->insertTerm(term+1,v);return;}
	}
	PrefixTreeNode *newNode = new PrefixTreeNode();
	keys.push_back(ch);
	children.push_back(newNode);
	newNode->insertTerm(term+1,v);
}

PrefixTreeNode::~PrefixTreeNode()
{
	vector<class PrefixTreeNode*>::iterator it;
	for(it=children.begin();it!=children.end();it++) delete *it;
}
	
//-----------------------------------------------------------//

class PrefixTree
{
public:
	PrefixTree();
	unsigned lookupTerm(char *term);
	void insertTerm(char *term,unsigned v);
	void insertTerm(string s,unsigned v);
	void writeToFile(char *filePath);
	
	~PrefixTree();
private:
	unsigned vSize;
	PrefixTreeNode *head;
};

PrefixTree::PrefixTree()
{
	head = new PrefixTreeNode();
	vSize=0;
}

unsigned PrefixTree::lookupTerm(char *term)
{
	if(strlen(term)>255) term[255] = '\0';
	if(*term == '\0') return 0;
	else return head->lookupTerm(term);
}

void PrefixTree::insertTerm(char *term,unsigned v)
{
	if(*term == '\0') return;
	head->insertTerm(term,v);
	vSize++;
}

void PrefixTree::insertTerm(string s,unsigned v)
{
	char term[256];
	if(s.length()>255) s=s.substr(0,255);
	strcpy(term,s.c_str());
	insertTerm(term,v);
}

void PrefixTree::writeToFile(char *filename)
{
	int fileHandle=open(filename,O_RDWR | O_CREAT | O_TRUNC, (mode_t)0644);
	if(fileHandle==-1) { cerr<<"Could not write the file "<<filename<<endl;return;}
	unsigned pos=0,num=0;
	unsigned *positions = new unsigned[vSize*2];
	stack<PrefixTreeNode *> nodeStack;
	stack<unsigned> numStack;
	nodeStack.push(head);
	numStack.push(num++);

	unsigned buffer[1024];
	unsigned lengthArray[256];
	char stringArray[256][16];
	unsigned numArray[256];

	while(!nodeStack.empty())
	{
		PrefixTreeNode *curNode = nodeStack.top();
		nodeStack.pop();
		unsigned curNum = numStack.top();
		numStack.pop();
		unsigned i,l;
		unsigned size = curNode->keys.size();
		for(i=0;i<size;i++)
		{
			char chs[16];
			chs[0]=curNode->keys[i];
			int length=1;
			PrefixTreeNode *childNode = curNode->children[i]->simplifyPath(chs,length);
			memmove(stringArray[i],chs,16);
			lengthArray[i]=length;
			numArray[i]=num;
			nodeStack.push(childNode);
			numStack.push(num++);
		}
		//write to the buffer
		buffer[0]=curNode->value;
		buffer[1]=size;
		for(i=0;i*8<size;i++)
		{
			buffer[2+i]=0;
			for(l=0;l<8 && i*8+l<size;l++)
			{
				buffer[2+i] |= (lengthArray[i*8+l] & 15)<<(l*4);
			}
		}
		unsigned length = 2+(size+7)/8;
		char *bufferchar = (char*)(buffer+length);
		unsigned charLength = 0;
		for(i=0;i<size;i++)
		{
			memmove(bufferchar,stringArray[i],lengthArray[i]);
			bufferchar+=lengthArray[i];
			charLength+=lengthArray[i];
		}
		while(charLength%4!=0) {*(bufferchar++)='\0';charLength++;}
		length+=charLength/4;
		buffer[1] |= length<<8;
		for(i=0;i<size;i++) buffer[length++]=numArray[i];
		positions[curNum]=pos;
		pos+=length;
		write(fileHandle,(char*)buffer,length*4);
	}
	// fix the position imformation
	unsigned totalSize = pos;
	unsigned *buff = (unsigned *)mmap(0,totalSize*4,PROT_READ | PROT_WRITE,MAP_SHARED,fileHandle,0);
	pos = 0;
	while(pos < totalSize)
	{
		unsigned size = buff[++pos] & 255;
		unsigned offset = buff[pos] >> 8;
		pos += offset-1;
		unsigned i;
		for(i=0;i<size;i++)
		{
			buff[pos] = positions[buff[pos]];
			pos++;
		}
	}
	if(munmap((char*)buff,totalSize*4) == -1) {cerr<<"unmap error at postion fix"<<endl;}
	close(fileHandle);
	delete[] positions;
}
			
PrefixTree::~PrefixTree()
{
	delete head;
}


	
