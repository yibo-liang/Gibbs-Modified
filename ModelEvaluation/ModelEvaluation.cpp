// ModelEvaluation.cpp : Defines the entry point for the console application.
//
#include "shared_header.h"
#include "EvalModule.hpp"
#include <iostream>
#include <strstream>

void evalModel(Model & m, Corpus & c, string prefix) {
	EvalModule em(m,c);
	std::strstream ss;
	ss << "Topic\tTC-NZ\tTC-PMI\tTC-LCP" << endl;

	auto result = em.evalAll(10);

	for (int k = 0; k < result.size(); k++) {
		cout << prefix << k << "";
		ss << prefix << k << "";
		for (auto & it : result[k]) {
			cout << "\t" << it.second;
			ss << "\t" << it.second;
		}
		cout << endl;
		ss << endl;
	}

	std::ofstream ofs(prefix + "eval.txt");
	ofs << ss.str();
	ofs.flush();
	ofs.close();
}


int main(int argc, const char * argv[])
{
	if (argc < 3) {
		std::cerr << "NEED A MODEL FILE: filename.model and a corpus.ser" << endl;
		return 1;
	}
	string filename(argv[1]);
	Model m;
	cout << filename << endl;
	loadSerialisable2<Model>(m, filename);
	Corpus c;
	string cfile(argv[2]);
	cout << cfile << endl;
	loadSerialisable2<Corpus>(c,cfile);
	evalModel(m,c, "S0-");

	for (int i = 0; i < m.submodels.size(); i++) {
		Model & submodel = m.submodels[i];
		evalModel(submodel, c, "S1-" + to_string(i) + "-");
		for (int k = 0; k < submodel.submodels.size(); k++) {
			Model & subsubmodel = submodel.submodels[k];
			evalModel(subsubmodel, c, "S2-" + to_string(i) + "-" + to_string(k) + "-");
		}
	}


	return 0;
}

