#include "sampler.h"

inline void Sampler::clearSyncBuffer()
{
	fill2D(ndDiff, 0, partialM, K);
	fill2D(nwDiff, 0, partialV, K);
	std::fill(nwsumDiff.begin(), nwsumDiff.end(), 0);
	std::fill(Mchange.begin(), Mchange.end(), false);
	std::fill(Vchange.begin(), Vchange.end(), false);
}

inline int Sampler::mapM(int m)
{
	return documentOffset + m;
}

inline int Sampler::mapV(int v)
{
	return vocabOffsetMap[v];
}

SlaveSyncData Sampler::sample()
{
	/* ------------- initialising sync buffer ------------  */
	clearSyncBuffer();
	/* ------------- Sample start ------------  */

	double Vbeta = (double)V * beta;
	double Kalpha = (double)K * alpha;

	for (int wi = 0; wi < wordInsNum; wi++) {
		int m = readvec2D(wordSampling, wi, 0, 2);
		int w = readvec2D(wordSampling, wi, 1, 2);
		//int n = readvec2D(wordSampling, wi, 2, wordInfoSize);

		int topic = z.at(wi);

		//sync buffer
		plusIn2D(nwDiff, -1, w, topic, K);
		plusIn2D(ndDiff, -1, m, topic, K);
		nwsumDiff[topic] -= 1;

		//local partial model
		plusIn2D(nw, -1, w, topic, K);
		plusIn2D(nd, -1, m, topic, K);
		nwsum[topic] -= 1;
		//ndsum[m] -= 1; // no need for changing the value, only need ref of ndsum-1

		Mchange[m] = true;
		Vchange[w] = true;

		for (int k = 0; k < K; k++) {
			double A = readvec2D(nw, w, k, K);
			double B = nwsum.at(k);
			double C = readvec2D(nd, m, k, K);
			double D = ndsum.at(m) - 1;
			p[k] = (A + beta) / (B + Vbeta) *
				(C + alpha) / (D + Kalpha);
		}
		for (int k = 1; k < K; k++) {
			p[k] += p.at(k - 1);
		}
		double u = ((double)rand() / (double)RAND_MAX) * p[K - 1];
		for (topic = 0; topic < K; topic++) {
			if (p[topic] >= u) {
				break;
			}
		}
		z[wi] = topic;


		//local partial model
		plusIn2D(nw, 1, w, topic, K);
		plusIn2D(nd, 1, m, topic, K);
		nwsum[topic] += 1;
		//ndsum[m] += 1;

		//sync buffer
		plusIn2D(nwDiff, 1, w, topic, K);
		plusIn2D(ndDiff, 1, m, topic, K);
		nwsumDiff[topic] += 1;

	}

	//generate sync data
	SlaveSyncData result;
	result.pid = pid;
	result.task_id = task_id;
	for (int i = 0; i < Mchange.size(); i++) {
		if (Mchange[i]) {
			int m = mapM(i);
			result.ndDiff[m] = vector<int>(K);
			for (int k = 0; k < K; k++) {
				result.ndDiff[m][k] = readvec2D(ndDiff, i, k, K);
			}
		}
	}
	for (int i = 0; i < Vchange.size(); i++) {
		if (Vchange[i]) {
			int v = mapV(i);
			result.nwDiff[v] = vector<int>(K);
			for (int k = 0; k < K; k++) {
				result.nwDiff[v][k] = readvec2D(nwDiff, i, k, K);
			}
		}
	}
	for (int i = 0; i < K; i++) {
		result.nwsumDiff[i] = nwsumDiff[i];
	}
	return result;
}

void Sampler::update(SlaveSyncData & syncData)
{
	for (auto&it : syncData.ndDiff) {
		int global_m = it.first;
		if (docMap.count(global_m)) {
			int m = docMap.at(global_m);
			for (int i = 0; i < it.second.size(); i++) {
				writevec2D(it.second[i], nd, m, i, K);
			}
		}
	}

	for (auto&it : syncData.nwDiff) {
		int global_v = it.first;
		if (vocabMap.count(global_v)) {
			int v = vocabMap.at(global_v);
			for (int i = 0; i < it.second.size(); i++) {
				writevec2D(it.second[i], nw, v, i, K);
			}
		}
	}
	for (auto &doc : syncData.nwsumDiff) {
		nwsum[doc.first] = doc.second;
	}



}

void Sampler::fromTask(const Task& task)
{
	//allocate memory and copy values from task
	wordInsNum = task.wordSampling.size();
	wordInfoSize = task.wordSampling.at(0).size(); //should be 3


	this->task_id = task.id;

	this->z = task.z;

	this->alpha = task.alpha;
	this->beta = task.beta;

	this->K = task.K;
	this->p = vector<double>(K, 0);

	this->V = task.V; //total vocabulary 

	this->partialM = task.nd.size();
	this->partialV = task.nw.size();

	this->nd = newVec2D<int>(partialM, K);
	this->nw = newVec2D<int>(partialV, K);

	this->nwsum = task.nwsum;
	this->ndsum = vector<int>(task.ndsum.size(), K);

	// store vocabulary offset
	this->vocabOffsetMap = vector<int>(task.vocabulary.size());

	int i = 0;
	for (auto& it : task.vocabulary) {
		vocabOffsetMap[i] = it.first;
		i++;
	}

	//nd & ndsum 
	this->documentOffset = task.ndsum.begin()->first;
	Mchange = vector<bool>(partialM);
	for (int m = 0; m < partialM; m++) {
		int global_m = mapM(m);
		docMap[global_m] = m;
		ndsum[m] = task.ndsum.at(global_m);
		for (int k = 0; k < K; k++) {
			int tmp = task.nd.at(global_m).at(k);
			writevec2D<int>(tmp, nd, m, k, K);
		}
	}

	//nw & nwsum
	Vchange = vector<bool>(partialV);
	for (int v = 0; v < partialV; v++) {
		int global_v = mapV(v);
		vocabMap[global_v] = v;

	
		for (int k = 0; k < K; k++) {
			int tmp = task.nw.at(global_v).at(k);
			writevec2D<int>(tmp, nw, v, k, K);
		}
	}

	//create sync buffer for sampling
	this->ndDiff = newVec2D<int>(partialM, K);
	this->nwDiff = newVec2D<int>(partialV, K);
	this->nwsumDiff = vector<int>(K, 0);

	//word instances 
	this->wordSampling = newVec2D<int>(wordInsNum, 2);
	for (int i = 0; i < wordInsNum; i++) {
		int m = task.wordSampling.at(i).at(0);
		m = m - documentOffset;
		int w = task.wordSampling.at(i).at(1);
		w = vocabMap.at(w);
		int wordIndexInDoc = task.wordSampling.at(i).at(2);

		writevec2D<int>(m, wordSampling, i, 0, 2);
		writevec2D<int>(w, wordSampling, i, 1, 2);

	}

}

Sampler::Sampler(const Task & task)
{
	fromTask(task);
}

Sampler::Sampler(const Sampler & s)
{
	throw 20; //should never be copied
}

Sampler::Sampler()
{
}

Sampler::~Sampler()
{
	free(nd);
	free(nw);
	free(ndDiff);
	free(nwDiff);
	free(wordSampling);

}
