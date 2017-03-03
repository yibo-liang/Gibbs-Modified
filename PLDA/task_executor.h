#pragma once

#ifndef TASK_EXECUTOR
#define TASK_EXECUTOR
#include "shared_header.h"
#include "job_config.h"
#include "task_partition.h"
#include "model.h"
#include "sampler.h"

class TaskExecutor
{
public:
	
	JobConfig config;

	int MODE = P_MPI;
	int procNumber;
	bool isMaster = false;

	Model * model; //model for this topic model task, only available on Master node executor. Generated & initialised from job.
	Model * inferModel;

	void receiveMasterTasks(vector<TaskPartition> & tasks, Model * model); // master node task executor should use this, so no mpi is used for faster speed
	void receiveRemoteTasks(); //all other proecess should use this

	void execute();

	TaskExecutor( JobConfig config);
	~TaskExecutor();

private:




	Timer timer;
	map<string, double> timeRecord;

	void rtime(string str) {
		timeRecord[str]  += timer.elapsed();
	}

	//vector<Task> tasks;
	vector<Sampler> samplers;


	//SlaveSyncData sampleTask(Task &task); //since each task is only subset of the corpus, we need to return all data in the model that need to be synchronized.
	
	void exchangeData(Sampler& currentSampler, int partiton_i, int receiver_PID, int sender_PID);
	void executePartition();
	
	void execMaster();
	void execSlave();

};

#endif // !TASK_EXECUTOR
