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
			ss << topic_word_distrib[j].first <<", ";
		}
		ss << endl;
	}

	string result = ss.str();

	return result;
}

Model::Model(const Model &m)
{
	this->K = m.K;
	this->M = m.M;
	this->V = m.V;
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
