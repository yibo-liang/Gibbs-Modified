#pragma once


#ifndef JOB
#define JOB

#include "shared_header.h"
#include "job_config.h"
#include "corpus.h"
#include "task.h"
#include "task_executor.h"
#include "model.h"

class Job
{
public:


	Job(JobConfig &config);
	~Job();

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