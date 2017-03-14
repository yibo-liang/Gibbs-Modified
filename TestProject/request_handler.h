#pragma once
#ifndef REQUEST_HANDLER
#define REQUEST_HANDLER

#include "corpus.h"
#include "model.h"

#include "boost\algorithm\string.hpp"
#include "json.hpp"
using namespace ParallelHLDA;
using json = nlohmann::json;
class RequestHandler
{



public:
	std::shared_ptr< Model*> model;
	std::shared_ptr<Corpus*> corpus;

	RequestHandler(std::shared_ptr<Model*> model, std::shared_ptr<Corpus*> corpus);
	~RequestHandler();

	static int count;
	string respond(string url);

private:
	void recursiveLookupWords(vector<int> words, json & result, const Model & model) {

		vector<int> topic_word_count(model.K, 0);
		for (int & w : words) {
			for (int k = 0; k < model.K; k++) {
				topic_word_count[k] += model.nw[w][k];
			}
		}
		for (int k = 0; k < model.K; k++) {
			result["topic_word_sum"][std::to_string(k)] = topic_word_count[k];
		}
		json submodel_data;
		if (model.submodels.size()>0)
			for (int k = 0; k < model.K; k++) {
				recursiveLookupWords(words, submodel_data[std::to_string(k)], model.submodels[k]);
			}
		result["submodels"] = submodel_data;
	}

	string handleQueryWords(vector<int> words) {
		json result;
		recursiveLookupWords(words, result, *(*model));
		return result.dump();
	}

	string handleSearchWord(string word) {
		//if word is less than 3 char, return empty
		json result;
		if (word.size() < 3) {
			result = { "result", "null" };
		}
		else {
			result["result"];
			map<string, int> & wordIndex = (*corpus)->wordToIndex;
			for (auto it = wordIndex.begin(); it != wordIndex.end(); it++) {
				if (it->first.find(word) != std::string::npos) {
					result["result"].push_back(it->first);
				}
			}


		}
		return result.dump();
	}

	string attachDefaultHeader(string content) {
		std::stringstream ssOut;
		ssOut << "HTTP/1.1 200 OK" << std::endl;
		ssOut << "content-type: text/html" << std::endl;
		ssOut << "content-length: " << content.length() << std::endl;
		ssOut << "Access-Control-Allow-Origin: *" << endl;
		ssOut << std::endl;
		ssOut << content;
		return ssOut.str();
	}

};

std::string toLower(std::string& str)
{
	std::locale settings;
	std::string converted;

	for (short i = 0; i < str.size(); ++i)
		converted += (std::tolower(str[i], settings));

	return converted;
}
string RequestHandler::respond(string url) {
	url = toLower(url);
	boost::replace_all(url, "\\", "/");
	vector<string> segments;
	boost::split(segments, url, boost::is_any_of("/"));

	string result;
	if (segments.size() > 2)
		if (segments.at(1) == "search") {
			string word = segments.at(2);
			return attachDefaultHeader(handleSearchWord(word));

		}
		else if (segments.at(1) == "topic_word_count") {
			vector<string> queryWords;
			boost::split(queryWords, segments.at(2), boost::is_any_of("+"));
			vector<int> wordIdx;
			for (string & w : queryWords) {
				if ((*corpus)->wordToIndex.find(w) != (*corpus)->wordToIndex.end()) {
					wordIdx.push_back((*corpus)->wordToIndex[w]);
				}
			}
			return attachDefaultHeader(handleQueryWords(wordIdx));
		}

		return attachDefaultHeader("");
}

int RequestHandler::count = 0;

RequestHandler::RequestHandler(std::shared_ptr<Model*> model, std::shared_ptr<Corpus*> corpus)
{
	this->model = model;
	this->corpus = corpus;
}

RequestHandler::~RequestHandler()
{
}

#endif // !REQUEST_HANDLER
