#pragma once
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string>

#define PossibleCharNum 256

using namespace std;

class PrefixTreeReader
{
public:
	PrefixTreeReader(char *filename);
	unsigned lookupTerm(char *term);
	unsigned lookupTerm(string s);
	~PrefixTreeReader();

private:
	int fileHandle;
	int pageSize;

	unsigned headCharNum;
	char headChar[PossibleCharNum][16];
	unsigned headPoint[PossibleCharNum];
	unsigned headCharLength[PossibleCharNum];

	unsigned* buffer,*buf;
	unsigned bufferStart,bufferEnd,bufferSize;

	void locateBuffer(unsigned n);
	unsigned lookupRest(char *t,unsigned p);
	inline bool compareString(char *t1,char *t2,unsigned l);
};

PrefixTreeReader::PrefixTreeReader(char *filename)
{
	fileHandle = open(filename,O_RDONLY);
	if(fileHandle==-1) {cerr<<"Could not read the file "<<filename<<endl;return;}
	pageSize = sysconf(_SC_PAGESIZE)/4;
	//cout<<"pageSize="<<pageSize<<endl;
	bufferStart=0;
	bufferSize=0;bufferEnd=0;
	locateBuffer(0);
	unsigned size = buf[1] & 255;
	char* charBuf = (char*)(buf + 2 + (size+7)/8); 
	unsigned* posBuf = buf + (buf[1] >> 8);
	unsigned i,l;
	for(i=0;i*8<size;i++)
	{
		for(l=0;l<8 && i*8+l<size;l++)
		{
			unsigned length = (buf[2+i] >> (l*4)) & 15;
			memmove(headChar[i*8+l],charBuf,length);
			headChar[i*8+l][length]='\0';
			headCharLength[i*8+l] = length;
			charBuf+=length;
			headPoint[i*8+l] = *(posBuf++);
		}
	}
	headCharNum = size;
}

void PrefixTreeReader::locateBuffer(unsigned n)
{
	if(bufferStart > n || bufferEnd <= n+1) //output of range
	{
		if(bufferSize!=0) munmap((char*)buffer,bufferSize*4);
		bufferStart = n/pageSize*pageSize;
		if(bufferStart+pageSize-n<10) bufferSize = pageSize*2;//allocate bigger space
		else bufferSize = pageSize;
		bufferEnd=bufferStart+bufferSize;
		buffer = (unsigned *)mmap(0,bufferSize*4,PROT_READ,MAP_SHARED,fileHandle,bufferStart*4);
		if(buffer == MAP_FAILED) { cerr<<"map failed at "<<endl;return;}
	}
	buf = buffer + (n-bufferStart);
	while(true)
	{
		unsigned length = (buf[1] & 255) + (buf[1] >> 8);
		if(n + length > bufferEnd) // output of range
		{
			munmap((char*)buffer,bufferSize*4);
			bufferSize*=2;
			bufferEnd=bufferStart+bufferSize;
			buffer = (unsigned *)mmap(0,bufferSize*4,PROT_READ,MAP_SHARED,fileHandle,bufferStart*4);
			buf = buffer + (n-bufferStart);
		}
		else return;
	}
}

unsigned PrefixTreeReader::lookupTerm(char* term)
{
	if(strlen(term)>255) term[255] = '\0';
	if(*term=='\0') return 0;
	unsigned i;
	for(i=0;i<headCharNum;i++)
	{
		if(compareString(term,headChar[i],headCharLength[i]))
		{
			return lookupRest(term+1,headPoint[i]);
		}
	}
	return 0;
}

unsigned PrefixTreeReader::lookupTerm(string s)
{
	char term[256];
	if(s.length()>255) s=s.substr(0,255);
	strcpy(term,s.c_str());
	return lookupTerm(term);
}

inline bool PrefixTreeReader::compareString(char* t1,char* t2,unsigned l)
{
	while((l--)>0)
	{
		if(*(t1++) != *(t2++)) return false;
	}
	return true;
}

unsigned PrefixTreeReader::lookupRest(char *term,unsigned pos)
{
	locateBuffer(pos);
	if(*term == '\0') return buf[0];
	unsigned size = buf[1] & 255;
	char *charBuf = (char*)(buf + 2 + (size+7)/8);
	unsigned* posBuf = buf + (buf[1] >> 8);
	unsigned i,l;
	for(i=0;i*8<size;i++)
	{
		for(l=0;l<8 && i*8+l<size;l++)
		{
			unsigned length = (buf[2+i] >> (l*4)) & 15;
			if(compareString(term,charBuf,length)) //matched
			{
				return lookupRest(term+length,posBuf[i*8+l]);
			}
			charBuf+=length;
		}
	}
	return 0;
}

PrefixTreeReader::~PrefixTreeReader()
{
	if(bufferSize!=0) munmap((char*)buffer,bufferSize*4);
	close(fileHandle);
}
