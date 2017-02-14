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

void TaskExecutor::receiveMasterTasks(vector<Task> tasks, Model * model)
{
	//used only by ROOT
	this->tasks = tasks;
	this->model = model;
	cout << "Proc ID = " << this->procNumber << " Received " << this->tasks.size() << " tasks" << endl;
}

void TaskExecutor::receiveRemoteTasks()
{
	using namespace MPIHelper;
	mpiReceive2(this->tasks, ROOT);
	cout << "Proc ID = " << this->procNumber << " Received " << this->tasks.size() << " tasks" << endl;
}



SlaveSyncData updateModel(Model & model, vector<vector<SlaveSyncData>> & iterationSyncCollector) {



	SlaveSyncData result;

	for (auto &execCollect : iterationSyncCollector) {
		for (auto &sync_data : execCollect) {
			//nd diff
			for (auto &ndIterator : sync_data.ndDiff) {
				int m = ndIterator.first;
				for (auto &topicIterator : ndIterator.second) {
					int topic = topicIterator.first;
					model.nd[m][topic] += topicIterator.second;
					result.ndDiff[m][topic] = model.nd[m][topic];

				}
			}
			//nw diff

			for (auto &nwIterator : sync_data.nwDiff) {
				int i = nwIterator.first;
				for (auto &topicIterator : nwIterator.second) {
					int topic = topicIterator.first;
					model.nw[i][topic] += topicIterator.second;
					result.nwDiff[i][topic] = model.nw[i][topic];
				}
			}

			//ndsum diff
			for (auto &nwsumIterator : sync_data.nwsumDiff) {
				int topic = nwsumIterator.first;
				model.nwsum[topic] += nwsumIterator.second;
				result.nwsumDiff[topic] = model.nwsum[topic];
			}


		}
	}
	return result;
}


void TaskExecutor::execute()
{
	Timer timer;
	using namespace MPIHelper;
	int iteration_i = 0;
	while (iteration_i < config.iterationNumber) {

		if (config.processID == ROOT) {
			timer.reset();
		}

		vector<SlaveSyncData> executorSyncCollector;
		SlaveSyncData update;
		for (auto &task : tasks) {
			executorSyncCollector.push_back(sampleTask(task));
		}


		if (config.processID == MPIHelper::ROOT) {
			vector<vector<SlaveSyncData>> iterationSyncCollector(config.totalProcessCount);
			iterationSyncCollector[0] = executorSyncCollector;

			//receive all sync data from slaves
			for (int i = 0; i < config.totalProcessCount; i++) {
				if (i != ROOT) {
					//iterationSyncCollector.push_back(mpiReceive<vector<SynchronisationData>>(i));
					mpiReceive2<vector<SlaveSyncData>>(iterationSyncCollector[i], i);
				}
			}
			//iterate all syn data, sum all changes and broadcast to all workers
			update = updateModel(*model, iterationSyncCollector);
			mpiBroadCast(update, ROOT, config.processID);
		}
		else {
			//send slave sync data
			mpiSend(executorSyncCollector, ROOT);

			//receive master's update by broadcasting
			mpiBroadCast(update, ROOT, config.processID);


		}

		//update tasks for this 
		for (auto& task : tasks) {
			//task.nd = globalSyncData.ndDiff;
			//task.nw = globalSyncData.nwDiff;

			for (auto& it : update.ndDiff) {
				int m = it.first;
				if (task.nd.count(m))
					for (auto& k : it.second) {
						task.nd[m][k.first] = k.second;
					}
			}

			for (auto& it : update.nwDiff) {
				int v = it.first;
				if (task.nw.count(v))
					for (auto& k : it.second) {
						task.nw[v][k.first] = k.second;
					}
			}


			for (auto& doc : update.nwsumDiff) {
				task.nwsum[doc.first] = doc.second;
			}
		}
		iteration_i++;
		if (config.processID == ROOT) {
			cout << "Iteration " << iteration_i;
			double t2 = timer.elapsed();
			cout << ", elapsed" << t2 << "" << endl;
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

SlaveSyncData TaskExecutor::sampleTask(Task & task)
{

	SlaveSyncData syncData;

	double alpha = task.alpha;
	double beta = task.beta;
	int K = task.K;
	double Vbeta = (double)task.V * beta;
	double Kalpha = (double)task.K * alpha;


	for (int wi = 0; wi < task.wordSampling.size(); wi++) {

		/*
			TODO: has to test which is better

			1. Gibbs' sampling in the local with updating nw,nd,nwsum on every word, OR
			2. Updating nw, nd, nwsum only when synchronizing.

			The current implementation is 1.
		*/

		//in job.cpp, we did push_back(vector<int>({ doc_i, w, docWord_i }))
		vector<int> tempWord = task.wordSampling.at(wi);
		int m = tempWord.at(0);
		int w = tempWord.at(1);
		int n = tempWord.at(2);
		//z[m][n] 
		int topic = task.z.at(wi);

		//local copy of task
		//task.nw[w][topic] -= 1;
		//task.nd[m][topic] -= 1;
		//task.nwsum[topic] -= 1;
		//task.ndsum[m] -= 1;


		//changes to be sychronized
		syncData.nwDiff[w][topic] -= 1;
		syncData.ndDiff[m][topic] -= 1;
		syncData.nwsumDiff[topic] -= 1;

		double* p = new double[K];
		for (int k = 0; k < K; k++) {
			double A = task.nw.at(w).at(k) - 1;
			double B = task.nwsum.at(k) - 1;
			double C = task.nd.at(m).at(k) - 1;
			double D = task.ndsum.at(m) - 1;
			p[k] = (A + beta) / (B + Vbeta) *
				(C + alpha) / (D + Kalpha);
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
		delete[] p;

		//task.nw[w][topic] += 1;
		//task.nd[m][topic] += 1;
		//task.nwsum[topic] += 1;
		//task.ndsum[m] += 1;

		syncData.nwDiff[w][topic] += 1;
		syncData.ndDiff[m][topic] += 1;
		syncData.nwsumDiff[topic] += 1;

		task.z[wi] = topic;


	}
	return syncData;
}
