#pragma once
#ifndef MPI_HELPER
#define MPI_HELPER

#define LOGGING 1

#include <mpi.h>
#include <vector>
#include <string>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/back_inserter.hpp>

namespace MPIHelper {
	const int ROOT = 0;
	const int lentag = 0;
	const int datatag = 1;

	/* OOP send function for mpi, send object type N to process n*/
	/* Send and Receive implementation is trivial using boost turning object into char stream, then sent through*/
	template<typename N>
	int mpiSend(N & obj, int receiverProc_n) {

		std::string serialString;
		boost::iostreams::back_insert_device<std::string> inserter(serialString);
		boost::iostreams::stream<boost::iostreams::back_insert_device<std::string> > stream(inserter);
		boost::archive::binary_oarchive sendArchive(stream);

		sendArchive << BOOST_SERIALIZATION_NVP(obj);
		stream.flush();

		int len = serialString.size();

		MPI_Send(&len, 1, MPI_INT, receiverProc_n, lentag, MPI_COMM_WORLD);
		//cout << "Length sent. len=" << len << ", to proc_n=" << receiverProc_n << endl;
		MPI_Send((void *)serialString.data(), len, MPI_BYTE, receiverProc_n, datatag, MPI_COMM_WORLD);
		//cout << "Object sent." << ", to proc_n=" << receiverProc_n << endl;
		return 0;
	}


	template<typename N>
	int mpiReceive2(N& obj, int senderProc_n) { //second version uses pointer

		int len;

		//cout << "Receiving Object Lenth" << ", proc_n=" << senderProc_n << endl;
		MPI_Recv(&len, 1, MPI_INT, senderProc_n, lentag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

		//cout << "Length received. len=" << len << ", proc_n=" << senderProc_n << endl;
		char *data = new char[len + 1];
		MPI_Recv(data, len, MPI_BYTE, senderProc_n, datatag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

		//cout << "Object Received, proc_n=" << senderProc_n << endl;
		data[len] = '\0';

		boost::iostreams::basic_array_source<char> device(data, len);
		boost::iostreams::stream<boost::iostreams::basic_array_source<char> > stream(device);
		boost::archive::binary_iarchive receiveArchive(stream);

		
		receiveArchive >> obj;
		delete[] data;
		

		return 0;

	}


	template<typename N>
	N mpiReceive(int senderProc_n) {

		int len;

		//cout << "Receiving Object Lenth" << ", proc_n=" << senderProc_n << endl;
		MPI_Recv(&len, 1, MPI_INT, senderProc_n, lentag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

		//cout << "Length received. len=" << len << ", proc_n=" << senderProc_n << endl;
		char *data = new char[len + 1];
		MPI_Recv(data, len, MPI_BYTE, senderProc_n, datatag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

		//cout << "Object Received, proc_n=" << senderProc_n << endl;
		data[len] = '\0';

		boost::iostreams::basic_array_source<char> device(data, len);
		boost::iostreams::stream<boost::iostreams::basic_array_source<char> > stream(device);
		boost::archive::binary_iarchive receiveArchive(stream);

		N obj;
		receiveArchive >> obj;
		delete[] data;
		return obj;

	}

	/* OOP broadcast */
	template<typename N>
	void mpiBroadCast(N & obj, int ROOT, int currentProc) {

		std::string serialString;
		if (ROOT == currentProc) {
			boost::iostreams::back_insert_device<std::string> inserter(serialString);
			boost::iostreams::stream<boost::iostreams::back_insert_device<std::string> > stream(inserter);
			boost::archive::binary_oarchive sendArchive(stream);
			sendArchive << BOOST_SERIALIZATION_NVP(obj);
			stream.flush();
		}
		//broadcast length
		int len = serialString.size();
		MPI_Bcast(&len, 1, MPI_INT, ROOT, MPI_COMM_WORLD);

		char * buffer;
		if (ROOT == currentProc) {
			buffer = &serialString[0];
		}
		else {
			//buffer = (void*)malloc((len+1) * sizeof(char));
			buffer = new char[len + 1];
		}
		//broadcast object

		MPI_Bcast(buffer, len, MPI_BYTE, ROOT, MPI_COMM_WORLD);
		
		if (ROOT != currentProc) {
			buffer[len] = '\0';
			boost::iostreams::basic_array_source<char> device(buffer, len);
			boost::iostreams::stream<boost::iostreams::basic_array_source<char> > stream(device);
			boost::archive::binary_iarchive receiveArchive(stream);
			receiveArchive >> obj;
			delete[] buffer;
		}

	}

}

#endif // !MPI_HELPER

