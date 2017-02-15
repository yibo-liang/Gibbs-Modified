#pragma once


#ifndef JOB
#define JOB

#include "shared_header.h"
#include "job_config.h"
#include "corpus.h"
#include "task.h"
#include "task_executor.h"
#include "model.h"

class TaskGenerator
{
public:


	TaskGenerator(JobConfig &config);
	~TaskGenerator();

	void startMasterJob(TaskExecutor &executor);
	

private:

	Corpus corpus;
	JobConfig config;

	Model model;

	Model createInitialModel();
	void loadCorpus();

	vector<vector<Task>> generateSimpleTasks(Model &initial_model);
	vector<vector<Task>> generateHierarchicalTasks(Model &initial_model);

};
#endif