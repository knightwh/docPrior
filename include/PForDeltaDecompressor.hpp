#pragma once
#include <vector>

#ifndef PostingPerSet
#define PostingPerSet (64)
#endif

using namespace std;

class PForDeltaDecompressor
{
public:
	//unsigned content[PostingPerSet];
	unsigned *content;

	PForDeltaDecompressor();
	void decompressor(unsigned ap,unsigned bp,unsigned np,unsigned l,unsigned previous,unsigned *s,unsigned *con);
private:
	unsigned num,pos;
	unsigned *start;
	unsigned length;

	typedef void(PForDeltaDecompressor::*step1Funs)();
	typedef void(PForDeltaDecompressor::*step2Funs)(unsigned);

	//void ((PForDeltaDecompressor::*)step1[17])();//uncompress the base part
	step1Funs step1[17];
	//void (*step2[5])(unsigned b);//uncompress the exception part
	step2Funs step2[5];
	void step3Delta(unsigned previous);//convert delta code to normal delta
	void step3NonDelta();

	void step1B1();
	void step1B2();
	void step1B3();
	void step1B4();
	void step1B5();
	void step1B6();
	void step1B8();
	void step1B10();
	void step1B16();
	void step1Ex();

	void step2A1(unsigned b);
	void step2A2(unsigned b);
	void step2A3(unsigned b);
	void step2A4(unsigned b);
};

PForDeltaDecompressor::PForDeltaDecompressor()
{
	step1[0]=&PForDeltaDecompressor::step1Ex;
	step1[1]=&PForDeltaDecompressor::step1B1;
	step1[2]=&PForDeltaDecompressor::step1B2;
	step1[3]=&PForDeltaDecompressor::step1B3;
	step1[4]=&PForDeltaDecompressor::step1B4;
	step1[5]=&PForDeltaDecompressor::step1B5;
	step1[6]=&PForDeltaDecompressor::step1B6;
	step1[7]=&PForDeltaDecompressor::step1Ex;
	step1[8]=&PForDeltaDecompressor::step1B8;
	step1[9]=&PForDeltaDecompressor::step1Ex;
	step1[10]=&PForDeltaDecompressor::step1B10;
	step1[11]=&PForDeltaDecompressor::step1Ex;
	step1[12]=&PForDeltaDecompressor::step1Ex;
	step1[13]=&PForDeltaDecompressor::step1Ex;
	step1[14]=&PForDeltaDecompressor::step1Ex;
	step1[15]=&PForDeltaDecompressor::step1Ex;
	step1[16]=&PForDeltaDecompressor::step1B16;
	step2[1]=&PForDeltaDecompressor::step2A1;
	step2[2]=&PForDeltaDecompressor::step2A2;
	step2[3]=&PForDeltaDecompressor::step2A3;
	step2[4]=&PForDeltaDecompressor::step2A4;
}

void PForDeltaDecompressor::decompressor(unsigned a,unsigned b,unsigned n,unsigned l,unsigned previous,unsigned *s,unsigned *con)
{
	num=n;
	length=l;
	start=s;
	pos=0;
	content = con;
	(this->*step1[b])();
	(this->*step2[a])(b);
	if(previous != (~0)) step3Delta(previous);
	else step3NonDelta();
}

void PForDeltaDecompressor::step1B1()
{
	unsigned l=(num+31)/32;
	unsigned i,block;
	unsigned *con=content;
	for(i=0;i<l;i++)
	{
		block=start[pos++];
		con[0] = block & 1;
		con[1] = (block>>1) & 1;
		con[2] = (block>>2) & 1;
		con[3] = (block>>3) & 1;
		con[4] = (block>>4) & 1;
		con[5] = (block>>5) & 1;
		con[6] = (block>>6) & 1;
		con[7] = (block>>7) & 1;
		con[8] = (block>>8) & 1;
                con[9] = (block>>9) & 1;
                con[10] = (block>>10) & 1;
                con[11] = (block>>11) & 1;
                con[12] = (block>>12) & 1;
                con[13] = (block>>13) & 1;
                con[14] = (block>>14) & 1;
                con[15] = (block>>15) & 1;
		con[16] = (block>>16) & 1;
                con[17] = (block>>17) & 1;
                con[18] = (block>>18) & 1;
                con[19] = (block>>19) & 1;
                con[20] = (block>>20) & 1;
                con[21] = (block>>21) & 1;
                con[22] = (block>>22) & 1;
                con[23] = (block>>23) & 1;
		con[24] = (block>>24) & 1;
                con[25] = (block>>25) & 1;
                con[26] = (block>>26) & 1;
                con[27] = (block>>27) & 1;
                con[28] = (block>>28) & 1;
                con[29] = (block>>29) & 1;
                con[30] = (block>>30) & 1;
                con[31] = (block>>31) & 1;
		con+=32;
	}
}

void PForDeltaDecompressor::step1B2()
{
	unsigned l=(num+15)/16;
	unsigned i,block;
	unsigned *con=content;
	for(i=0;i<l;i++)
	{
		block=start[pos++];
		con[0] = block & 3;
		con[1] = (block>>2) & 3;
		con[2] = (block>>4) & 3;
		con[3] = (block>>6) & 3;
		con[4] = (block>>8) & 3;
		con[5] = (block>>10) & 3;
		con[6] = (block>>12) & 3;
		con[7] = (block>>14) & 3;
		con[8] = (block>>16) & 3;
                con[9] = (block>>18) & 3;
                con[10] = (block>>20) & 3;
                con[11] = (block>>22) & 3;
                con[12] = (block>>24) & 3;
                con[13] = (block>>26) & 3;
                con[14] = (block>>28) & 3;
                con[15] = (block>>30) & 3;
		con+=16;
	}
}

