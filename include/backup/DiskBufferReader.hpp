#pragma once
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <stdio.h>

#include "DiskItemReader.hpp"

using namespace std;

class DiskAsMemoryReader;

class DiskBufferReader
{
public:
	DiskBufferReader(DiskAsMemoryReader *theIO,unsigned begin,unsigned end,int fileHandle,int offset);
	DiskItemReader* localItem(unsigned itemNum);
	inline unsigned getUsage() {return usage;}
	void freeUsage();
	//friend class DiskAsMemory;
	const unsigned begin;
	const unsigned end;
	const unsigned itemSize;
	DiskAsMemoryReader* const theIO;
	~DiskBufferReader();
private:	
	char* buffer;
	unsigned usage;
};
