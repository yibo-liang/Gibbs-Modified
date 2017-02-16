#include "corpus.h"


Corpus::Corpus()
{
}

Corpus::~Corpus()
{
}

void Corpus::fromTextFile(string filename, int docn, int textIdxStart, map<int, string> otherAttrsIdx)
{
	using namespace std;
	std::ifstream file(filename);
	std::string str;


	while (std::getline(file, str))
	{
		if (str.length() < 1) {
			continue;
		}
		if (documents.size() >= docn) break;
		
		istringstream iss(str);
		vector<string> tokens{ istream_iterator<string>{iss},
			istream_iterator<string>{}};
		

		//new Document
		Document doc;

		//read text, and store new words to word map
		for (int i = textIdxStart; i < tokens.size(); i++) {
			string word = tokens[i];
			auto it = wordToIndex.find(word);
			if (it == wordToIndex.end()) {
				int new_index = wordToIndex.size();
				//add word index to document
				doc.words.push_back(new_index);
				//add to word->index map
				wordToIndex[word] = new_index;
				//add to index->word map
				indexToWord[new_index] = word;
			}
			else {
				doc.words.push_back(it->second);
			}
		}		
		for (auto const &it : otherAttrsIdx) {
			doc.info[tokens[it.first]] = it.second;
		}
		if (doc.wordCount() <= 0) continue;
		this->documents.push_back(doc);
		this->totalWordCount += doc.wordCount();
		//read other usefull information
	}
}