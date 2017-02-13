#include "task_executor.h"
#include "mpi_helper.h"
#include <random>

//the core method for Gibbs' LDA sampling.
/*

If Master:
	while (iterating)
		for each task in list,
			sample task
		receive changes of nd, nw, nwsum, wait and sync all
		Update model
		broadcast new nd, nw, nwsum to all
	output model

If Slave:
	while (iterating)
		for each task in list
			sample task
		send changes of nd, nw, nwsum, wait and sync all
		broatcast new nd, nw, nwsum
*/

void TaskExecutor::receiveMasterTasks(vector<Task> tasks, Model model)
{
	//used only by ROOT
	this->tasks = tasks;
	this->model = model;
	cout << "Proc ID = " << this->procNumber << " Received " << this->tasks.size() << " tasks" << endl;
}

void TaskExecutor::receiveRemoteTasks()
{
	using namespace MPIHelper;
	this->tasks = mpiReceive<vector<Task>>(ROOT);
	cout << "Proc ID = " << this->procNumber << " Received " << this->tasks.size() << " tasks" << endl;
}


template<typename A, typename B>
void sycMaps(std::map<A, B> & a, std::map<A, B> & b) {
	//adding changes from a to b. 

	for (auto const& source : a) {
		b[source.first] += a[source.first];
	}

}

void TaskExecutor::execute()
{

	using namespace MPIHelper;
	int iteration_i = 0;
	while (iteration_i < config.iterationNumber) {
		vector<SynchronisationData> executorSyncCollector;
		for (auto &task : tasks) {
			executorSyncCollector.push_back(sampleTask(task));
		}
		if (config.processID == MPIHelper::ROOT) {
			vector<vector<SynchronisationData>> iterationSyncCollector(1, executorSyncCollector);

			//receive all sync data from slaves
			for (int i = 0; i < config.totalProcessCount; i++) {
				if (i != ROOT) {
					iterationSyncCollector.push_back(mpiReceive<vector<SynchronisationData>>(i));
				}
			}
			//iterate all syn data, sum all changes and broadcast to all workers
			SynchronisationData globalSyncData;
			for (auto &execCollect : iterationSyncCollector) {
				for (auto &sync_data : execCollect) {
					//nd diff
					for (auto &ndIterator : sync_data.ndDiff) {
						int m = ndIterator.first;
						for (auto &topicIterator : ndIterator.second) {
							int topic = topicIterator.first;
							model.nd[m][topic] += topicIterator.second;
						}
					}

					//nw diff

					//ndsum diff

				}
			}

		}
		else {
			mpiSend(executorSyncCollector, ROOT);
		}
	}
}

TaskExecutor::TaskExecutor(JobConfig config)
{
	this->config = config;
	if (config.processID == MPIHelper::ROOT) isMaster = true;

}

TaskExecutor::~TaskExecutor()
{
}

SynchronisationData TaskExecutor::sampleTask(Task & task)
{
	SynchronisationData syncData;

	double alpha = task.alpha;
	double beta = task.beta;
	double K = task.K;
	double Vbeta = task.V * beta;
	double Kalpha = task.K * alpha;

	auto wordIterator = task.wordSampling.begin();
	auto assignmentIterator = task.z.begin();
	for (; wordIterator != task.wordSampling.end();) {

		/*
			TODO: has to test which is better

			1. Gibbs' sampling in the local with updating nw,nd,nwsum on every word, OR
			2. Updating nw, nd, nwsum only when synchronizing.

			The current implementation is 1.
		*/

		//in job.cpp, we did push_back(vector<int>({ doc_i, w, docWord_i }))
		int m = (*wordIterator)[0];
		int w = (*wordIterator)[1];
		int n = (*wordIterator)[2];
		//z[m][n] 
		int topic = *assignmentIterator;

		//local copy of task
		task.nw[w][topic] -= 1;
		task.nd[m][topic] -= 1;
		task.nwsum[topic] -= 1;
		task.ndsum[m] -= 1;

		//changes to be sychronized
		syncData.nwDiff[w][topic] -= 1;
		syncData.ndDiff[m][topic] -= 1;
		syncData.nwsumDiff[topic] -= 1;

		vector<double> p;
		for (int k = 0; k < K; k++) {
			p[k] = (task.nw[w][k] + beta) / (task.nwsum[k] + Vbeta) *
				(task.nd[m][k] + alpha) / (task.ndsum[m] + Kalpha);
		}

		// cumulate multinomial parameters
		for (int k = 1; k < K; k++) {
			p[k] += p[k - 1];
		}
		// scaled sample because of unnormalized p[]
		double u = ((double)rand() / (double)RAND_MAX) * p[K - 1];

		for (topic = 0; topic < K; topic++) {
			if (p[topic] >= u) {
				break;
			}
		}

		task.nw[w][topic] += 1;
		task.nd[m][topic] += 1;
		task.nwsum[topic] += 1;
		task.ndsum[m] += 1;

		syncData.nwDiff[w][topic] += 1;
		syncData.ndDiff[m][topic] += 1;
		syncData.nwsumDiff[topic] += 1;

		*assignmentIterator = topic;

		wordIterator++;
		assignmentIterator++;

	}
	return syncData;
}
