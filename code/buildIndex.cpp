#include "IndexBuilder.hpp"
#include <iostream>

using namespace std;

int main(int argc,char** argv)
{
	if(argc!=3) {cerr<<"Usage: "<<argv[0]<<" indriIndex indexPath"<<endl;return 0;}
	IndexBuilder *IB = new IndexBuilder(argv[1],argv[2]);
	IB -> buildIndex();
	delete(IB);
}
