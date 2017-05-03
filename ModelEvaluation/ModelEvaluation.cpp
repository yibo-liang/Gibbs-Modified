// ModelEvaluation.cpp : Defines the entry point for the console application.
//
#include "shared_header.h"
#include "EvalModule.hpp"
#include <iostream>

int main(int argc, const char * argv[])
{
	if (argc < 2) {
		std::cerr << "NEED A MODEL FILE: filename.model" << endl;
		return 1;
	}
	string filename(argv[1]);
	cout << filename << endl;
	Model m;
	loadSerialisable2<Model>(m,filename);
	EvalModule em(m);
	auto result = em.evalAll(10);

	for (int k = 0; k < result.size(); k++) {
		cout << "Topic " << k << ":" << endl;
		for (auto & it : result[k]) {
			cout << "\t" << it.first << "() = " << it.second << endl;
		}
	}

	return 0;
}

