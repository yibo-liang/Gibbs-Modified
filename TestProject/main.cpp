
#include <boost/asio.hpp>
#include <string>
#include <memory>
#include "plda_all.h"
#include "request_handler.h"
#include "json.hpp"
#include "session.h"

using namespace ParallelHLDA;
using namespace boost;
using namespace boost::system;
using namespace boost::asio;

using json = nlohmann::json;

const int nTopics = 25;
vector<vector<std::pair<string, int>>> get_topic_words(Model & m, Corpus & corpus) {

	using namespace std;
	struct word_prob_order
	{
		inline bool operator() (const pair<int, double>& a, const pair<int, double>& b)
		{
			return (a.second > b.second);
		}
	};

	vector<vector<pair<string, int>>> res;
	for (int k = 0; k < m.K; k++) {
		vector<pair<int, double> > words_probs;
		pair<int, double> word_prob;
		for (int w = 0; w < m.V; w++) {
			word_prob.first = w;
			word_prob.second = m.phi[k][w];
			words_probs.push_back(word_prob);
		}

		// quick sort to sort word-topic probability
		//utils::quicksort(words_probs, 0, words_probs.size() - 1);
		std::sort(words_probs.begin(), words_probs.end(), word_prob_order());

		vector<pair<string, int>> word_freqs;
		for (int i = 0; i < nTopics; i++) {

			auto it = corpus.indexToWord.find(words_probs[i].first);

			string word = (it->second).c_str();
			int w = words_probs[i].first;
			int freq = m.nw[w][k];
			pair<string, int> word_freq;
			word_freq.first = word;
			word_freq.second = freq;
			word_freqs.push_back(word_freq);
		}
		res.push_back(word_freqs);
	}
	return res;
}


double cosine_similarity(Model & m1, int ma, Model & m2, int mb, vector<int> & root_ndsum) {
	int M = m1.M;
	double sum_ab = 0;
	double sum_a2 = 0;
	double sum_b2 = 0;
	for (int i = 0; i < M; i++) {

		//recalculate theta value based on root ndsum.
		double da = ((double)m1.nd[i][ma] + m1.alpha) / ((double)root_ndsum[i] + m1.alpha);
		double db = ((double)m2.nd[i][mb] + m2.alpha) / ((double)root_ndsum[i] + m2.alpha);

		if (m1.nd[i][ma] < 0) {
			throw new std::exception();
		}

		sum_ab += da*db;
		sum_a2 += da*da;
		sum_b2 += db*db;
	}
	double result = sum_ab / (sqrt(sum_a2)*sqrt(sum_b2));
	if (result < 0) {
		cout << result << endl;
	}
	if (result > 1.01) {
		throw 11;
	}
	return result;
}

