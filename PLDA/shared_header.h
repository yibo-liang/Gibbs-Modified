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
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/program_options.hpp>
#include <stdexcept>

#include "timer.h"

#define P_MPI 10001
#define P_GPU 10002
#define P_HIBRID 10003

#define NAIVE_HIERARCHICAL_MODEL 801
#define NAIVE2_HIERARCHICAL_MODEL 802


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


template<typename A, typename B>
using hashmap = unordered_map<A, B>;

namespace fastVector2D {

	//1D array
	template<typename N>
	using vecFast = N *;


	//2D array stored as 1D
	template<typename N>
	using vecFast2D = N *;

	template<typename N>
	inline N* newVec2D(size_t row_size, size_t col_size) {
		N* result= (N*)malloc(row_size * col_size * sizeof(N));
		if (result == NULL) {
			throw 20;
		}
		return result;
	}

	template<typename N>
	inline N readvec2D(vecFast2D<N> vec, size_t x, size_t y, size_t col_size) {
		//read 1D vector in a 2D manner, as we are reading vec[x][y]; where col_size is the column size of this 2D array.
		return vec[y + x*col_size];
	}

	template<typename N>
	inline void writevec2D(N val, vecFast2D<N> vec, size_t x, size_t y, size_t col_size) {
		//write function same for above purpose
		vec[y + x*col_size] = val;
	}



	template<typename N>
	inline void fill2D(vecFast2D<N> vec, const N& value, size_t row_size, size_t col_size) {
		std::fill(vec, vec + row_size * col_size, value);
	}

	template<typename N>
	inline void plusIn2D(vecFast2D<N> vec, const N& value, size_t x, size_t y, size_t col_size) {
		vec[y + x*col_size] += value;
	}

}
