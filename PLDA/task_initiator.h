#pragma once


#ifndef JOB
#define JOB

#include "shared_header.h"
#include "job_config.h"
#include "corpus.h"
#include "task_partition.h"
#include "task_executor.h"
#include "model.h"

class TaskInitiator
{
public:



	TaskInitiator(JobConfig &config);
	~TaskInitiator();

	void createInitialModel(Corpus & corpus, Model & model, int K);

	void createInitialInferModel(Corpus & corpus, Model & inferModel, Model & newModel);

	void delieverTasks(TaskExecutor &executor, Model & model);

	void loadSerializedCorpus(string filename,Corpus & corpus);

	void loadCorpus(Corpus & corpus, JobConfig & config);
	void loadInferencingCorpus(Corpus & corpus, JobConfig & config);


private:

	JobConfig config;


	vector<vector<TaskPartition>> tasksFromModel(Model &initial_model);

};
#endif