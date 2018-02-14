// ModelEvaluation.cpp : Defines the entry point for the console application.
//
#include "shared_header.h"
#include "EvalModule.hpp"
#include <iostream>
#include <strstream>

void evalModel(Model & m, Corpus & c, string prefix, std::ofstream & ofs) {
	EvalModule em(m, c);
	string str;
	auto result = em.evalAll(10);

	for (int k = 0; k < result.size(); k++) {

		auto vals = result[k].first;
		string words = result[k].second;

		cout << prefix << k << "";
		//ss << prefix << k << "";
		str += prefix + to_string(k);
		for (auto & it : vals) {
			cout << "\t" << it.second;
			//ss << "\t" << it.second;
			str += "\t" + to_string(it.second);
		}
		//ss << "\t" << words << endl;
		cout << "\t" << words << endl;
		str += "\t" + words+"\n";
	}
	ofs << str;
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
	loadSerialisable2<Corpus>(c, cfile);



	std::ofstream ofs("eval_summary.txt");

	evalModel(m, c, "S0-", ofs);

	for (int i = 0; i < m.submodels.size(); i++) {
		Model & submodel = m.submodels[i];
		evalModel(submodel, c, "S1-" + to_string(i) + "-", ofs);
		for (int k = 0; k < submodel.submodels.size(); k++) {
			Model & subsubmodel = submodel.submodels[k];
			evalModel(subsubmodel, c, "S2-" + to_string(i) + "-" + to_string(k) + "-", ofs);
		}
	}


	ofs.flush();
	ofs.close();
	return 0;
}

