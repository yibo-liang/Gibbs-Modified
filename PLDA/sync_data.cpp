#include "sync_data.h"

SynchronisationData::SynchronisationData(const SynchronisationData & d)
{
	this->ndDiff = d.ndDiff;
	this->nwDiff = d.nwDiff;
	this->nwsumDiff = d.nwsumDiff;
}

SynchronisationData::SynchronisationData()
{
}

SynchronisationData::~SynchronisationData()
{
}
