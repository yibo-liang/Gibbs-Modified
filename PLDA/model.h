#pragma once


#ifndef MODEL
#define MODEL
#include <sstream>
#include "shared_header.h"
#include "corpus.h"

namespace ParallelHLDA {
	class Model
	{
	public:

		Corpus * corpus;

		int id;
		int super_model_id;

		int K;//topic number
		int M;//document number
		int V;//vocabulary size

		double alpha, beta;

		vector<vector<int>> w;
		vector<vector<int>> z;
		vector<vector<int>> nw;
		vector<vector<int>> nd;
		vector<int> nwsum;
		vector<int> ndsum;

		vector<vector<double>> theta;
		vector<vector<double>> phi;

		string getTopicWords(int n);
		string getTopicWordsTree(int n);
		string getTopicWordDistributionTree();//given the infered model, how is this model distributed over the infered model


		vector<Model> getInitalSubmodel(int K_sublevel);
		Model(const Model &m);
		Model();
		~Model();

		void assignCorpus(Corpus * corpus);
		void update();

		vector<Model> submodels;

	private:
		void computeTheta();
		void computePhi();
		void getTopicWordsTree(int n, std::stringstream & strStream, Model & model, int depth);
		void getTopicWordDistributionTree(std::stringstream & ss, int depth);


		friend class boost::serialization::access;
		template<class Archive>
		void serialize(Archive & ar, const unsigned int version)
		{
			ar & id;
			ar & super_model_id;
			ar & alpha;
			ar & beta;
			ar & K;
			ar & V;
			ar & M;
			ar & BOOST_SERIALIZATION_NVP(z);
			ar & BOOST_SERIALIZATION_NVP(w);
			ar & BOOST_SERIALIZATION_NVP(nd);
			ar & BOOST_SERIALIZATION_NVP(nw);
			ar & BOOST_SERIALIZATION_NVP(nwsum);
			ar & BOOST_SERIALIZATION_NVP(ndsum);
			ar & BOOST_SERIALIZATION_NVP(theta);
			ar & BOOST_SERIALIZATION_NVP(phi);
			ar & BOOST_SERIALIZATION_NVP(submodels);
		}

	};
}
#endif