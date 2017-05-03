
#include "shared_header.h"
#include "job_config.h"
#include "lda_interface.h"
#include <iostream>
#include <fstream>
#include <sstream>
/*
Main file : controlls the load, running mode and exit of the program
*/

using namespace ParallelHLDA;
namespace po = boost::program_options;

int getProgramOption(int argc, char *argv[], JobConfig * config) {
	po::options_description desc("Allowed options");
	int n = 0;
	double alpha = 0.01;
	double beta = 0.01;
	int task_proc = 1;

	desc.add_options()
		("help,?", "produce help message")
		("file,f", po::value<string>(), "set text filename")
		("filetype,y", po::value<string>()->default_value("txt"), "set corpus type [txt/json/csv/ser(using saved serialized corpus \".ser\" type file)]")
		("niter", po::value<int>(), "set iteration number")
		("docn,d", po::value<int>(&n), "set document number")
		("text-start,t", po::value<int>(), "set starting index to be used for each line of document, if the document is text file only.")
		("alpha", po::value<double>(&alpha), "set alphta number")
		("beta", po::value<double>(&beta), "set beta number")
		("hierarch,h", po::value<std::vector<int> >()->multitoken(), "set hierarchical structure in form (ignore brackets) [n1 n2 n3 ...], if unset, it will be a single topic model")
		("parallel,p", po::value<string>(), "set parallel mode, choose from GPU CPU (default).")
		("inference,i", po::value<string>(), "inference mode, followed by the file name of the existing model.")
		("infer-corpus,c", po::value<string>(), "using existing serialized corpus file. give the file name")
		("document-attributes,a", po::value<vector<string>>()->multitoken(), "Set the attributes indices of a document. [attr 1] [attr 2] .... [attr n]")
		;

	po::variables_map vm;
	po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
	po::notify(vm);

	if (vm.count("help")) {
		cout << desc << "\n";
		return 1;
	}

	if (vm.count("file")) {
		config->filename = vm["file"].as<string>();
	}
	else {
		return 1;
	}
	if (vm.count("filetype")) {
		config->filetype = vm["filetype"].as<string>();
	}
	if (vm.count("niter")) {
		config->iterationNumber = vm["niter"].as<int>();
	}
	if (vm.count("docn")) {
		config->documentNumber = vm["docn"].as<int>();
	}
	if (vm.count("text-start")) {
		config->documentWordStart = vm["text-start"].as<int>();
	}
	if (vm.count("alpha")) {
		config->alpha = vm["alpha"].as<double>();
	}
	if (vm.count("beta")) {
		config->beta = vm["beta"].as<double>();
	}
	if (vm.count("hierarch")) {
		config->hierarchStructure = vm["hierarch"].as<vector<int>>();
	}
	else {
		config->hierarchStructure = vector<int>({ 1 });
	}

	if (vm.count("parallel")) {
		config->parallelType = (vm["parallel"].as<string>() == "gpu") ? P_GPU : P_MPI;

	}
	if (vm.count("inference")) {
		config->inferedModelFile = vm["inference"].as<string>();
		config->inferCorpusFile = vm["infer-corpus"].as<string>();
		config->inferencing = true;
	}
	if (vm.count("document-attributes")) {
		int i=0;
		for (string attr : vm["document-attributes"].as<vector<string>>()) {
			config->otherAttrsIndx[i] = attr;
			i++;
		}
	}


	return 0;
}


int main(int argc, char *argv[]) {

	MPI_Init(&argc, &argv);

	// Get the rank of the process
	int worldRank;
	MPI_Comm_rank(MPI_COMM_WORLD, &worldRank);

	int worldSize;
	MPI_Comm_size(MPI_COMM_WORLD, &worldSize);

	JobConfig config;
	config.processID = worldRank;
	config.totalProcessCount = worldSize;
	config.taskPerProcess = worldSize;
	if (getProgramOption(argc, argv, &config) != 0) return 1;
	LdaInterface interface;

	if (worldRank == 0) {


		cin.ignore();
		interface.master(config);
	}
	else {
		interface.slave(config);
	}

	MPI_Finalize();
}