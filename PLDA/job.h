#pragma once


#ifndef JOB
#define JOB

#include "shared_header.h"
#include "job_config.h"
#include "corpus.h"
#include "task.h"
#include "model.h"

class Job
{
public:


	Job(JobConfig config, Corpus corpus);
	~Job();

	

private:

	Corpus corpus;
	JobConfig config;
	vector<Task> tasks;
	vector<Task> generateSimpleTasks();
	vector<Task> generateHierarchicalTasks();

};
#endif