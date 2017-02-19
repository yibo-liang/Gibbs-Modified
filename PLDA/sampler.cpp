#include "sampler.h"

inline void Sampler::clearSyncBuffer()
{

}

inline int Sampler::mapM(int m)
{
	return documentOffset + m;
}

void Sampler::sample()
{
	//Sampling using AD-LDA algorithm for one iteration
	//asume that task are partitioned into different document groups, no 2 task have a same document to sample

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


		//local partial model
		plusIn2D(nw, -1, w, topic, K);
		plusIn2D(nd, -1, m, topic, K);
		nwsum[topic]--;
		ndsum[m]--;

		//ndsum[m] -= 1; // no need for changing the value, only need ref of ndsum-1

		for (int k = 0; k < K; k++) {
			double A = readvec2D(nw, w, k, K);
			double B = nwsum[k];
			double C = readvec2D(nd, m, k, K);
			double D = ndsum[m];
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
		nwsum[topic]++;
		ndsum[m]++;


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
	//this->vocabOffsetMap = vector<int>(task.vocabulary.size());

	//int i = 0;
	//for (auto& it : task.vocabulary) {
	//	vocabOffsetMap[i] = it.first;
	//	i++;
	//}

	//nd & ndsum 
	this->documentOffset = task.ndsum.begin()->first;
	//cout << "sampler id=" << this->task_id << ", d offset= " << this->documentOffset<<endl;
	for (int m = 0; m < partialM; m++) {
		int global_m = mapM(m);
		ndsum[m] = task.ndsum.at(global_m);
		for (int k = 0; k < K; k++) {
			int tmp = task.nd.at(global_m).at(k);
			writevec2D<int>(tmp, nd, m, k, K);
		}
	}

	//nw & nwsum
	for (int v = 0; v < task.V; v++) {
		for (int k = 0; k < K; k++) {
			int tmp = task.nw.at(v).at(k);
			writevec2D<int>(tmp, nw, v, k, K);
		}
	}

	//word instances 
	this->wordSampling = newVec2D<int>(wordInsNum, 2);
	for (int i = 0; i < wordInsNum; i++) {
		int m = task.wordSampling.at(i).at(0);
		m = m - documentOffset;
		int w = task.wordSampling.at(i).at(1);
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
	delete[] nd;
	delete[] nw;
	delete[] wordSampling;

	//free(nd);
	//free(nw);
	//free(wordSampling);

}
