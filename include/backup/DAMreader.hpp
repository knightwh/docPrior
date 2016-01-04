#pragma once
#include <iostream>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <stdio.h>
#include <vector>

#include "DiskItemReader.hpp"
#include "DiskBufferReader.hpp"

using namespace std;

#ifndef MaxFileSize
#define MaxFileSize (1024*1024*1024)
#endif

class DiskAsMemoryReader
{
public:
	DiskAsMemoryReader(char *foldname);
	DiskItemReader* localItem(unsigned itemNum);
	unsigned createHeadBuffer(unsigned headBufferSize);
	inline unsigned getItemCounter() const {return *itemCounter;}
	inline unsigned getItemSize() const {return itemSize;}
	inline unsigned getBufferSize() const {return bufferSize;}
	inline unsigned getHeadBufferSize();
	bool freeBuffer(DiskBufferReader* DB);
	~DiskAsMemoryReader();
private:
	char foldPath[256];
	int fileHandle[256];
	int fileHandleSummary;
	
	unsigned itemSize;
	unsigned bufferPerFile;
	unsigned bufferSpace;
	unsigned bufferSize;
	DiskBufferReader* specialHead;
	vector<DiskBufferReader*> bodys;
	unsigned tailUsage;

	char* metaBuffer;
	unsigned *itemCounter;
	int lastFile;
	unsigned *allocatedSpace;

	bool validPara();
};


	
