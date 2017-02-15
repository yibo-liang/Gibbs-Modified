
#include "shared_header.h"
#include "job_config.h"
#include "task_generator.h"
#include "task_executor.h"
#include <mpi.h>

/*
Main file : controlls the load, running mode and exit of the program
*/

namespace po = boost::program_options;

int getProgramOption(int argc, char *argv[], JobConfig * config) {
	po::options_description desc("Allowed options");
	int niter = 2000;
	int n = 0;
	double alpha = 0.01;
	double beta = 0.01;


	desc.add_options()
		("help,?", "produce help message")
		("file,f", po::value<string>(), "set corpus filename")
		("filetype,ft", po::value<string>()->default_value("txt"), "set corpus type [txt/json/csv]")
		("iter-number,itern", po::value<int>(&niter)->default_value(2000), "set iteration number")
		("docn,dn", po::value<int>(&n), "set document number")
		("alpha", po::value<double>(&alpha), "set alphta number")
		("beta", po::value<double>(&beta), "set beta number")
		("hierarch,h", po::value<std::vector<int> >()->multitoken(), "set hierarchical structure in form (ignore brackets) [n1 n2 n3 ...], if unset, it will be a single topic model")
		("task-per-proc,tpp", po::value<int>(&n), "set task number per process")
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
	if (vm.count("iter-number")) {
		config->iterationNumber = vm["iter-number"].as<int>();
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
	if (vm.count("task-per-proc")) {
		config->taskPerProcess = vm["task-per-proc"].as<int>();
	}


	return 0;
}

int master(JobConfig &config) {
	TaskGenerator lda_job(config);
	TaskExecutor executor(config);
	lda_job.startMasterJob(executor);
	executor.execute();
	cout << "All Job Done.";
	MPI_Barrier(MPI_COMM_WORLD);
	return 0;
}

int slave(JobConfig config) {
	cout << config.processID << "," << 1 << endl; 
	TaskExecutor executor(config);
	cout << config.processID << "," << 2 << endl;
	executor.receiveRemoteTasks();
	cout << config.processID << "," << 3 << endl;
	executor.execute();
	cout << config.processID << "," << 4 << endl;
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

	if (getProgramOption(argc, argv, &config) != 0) return 1;

	if (worldRank == 0) {
		//std::cin.ignore();
		master(config);
	}
	else {
		slave(config);
	}

	MPI_Finalize();
}