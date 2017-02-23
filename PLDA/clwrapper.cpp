#include "clwrapper.h"
#include <string>
#include <fstream>

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
	int d_id = 0;
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

		//test time
		double elapsed = clTimer.elapsed();
		if (displayInformation)
			cout << "Benchmark on " << std::get<0>(devicesInformation[d_id]) << " : \t" << elapsed << " seconds" << endl;
		if (elapsed < min_time) {
			min_time = elapsed;
			selected_device = device;
			select_id = d_id;
		}


		ret = clFlush(command_queue);
		ret = clFinish(command_queue);
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
					clock_frequency
					)
				);

		}
	}

	benchmark();
}

void clWrapper::release()
{
}

clWrapper::clWrapper()
{
}


clWrapper::~clWrapper()
{
}
