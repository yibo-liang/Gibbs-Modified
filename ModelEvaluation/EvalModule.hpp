#pragma once
#ifndef EVAL_MOD
#define EVAL_MOD
#include "model.h"
#include <map>

using namespace ParallelHLDA;
using namespace std;
class EvalModule
{
public:

	Model & model;

	map<string, double> eval(int topic_n, int top_word_n) {
		map<string, double> result;
		vector<pair<int, int>> top_words = getTopicWords(topic_n, top_word_n);
		result["TC-PMI"] = TC_PMI(top_words);
		result["TC-LCP"] = TC_LCP(top_words);
		result["TC-NZ"] = TC_NZ(top_words);
		return result;
	}

	vector<map<string, double>> evalAll(int top_word_n) {
		vector<map<string, double>>  result;
		for (int k = 0; k < model.K; k++) {
			auto t = eval(k, top_word_n);
			result.push_back(t);
		}
		return result;
	}

	EvalModule(Model & model) : model(model) {
	};
	~EvalModule() {};

private:

	/*
		nw_k =  col k in nw matrix, vector of each word in topic k
		nd_k = col k in nd matrix, vecotr of number of word in each document in topic k

	*/

	vector<pair<int, int>> getTopicWords(int topic_n, int top_word_n) {
		vector<pair<int, int>> topic_w(model.V);

		for (int v = 0; v < model.V; v++) {
			topic_w[v].first = v;
			topic_w[v].second = model.nw[v][topic_n];
		}
		sort(topic_w.begin(), topic_w.end(),
			[](const pair<int, int> & a, const pair<int, int> & b) {return a.second > b.second; });

		vector<pair<int, int>> result(topic_w.begin(), topic_w.begin() + top_word_n);
		return result;
	}

	//D(w), the sum number of document with at least one w word occurence
	double D(int w) {
		int sum = 0;
		for (int m = 0; m < model.M; m++) {
			for (int i = 0; i < model.w[m].size(); i++) {
				if (w == model.w.at(m).at(i)) {
					sum += 1;
					break;
				}
			}
		}
		return  (double)sum;
	}

	//D(w2 , w2), the sum number of document with co-occurence of w1 and w2 
	double D(int w1, int w2) {
		int sum = 0;
		for (int m = 0; m < model.M; m++) {
			bool has_w1 = false;
			bool has_w2 = false;
			for (int i = 0; i < model.w[m].size(); i++) {
				if (w1 == model.w.at(m).at(i)) {
					has_w1 = true;
				}
				if (w2 = model.w.at(m).at(i)) {
					has_w2 = true;
				}
				if (has_w1 && has_w2) {
					sum += 1;
					break;
				}
			}
		}
		return (double)sum;
	}

	//number of times w1, w2  appear in a sliding window of each document
	int N(int w1, int w2, int windowSize) {
		int sum = 0;
		for (int m = 0; m < model.M; m++) {
			int slidingStart = 0;
			int slidingEnd = model.w.at(m).size() - windowSize - 1;
			slidingEnd = std::max(slidingEnd, 0);

			bool has_both = false;

			for (int s = slidingStart; s <= slidingEnd; s++) {
				bool has_w1 = false;
				bool has_w2 = false;
				for (int i = s; i < s + windowSize && i < model.w.at(m).size(); i++) {
					if (model.w.at(m).at(i) == w1) {
						has_w1 = true;
					}
					if (model.w.at(m).at(i) == w2) {
						has_w2 = true;
					}
					if (has_w1 && has_w2) {
						has_both = true;
						break;
					}
				}
				if (has_both) {
					break;
				}
			}
			if (!has_both) {
				sum += 1;
			}
		}
		return sum;
	}

	double TC_f(vector<pair<int, int>> & top_words, double (EvalModule::*f)(int, int)) {
		//calculate topic coherence for topic n
		double sum = 0;
		for (int m = 1; m < top_words.size(); m++) {
			double subsum = 0;
			for (int l = 0; l < m; l++) {
				int w1 = top_words[m].first;
				int w2 = top_words[l].first;
				subsum += (this->*f)(w1, w2);
			}
			sum += subsum;
		}
		return sum;
	}

	double LCP(int w1, int w2) {
		return log((D(w1, w2) + 1) / (D(w2)));;
	}

	double PMI(int w1, int w2) {
		return log((D(w1, w2) + 1) / (D(w1)*D(w2)));
	}

	double NZ(int w1, int w2) {
		return N(w1, w2, 20) == 0 ? 1 : 0;
	}

	//metrixs;
	double TC_PMI(vector<pair<int, int>> & top_words) {
		return TC_f(top_words, &EvalModule::PMI);
	}

	double TC_NZ(vector<pair<int, int>> & top_words) {
		return TC_f(top_words, &EvalModule::NZ);
	}

	double TC_LCP(vector<pair<int, int>> & top_words) {
		return TC_f(top_words, &EvalModule::LCP);
	}

};


#endif // !EVAL_MOD


