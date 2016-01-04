#pragma once
//#include "DiskBuffer.hpp"

using namespace std;

class DiskBuffer;

class DiskItem
{
public:
	DiskItem(char* c,unsigned i,DiskBuffer* b) : content(c),buffer(b),itemNum(i) {};
	//inline unsigned getItemNum() {return itemNum;}
	unsigned getItemSize();
	~DiskItem();
	char* const content;
	DiskBuffer* const buffer;
	const unsigned itemNum;
};
