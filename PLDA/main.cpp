
#include "shared_header.h"
#include "job_config.h"
#include "task_initiator.h"
#include "task_executor.h"
#include "mpi_helper.h"
#include <iostream>
#include <fstream>
#include <sstream>
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
		("file,f", po::value<string>(), "set text filename")
		("filetype,ft", po::value<string>()->default_value("txt"), "set corpus type [txt/json/csv]")
		("niter", po::value<int>(), "set iteration number")
		("docn,dn", po::value<int>(&n), "set document number")
		("alpha", po::value<double>(&alpha), "set alphta number")
		("beta", po::value<double>(&beta), "set beta number")
		("hierarch,h", po::value<std::vector<int> >()->multitoken(), "set hierarchical structure in form (ignore brackets) [n1 n2 n3 ...], if unset, it will be a single topic model")
		("parallel,p", po::value<string>(), "set parallel mode, choose from GPU CPU (default).")
		("inference,inf", po::value<string>(), "inference mode, followed by the file name of the existing model.")
		("infer-corpus,ic", po::value<string>(), "using existing serialized corpus file. give the file name")
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

	if (vm.count("parallel")) {
		config->parallelType = (vm["parallel"].as<string>() == "gpu") ? P_GPU : P_MPI;

	}
	if (vm.count("inference")) {
		config->inferedModelFile = vm["inference"].as<string>();
		config->inferCorpusFile = vm["infer-corpus"].as<string>();
		config->inferencing = true;
	}


	return 0;
}

void recursiveEstimation(Model & model, TaskInitiator & initiator, TaskExecutor & executor, JobConfig & config, int level) {

	executor.model = &model;
	initiator.delieverTasks(executor, model);
	executor.execute();

	//juust for test
	/*string filename = (boost::format("MODEL-L%d-K%d.txt") % level % model.id).str();
	std::ofstream ofs(filename);
	ofs << model.getTopicWords(25);
	ofs.close();
	cout << endl;
	for (int i = 0; i < level; i++) {
		cout << "\t";
	}
	cout << "Sampling Model K = " << model.K;*/

	level += 1;
	if (level < config.hierarchStructure.size()) {
		model.submodels = model.getInitalSubmodel(config.hierarchStructure[level]);
		for (int i = 0; i < model.K; i++) {
			recursiveEstimation(model.submodels[i], initiator, executor, config, level);
		}
	}
}

void recursiveInference(Model & inferModel, Model & newModel, TaskInitiator & initiator, TaskExecutor & executor, JobConfig & config, int level) {
	string msg = "infer";
	MPIHelper::mpiBroadCast(msg, MPIHelper::ROOT, config.processID);

	executor.model = &newModel;
	initiator.delieverTasks(executor, newModel);
	executor.execute();

	level += 1;
	if (inferModel.submodels.size()>0) {
		newModel.submodels = newModel.getInitalSubmodel(inferModel.submodels[0].K);
		for (int i = 0; i < newModel.K; i++) {
			recursiveInference(inferModel.submodels[i], newModel.submodels[i], initiator, executor, config, level);
		}
	}
}


string nameModel(JobConfig &config) {
	std::stringstream ss;
	ss << "Model";
	for (int i = 0; i < config.hierarchStructure.size(); i++) {
		ss << "-" << config.hierarchStructure[i];
	}
	ss << ".model";
	return ss.str();
}

void masterHierarchical(JobConfig &config) {
	Corpus corpus;
	Model model;
	TaskInitiator initiator(config);
	initiator.loadCorpus(corpus, config);
	initiator.createInitialModel(corpus, model, config.hierarchStructure[0]);
	TaskExecutor executor(config);
	recursiveEstimation(model, initiator, executor, config, 0);
	//save after sampling
	saveSerialisable<Model>(model, nameModel(config));
	saveSerialisable<Corpus>(corpus, "corpus.ser");
	corpus = loadSerialisable<Corpus>("corpus.ser");
	model=loadSerialisable<Model>(nameModel(config));

}

void masterHierarchicalInference(JobConfig & config) {
	Corpus corpus;
	Model inferedModel = loadSerialisable<Model>(config.inferedModelFile);
	Model newModel;
	TaskInitiator initiator(config);
	initiator.loadSerializedCorpus(config.inferCorpusFile, corpus); //Load existing corpus
	initiator.loadInferencingCorpus(corpus, config);

	initiator.createInitialInferModel(corpus, inferedModel, newModel); //with existing model, create new model for inferencing
	TaskExecutor executor(config);
	executor.inferModel = &inferedModel;
	executor.model = &newModel;
	recursiveInference(inferedModel, newModel, initiator, executor, config, 0);
	MPIHelper::mpiBroadCast(string("END"), MPIHelper::ROOT, config.processID);

}


int master(JobConfig &config) {
	using namespace std;
	Timer overall_time;
	overall_time.reset();

	if (!config.inferencing) {
		masterHierarchical(config);
	}
	else {
		masterHierarchicalInference(config);
	}
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
	if (!config.inferencing) {
		int sum = 0;
		int base = 1;
		for (int i = 0; i < config.hierarchStructure.size(); i++) {
			sum += base;
			base *= config.hierarchStructure[i];
		}

		for (int i = 0; i < sum; i++) {
			executor.receiveRemoteTasks();
			executor.execute();
		}
	}
	else {
		string msg = "";
		MPIHelper::mpiBroadCast(msg, MPIHelper::ROOT, config.processID);
		while (msg != "END") {
			executor.receiveRemoteTasks();
			executor.execute();
			MPIHelper::mpiBroadCast(msg, MPIHelper::ROOT, config.processID);
		}
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