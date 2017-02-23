#pragma once
#ifndef _CL_PARTITION_DATA
#define _CL_PARTITION_DATA
#include "shared_header.h"
#include "CL/cl.h"

class clPartitionData
{
public:

	int * w;
	int * z;
	

	clPartitionData();
	~clPartitionData();
};

#endif // !_CL_PARTITION_DATA

