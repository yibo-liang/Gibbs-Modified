#pragma once

#ifndef MASTER_SYNCHRONIZATION_DATA
#define MASTER_SYNCHRONIZATION_DATA
#include "shared_header.h"

class MasterSyncData
{
public:

	bool isNull = false;
	vec2d<int> nw;
	vec2d<int> nd;
	vector<int> ndsum;

	MasterSyncData(const MasterSyncData& d);
	MasterSyncData(int nullArg);
	MasterSyncData();
	~MasterSyncData();

private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & BOOST_SERIALIZATION_NVP(nw);
		ar & BOOST_SERIALIZATION_NVP(nd);
		ar & BOOST_SERIALIZATION_NVP(ndsum);
	}
};

#endif