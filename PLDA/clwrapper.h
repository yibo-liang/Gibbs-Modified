#pragma once


#ifndef _CL_WRAPPER
#define _CL_WRAPPER
#include "shared_header.h"
#include "CL/cl.h"
#include <tuple>

using deviceInfo = tuple<string, cl_uint, cl_uint>;
using platformInfo = tuple<string, cl_uint>;

using namespace fastVector2D;
class clWrapper
{
public:
	bool displayInformation = false;

	vector<cl_platform_id> platforms;
	vector<cl_device_id> devices;
	vector<deviceInfo> devicesInformation;
	vector<platformInfo> platformInformation;

	cl_device_id selected_device;

	cl_context context;
	cl_command_queue queue;
	cl_program program;
	cl_kernel kernel;

	vecFast2D<int> partition_offset; // size P * P, each is a pointer position offset on buffer 'words'
	vecFast2D<int> partition_word_count; //
	vecFast2D<int> words; //words[i]=(m of word_i, w of word_i)
	int * z; //z[w_i]

	void benchmark();
	void initialise();
	void release();

	clWrapper();
	~clWrapper();
};


#endif