void PForDeltaDecompressor::step1B3()
{
	unsigned l=(num+9)/10;
	unsigned i,block;
	unsigned *con=content;
	for(i=0;i<l;i++)
	{
		block=start[pos++];
		con[0] = block & 7;
		con[1] = (block>>3) & 7;
		con[2] = (block>>6) & 7;
		con[3] = (block>>9) & 7;
		con[4] = (block>>12) & 7;
		con[5] = (block>>15) & 7;
		con[6] = (block>>18) & 7;
		con[7] = (block>>21) & 7;
		con[8] = (block>>24) & 7;
		con[9] = (block>>27) & 7;
		con+=10;
	}
}

void PForDeltaDecompressor::step1B4()
{
	unsigned l=(num+7)/8;
	unsigned i,block;
	unsigned *con=content;
	for(i=0;i<l;i++)
	{
		block=start[pos++];
		con[0] = block & 15;
		con[1] = (block>>4) & 15;
		con[2] = (block>>8) & 15;
		con[3] = (block>>12) & 15;
		con[4] = (block>>16) & 15;
		con[5] = (block>>20) & 15;
		con[6] = (block>>24) & 15;
		con[7] = (block>>28) & 15;
		con+=8;
	}
}

void PForDeltaDecompressor::step1B5()
{
	unsigned l=(num+5)/6;
	unsigned i,block;
	unsigned *con=content;
	for(i=0;i<l;i++)
	{
		block=start[pos++];
		con[0] = block & 31;
		con[1] = (block>>5) & 31;
		con[2] = (block>>10) & 31;
		con[3] = (block>>15) & 31;
		con[4] = (block>>20) & 31;
		con[5] = (block>>25) & 31;
		con+=6;
	}
}

void PForDeltaDecompressor::step1B6()
{
	unsigned l=(num+4)/5;
	unsigned i,block;
	unsigned *con=content;
	for(i=0;i<l;i++)
	{
		block=start[pos++];
		con[0] = block & 63;
		con[1] = (block>>6) & 63;
		con[2] = (block>>12) & 63;
		con[3] = (block>>18) & 63;
		con[4] = (block>>24) & 63;
		con+=5;
	}
}

void PForDeltaDecompressor::step1B8()
{
	unsigned l=(num+3)/4;
	unsigned i,block;
	unsigned *con=content;
	for(i=0;i<l;i++)
	{
		block=start[pos++];
		con[0] = block & 255;
		con[1] = (block>>8) & 255;
		con[2] = (block>>16) & 255;
		con[3] = (block>>24) & 255;
		con+=4;
	}
}

void PForDeltaDecompressor::step1B10()
{
	unsigned l=(num+2)/3;
	unsigned i,block;
	unsigned *con=content;
	for(i=0;i<l;i++)
	{
		block=start[pos++];
		con[0] = block & 1023;
		con[1] = (block>>10) & 1023;
		con[2] = (block>>20) & 1023;
		con+=3;
	}
}

void PForDeltaDecompressor::step1B16()
{
	unsigned l=(num+1)/2;
	unsigned i,block;
	unsigned *con=content;
	for(i=0;i<l;i++)
	{
		block=start[pos++];
		con[0] = block & ((1<<16)-1);
		con[1] = block>>16;
		con+=2;
	}
}
	
void PForDeltaDecompressor::step1Ex()
{
	cerr<<"Invalid b value"<<endl;
}

void PForDeltaDecompressor::step2A1(unsigned b)	
{
	unsigned block;
	for(;pos<length;pos++)
	{
		block=start[pos];
		content[block & 255]+=((block>>8) & 255)<<b;
		content[(block>>16) & 255]+=((block>>24))<<b;
	}
}

void PForDeltaDecompressor::step2A2(unsigned b)
{
	unsigned block1,block2;
	while(true)
	{
		if(pos>=length) break;
		block1 = start[pos++];
		content[block1 & 255]+=((block1>>8) & 65535)<<b;
		if(pos>=length) break;
		block2 = start[pos++];
		content[block1>>24]+=(block2 & 65535)<<b;
		if(pos>=length) break;
		block1 = start[pos++];
		content[(block2>>16) & 255]+=((block2>>24) + ((block1 & 255)<<8))<<b;
		content[(block1>>8) & 255]+=(block1>>16)<<b;
	}
}

void PForDeltaDecompressor::step2A3(unsigned b)
{
	unsigned block;
	for(;pos<length;pos++)
	{
		block=start[pos];
		content[block & 255]+=(block>>8)<<b;
	}
}

void PForDeltaDecompressor::step2A4(unsigned b)
{
	unsigned block1,block2;
	while(true)
	{
		if(pos+1>=length) break;
		block1 = start[pos++];
		block2 = start[pos++];
		content[block1 & 255]+=( (block1>>8) + ((block2 & 255)<<24) )<<b;
		if(pos>=length) break;
		block1 = start[pos++];
		content[(block2>>8) & 255]+=( (block2>>16) + ((block1 && 65535)<<16) )<<b;
		if(pos>=length) break;
		block2 = start[pos++];
		content[(block1>>16) & 255]+=( (block1>>24) + ((block2 && 16777215)<<8) )<<b;
		if(pos>=length) break;
		block1 = start[pos++];
		content[block2>>24]+=block1;
	}
}

void PForDeltaDecompressor::step3Delta(unsigned pre)
{
	unsigned i;
	for(i=0;i<PostingPerSet;i++)
	{
		content[i]+=pre+1;
		pre = content[i];
	}
}

void PForDeltaDecompressor::step3NonDelta()
{
	unsigned i;
	for(i=0;i<PostingPerSet;i++)
	{
		content[i]++;
	}
}
