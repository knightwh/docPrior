#pragma once
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <stdio.h>

#include "DiskItem.hpp"

using namespace std;

class DiskAsMemory;

class DiskBuffer
{
public:
	DiskBuffer(DiskAsMemory *theIO,unsigned begin,unsigned end,int fileHandle,int offset);
	DiskItem* localItem(unsigned itemNum);
	inline unsigned getUsage() {return usage;}
	void freeUsage();
	//friend class DiskAsMemory;
	const unsigned begin;
	const unsigned end;
	const unsigned itemSize;
	DiskAsMemory* const theIO;
	~DiskBuffer();
private:	
	char* buffer;
	unsigned usage;
};
