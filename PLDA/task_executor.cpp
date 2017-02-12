#include "task_executor.h"
#include "mpi_helper.h"

void TaskExecutor::receiveLocalTasks(vector<Task> tasks)
{
	//used only by ROOT
	this->tasks = tasks;
	cout << "Proc ID = " << this->procNumber << " Received " << this->tasks.size() << " tasks" << endl;
}

void TaskExecutor::receiveRemoteTasks()
{
	using namespace MPIHelper;
	this->tasks = mpiReceive<vector<Task>>(procNumber);

	cout << "Proc ID = " << this->procNumber << " Received " << this->tasks.size() << " tasks" <<endl;
}

TaskExecutor::TaskExecutor(int procNumber)
{
	this->procNumber = procNumber;
	if (procNumber == 0) isMaster = true;

}

TaskExecutor::~TaskExecutor()
{
}
