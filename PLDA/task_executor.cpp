#include "task_executor.h"
#include "mpi_helper.h"
#include <random>

#define WAIT_FOR_ATTACH false
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

void TaskExecutor::receiveMasterTasks(vector<Task> & tasks, Model * model)
{
	//used only by ROOT
	this->samplers = vector<Sampler>(tasks.size());
	for (auto &task : tasks) {
		this->samplers[task.id].fromTask(task);
		this->samplers[task.id].pid = config.processID;
	}


	this->model = model;
	cout << "Proc ID = " << this->procNumber << " Received " << tasks.size() << " tasks" << endl;
}

void TaskExecutor::receiveRemoteTasks()
{

	using namespace MPIHelper;
	vector<Task> tasks;
	mpiReceive2(tasks, ROOT);

	this->samplers = vector<Sampler>(tasks.size());

	for (auto &task : tasks) {

		this->samplers[task.id].fromTask(task);
		this->samplers[task.id].pid = config.processID;
	}

	cout << "Proc ID = " << this->procNumber << " Received " << tasks.size() << " tasks" << endl;
}



vector<vector<SlaveSyncData>> updateModel(Model & model, const vector<vector<SlaveSyncData>> &iterationSyncCollector, JobConfig& config) {


	vector<vector<SlaveSyncData>> resultArray(config.totalProcessCount, vector<SlaveSyncData>(config.taskPerProcess, SlaveSyncData()));

	for (int collection_id = 0; collection_id < iterationSyncCollector.size(); collection_id++) {
		for (int task_iter_id = 0; task_iter_id < iterationSyncCollector[collection_id].size(); task_iter_id++) {
			const SlaveSyncData& old_sync_data = iterationSyncCollector.at(collection_id).at(task_iter_id);
			int pid = old_sync_data.pid;
			int task_id = old_sync_data.task_id;

			SlaveSyncData& new_sync_data = resultArray[pid][task_id];
			new_sync_data.pid = pid;
			new_sync_data.task_id = task_id;
			//nd diff
			for (auto &ndIterator : old_sync_data.ndDiff) {
				int m = ndIterator.first;
				for (int i = 0; i < model.K; i++) {
					model.nd[m][i] += ndIterator.second.at(i);

					if (new_sync_data.ndDiff[m].size() == 0) {
						new_sync_data.ndDiff[m] = vector<int>(model.K);
					}
					new_sync_data.ndDiff[m][i] = model.nd[m][i];
				}
			}
			//nw diff

			for (auto &nwIterator : old_sync_data.nwDiff) {
				int w = nwIterator.first;
				for (int i = 0; i < model.K; i++) {
					model.nw[w][i] += nwIterator.second.at(i);

					if (new_sync_data.nwDiff[w].size() == 0) {
						new_sync_data.nwDiff[w] = vector<int>(model.K);
					}
					new_sync_data.nwDiff[w][i] = model.nw[w][i];
				}
			}

			//ndsum diff
			for (auto &nwsumIterator : old_sync_data.nwsumDiff) {
				int topic = nwsumIterator.first;
				model.nwsum[topic] += nwsumIterator.second;
				new_sync_data.nwsumDiff[topic] = model.nwsum[topic];
			}

		}
	}


	return resultArray;
}


