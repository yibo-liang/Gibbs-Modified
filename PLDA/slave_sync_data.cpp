#include "slave_sync_data.h"

SlaveSyncData::SlaveSyncData(const SlaveSyncData & d)
{
	this->pid = d.pid;
	this->task_id = d.task_id;
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