json modelToJson(Model & model, Corpus & corpus, Model & topLevelModel) {
	json result;
	json topics;
	json topicsSimilarities;
	json topicClassesDistrib;
	json metadata;
	json topicDocDistribution;
	model.update();

	using namespace std;
	{ //turning single model/submodel to json file
		metadata["nDocs"] = model.M;
		metadata["nTopics"] = model.K;
		metadata["nWordsPerTopic"] = nTopics;
		metadata["docClasses"] = { "EU", "UK" };
		metadata["subTopicNumber"] = model.submodels.size();


		vector<vector<pair<string, int>>> word_freqs = get_topic_words(model, corpus);
		for (int i = 0; i < word_freqs.size(); i++) {
			vector<pair<string, int>> word_freq = word_freqs[i];
			json topic_word_list;
			for (int w = 0; w < word_freq.size(); w++) {

				json ins = { { "label", word_freq[w].first },{ "weight", word_freq[w].second } };
				topic_word_list.push_back(ins);
			}

			string tmp = std::to_string(i);
			topics[tmp] = (topic_word_list);
		}

		//similarity matrix of topics of this single model
		for (int k1 = 0; k1 < model.K; k1++) {
			for (int k2 = 0; k2 < model.K; k2++) {
				double sim = cosine_similarity(model, k1, model, k2, topLevelModel.ndsum);
				string sma = std::to_string(k1);
				string smb = std::to_string(k2);
				topicsSimilarities[sma][smb] = sim;
			}
		}




		vector<double> weight_sum_eu_vec;
		vector<double> weight_sum_uk_vec;

		vector<double> value_sum_eu_vec;
		vector<double> value_sum_uk_vec;


		double weight_sum_all = 0;
		double value_sum_all = 0;

		for (int i = 0; i < model.K; i++) {

			//for topic doc 
			json topic_doc_dist;


			//for topic classes (EU/UK)
			json class_eu;
			json class_uk;

			class_eu["classID"] = "EU";
			class_uk["classID"] = "UK";

			double weight_sum_uk = 0;
			double weight_sum_eu = 0;

			double weight_value_sum_uk = 0;
			double weight_value_sum_eu = 0;


			vector<json> unsorted;
			for (int m = 0; m < model.M; m++) {
				string topic_id = to_string(m);
				double tweight = (double)model.nd[m][i] / (double)topLevelModel.ndsum[m]; //mod->theta[m][i];
				json tmp;

				if (corpus.documents.at(m).info["class"] != "UK") {
					weight_sum_eu += tweight;
					double val = atof(corpus.documents.at(m).info["value"].c_str());
					weight_value_sum_eu += tweight*val;

				}
				else {
					weight_sum_uk += tweight;
					double val = atof(corpus.documents.at(m).info["value"].c_str());
					weight_value_sum_uk += tweight* val;
				}

				if (tweight > 1.01) {
					throw 11;
				}
				tmp["topicWeight"] = tweight;
				tmp["docId"] = corpus.documents.at(m).info["id"];
				tmp["docClass"] = corpus.documents.at(m).info["class"];
				unsorted.push_back(tmp);
			}
			std::sort(unsorted.begin(), unsorted.end(), [](auto && a, auto && b) {return a["topicWeight"] > b["topicWeight"]; });
			for (int doc_i = 0; doc_i < 100; doc_i++) {
				topic_doc_dist.push_back(unsorted[doc_i]);
			}

			class_eu["weightSum"] = weight_sum_eu;
			class_eu["weightedValueSum"] = weight_value_sum_eu;
			class_uk["weightSum"] = weight_sum_uk;
			class_uk["weightedValueSum"] = weight_value_sum_uk;


			weight_sum_eu_vec.push_back(weight_sum_eu);
			weight_sum_uk_vec.push_back(weight_sum_uk);

			value_sum_eu_vec.push_back(weight_value_sum_eu);
			value_sum_uk_vec.push_back(weight_value_sum_uk);


			weight_sum_all += (weight_sum_eu + weight_sum_uk);
			value_sum_all += (weight_value_sum_eu + weight_value_sum_uk);


			topicClassesDistrib[to_string(i)] = { class_eu, class_uk };
			topicDocDistribution[to_string(i)] = topic_doc_dist;
		}

		cout << "-----------------------" << endl;
		for (int k = 0; k < model.K; k++) {
			topicClassesDistrib[to_string(k)][0]["weightSumNorm"] = weight_sum_eu_vec[k] / weight_sum_all;
			topicClassesDistrib[to_string(k)][1]["weightSumNorm"] = weight_sum_uk_vec[k] / weight_sum_all;

			//cout << value_sum_eu_vec[k] << "/ " << value_sum_all << "=" << value_sum_eu_vec[k] / value_sum_all << endl;

			topicClassesDistrib[to_string(k)][0]["weightedValueSumNorm"] = value_sum_eu_vec[k] / value_sum_all;
			topicClassesDistrib[to_string(k)][1]["weightedValueSumNorm"] = value_sum_uk_vec[k] / value_sum_all;
		}


		metadata["weightSumAll"] = weight_sum_all;
		metadata["valueSumAll"] = value_sum_all;

		model.nd = vec2d<int>();
		model.nw = vec2d<int>();
		model.nwsum = vector<int>();
		model.theta = vec2d<double>();
		model.phi = vec2d<double>();

	}

	json submodels;
	for (int i = 0; i < model.submodels.size(); i++) {
		json submodel = modelToJson(model.submodels[i], corpus, topLevelModel);
		submodels.push_back(submodel);
	}
	result["metadata"] = metadata;
	result["topicsSimilarities"] = topicsSimilarities;
	result["topicsDocsDistrib"] = topicDocDistribution;
	result["topicClassesDistrib"] = topicClassesDistrib;
	result["topics"] = topics;
	result["submodels"] = submodels;
	return result;
}


void accept_and_run(ip::tcp::acceptor& acceptor, io_service& io_service, std::shared_ptr<RequestHandler>& requestHandler)
{
	std::shared_ptr<session> sesh = std::make_shared<session>(io_service);

	acceptor.async_accept(sesh->socket, [sesh, &acceptor, &io_service, &requestHandler](const error_code& accept_error)
	{
		accept_and_run(acceptor, io_service , requestHandler);
		if (!accept_error)
		{
			session::interact(sesh, requestHandler);
		}
	});
}

int main(int argc, char *argv[])
{

	using namespace std;

	string modelName;
	if (argc > 1) {
		string type = string(argv[1]);
		if (type == "json") {
			modelName = string(argv[2]);
			string path = "";
			Corpus corpus = loadSerialisable<Corpus>(path + "corpus.ser");
			Model model = loadSerialisable<Model>(path + modelName + ".model");
			model.update();
			json modelJSON = modelToJson(model, corpus, model);
			ofstream ofs(path + modelName + ".json");
			ofs << modelJSON.dump();
			ofs.close();
		}
		else if (type == "server") {
			modelName = string(argv[2]);
			cout << "Http Server for Hexamap data" << endl;
			cout << "Loading data ...";
			string path = "";

			//load corpus and model into HEAP
			Corpus * corpus = new Corpus();
			Model * model = new Model();
			loadSerialisable2<Corpus>(*corpus, path + "corpus.ser");
			loadSerialisable2<Model>(*model, path + modelName + ".model");

			//make shared pointers for loaded data and handler
			std::shared_ptr<Corpus*> p_corpus = std::make_shared<Corpus*>(corpus);
			std::shared_ptr<Model*> p_model = std::make_shared<Model*>(model);
			std::shared_ptr<RequestHandler> requestHandler = std::make_shared<RequestHandler>(p_model, p_corpus);

			cout << "\tdone" << endl;

			// start Server
			io_service io_service;
			ip::tcp::endpoint endpoint{ ip::tcp::v4(), 8081 };
			ip::tcp::acceptor acceptor{ io_service, endpoint };

			acceptor.listen();
			accept_and_run(acceptor, io_service, requestHandler);
			io_service.run();

			//stop server, which should not happen though.
			delete corpus;
			delete model;
			return 0;
		}
	}


	return 0;
}