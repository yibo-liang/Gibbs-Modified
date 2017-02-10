#pragma once

#ifndef CORPUS
#define CORPUS

#include "shared_header.h"
#include "document.h"

class Corpus
{
public:
	
	vector<Document> documents;
	map<string, int> word2IndexMap;
	map<int, string> index2WordMap;

	int totalWordCount = 0;

	void fromTextFile(string filename, int textIdxStart, map<int, string> otherAttrsIdx);
	void fromJSONFile(string filename, string textKey, vector<string> otherAttrs);
	void fronCSVFILE(string filename, string textKey, vector<string> otherAttrs);//TODO
	
	Corpus();
	~Corpus();


private:

};

#endif