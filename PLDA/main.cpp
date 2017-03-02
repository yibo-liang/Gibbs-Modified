
#include "shared_header.h"
#include "job_config.h"
#include "task_initiator.h"
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
		("mode,m", po::value<string>(), "set parallel mode, choose from GPU CPU (default).")
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

	if (vm.count("mode")) {
		config->parallelType = (vm["mode"].as<string>() == "gpu") ? P_GPU : P_MPI;
		
	}

	return 0;
}

void recursiveEstimation(Model & model, TaskInitiator & initiator, TaskExecutor & executor, JobConfig & config, int level) {
	initiator.model = &model;
	executor.model = &model;
	initiator.startMasterWithExecutor(executor);
	executor.execute();

	//juust for test
	string filename = (boost::format("MODEL-L%d-K%d.txt") % level % model.id).str();
	std::ofstream ofs(filename);
	ofs << model.getTopicWords(25);
	ofs.close();
	cout << endl;
	for (int i = 0; i < level; i++) {
		cout << "\t";
	}
	cout << "Sampling Model K = " << model.K;

	level += 1;
	if (level < config.hierarchStructure.size()) {
		model.submodels = model.getInitalSubmodel(config.hierarchStructure[level]);
		for (int i = 0; i < model.K; i++) {
			recursiveEstimation(model.submodels[i], initiator, executor, config, level);
		}
	}
}


void masterHierarchical(JobConfig &config) {

	int hierarch_level = config.hierarchStructure.size();

	Corpus corpus;
	Model model;

	TaskInitiator initiator(config);
	initiator.loadCorpus(corpus);
	initiator.model = &model;
	initiator.createInitialModel(model);
	TaskExecutor executor(config);
	recursiveEstimation(model, initiator, executor, config, 0);
}


int master(JobConfig &config) {
	using namespace std;
	Timer overall_time;
	overall_time.reset();

	masterHierarchical(config);
	MPI_Barrier(MPI_COMM_WORLD);

	double elapsed = overall_time.elapsed();
	{
		cout << "LDA Gibbs Sampling finished" << endl;
		cout << "Total time consumed: " << elapsed << " seconds" << endl;
		cout << "\nPress ENTER to exit.\n";
	}

	cin.ignore();
	return 0;
}

int slave(JobConfig config) {
	TaskExecutor executor(config);
	int sum = 1;
	int base = 1;
	for (int i = 0; i < config.hierarchStructure.size() - 1; i++) {
		base *= config.hierarchStructure[i];
	}
	if (base > 1) {
		sum += base;
	}
	for (int i = 0; i < sum; i++) {
		executor.receiveRemoteTasks();
		executor.execute();
	}
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
		master(config);
	}
	else {
		slave(config);
	}

	MPI_Finalize();
}