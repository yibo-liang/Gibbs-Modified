#pragma once

#ifndef CORPUS
#define CORPUS

#include "shared_header.h"
#include "document.h"

class Corpus
{
public:
	
	vector<Document> documents;
	map<string, int> wordToIndex;
	map<int, string> indexToWord;

	int totalWordCount = 0;

	void fromTextFile(string filename, int docn, int textIdxStart, map<int, string> otherAttrsIdx);
	void fromJSONFile(string filename, int docn, string textKey, vector<string> otherAttrs);
	void fronCSVFILE(string filename, int docn, string textKey, vector<string> otherAttrs);//TODO
	
	Corpus();
	~Corpus();


private:

};

#endif