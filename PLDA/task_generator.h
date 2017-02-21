#pragma once


#ifndef JOB
#define JOB

#include "shared_header.h"
#include "job_config.h"
#include "corpus.h"
#include "task_partition.h"
#include "task_executor.h"
#include "model.h"

class TaskGenerator
{
public:

	Corpus corpus;

	TaskGenerator(JobConfig &config);
	~TaskGenerator();

	void startMasterJob(TaskExecutor &executor);
	

private:

	JobConfig config;

	Model model;

	Model createInitialModel();
	void loadCorpus();

	vector<vector<TaskPartition>> generateSimpleTasks(Model &initial_model);
	vector<vector<TaskPartition>> generateHierarchicalTasks(Model &initial_model);

};
#endif