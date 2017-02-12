#pragma once

#ifndef TASK_EXECUTOR
#define TASK_EXECUTOR
#include "shared_header.h"
#include "task.h"

class TaskExecutor
{
public:
	
	int MODE = P_MPI;
	int procNumber;
	bool isMaster = false;

	

	void receiveLocalTasks(vector<Task> tasks); // master node task executor should use this, so no mpi is used for faster speed
	void receiveRemoteTasks(); //all other proecess should use this


	TaskExecutor(int procNumber);
	~TaskExecutor();

private:
	
	vector<Task> tasks;

	void sampleTask(Task &task);
	void runMaster();
	void runSlave();

};

#endif // !TASK_EXECUTOR
