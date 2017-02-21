#pragma once


#ifndef MODEL
#define MODEL
#include <sstream>
#include "shared_header.h"
#include "corpus.h"

class Model
{
public:

	Corpus * corpus;

	int K;//topic number
	int M;//document number
	int V;//vocabulary size

	double alpha, beta;

	vec2d<int> z;
	vec2d<int> nw;
	vec2d<int> nd;
	vector<int> nwsum;
	vector<int> ndsum;
	
	vec2d<double> theta;
	vec2d<double> phi;

	string getTopicWords(int n);

	Model(const Model &m);
	Model();
	~Model();

	void updateSums();

private:
	void computeTheta();
	void computePhi();

};

#endif