#pragma once


#ifndef TASK
#define TASK
#include "shared_header.h"

/*
Class for parallel tasks, each mpi process will be handling a single task,
each task contains the information of a subset of the documents
the information are synchronised every iteration for all tasks

TODO : if gpu is used, every task will be further divided into subtasks for gpu threads.
*/
class Task
{
public:


	//K & V both used in sampling for Dirichlet process
	int id;
	int K; //topic number
	int V; //vocabulary size

	vector<vector<int>> wordSampling; //a collection of word instance in a subset of the documents, in form of [(doc_id, w, word_id),...]
	//w is the index in overall word map, word_id is the id in document.
	//since these 3 number will be read only, they are grouped in a tuple for  

	vector<int> z; //the topic assignment of word word_id in document doc_id, z should have same length as wordSampling 
	//index is the number where this word appear in the 

	map<int, bool> vocabulary;
	hashmap<int, bool> docCollection;
	

	double alpha, beta;


	hashmap<int, vector<int>> nd; //partial nd
	vec2d<int> nw; //partial nw
	vector<int> nwsum;
	map<int, int> ndsum;


	/*
	map<int, map<int, int>> ndDiff; //nd difference, stored in map, maximum size = M * K, will be smaller over the iterations
	map<int, map<int, int>> nwDiff; //nw difference, maximum size = V * K, will be smaller as well
	map<int, int> nwsumDiff;//nwsum difference, 
	*/

	/* ndsum difference will be none, since the word in each document will not be changed */
	


	//boost serialisation



	//copy
	Task(const Task& t) {
		this->id = t.id;
		this->K = t.K;
		this->V = t.V;

		this->wordSampling = t.wordSampling;
		this->z = t.z;

		this->nd = t.nd;
		this->nw = t.nw;
		this->ndsum = t.ndsum;
		this->nwsum = t.nwsum;

		this->alpha = t.alpha;
		this->beta = t.beta;
		/*
		this->ndDiff = t.ndDiff;
		this->nwDiff = t.nwDiff;
		this->nwsumDiff = t.nwsumDiff;
		*/
	};
	Task();
	~Task();

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
		ar & BOOST_SERIALIZATION_NVP(vocabulary);
		ar & BOOST_SERIALIZATION_NVP(wordSampling);
		ar & BOOST_SERIALIZATION_NVP(z);
		ar & BOOST_SERIALIZATION_NVP(nd);
		ar & BOOST_SERIALIZATION_NVP(nw);
		ar & BOOST_SERIALIZATION_NVP(nwsum);
		ar & BOOST_SERIALIZATION_NVP(ndsum);

	}


};

#endif