#pragma once

#include <string>
#include <vector>


using std::string;
class Document
{
public:

	vector<int> words;
	string text;

	Document();
	~Document();

private:

};

Document::Document()
{
}

Document::~Document()
{
}