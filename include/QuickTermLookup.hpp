#pragma once
#include <string>
#include <stdio.h>
#include <string.h>
#include <iostream>

using namespace std;

typedef struct
{
	unsigned validA;
	unsigned validB;
	unsigned value;
} MPQHashTable;

class QuickTermLookup
{
public:	
	QuickTermLookup();
	QuickTermLookup(unsigned s);	
	void initial(unsigned s);
	unsigned termLookup(char* term);
	unsigned termLookup(string s);
	bool insertTerm(char* term,unsigned v);
	bool insertTerm(string s,unsigned v);
	void readFromFile(char* filename);
	void writeToFile(char* filename);
	void clear();
	~QuickTermLookup();
private:
	unsigned size;
	unsigned cryptTable[0x500];
	MPQHashTable *table;
	
	unsigned hashString(char *t,unsigned offset);
	void prepareCryptTable();
};

QuickTermLookup::QuickTermLookup()
{
	table = NULL;
	size = 0;
	prepareCryptTable();
}

QuickTermLookup::QuickTermLookup(unsigned s)
{
	table = NULL;
	prepareCryptTable();
	initial(s);
}

void QuickTermLookup::initial(unsigned s)
{
	size = s;
	if(table!=NULL) delete[] table;
	table = new MPQHashTable[s];
	clear();
}

void QuickTermLookup::prepareCryptTable()
{
	unsigned seed = 0x00100001;
	unsigned i,l;
	for(i = 0;i < 0x100; i++)
	{
		for(l=i;l<i+0x500;l+=0x100)
		{
			unsigned temp1,temp2;
			seed = (seed * 125 + 3) % 0x2AAAAB;
			temp1 = (seed & 0xFFFF) << 0x10;
			seed = (seed * 125 + 3) % 0x2AAAAB;
			temp2 = (seed & 0xFFFF);
			cryptTable[l] = (temp1 | temp2);
		}
	}
}

unsigned QuickTermLookup::hashString(char* t,unsigned offset)
{
	unsigned a = 0x7FED7FED, b = 0xEEEEEEEE;
	unsigned char* k = (unsigned char*)t;
	unsigned ch;

	while(*k != 0)
	{
		ch = *k++;
		a = cryptTable[(offset << 8) + ch] ^ (a+b);
		b = ch + a + b + (b<<5) + 3;
	}
	return a;
}

void QuickTermLookup::clear()
{
	unsigned i;
	for(i=0;i<size;i++) 
	{
		table[i].value = 0;
		table[i].validA = 0;
		table[i].validB = 0;
	}
}

bool QuickTermLookup::insertTerm(char *term,unsigned v)
{
	if(strlen(term)>255) term[255]='\0';
	unsigned h0 = hashString(term,0);
	unsigned h1 = hashString(term,1);
	unsigned h2 = hashString(term,2);
	unsigned start = h0 % size;
	unsigned pos = start;

	while( table[pos].value != 0)
	{
		pos = (pos + 1) % size;
		if(pos == start) return false;
	}
	table[pos].value = v;
	table[pos].validA = h1;
	table[pos].validB = h2;
	return true;
}

bool QuickTermLookup::insertTerm(string s,unsigned v)
{
	if(s.length()>255) s=s.substr(0,255);
	char term[256];
	strcpy(term,s.c_str());
	return insertTerm(term,v);
}

unsigned QuickTermLookup::termLookup(char *term)
{
	if(strlen(term)>255) term[255]='\0';
	unsigned h0 = hashString(term,0);
	unsigned h1 = hashString(term,1);
	unsigned h2 = hashString(term,2);
	unsigned start = h0 % size;
	unsigned pos = start;

	while( table[pos].value != 0)
	{
		if(h1 == table[pos].validA && h2 == table[pos].validB) return table[pos].value;
		else pos = (pos+1)%size;
		if( pos == start) return 0;
	}
	return 0;
}

unsigned QuickTermLookup::termLookup(string s)
{
	if(s.length()>255) s=s.substr(0,255);
	char term[256];
	strcpy(term,s.c_str());
	return termLookup(term);
}

void QuickTermLookup::writeToFile(char *filename)
{
	FILE *fileHandle = fopen(filename,"wb");
	if(!fileHandle) {cerr<<"could not write to the file "<<filename<<endl;return;}
	fwrite(&size,4,1,fileHandle);
	fwrite(table,sizeof(MPQHashTable),size,fileHandle);
	fclose(fileHandle);
}

void QuickTermLookup::readFromFile(char *filename)
{
	FILE *fileHandle = fopen(filename,"rb");
	if(!fileHandle) {cerr<<"could not read the file "<<filename<<endl;return;}
	unsigned size;
	fread(&size,4,1,fileHandle);
	initial(size);
	fread(table,sizeof(MPQHashTable),size,fileHandle);
	fclose(fileHandle);
}

QuickTermLookup::~QuickTermLookup()
{
	if(table!=NULL) delete[] table;
}

