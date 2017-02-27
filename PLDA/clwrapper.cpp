#include "clwrapper.h"
#include <string>
#include <fstream>
#include <boost/format.hpp>

#define MAX_SOURCE_SIZE (0x100000)

inline string read_file(string filename) {
	std::ifstream t(filename);
	std::stringstream buffer;
	buffer << t.rdbuf();
	return buffer.str();
}

inline cl_int clDebug(cl_int ret) {
	if (ret != CL_SUCCESS) {
		cout << "OPENCL ERROR: " << ret << endl;
	}
	return ret;
}

void clWrapper::benchmark()
{

	const int LIST_SIZE = 10240;
	int *A = (int*)malloc(sizeof(int)*LIST_SIZE);
	int *B = (int*)malloc(sizeof(int)*LIST_SIZE);
	int *C = (int*)malloc(sizeof(int)*LIST_SIZE);

	for (int i = 0; i < LIST_SIZE; i++) {
		A[i] = i;
		B[i] = LIST_SIZE - i;
	}

	Timer clTimer;

	double min_time = 9999999;
	int d_id = 0; //device counter
	int select_id = 0;
	for (cl_device_id device : devices) {


		const char *source_str;
		size_t source_size;

		string rng_kernel = read_file("cl_lib/rng_simplified.cl");
		string benchmark_kernel = read_file("cl_lib/naive_benchmark.cl");
		string final_kernel = rng_kernel + benchmark_kernel;

		source_str = final_kernel.c_str();
		source_size = final_kernel.length();

		cl_int ret;
		cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, &ret);
		cl_command_queue command_queue = clCreateCommandQueue(context, device, 0, &ret);

		// Create memory buffers on the device for each vector 
		cl_mem a_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY,
			LIST_SIZE * sizeof(int), NULL, &ret);
		cl_mem b_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY,
			LIST_SIZE * sizeof(int), NULL, &ret);
		cl_mem c_mem_obj = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
			LIST_SIZE * sizeof(int), NULL, &ret);

		clDebug(
			clEnqueueWriteBuffer(command_queue, a_mem_obj, CL_TRUE, 0,
				LIST_SIZE * sizeof(int), A, 0, NULL, NULL)
			);

		clDebug(
			clEnqueueWriteBuffer(command_queue, b_mem_obj, CL_TRUE, 0,
				LIST_SIZE * sizeof(int), B, 0, NULL, NULL)
			);

		cl_program program = clCreateProgramWithSource(context, 1,
			(const char **)&source_str, (const size_t *)&source_size, &ret);

		ret = clDebug(
			clBuildProgram(program, 1, &device, NULL, NULL, NULL)
			);

		if (ret == CL_BUILD_PROGRAM_FAILURE) {
			// Determine the size of the log
			size_t log_size;
			clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);

			// Allocate memory for the log
			char *log = (char *)malloc(log_size);

			// Get the log
			clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);

			// Print the log
			printf("%s\n", log);
			delete[] log;
			throw 500;
		}

		cl_kernel kernel = clCreateKernel(program, "vector_add", &ret);

		clTimer.reset();
		// Set the arguments of the kernel
		ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&a_mem_obj);
		ret = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&b_mem_obj);
		ret = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&c_mem_obj);
		size_t global_item_size = LIST_SIZE; // Process the entire lists
		size_t local_item_size = 64; // Divide work items into groups of 64
		ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL,
			&global_item_size, &local_item_size, 0, NULL, NULL);

		// Read the memory buffer C on the device to the local variable C
		ret = clDebug(
			clEnqueueReadBuffer(command_queue, c_mem_obj, CL_TRUE, 0,
				LIST_SIZE * sizeof(int), C, 0, NULL, NULL)
			);

		ret = clFlush(command_queue);
		ret = clFinish(command_queue);
		//test time
		double elapsed = clTimer.elapsed();
		if (displayInformation)
			cout << "Benchmark on " << std::get<0>(devicesInformation[d_id]) << " : \t" << elapsed << " seconds" << endl;
		if (elapsed < min_time) {
			min_time = elapsed;
			selected_device = device;
			select_id = d_id;
		}


		ret = clReleaseKernel(kernel);
		ret = clReleaseProgram(program);
		ret = clReleaseMemObject(a_mem_obj);
		ret = clReleaseMemObject(b_mem_obj);
		ret = clReleaseMemObject(c_mem_obj);
		ret = clReleaseCommandQueue(command_queue);
		ret = clReleaseContext(context);
		d_id++;
	}


	free(A);
	free(B);
	free(C);
	if (displayInformation)
		cout << "\nAutomatically select Device: " << std::get<0>(devicesInformation[select_id]) << endl;

	int platform_id = std::get<3>(devicesInformation[select_id]);
	selected_platform = platforms[platform_id];
}

