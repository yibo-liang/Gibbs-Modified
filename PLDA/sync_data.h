#pragma once

#ifndef SYNCHRONIZATION_DATA
#define SYNCHRONIZATION_DATA
#include "shared_header.h"

class SynchronisationData
{
public:
	map<int, map<int, int>> ndDiff; //nd difference, stored in map, maximum size = M * K, will be smaller over the iterations
	map<int, map<int, int>> nwDiff; //nw difference, maximum size = V * K, will be smaller as well
	map<int, int> nwsumDiff;//nwsum difference, 

	SynchronisationData(const SynchronisationData& d);
	SynchronisationData();
	~SynchronisationData();

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
