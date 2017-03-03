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

	Corpus * corpus;

	Model * model; //Model to be used for initiating, if inferencing , this will be the model tobe infered

	TaskInitiator(JobConfig &config);
	~TaskInitiator();

	Model createInitialModel(Model & model);
	Model createInitialInferModel(Model & inferModel, Model & newModel);

	void startSampling(TaskExecutor &executor);

	void loadCorpus(Corpus & corpus);
	void loadCorpus(string corpusFilename, Corpus & corpus);


private:

	JobConfig config;


	vector<vector<TaskPartition>> tasksFromModel(Model &initial_model);

};
#endif