void clWrapper::prepareSampling()
{


	string rng_kernel = read_file("cl_lib/rng_simplified.cl");
	string sampling_kernel = read_file("cl_lib/lda_sample.cl");

	boost::format sampleKernelFormmater = boost::format(sampling_kernel)
		% partition_number % K % K % V % M % alpha % beta //kernel sample
		% K % partition_number;  //

	sampling_kernel = rng_kernel + sampleKernelFormmater.str();
	const char * kernel_source = sampling_kernel.c_str();
	int kernel_length = sampling_kernel.size();

	cl_int ret;
	//build kernel
	{
		context = clCreateContext(NULL, 1, &selected_device, NULL, NULL, &ret);
		command_queue = clCreateCommandQueue(context, selected_device, 0, &ret);
		program = clCreateProgramWithSource(context, 1,
			(const char **)&kernel_source, NULL, &ret);
		ret = clBuildProgram(program, 1, &selected_device, NULL, NULL, NULL);

		if (ret == CL_BUILD_PROGRAM_FAILURE) {
			// Determine the size of the log
			size_t log_size;
			clGetProgramBuildInfo(program, selected_device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);

			// Allocate memory for the log
			char *log = (char *)malloc(log_size);

			// Get the log
			clGetProgramBuildInfo(program, selected_device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);

			// Print the log
			printf("%s\n", log);
			delete[] log;
			throw 500;
		}

		kernels[SAMPLING_KERNEL] = clCreateKernel(program, "sample", &ret);
		kernels[REDUCE_KERNEL] = clCreateKernel(program, "nwsum_allreduce", &ret);

	}
	// Create memory buffers on the device for each vector 
	{
		int iter = 0;
		memoryObjects[MEM_iter] = clCreateBuffer(context, CL_MEM_READ_WRITE, 1 * sizeof(int), NULL, &ret);
		writeBuffer(memoryObjects[MEM_iter], &iter, 1);

		memoryObjects[MEM_partition_offset] = clCreateBuffer(context, CL_MEM_READ_ONLY, partition_number * partition_number * sizeof(int), NULL, &ret);
		writeBuffer(memoryObjects[MEM_partition_offset], &partition_offset[0], partition_number * partition_number);

		memoryObjects[MEM_partition_word_count] = clCreateBuffer(context, CL_MEM_READ_ONLY, partition_number * partition_number * sizeof(int), NULL, &ret);
		writeBuffer(memoryObjects[MEM_partition_word_count], &partition_word_count[0], partition_number * partition_number);

		memoryObjects[MEM_w] = clCreateBuffer(context, CL_MEM_READ_ONLY, wordCount * 2 * sizeof(int), NULL, &ret);
		writeBuffer(memoryObjects[MEM_w], words, wordCount * 2);

		memoryObjects[MEM_z] = clCreateBuffer(context, CL_MEM_READ_WRITE, wordCount * sizeof(int), NULL, &ret);
		writeBuffer(memoryObjects[MEM_z], z, wordCount);

		memoryObjects[MEM_nd] = clCreateBuffer(context, CL_MEM_READ_WRITE, M*K * sizeof(int), NULL, &ret);
		writeBuffer(memoryObjects[MEM_nd], nd, M*K);

		memoryObjects[MEM_nw] = clCreateBuffer(context, CL_MEM_READ_WRITE, partialV*K * sizeof(int), NULL, &ret);
		writeBuffer(memoryObjects[MEM_nw], nw, partialV*K);

		memoryObjects[MEM_ndsum] = clCreateBuffer(context, CL_MEM_READ_ONLY, M * sizeof(int), NULL, &ret);
		writeBuffer(memoryObjects[MEM_ndsum], ndsum, M);

		memoryObjects[MEM_nwsum_unsync] = clCreateBuffer(context, CL_MEM_READ_WRITE, K * partition_number * sizeof(int), NULL, &ret);
		vector<int> temp(K* partition_number, 0);
		writeBuffer(memoryObjects[MEM_nwsum_unsync], &temp[0], K * partition_number);

		memoryObjects[MEM_nwsum_global] = clCreateBuffer(context, CL_MEM_READ_WRITE, K * sizeof(int), NULL, &ret);
		writeBuffer(memoryObjects[MEM_nwsum_global], nwsum, K);

		int debug_size = 1024 * partition_number;
		vector<int> temp2(debug_size, 0);
		memoryObjects[MEM_DEBUG] = clCreateBuffer(context, CL_MEM_READ_WRITE, debug_size * sizeof(int), NULL, &ret);
		writeBuffer(memoryObjects[MEM_DEBUG], &temp2[0], debug_size);


	}
	// Set kernel arguments
	{
		ret = clSetKernelArg(kernels[SAMPLING_KERNEL], 0, sizeof(cl_mem), (void *)&memoryObjects[MEM_iter]);
		ret = clSetKernelArg(kernels[SAMPLING_KERNEL], 1, sizeof(cl_mem), (void *)&memoryObjects[MEM_partition_offset]);
		ret = clSetKernelArg(kernels[SAMPLING_KERNEL], 2, sizeof(cl_mem), (void *)&memoryObjects[MEM_partition_word_count]);
		ret = clSetKernelArg(kernels[SAMPLING_KERNEL], 3, sizeof(cl_mem), (void *)&memoryObjects[MEM_w]);
		ret = clSetKernelArg(kernels[SAMPLING_KERNEL], 4, sizeof(cl_mem), (void *)&memoryObjects[MEM_z]);
		ret = clSetKernelArg(kernels[SAMPLING_KERNEL], 5, sizeof(cl_mem), (void *)&memoryObjects[MEM_nd]);
		ret = clSetKernelArg(kernels[SAMPLING_KERNEL], 6, sizeof(cl_mem), (void *)&memoryObjects[MEM_nw]);
		ret = clSetKernelArg(kernels[SAMPLING_KERNEL], 7, sizeof(cl_mem), (void *)&memoryObjects[MEM_ndsum]);
		ret = clSetKernelArg(kernels[SAMPLING_KERNEL], 8, sizeof(cl_mem), (void *)&memoryObjects[MEM_nwsum_unsync]);
		ret = clSetKernelArg(kernels[SAMPLING_KERNEL], 9, K * sizeof(int), NULL);
		ret = clSetKernelArg(kernels[SAMPLING_KERNEL], 10, sizeof(cl_mem), (void *)&memoryObjects[MEM_nwsum_global]);
		ret = clSetKernelArg(kernels[SAMPLING_KERNEL], 11, sizeof(cl_mem), (void *)&memoryObjects[MEM_DEBUG]);


		ret = clSetKernelArg(kernels[REDUCE_KERNEL], 0, sizeof(cl_mem), (void *)&memoryObjects[MEM_iter]);
		ret = clSetKernelArg(kernels[REDUCE_KERNEL], 1, sizeof(cl_mem), (void *)&memoryObjects[MEM_nwsum_unsync]);
		ret = clSetKernelArg(kernels[REDUCE_KERNEL], 2, sizeof(cl_mem), (void *)&memoryObjects[MEM_nwsum_global]);

	}



}

