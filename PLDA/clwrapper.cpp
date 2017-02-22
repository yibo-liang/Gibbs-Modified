#include "clwrapper.h"



void clwrapper::benchmark()
{
}

void clwrapper::initialise()
{
	cl_int i, err;

	cl_uint num_devices, addr_data;

	/* Extension data */

	const cl_uint PlatformEntriesLen = 16;
	const size_t PlatformProfileLen = 256;

	cl_uint num_platforms = 0;
	cl_platform_id platforms[PlatformEntriesLen];

	char name_data[256], ext_data[4096];

	/* Identify platforms */
	err = clGetPlatformIDs(PlatformEntriesLen,
		platforms,
		&num_platforms);
	if (err < 0) {
		perror("Couldn't find any platforms");
		exit(1);
	}

	/* Save all platform information, and enumerate device on each platform */
	std::cout << "Platforms:" << std::endl;
	for (cl_uint i = 0; i < num_platforms; ++i) {

		char platformProfile[PlatformProfileLen];
		size_t platformProfileRet = 0;

		if (clGetPlatformInfo(platforms[i],
			CL_PLATFORM_NAME,
			PlatformProfileLen,
			platformProfile,
			&platformProfileRet)
			!= CL_SUCCESS) {
			std::cerr << "[ERROR] Unable to obtain platform info";
			throw 500;
		}
		platformInformation.push_back(
			platformInfo(
				string(platformProfile), 
				platformProfileRet
			)
		);

		err = clGetDeviceIDs(platform,
			CL_DEVICE_TYPE_GPU,
			1,
			NULL,
			&num_devices);

		if (err < 0) {
			std::cerr << "[ERROR] Couldn't find any devices on Platform id=" << i << endl;
		}


		int device_count = devices.size();
		devices.reserve(num_devices+devices.capacity());

		/* Access connected devices */
		clGetDeviceIDs(platform,
			CL_DEVICE_TYPE_GPU,
			num_devices,
			&devices[device_count],
			NULL);

		/* Obtain data for each connected device */
		for (i = 0; i < num_devices; i++) {

			err = clGetDeviceInfo(devices[i], CL_DEVICE_NAME,
				sizeof(name_data), name_data, NULL);
			if (err < 0) {
				std::cerr << "ERROR! Couldn't read extension data" <<endl;
				exit(1);
			}
			clGetDeviceInfo(devices[i], CL_DEVICE_ADDRESS_BITS,
				sizeof(ext_data), &addr_data, NULL);

			clGetDeviceInfo(devices[i], CL_DEVICE_EXTENSIONS,
				sizeof(ext_data), ext_data, NULL);


			
			printf("\n\tNAME: %s\n\tADDRESS_WIDTH: %u\n\tEXTENSIONS: %s\n",
				name_data, addr_data, ext_data);
			devicesInformation.push_back(
				deviceInfo(
					string(name_data),
					addr_data, 
					string(ext_data)
				)
			);

		}
	}


	


}

void clwrapper::release()
{
}

clwrapper::clwrapper()
{
}


clwrapper::~clwrapper()
{
}
