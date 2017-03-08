#include "lda_interface.h"

using namespace ParallelHLDA;
LdaInterface::LdaInterface()
{
}


LdaInterface::LdaInterface(JobConfig config)
{
}

LdaInterface::~LdaInterface()
{
}


void LdaInterface::recursiveEstimation(Model & model, TaskInitiator & initiator, TaskExecutor & executor, JobConfig & config, int level) {

	executor.model = &model;
	initiator.delieverTasks(executor, model);
	executor.execute();

	level += 1;
	if (level < config.hierarchStructure.size()) {
		model.submodels = model.getInitalSubmodel(config.hierarchStructure[level]);
		for (int i = 0; i < model.K; i++) {
			recursiveEstimation(model.submodels[i], initiator, executor, config, level);
		}
	}
}

void LdaInterface::recursiveInference(Model & inferModel, Model & newModel, TaskInitiator & initiator, TaskExecutor & executor, JobConfig & config, int level) {

	executor.model = &newModel;
	initiator.delieverTasks(executor, newModel);
	executor.execute();

	level += 1;
	if (inferModel.submodels.size() > 0) {
		newModel.submodels = newModel.getInitalSubmodel(inferModel.submodels[0].K);
		for (int i = 0; i < newModel.K; i++) {
			recursiveInference(inferModel.submodels[i], newModel.submodels[i], initiator, executor, config, level);
		}
	}
}


string LdaInterface::nameModel(JobConfig &config) {
	std::stringstream ss;
	ss << "Model";
	for (int i = 0; i < config.hierarchStructure.size(); i++) {
		ss << "-" << config.hierarchStructure[i];
	}
	ss << ".model";
	return ss.str();
}

void LdaInterface::masterHierarchical(JobConfig &config) {
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


	std::ofstream ofs("tree.txt");
	ofs << model.getTopicWordsTree(25);
	ofs.close();
}

void LdaInterface::masterHierarchicalInference(JobConfig & config) {
	Corpus corpus;
	Model inferedModel = loadSerialisable<Model>(config.inferedModelFile);
	Model newModel;
	TaskInitiator initiator(config);
	initiator.loadSerializedCorpus(config.inferCorpusFile, corpus); //Load existing corpus
	initiator.loadInferencingText(corpus, config);
	inferedModel.assignCorpus(&corpus);

	initiator.createInitialInferModel(corpus, inferedModel, newModel); //with existing model, create new model for inferencing
	TaskExecutor executor(config);
	executor.inferModel = &inferedModel;
	executor.model = &newModel;
	recursiveInference(inferedModel, newModel, initiator, executor, config, 0);

	std::ofstream ofs("inference_tree.txt");
	ofs << newModel.getTopicWordsTree(25);
	ofs.close();

	std::ofstream ofs2("distrib_tree.txt");
	ofs2 << newModel.getTopicWordDistributionTree();
	ofs2.close();
}


int LdaInterface::master(JobConfig &config) {
	using namespace std;
	Timer overall_time;
	overall_time.reset();

	if (!config.inferencing) {
		masterHierarchical(config);
	}
	else {
		config.taskPerProcess = 1;
		config.totalProcessCount = 1;
		config.processID = 0;
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

int LdaInterface::slave(JobConfig config) {
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


		/* No paralle inferencing for now
		reason: each process will need a copy of model, which would be inpractical in memory */

	}
	MPI_Barrier(MPI_COMM_WORLD);
	return 0;
}