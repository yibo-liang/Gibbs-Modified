#include "sampler.h"

void Sampler::sample() {
	if (sampleMode == P_MPI) {
		sample_MPI();
	}
	else {
		sample_OPENCL();
	}
}

void Sampler::sample_MPI()
{
	//Sampling using AD-LDA algorithm for one iteration
	//asume that task are partitioned into different document groups, no 2 task have a same document to sample

	/* ------------- initialising sync buffer ------------  */
	/* ------------- Sample start ------------  */

	double Vbeta = (double)partialV * beta;
	double Kalpha = (double)K * alpha;

	for (int wi = 0; wi < wordInsNum; wi++) {
		int m = readvec2D(wordSampling, wi, 0, 3);
		int w = readvec2D(wordSampling, wi, 1, 3);

		int topic = readvec2D(wordSampling, wi, 2, 3);



		//local partial model
		plusIn2D(&nw[0], -1, w, topic, K);
		plusIn2D(&nd[0], -1, m, topic, K);

		nwsum[topic]--;
		nwsumDiff[topic]--;
		//ndsum[m]--;

		for (int k = 0; k < K; k++) {
			double A = readvec2D<int>(&nw[0], w, k, K);
			double B = nwsum[k];
			double C = readvec2D<int>(&nd[0], m, k, K);
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

		writevec2D<int>(topic, wordSampling, wi, 2, 3);


		//local partial model
		plusIn2D(&nw[0], 1, w, topic, K);
		plusIn2D(&nd[0], 1, m, topic, K);
		nwsum[topic]++;
		nwsumDiff[topic]++;
		//ndsum[m]++;

	}


}

void Sampler::fromTask(TaskPartition& task)
{
	//allocate memory and copy values from task
	wordInsNum = task.words.size();
	wordInfoSize = 3; //should be 3


	this->partition_id = task.partition_id;
	this->pid = task.proc_id;

	this->alpha = task.alpha;
	this->beta = task.beta;

	this->K = task.K;
	this->V = task.V;
	this->p = vector<double>(K, 0); // vector used for rollete wheel selection from accumulated distribution

	this->partialM = task.partitionM;
	this->partialV = task.partitionV;
	this->offsetM = task.offsetM;
	this->offsetV = task.offsetV;


	cout << "*** task ID=" << this->partition_id << ", PID=" << pid << ", partitionM=" << this->partialM << ", partitionV=" << this->partialV << ", offsetM=" << offsetM << ", offsetV=" << offsetV << endl;


	this->nd = vector<int>(partialM*K);// newVec2D<int>(partialM, K);
	this->nw = vector<int>(partialV*K);// newVec2D<int>(partialV, K);

	this->nwsum = task.nwsum;
	this->ndsum = task.ndsum;
	this->nwsumDiff = vector<int>(nwsum.size());


	for (int m = 0; m < partialM; m++) {
		ndsum[m] = task.ndsum.at(m);
		for (int k = 0; k < K; k++) {
			int tmp = task.nd.at(m).at(k);
			writevec2D<int>(tmp, &nd[0], m, k, K);
		}
	}

	//nw & nwsum
	for (int v = 0; v < task.partitionV; v++) {
		for (int k = 0; k < K; k++) {
			int tmp = task.nw.at(v).at(k);
			writevec2D<int>(tmp, &nw[0], v, k, K);
		}
	}

	//word instances 
	this->wordSampling = newVec2D<int>(wordInsNum, 3);

	std::sort(task.words.begin(), task.words.end(), [](const vector<int>& w1, const vector<int>& w2) {
		return w1.at(1) < w2.at(1);
	});

	for (int i = 0; i < wordInsNum; i++) {
		int m = task.words.at(i).at(0);
		int w = task.words.at(i).at(1);
		int z = task.words.at(i).at(2);

		if (z >= 30) {
			cout << "error topic n=" << z << endl;
			cout << this->pid << ", " << this->partition_id << endl;
			throw 20;
		}

		writevec2D<int>(m, wordSampling, i, 0, 3);
		writevec2D<int>(w, wordSampling, i, 1, 3);
		writevec2D<int>(z, wordSampling, i, 2, 3);

	}

}

Sampler::Sampler(TaskPartition & task)
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
	//delete[] nd;
	//delete[] nw;
	delete[] wordSampling;

	//free(nd);
	//free(nw);
	//free(wordSampling);

}