void clWrapper::writeBuffer(cl_mem & cl_memobj, void * buffer, size_t size)
{
	clDebug(
		clEnqueueWriteBuffer(command_queue, cl_memobj, CL_TRUE, 0, size * sizeof(int), buffer, 0, NULL, NULL)
		);
}

void clWrapper::initialise()
{
	cl_int i, err;

	cl_platform_id platform;
	cl_uint num_devices, compute_unit, clock_frequency;

	/* Extension data */

	const cl_uint PlatformEntriesLen = 16;
	const size_t PlatformProfileLen = 256;

	cl_uint num_platforms = 0;
	cl_platform_id platforms_temp[PlatformEntriesLen];

	char name_data[256];

	/* Identify platforms */
	err = clGetPlatformIDs(PlatformEntriesLen,
		platforms_temp,
		&num_platforms);
	if (err < 0) {
		perror("Couldn't find any platforms");
		throw 404;
	}

	/* Save all platform information, and enumerate device on each platform */
	std::cout << "Platforms:" << std::endl;
	for (cl_uint i = 0; i < num_platforms; ++i) {

		char platformProfile[PlatformProfileLen];
		size_t platformProfileRet = 0;

		if (clGetPlatformInfo(platforms_temp[i],
			CL_PLATFORM_NAME,
			PlatformProfileLen,
			platformProfile,
			&platformProfileRet)
			!= CL_SUCCESS) {
			std::cerr << "[ERROR] Unable to obtain platform info";
			throw 500;
		}
		if (displayInformation)
			cout << platformProfile << ":" << endl;
		platforms.push_back(platforms_temp[i]);
		platformInformation.push_back(
			platformInfo(
				string(platformProfile),
				platformProfileRet
				)
			);

		err = clGetDeviceIDs(platforms[i],
			CL_DEVICE_TYPE_GPU,
			1,
			NULL,
			&num_devices);

		if (err < 0) {
			std::cerr << "[ERROR] Couldn't find any devices on Platform id=" << i << endl;
		}


		int old_device_count = devices.size();
		devices.resize(num_devices + devices.capacity());

		/* Access connected devices */
		clGetDeviceIDs(platforms[i],
			CL_DEVICE_TYPE_GPU,
			num_devices,
			&devices[old_device_count],
			NULL);

		/* Obtain data for each connected device */
		for (int j = 0; j < num_devices; j++) {

			err = clGetDeviceInfo(devices[j + old_device_count], CL_DEVICE_NAME,
				sizeof(name_data), name_data, NULL);
			if (err < 0) {
				std::cerr << "ERROR! Couldn't read extension data" << endl;
				throw 404;
			}


			clGetDeviceInfo(devices[j + old_device_count], CL_DEVICE_MAX_COMPUTE_UNITS,
				sizeof(compute_unit), &compute_unit, NULL);

			clGetDeviceInfo(devices[j + old_device_count], CL_DEVICE_MAX_CLOCK_FREQUENCY,
				sizeof(clock_frequency), &clock_frequency, NULL);

			if (displayInformation)
				printf("\tNAME: %s\n", name_data);

			devicesInformation.push_back(
				deviceInfo(
					string(name_data),
					compute_unit,
					clock_frequency,
					i
					)
				);

		}
	}

	benchmark();
	prepareSampling();
}

