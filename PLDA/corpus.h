#pragma once
#include "document.h"
#include <string>
#include <vector>
#include <map>

using std::vector;
using std::string;
using std::map;
class Corpus
{
public:
	
	vector<Document> documents;
	map<string, int> word2IndexMap;
	map<int, string> index2WordMap;

	
	Corpus();
	~Corpus();


private:

};

Corpus::Corpus()
{
}

Corpus::~Corpus()
{
}