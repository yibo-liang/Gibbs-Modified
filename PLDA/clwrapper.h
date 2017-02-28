#pragma once


#ifndef _CL_WRAPPER
#define _CL_WRAPPER
#include "shared_header.h"
#include "CL/cl.h"
#include <tuple>

using deviceInfo = tuple<string, cl_uint, cl_uint, int>;
using platformInfo = tuple<string, cl_uint>;

using namespace fastVector2D;
class clWrapper
{
public:
	bool displayInformation = false;

	const int SAMPLING_KERNEL = 0;
	const int REDUCE_KERNEL = 1;

	const int MEM_iter = 0;
	const int MEM_partition_offset = 1;
	const int MEM_partition_word_count = 2;
	const int MEM_w = 3;
	const int MEM_z = 4;
	const int MEM_nd = 5;
	const int MEM_nw = 6;
	const int MEM_ndsum = 7;
	const int MEM_nwsum_unsync = 8;
	const int MEM_nwsum_local = 9;
	const int MEM_nwsum_global = 10;

	const int MEM_DEBUG = 11;

	const int MEM_INDEX[12] = { 0,1,2,3,4,5,6,7,8,9,10,11 };

	vector<cl_platform_id> platforms;
	vector<cl_device_id> devices;
	vector<deviceInfo> devicesInformation;
	vector<platformInfo> platformInformation;

	cl_platform_id selected_platform;
	cl_device_id selected_device;

	cl_context context;
	cl_command_queue command_queue;
	cl_program program;
	cl_kernel kernels[2];
	cl_mem memoryObjects[11];

	vector<int> partition_offset; // size P * P, each is a pointer position offset on buffer 'words'
	vector<int> partition_word_count; //
	
	int partition_root_size;
	int wordCount;
	vecFast2D<int> words; //words[i]=(m of word_i, w of word_i)

	int K;
	int V;
	int partialV;
	
	float alpha;
	float beta;

	int M;
	int * z; //z[w_i]
	int * nw;
	int * nd;
	int * ndsum;
	int * nwsum;

	void initialise();
	void release();
	void sample();
	void readFromDevice();

	clWrapper();
	~clWrapper();

private:
	void benchmark();
	void prepareSampling();

	void writeBuffer(cl_mem & cl_memobj, void * buffer, size_t size);
	

};


#endif