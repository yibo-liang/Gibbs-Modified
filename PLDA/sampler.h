#pragma once

#ifndef SAMPLER
#define SAMPLER

#include "shared_header.h"
#include "job_config.h"
#include "task_partition.h"
#include "slave_sync_data.h"

#include "clwrapper.h"
using namespace fastVector2D;


class Sampler
{
public:

	/*
	sampler executor contains all information to construct a LDA model and doing Gibbs Sampling, in a low level way
	where all information stored in raw array in heap, for faster access and sampling

	where all word w will be transfered to array starting index from 0, instead of using
	vocabulary index w which requires a V sized array or a Hashmap;

	*/
	int sampleMode = P_MPI;

	int pid;
	int partition_id;
	int K;
	int V;
	vecFast2D<int> wordSampling; //size of W * 3 contains [(doc_id, w, z)....]

	vector<int> z;

	//vector<int> vocabOffsetMap;  // [(local v index=>global vocabulary index)]
	int offsetM; //starting document offset, since an executor only contains a subset of the model, we need an offset to calculate its mapping to the full model;
	int offsetV;

	double alpha, beta;

	int partialM;  // the document count for this partial model
	int partialV; // the vocabulary count for this partial model
	int wordInfoSize; // the size of sub vector of the information of a word,
	int wordInsNum; // number of words in this sampler

	vector<int> nd;// na[i][j]: number of words in document i assigned to topic j, size M x K
	vector<int> nw;// cwt[i][j]: number of instances of word/term i assigned to topic j, size V x K

	vector<int> nwsum;// nwsum[j]: total number of words assigned to topic j, size K
	vector<int> nwsumDiff; //nwsum difference

	vector<int> ndsum;// ndsum[i]: total number of words in document i, size M


	clWrapper opencl;

	/* ------- Methods -----*/

	void sample();


	void fromTask(TaskPartition& task);
	Sampler(TaskPartition& task);
	Sampler(const Sampler& s);
	Sampler();
	~Sampler();
private:
	/* ------- arrays used for Sampling, allocated only once for speed ----*/
	friend class Sampler;
	vector<double> p;

	void sample_MPI();
	inline int getPartitionID(vector<size_t> partitionVec, int i);
	void prepare_GPU(TaskPartition & task);
	void sample_OPENCL();
	void release_GPU();
	//inline int mapV(int v); //map from nw array index to global vocabulary id;

};
#endif // !SAMPLER

