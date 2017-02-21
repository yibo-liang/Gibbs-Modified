
#include "shared_header.h"
#include "job_config.h"
#include "task_generator.h"
#include "task_executor.h"
#include <mpi.h>
#include <iostream>
#include <fstream>
/*
Main file : controlls the load, running mode and exit of the program
*/

namespace po = boost::program_options;

int getProgramOption(int argc, char *argv[], JobConfig * config) {
	po::options_description desc("Allowed options");
	int n = 0;
	double alpha = 0.01;
	double beta = 0.01;
	int task_proc = 1;

	desc.add_options()
		("help,?", "produce help message")
		("file,f", po::value<string>(), "set corpus filename")
		("filetype,ft", po::value<string>()->default_value("txt"), "set corpus type [txt/json/csv]")
		("niter", po::value<int>(), "set iteration number")
		("docn,dn", po::value<int>(&n), "set document number")
		("alpha", po::value<double>(&alpha), "set alphta number")
		("beta", po::value<double>(&beta), "set beta number")
		("hierarch,h", po::value<std::vector<int> >()->multitoken(), "set hierarchical structure in form (ignore brackets) [n1 n2 n3 ...], if unset, it will be a single topic model")
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


	return 0;
}

int master(JobConfig &config) {
	using namespace std;
	TaskGenerator lda_job(config);
	TaskExecutor executor(config);
	lda_job.startMasterJob(executor);
	executor.model->corpus = &lda_job.corpus;
	executor.execute();
	cout << "All Job Done.";
	string result = executor.model->getTopicWords(25);
	ofstream myfile;
	myfile.open("result.txt");
	myfile << result;
	myfile.close();
	MPI_Barrier(MPI_COMM_WORLD);

	return 0;
}

int slave(JobConfig config) {
	TaskExecutor executor(config);
	executor.receiveRemoteTasks();
	executor.execute();
	MPI_Barrier(MPI_COMM_WORLD);
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

	if (worldRank == 0) {

		cin.ignore();
		//std::cin.ignore();
		master(config);
	}
	else {
		slave(config);
	}

	MPI_Finalize();
}