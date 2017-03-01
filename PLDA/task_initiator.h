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

	Model * model;

	TaskInitiator(JobConfig &config);
	~TaskInitiator();

	Model createInitialModel(Model & model);
	void startMasterWithExecutor(TaskExecutor &executor);
	void loadCorpus(Corpus & corpus);
	

private:

	JobConfig config;


	vector<vector<TaskPartition>> tasksFromModel(Model &initial_model);

};
#endif