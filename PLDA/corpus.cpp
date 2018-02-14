#include "corpus.h"


using namespace ParallelHLDA;

void ParallelHLDA::Corpus::saveToFile(string filename)
{
	std::ofstream file;
	//can't enable exception now because of gcc bug that raises ios_base::failure with useless message
	//file.exceptions(file.exceptions() | std::ios::failbit);
	file.open(filename, std::ios::out);
	if (file.fail())
		throw std::ios_base::failure(std::strerror(errno));

	for (auto it = this->wordToIndex.begin(); it != this->wordToIndex.end(); it++) {
		file << it->first << " " << it->second << "\n";
	}
}

Corpus::Corpus()
{
}

Corpus::~Corpus()
{
}

void Corpus::inferencingTextFile(string filename, int docn, int textIdxStart, map<int, string> otherAttrsIdx)
{
	using namespace std;
	std::ifstream file(filename);
	std::string str;

	cout << "Open file: " << filename << endl;
	while (std::getline(file, str))
	{
		if (str.length() < 1) {
			continue;
		}
		if (docn>0 && inferDocuments.size() >= docn) break;

		istringstream iss(str);
		vector<string> tokens{ istream_iterator<string>{iss},
			istream_iterator<string>{} };


		//new Document
		Document doc;
		//read text, and store new words to word map
		for (int i = textIdxStart; i < tokens.size(); i++) {
			string word = tokens[i];
			auto it = wordToIndex.find(word);
			if (it == wordToIndex.end()) {
				//Do nothing, unknown new word is not infered 
				//TODO: deal with new words
			}
			else {
				doc.words.push_back(it->second);
			}
		}
		for (auto const &it : otherAttrsIdx) {
			doc.info[tokens[it.first]] = it.second;
		}
		if (doc.wordCount() <= 0) continue;
		this->inferDocuments.push_back(doc);
		this->inferTotalWordCount += doc.wordCount();
		//read other usefull information
	}
}

void Corpus::fromTextFile(string filename, int docn, int textIdxStart, map<int, string> otherAttrsIdx)
{
	using namespace std;
	std::ifstream file(filename);
	std::string str;

	cout << "Open file: " << filename << endl;
	while (std::getline(file, str))
	{
		if (str.length() < 1) {
			continue;
		}
		if (docn>0 && documents.size() >= docn) break;
		
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
			doc.info[it.second] = tokens[it.first];
		}
		if (doc.wordCount() <= 0) continue;
		this->documents.push_back(doc);
		this->totalWordCount += doc.wordCount();
		//read other usefull information
	}
}