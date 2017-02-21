#pragma once


#ifndef TASK
#define TASK
#include "shared_header.h"

/*
Class for parallel tasks, each is a partition of the overall model.

TODO : if gpu is used, every task will be further divided into subtasks for gpu threads.
*/
class TaskPartition
{
public:


	//K & V both used in sampling for Dirichlet process
	int id;
	int K; //topic number
	int V; //vocabulary size

	int partitionM; //partition M size
	int partitionV; //partition V size

	int offsetM;
	int offsetV;


	vector<vector<int>> words; //a collection of word instance in a subset of the documents, in form of [(doc_id, w, z),...]
	//w is the index in overall word map, word_id is the id in document.
	//since these 3 number will be read only, they are grouped in a tuple for  


	double alpha, beta;


	vec2d<int> nd;
	vec2d<int> nw; //partial nw
	vector<int> nwsum;
	vector<int> ndsum;

	

	//copy
	TaskPartition(const TaskPartition& t) {
		this->id = t.id;
		this->K = t.K;
		this->V = t.V;

		this->partitionM = t.partitionM;
		this->partitionV = t.partitionV;
		this->offsetM = t.offsetM;
		this->offsetV = t.offsetV;

		this->words = t.words;
		
		this->nd = t.nd;
		this->nw = t.nw;
		this->ndsum = t.ndsum;
		this->nwsum = t.nwsum;

		this->alpha = t.alpha;
		this->beta = t.beta;
		
	};
	TaskPartition();
	~TaskPartition();

private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & id;
		ar & alpha;
		ar & beta;
		ar & K;
		ar & V;
		ar & (partitionM);
		ar & (partitionV);
		ar & (offsetM);
		ar & (offsetV);
		ar & BOOST_SERIALIZATION_NVP(words);
		ar & BOOST_SERIALIZATION_NVP(nd);
		ar & BOOST_SERIALIZATION_NVP(nw);
		ar & BOOST_SERIALIZATION_NVP(nwsum);
		ar & BOOST_SERIALIZATION_NVP(ndsum);

	}


};

#endif