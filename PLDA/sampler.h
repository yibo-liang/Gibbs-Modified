#pragma once

#ifndef SAMPLER
#define SAMPLER

#include "shared_header.h"
#include "job_config.h"
#include "task.h"
#include "slave_sync_data.h"

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

	int pid;
	int task_id;
	int K;
	int V;
	vecFast2D<int> wordSampling; //size of W * 3 contains [(doc_id, w, word_id)....]

	vector<int> z;

	vector<int> vocabOffsetMap;  // [(local v index=>global vocabulary index)]
	int documentOffset; //starting document offset, since an executor only contains a subset of the model, we need an offset to calculate its mapping to the full model;

	double alpha, beta;

	int partialM;  //the document count for this partial model
	int partialV; //the vocabulary count for this partial model
	int wordInfoSize;
	int wordInsNum;

	vecFast2D<int> nd;
	vecFast2D<int> nw;

	vector<int> nwsum;
	vector<int> ndsum;
	
	/* ------ Mapping for update ----*/

	hashmap<int, int> docMap; // global m to local m
	hashmap<int, int> vocabMap; //global v to local v

	

	/* ------- Methods -----*/
	SlaveSyncData sample();
	void update(SlaveSyncData& syncData);
	
	void fromTask(const Task& task);
	Sampler(const Task& task);
	Sampler(const Sampler& s);
	Sampler();
	~Sampler();

private:
	/* ------- arrays used for Sampling, allocated only once for speed ----*/
	friend class Sampler;
	vector<double> p;
	vector<bool> Mchange;
	vector<bool> Vchange;

	vecFast2D<int> ndDiff;
	vecFast2D<int> nwDiff;
	vector<int> nwsumDiff;

	inline void clearSyncBuffer();

	inline int mapM(int m); //map from nd array index to global document id;
	inline int mapV(int v); //map from nw array index to global vocabulary id;

};
#endif // !SAMPLER

