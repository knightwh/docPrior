#include "BlockInfoBuilder.hpp"
#include <iostream>

using namespace std;

int main(int argc,char** argv)
{
	if(argc!=3) {cerr<<"Usage: "<<argv[0]<<" indexPath theName"<<endl;return 0;}
	BlockInfoBuilder *BIB = new BlockInfoBuilder(argv[1],argv[2]);
	BIB -> buildBlockInfo();
	delete BIB;
	return 0;
}
