#include "master_sync_data.h"

MasterSyncData::MasterSyncData(const MasterSyncData & d)
{
	nw = d.nw;
	nd = d.nd;
	ndsum = d.ndsum;
	
}

MasterSyncData::MasterSyncData(int nullArg)
{
	this->isNull = (nullArg == 0);
}

MasterSyncData::MasterSyncData()
{
}

MasterSyncData::~MasterSyncData()
{
}
