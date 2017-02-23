#include "sampler.h"


void Sampler::sample() {
	if (sampleMode == P_MPI) {
		sample_MPI();
	}
	else {
		sample_OPENCL();
	}
}

inline int getPartitionID(vector<size_t> partitionVec, int i) {
	if (partitionVec.size() == 1) return 0;
	for (int j = 1; j < partitionVec.size(); j++) {
		size_t p = partitionVec[j];
		if (i < p) {
			return j - 1;
		}
		else if (i >= p && j == partitionVec.size() - 1) {
			return j;
		}
	}
	cout << "Error: sampler partitioning failed!" << endl;
	throw 20;
}

void Sampler::prepare_GPU(TaskPartition & task)
{
	//prepare data for opencl implementation

	//data from task is already partitioned, but for opencl, we need further partition it using same idea

	//The number of paritition on nd and nw

	using clpartition = tuple<int, int, int>;
	int partition_num = partialM > partialV ? partialM : partialV;
	int word_count = wordInsNum;
	int row_average = word_count / partition_num;
	int col_average = word_count / partition_num;

	vector<vector<vector<clpartition>>> parts;
	vector<size_t> nd_partition_offsets;
	vector<size_t> nw_partition_offsets;

	//nd partition offsets
	int sum = 0;
	nd_partition_offsets.push_back(0);
	for (int m = 0; m < partialM; m++) {
		int sum_row = 0;
		for (int k = 0; k < K; k++) {
			sum_row += task.nd[m][k];
		}
		sum += sum_row;
		if (sum > row_average) {
			sum = 0;
			nd_partition_offsets.push_back(m);
		}
	}

	//nw partition offsets
	sum = 0;
	nw_partition_offsets.push_back(0);
	for (int v = 0; v < partialV; v++) {
		int sum_col = 0;
		for (int k = 0; k < K; k++) {
			sum_col += task.nw[v][k];
		}
		sum += sum_col;
		if (sum > col_average) {
			sum = 0;
			nw_partition_offsets.push_back(v);
		}
	}


	//iterate all word, put them into boxes of partitions
	for (int i = 0; i < wordInsNum; i++) {
		int m = task.words.at(i).at(0);
		int w = task.words.at(i).at(1);
		int task_z = task.words.at(i).at(2);

		int col = getPartitionID(nw_partition_offsets, w);
		int row = getPartitionID(nd_partition_offsets, m);
		parts[row][col].push_back(clpartition(m, w, task_z));
	}

	//iterate all paritions, flatten the partition matrix, but remember the partition structure by using offset
	opencl.partition_offset = newVec2D<int>(partition_num, partition_num);
	opencl.partition_word_count = newVec2D<int>(partition_num, partition_num);
	opencl.words = newVec2D<int>(wordInsNum, 2);
	int wi = 0;
	for (int row = 0; row < partition_num; row++) {
		vector<vector<clpartition>> * part_row = &parts[row];
		for (int col = 0; col < partition_num; col++) {
			vector<clpartition> * p = &(*part_row)[col];
			writevec2D<int>(wi, opencl.partition_offset, row, col, partition_num);
			writevec2D<int>(p->size(), opencl.partition_offset, row, col, partition_num);
			for (int i = 0; i < p->size(); i++) {
				clpartition * part = &(*p)[i];
				int m = std::get<0>(*part);
				int w = std::get<1>(*part);
				int z = std::get<2>(*part);

			}
		}
	}

}

void Sampler::sample_OPENCL() {

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

		int topic = z[wi];



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

		z[wi] = topic;

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


	//cout << "*** task ID=" << this->partition_id << ", PID=" << pid << ", partitionM=" << this->partialM << ", partitionV=" << this->partialV << ", offsetM=" << offsetM << ", offsetV=" << offsetV << endl;


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




	if (sampleMode == P_GPU) {
		if (pid == 0) {
			opencl.displayInformation = true;
		}
		opencl.initialise();

	}
	else {

		std::sort(task.words.begin(), task.words.end(), [](const vector<int>& w1, const vector<int>& w2) {
			return w1.at(1) < w2.at(1);
		});
		for (int i = 0; i < wordInsNum; i++) {
			int m = task.words.at(i).at(0);
			int w = task.words.at(i).at(1);
			int task_z = task.words.at(i).at(2);

			writevec2D<int>(m, wordSampling, i, 0, 3);
			writevec2D<int>(w, wordSampling, i, 1, 3);
			//writevec2D<int>(task_z, wordSampling, i, 2, 3);
			z.push_back(task_z);
		}
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