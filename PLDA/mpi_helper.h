#pragma once
#include <mpi.h>
#include <vector>
#include <string>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/back_inserter.hpp>

namespace MPIHelper {

	const int lentag = 0;
	const int datatag = 1;
	/* OOP send function for mpi, send object type N to process n*/
	/* Send and Receive implementation is trivial using boost turning object into char stream, then sent through*/
	template<typename N>
	int mpiSend(N obj, int proc_n) {
		std::string serialString;
		boost::iostreams::back_insert_device<std::string> inserter(serialString);
		boost::iostreams::stream<boost::iostreams::back_insert_device<std::string> > stream(inserter);
		boost::archive::binary_oarchive sendArchive(stream);

		sendArchive << obj;
		stream.flush();

		int len = serialString.size();

		MPI_Send(&len, 1, MPI_INT, proc_n, lentag, MPI_COMM_WORLD);
		MPI_Send((void *)serialString.data(), len, MPI_BYTE, 1, datatag, MPI_COMM_WORLD);


	}

	template<typename N>
	N mpiReceive(int proc_n) {

		int len;
		MPI_Recv(&len, 1, MPI_INT, proc_n, lentag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

		char *data = new char[len + 1];
		MPI_Recv(data, len, MPI_BYTE, proc_n, datatag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		data[len] = '\0';

		boost::iostreams::basic_array_source<char> device(data, len);
		boost::iostreams::stream<boost::iostreams::basic_array_source<char> > stream(device);
		boost::archive::binary_iarchive receiveArchive(stream);

		N obj;
		receiveArchive >> obj;
		delete [] data;
		return obj;

	}

	/* OOP broadcast */
	template<typename N>
	void mpiBroadCast(N &obj, int ROOT, int currentProc) {

		std::string serialString;
		if (ROOT == currentProc) {
			boost::iostreams::back_insert_device<std::string> inserter(serialString);
			boost::iostreams::stream<boost::iostreams::back_insert_device<std::string> > stream(inserter);
			boost::archive::binary_oarchive sendArchive(stream);
			sendArchive << obj;
			stream.flush();
		}

		int len = serialString.size();
		MPI_Bcast(&len, 1, MPI_INT, ROOT, MPI_COMM_WORLD);

		char * buffer;
		if (ROOT == currentProc) {
			buffer = serialString.c_str();
		}
		else {
			//buffer = (void*)malloc((len+1) * sizeof(char));
			buffer = new char[len + 1];
		}
		MPI_Bcast(buffer, len, MPI_BYTE, ROOT, MPI_COMM_WORLD);

		if (ROOT != currentProc) {
			boost::iostreams::basic_array_source<char> device(data, len);
			boost::iostreams::stream<boost::iostreams::basic_array_source<char> > stream(device);
			boost::archive::binary_iarchive receiveArchive(stream);
			receiveArchive >> obj;
		}
		delete [] buffer;

	}

}