
#include "plda_all.h"
using namespace ParallelHLDA;
int main()
{
	Corpus corpus;
	map<int, string> attrs;
	attrs[0] = "id";
	attrs[1] = "class";
	attrs[2] = "value";
	corpus.fromTextFile("C:/Users/Devid/OneDrive/corpus/all.txt", 0, 3, attrs);

	return 0;
}