#pragma once
#include <vector>
#include <iostream>

#ifndef MaxQueryLength
#define MaxQueryLength 32
#endif

using namespace std;

class Tier
{
public:
	vector<pair<unsigned,unsigned*> > record;

	Tier(unsigned n) {num=n;}
	void addRecord(unsigned docID,unsigned tf[]);
	inline unsigned size();
	~Tier();
private:
	unsigned num;
};

void Tier::addRecord(unsigned docID,unsigned tf[])
{
	unsigned *theTF = new unsigned[num];
	memcpy(theTF,tf,sizeof(unsigned)*num);
	record.push_back(pair<unsigned,unsigned*>(docID,theTF));
}

inline unsigned Tier::size() {return record.size();}

Tier::~Tier()
{
	vector<pair<unsigned,unsigned*> >::iterator it;
	for(it=record.begin();it!=record.end();it++) delete[] (it->second);
}