void TaskExecutor::execute()
{
	using namespace MPIHelper;
	if (WAIT_FOR_ATTACH && config.processID == ROOT)
		cin.ignore();
	Timer t;
	int iteration_i = 0;
	while (iteration_i < config.iterationNumber) {

		if (config.processID == ROOT) {

			t.reset();
		}

		vector<SlaveSyncData> executorSyncCollector;


		for (auto &sampler : samplers) {
			executorSyncCollector.push_back(sampler.sample());

		}
		//debug
		for (auto &str_t : timeRecord) {
			cout << str_t.first << ": " << str_t.second << endl;
		}

		if (config.processID == MPIHelper::ROOT) {
			vector<vector<SlaveSyncData>> iterationSyncCollector(config.totalProcessCount);
			iterationSyncCollector[ROOT] = executorSyncCollector;

			//receive all sync data from slaves
			for (int i = 0; i < config.totalProcessCount; i++) {
				if (i != ROOT) {
					//iterationSyncCollector.push_back(mpiReceive<vector<SynchronisationData>>(i));
					mpiReceive2<vector<SlaveSyncData>>(iterationSyncCollector[i], i);
					
				}
			}

			//iterate all syn data, sum all changes and broadcast to all workers
			vector<vector<SlaveSyncData>>  updateCollection = updateModel(*model, iterationSyncCollector, config);

			for (int pi = 0; pi < config.totalProcessCount; pi++) {
				if (pi != ROOT)
					mpiSend(updateCollection[pi], pi);
			}

			//update
			vector<SlaveSyncData>& update = updateCollection[ROOT];
			for (auto& updateData : update) {
				int task_id = updateData.task_id;
				samplers[task_id].update(updateData);
			}

			//mpiBroadCast(update, ROOT, config.processID);
		}
		else {
			//send slave sync data
			mpiSend(executorSyncCollector, ROOT);

			//update
			vector<SlaveSyncData> update;
			mpiReceive2(update, ROOT);
			for (auto& updateData : update) {
				int task_id = updateData.task_id;
				samplers[task_id].update(updateData);
			}
		}

		//update tasks for this 


		iteration_i++;
		if (config.processID == ROOT) {
			cout << "Iteration " << iteration_i;
			double t2 = t.elapsed();
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
	syncData.pid = config.processID;
	syncData.task_id = task.id;

	double alpha = task.alpha;
	double beta = task.beta;
	int K = task.K;
	double Vbeta = (double)task.V * beta;
	double Kalpha = (double)task.K * alpha;


	for (int wi = 0; wi < task.wordSampling.size(); wi++) {

		//timer.reset(); //reset at each word

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
		//rtime("1"); timer.reset();
		//local copy of task
		//task.nw[w][topic] -= 1;
		//task.nd[m][topic] -= 1;
		//task.nwsum[topic] -= 1;
		//task.ndsum[m] -= 1;
		if (syncData.nwDiff[w].size() == 0) {
			syncData.nwDiff[w] = vector<int>(K, 0);
		}
		if (syncData.ndDiff[m].size() == 0) {
			syncData.ndDiff[m] = vector<int>(K, 0);
		}

		//changes to be sychronized
		syncData.nwDiff[w][topic] -= 1;
		syncData.ndDiff[m][topic] -= 1;
		syncData.nwsumDiff[topic] -= 1;
		//rtime("2"); timer.reset();
		double* p = new double[K];

		vector<int>& nw = task.nw.at(w);
		vector<int>& nwsum = task.nwsum;
		vector<int>& nd = task.nd.at(m);
		map<int, int>& ndsum = task.ndsum;


		for (int k = 0; k < K; k++) {
			//timer.reset();
			double A = nw.at(k) - 1;
			//rtime("2.1"); timer.reset();
			double B = nwsum.at(k) - 1;
			//rtime("2.2"); timer.reset();
			double C = nd.at(k) - 1;
			//rtime("2.3"); timer.reset();
			double D = ndsum.at(m) - 1;
			//rtime("2.4"); timer.reset();
			p[k] = (A + beta) / (B + Vbeta) *
				(C + alpha) / (D + Kalpha);
			//rtime("2.5"); timer.reset();
		}
		//rtime("3"); timer.reset();
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
		//rtime("4"); timer.reset();
		//task.nw[w][topic] += 1;
		//task.nd[m][topic] += 1;
		//task.nwsum[topic] += 1;
		//task.ndsum[m] += 1;

		syncData.nwDiff[w][topic] += 1;
		//rtime("4.1"); timer.reset();
		syncData.ndDiff[m][topic] += 1;
		//rtime("4.2"); timer.reset();
		syncData.nwsumDiff[topic] += 1;
		//rtime("4.3"); timer.reset();
		task.z[wi] = topic;
		//rtime("5"); timer.reset();

	}
	return syncData;
}
