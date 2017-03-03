#pragma once

#ifndef CORPUS
#define CORPUS

#include "shared_header.h"
#include "document.h"

class Corpus
{
public:
	
	vector<Document> inferDocuments;
	vector<Document> documents;
	hashmap<string, int> wordToIndex;
	hashmap<int, string> indexToWord;

	int totalWordCount = 0;
	int inferTotalWordCount = 0;

	void inferencingTextFile(string filename, int docn, int textIdxStart, map<int, string> otherAttrsIdx);
	
	void fromTextFile(string filename, int docn, int textIdxStart, map<int, string> otherAttrsIdx);

	void fromJSONFile(string filename, int docn, string textKey, vector<string> otherAttrs);
	void fronCSVFILE(string filename, int docn, string textKey, vector<string> otherAttrs);//TODO
	
	Corpus();
	~Corpus();


private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & documents;
		ar & wordToIndex;
		ar & indexToWord;
		ar & totalWordCount;
		
	}
};

#endif