#pragma once


#ifndef JOBCONFIG
#define JOBCONFIG
#include "shared_header.h"

class JobConfig
{
public:

	int processID;
	int totalProcessCount;

	int taskPerProcess = 1;

	//modelling information

	int iterationNumber = 2000;
	int documentNumber = 0;// use all documents loaded if 0
	double alpha = 0.01;
	double beta = 0.01;

	vector<int> hierarchStructure; // [n1,n2,n3,..] => a hierarchical topic model with topic number = n1 as first level, n2 to the second, etc.

	int model_type = NAIVE_HIERARCHICAL_MODEL;

	int parallelType = P_MPI;

	string filename = "";
	string filetype = "txt";

	string inferedModelFile = "";
	string inferCorpusFile = "";

	int documentWordStart = 0;

	bool inferencing = false;

	JobConfig(const JobConfig &c);
	JobConfig();
	~JobConfig();

private:

};

#endif