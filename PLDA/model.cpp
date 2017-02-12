#include "model.h"

Model::Model(const Model &m)
{
	this->K = m.K;
	this->M = m.M;
	this->V = m.V;
	this->z = m.z;
	this->nw = m.nw;
	this->nd = m.nd;
	this->nwsum = m.nwsum;
	this->ndsum = m.ndsum;
	this->theta = m.theta;
	this->phi = m.phi;
}

Model::Model()
{
}

Model::~Model()
{
}