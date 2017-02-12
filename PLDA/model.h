#pragma once


#ifndef MODEL
#define MODEL

#include "shared_header.h"

class Model
{
public:

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

	Model(const Model &m);
	Model();
	~Model();

private:

};

#endif