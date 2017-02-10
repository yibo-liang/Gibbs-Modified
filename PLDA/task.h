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


	map<int,vector<int>> words; //a subset of the documents, each submap in the map is a doc, with word index=>id
	map<int, vector<int>> z; //the topic assignment of word n in document m
	//index is the number where this word appear in the 

	vector<int> nd;
	vector<int> nw;
	vec2d<int> ndsum;
	vec2d<int> nwsum;
	
	
	
	void run_task(int mode);

	Task(const Task& t) {
		this->words = t.words;
		this->z = t.z;

		this->nd = t.nd;
		this->nw = t.nw;
		this->ndsum = t.ndsum;
		this->nwsum = t.nwsum;
	};
	Task();
	~Task();

private:

	void iterate_mpi();
	void iterate_gpu();
	void iterate_hibrid();

};

#endif