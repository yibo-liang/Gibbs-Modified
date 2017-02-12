#pragma once

#include <iostream>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <unordered_map>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/program_options.hpp>

#define P_MPI 10001
#define P_GPU 10002
#define P_HIBRID 10003

#define NAIVE_HIERARCHICAL_MODEL 801
#define NAIVE2_HIERARCHICAL_MODEL 802



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
