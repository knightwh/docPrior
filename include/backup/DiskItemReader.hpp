#pragma once
//#include "DiskBuffer.hpp"

using namespace std;

class DiskBufferReader;

class DiskItemReader
{
public:
	DiskItemReader(char* c,unsigned i,DiskBufferReader* b) : content(c),buffer(b),itemNum(i) {};
	//inline unsigned getItemNum() {return itemNum;}
	unsigned getItemSize();
	~DiskItemReader();
	char* const content;
	DiskBufferReader* const buffer;
	const unsigned itemNum;
};
