#include "model.h"

using std::pair;



string Model::getTopicWords(int n)
{
	using std::stringstream;
	computePhi();
	computeTheta();
	stringstream ss;
	for (int i = 0; i < K; i++) {
		vector<double> phiRow = phi[i];
		vector<pair<string, double>> topic_word_distrib;
		for (int j = 0; j < phiRow.size(); j++) {
			string w = this->corpus->indexToWord[j];
			topic_word_distrib.push_back(pair<string, double>(w, phiRow[j]));
		}


		std::sort(topic_word_distrib.begin(), topic_word_distrib.end(), [](pair<string, double> const& a, pair<string, double> const& b) {return a.second > b.second; });
		ss << "Topic " << i << " ";
		for (int j = 0; j < n; j++) {
			ss << topic_word_distrib[j].first << "(" << topic_word_distrib[j].second << "), ";
		}
		ss << endl;
	}

	string result = ss.str();

	return result;
}

vector<Model> Model::getInitalSubmodel(int K_sublevel)
{
	//Create submodels after this model has been built
	vector<Model> models(K);

	for (int k = 0; k < K; k++) {
		models[k].id = k;
		models[k].super_model_id = this->id;
		models[k].corpus = this->corpus;
		models[k].K = K_sublevel;
		models[k].M = this->M;
		models[k].V = this->V;
		models[k].alpha = this->alpha;
		models[k].beta = this->beta;
		models[k].nw = vector<vector<int>>(V, vector<int>(K_sublevel, 0));
		models[k].nd = vector<vector<int>>(M, vector<int>(K_sublevel, 0));
		models[k].nwsum = vector<int>(K_sublevel);
		models[k].ndsum = vector<int>(M);
		models[k].z = vec2d<int>(M);
		models[k].w = vec2d<int>(M);


		for (int m = 0; m < z.size(); m++) {
			Document &doc = corpus->documents[m];
			int word_in_k_count = 0;
			for (int i = 0; i < z.at(m).size(); i++) {
				if (z.at(m).at(i) == k) {


					int word = w.at(m).at(i);
					int topic = RandInteger(0, K_sublevel - 1);

					models[k].z.at(m).push_back(topic);
					models[k].w.at(m).push_back(word);

					models[k].nwsum[topic]++;
					models[k].nw[word][topic]++;
					models[k].nd[m][topic]++;
				}
			}
			models[k].ndsum[m] = models[k].w.at(m).size();
		}
	}

	return models;
}

Model::Model(const Model &m)
{
	this->id = m.id;
	this->super_model_id = m.super_model_id;

	this->K = m.K;
	this->M = m.M;
	this->V = m.V;
	this->w = m.w;
	this->z = m.z;
	this->nw = m.nw;
	this->nd = m.nd;
	this->nwsum = m.nwsum;
	this->ndsum = m.ndsum;
	this->theta = m.theta;
	this->phi = m.phi;
}

Model::Model()
{
}

Model::~Model()
{
}

void Model::update()
{

	for (int k = 0; k < K; k++) {
		nwsum[k] = 0;
		for (int v = 0; v < V; v++) {
			nwsum[k] += nw[v][k];
		}
	}
	computePhi();
	computeTheta();
}

void Model::computeTheta()
{
	theta.resize(M, vector<double>(K));
	for (int m = 0; m < M; m++) {
		for (int k = 0; k < K; k++) {
			theta[m][k] = (nd[m][k] + alpha) / (ndsum[m] + K * alpha);
		}
	}
}

void Model::computePhi()
{
	phi.resize(K, vector<double>(V));
	for (int k = 0; k < K; k++) {
		for (int w = 0; w < V; w++) {
			phi[k][w] = (nw[w][k] + beta) / (nwsum[k] + V * beta);
		}
	}
}
