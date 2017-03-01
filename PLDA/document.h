#pragma once


#ifndef DOCUMENT
#define DOCUMENT
#include "shared_header.h"

using std::string;
class Document
{
public:

	vector<int> words;

	map<string, string> info;

	Document(const Document& doc) {
		this->words = doc.words;
		this->info = doc.info;
	}
	Document();
	~Document();

	int wordCount();

private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & BOOST_SERIALIZATION_NVP(words);
		ar & BOOST_SERIALIZATION_NVP(info);
	}
};

#endif // !DOCUMENT
