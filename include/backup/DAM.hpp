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

#include "DiskItem.hpp"
#include "DiskBuffer.hpp"

using namespace std;

#ifndef MaxFileSize
#define MaxFileSize (1024*1024*1024)
#endif

class DiskAsMemory
{
public:
	DiskAsMemory(char *foldname);
	DiskAsMemory(char *foldname,unsigned itemSize,unsigned bufferSize);
	DiskItem* addNewItem();
	DiskItem* localItem(unsigned itemNum);
	void deleteItem(unsigned itemNum);
	unsigned createHeadBuffer(unsigned headBufferSize);
	inline unsigned getItemCounter() const {return *itemCounter;}
	inline unsigned getItemSize() const {return itemSize;}
	inline unsigned getBufferSize() const {return bufferSize;}
	inline unsigned getHeadBufferSize();
	bool truncateItem(unsigned counter);
	bool freeBuffer(DiskBuffer* DB);
	bool copyItem(unsigned destination,unsigned source);
	bool switchItem(unsigned A,unsigned B);
	~DiskAsMemory();
private:
	char foldPath[256];
	int fileHandle[256];
	int fileHandleSummary;
	
	unsigned itemSize;
	unsigned bufferPerFile;
	unsigned bufferSpace;
	unsigned bufferSize;
	DiskBuffer* specialHead;
	vector<DiskBuffer*> bodys;
	unsigned tailUsage;

	char* metaBuffer;
	unsigned *itemCounter;
	int lastFile;
	unsigned *allocatedSpace;

	void expandSize();
	void localTail();
	bool validPara();
};


	
