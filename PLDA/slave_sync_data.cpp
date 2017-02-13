#include "slave_sync_data.h"

SlaveSyncData::SlaveSyncData(const SlaveSyncData & d)
{
	this->ndDiff = d.ndDiff;
	this->nwDiff = d.nwDiff;
	this->nwsumDiff = d.nwsumDiff;
}

SlaveSyncData::SlaveSyncData()
{
}

SlaveSyncData::~SlaveSyncData()
{
}
