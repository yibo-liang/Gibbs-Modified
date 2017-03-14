#pragma once

#include <iostream>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <map>
#include <unordered_map>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/format.hpp>
#include <stdexcept>

#include <sys/stat.h>
#include "timer.h"

#define P_MPI 10001
#define P_GPU 10002
#define P_HIBRID 10003

#define NAIVE_HIERARCHICAL_MODEL 801
#define NAIVE2_HIERARCHICAL_MODEL 802

namespace ParallelHLDA {

	using std::clock;
	using std::vector;
	using std::string;
	using std::map;
	using std::unordered_map;
	using std::tuple;
	using std::cout;
	using std::cin;
	using std::endl;
	template<typename N>
	using vec2d = vector<vector<N>>;


	template<typename N>
	void saveSerialisable(const N & obj, string filename) {
		using namespace boost;

		std::ofstream ofs(filename, std::ofstream::binary | std::ofstream::out);
		archive::binary_oarchive ta(ofs);
		ta << obj;
		ofs.close();
	}


	template<typename N>
	N loadSerialisable(string filename) {
		using namespace boost;

		N obj;
		std::ifstream ifs(filename, std::ifstream::binary | std::ifstream::in);
		archive::binary_iarchive ta(ifs);

		ta >> obj;
		ifs.close();

		return obj;
	}

	template<typename N>
	void loadSerialisable2(N& obj, string filename) {
		using namespace boost;

		std::ifstream ifs(filename, std::ifstream::binary | std::ifstream::in);
		archive::binary_iarchive ta(ifs);

		ta >> obj;
		ifs.close();

		return;
	}


	template<typename A, typename B>
	using hashmap = unordered_map<A, B>;

	inline int RandInteger(int min, int max)
	{
		int range = max - min + 1;
		int num = rand() % range + min;

		return num;
	}
	namespace fastVector2D {

		//1D array
		template<typename N>
		using vecFast = N *;


		//2D array stored as 1D
		template<typename N>
		using vecFast2D = N *;

		template<typename N>
		inline N* newVec2D(size_t row_size, size_t col_size) {
			N* result = (N*)malloc(row_size * col_size * sizeof(N));
			if (result == NULL) {
				throw 20;
			}
			return result;
		}

		template<typename N>
		inline N readvec2D(vecFast2D<N> vec, size_t row_num, size_t col_num, size_t col_size) {
			//read 1D vector in a 2D manner, as we are reading vec[x][y]; where col_size is the column size of this 2D array.
			return vec[col_num + row_num*col_size];
		}

		template<typename N>
		inline void writevec2D(N val, vecFast2D<N> vec, size_t row_num, size_t col_num, size_t col_size) {
			//write function same for above purpose
			vec[col_num + row_num*col_size] = val;
		}



		template<typename N>
		inline void fill2D(vecFast2D<N> vec, const N& value, size_t row_size, size_t col_size) {
			std::fill(vec, vec + row_size * col_size, value);
		}

		template<typename N>
		inline void plusIn2D(vecFast2D<N> vec, const N& value, size_t row_num, size_t col_num, size_t col_size) {
			vec[col_num + row_num*col_size] += value;
		}



	}
}