void clWrapper::release()
{
	cl_int ret;
	ret = clReleaseKernel(kernels[SAMPLING_KERNEL]);
	ret = clReleaseKernel(kernels[REDUCE_KERNEL]);

	ret = clReleaseProgram(program);

	for (int i = 0; i < 10; i++) {
		ret = clReleaseMemObject(memoryObjects[i]);
	}

	ret = clReleaseCommandQueue(command_queue);
	ret = clReleaseContext(context);
	cout << "All resources released." << endl;
}

inline size_t ceilUpToMultiple(size_t num, size_t base) {

	size_t result = ((num / base) + 1) * base;
	return result;

}

void clWrapper::sample()
{
	size_t local_workgroup_dim = 1024;
	cl_int ret;
	ret = clGetKernelWorkGroupInfo(kernels[SAMPLING_KERNEL], selected_device, CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &local_workgroup_dim, NULL);



	size_t global_item_size = local_workgroup_dim * partition_number;
	//sample all diagonally
	for (int i = 0; i < partition_number; i++) {
		ret = clEnqueueNDRangeKernel(command_queue, kernels[SAMPLING_KERNEL], 1, NULL,
			&global_item_size, &local_workgroup_dim, 0, NULL, NULL);
		//ret = clEnqueueNDRangeKernel(command_queue, kernels[REDUCE_KERNEL], 1, NULL,
		//	&local_workgroup_dim, &local_workgroup_dim, 0, NULL, NULL);


		clFinish(command_queue);
		int debug_size = global_item_size;
		vector<int> temp(debug_size, 0);
		ret = clEnqueueReadBuffer(command_queue, memoryObjects[MEM_DEBUG], CL_TRUE, 0, debug_size * sizeof(int), &temp[0], 0, NULL, NULL);
		cout << "One part done" << endl;
		//copy result from device
		ret = clEnqueueReadBuffer(command_queue, memoryObjects[MEM_nw], CL_TRUE, 0, partialV * K * sizeof(int), nw, 0, NULL, NULL);
		ret = clEnqueueReadBuffer(command_queue, memoryObjects[MEM_nwsum_global], CL_TRUE, 0, K * sizeof(int), nwsum, 0, NULL, NULL);
		ret = clEnqueueReadBuffer(command_queue, memoryObjects[MEM_z], CL_TRUE, 0, wordCount * sizeof(int), z, 0, NULL, NULL);
		ret = clEnqueueReadBuffer(command_queue, memoryObjects[MEM_nd], CL_TRUE, 0, M * K * sizeof(int), nd, 0, NULL, NULL);
	}



}

clWrapper::clWrapper()
{
}


clWrapper::~clWrapper()
{
	release();
	//delete[] partition_word_count;
	//delete[] partition_offset;
}
