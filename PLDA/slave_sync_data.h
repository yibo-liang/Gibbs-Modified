#pragma once

#ifndef SLAVE_SYNCHRONIZATION_DATA
#define SLAVE_SYNCHRONIZATION_DATA
#include "shared_header.h"

class SlaveSyncData
{
public:
	hashmap<int, hashmap<int,int>> ndDiff; //nd difference, stored in map, maximum size = M * K, will be smaller over the iterations
	hashmap<int, hashmap<int,int>> nwDiff; //nw difference, maximum size = V * K, will be smaller as well
	hashmap<int, int> nwsumDiff;//nwsum difference, 

	SlaveSyncData(const SlaveSyncData& d);
	SlaveSyncData();
	~SlaveSyncData();

private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & BOOST_SERIALIZATION_NVP(ndDiff);
		ar & BOOST_SERIALIZATION_NVP(nwDiff);
		ar & BOOST_SERIALIZATION_NVP(nwsumDiff);
	}
};

#endif // !SYNCHRONIZATION_DATA
