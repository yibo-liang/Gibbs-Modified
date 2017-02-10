#pragma once


#ifndef DOCUMENT
#define DOCUMENT
#include "shared_header.h"

using std::string;
class Document
{
public:

	vector<int> words;
	string text;

	map<string, string> info;

	Document(const Document& doc) {
		this->words = doc.words;
		this->text = doc.text;
		this->info = doc.info;
	}
	Document();
	~Document();

	int wordCount();
	int textLength();

private:

};

#endif // !DOCUMENT
