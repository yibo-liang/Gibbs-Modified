#include "task_executor.h"
#include "mpi_helper.h"
#include "shared_header.h"
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


void TaskExecutor::execute2()
{
	using namespace MPIHelper;
	using namespace fastVector2D;

	int ** nwCollection;
	int totalTask = config.processID * config.taskPerProcess;
	if (config.processID == ROOT) {
		size_t s = model->V*model->K;
		nwCollection = new int *[totalTask];
		for (int i = 0; i < totalTask; i++) {
			nwCollection[i] = new int[s];
		}
	}

	int iteration_i = 0;
	Timer t;
	while (iteration_i < config.iterationNumber) {
		if (config.processID == ROOT) {
			t.reset();
		}

		int i = 0;
		for (auto &sampler : samplers) {
			sampler.sample2();
			//after sampling each task, send nw to sync, master only need pointer
			if (config.processID == ROOT) {
				nwCollection[i] = sampler.nw;
				++i;
			}
		}

		//after each sampler task is executed, collect all separate nw

		if (config.processID != ROOT) {
			for (auto &sampler : samplers) {
				MPI_Send(sampler.nw, sampler.V*sampler.K, MPI_INT, ROOT, datatag, MPI_COMM_WORLD);
			};

			for (auto& sampler : samplers) {
				MPI_Bcast(sampler.nw, sampler.V*sampler.K, MPI_INT, ROOT, MPI_COMM_WORLD);
				MPI_Bcast(&(sampler.nwsum[0]), sampler.K, MPI_INT, ROOT, MPI_COMM_WORLD);
			}
		}
		else { //if root
			size_t s = model->V*model->K;
			for (int j = 0; j < config.totalProcessCount; j++) {
				if (j != config.processID) {

					MPI_Recv(nwCollection[i], s, MPI_INT, j, datatag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
					
				}
			}
			// sync model with all sampler's data, using n[w,k] <- n[w,k] + sum (n[p][w,k] - n[w|k])
			// and also calculate new nwsum
			int K = model->K;
			vecFast2D<int> temp_nw = newVec2D<int>(model->V, K);
			for (int k = 0; k < K; k++) {
				int nwsum = 0;
				for (int w = 0; w < model->V; w++) {
					int sum = 0;
					int old_nwk = model->nw[w][k];
					for (int j = 0; j < totalTask; j++) {
						vecFast2D<int> n = nwCollection[j];
						sum += readvec2D(n, w, k, K) - old_nwk;
					}
					model->nw[w][k] += sum;
					writevec2D<int>(model->nw[w][k], temp_nw, w, k, K);
					nwsum += model->nw[w][k];
				}
				model->nwsum[k] = nwsum;
			}
			//send back nw nwsum
			MPI_Bcast(temp_nw, s, MPI_INT, ROOT, MPI_COMM_WORLD);
			MPI_Bcast(&(model->nwsum[0]), K, MPI_INT, ROOT, MPI_COMM_WORLD);
			free(temp_nw);
		}

		iteration_i++;
		if (config.processID == ROOT) {
			cout << "Iteration " << iteration_i;
			double t2 = t.elapsed();
			cout << ", elapsed" << t2 << "" << endl;
		}

	}
	//after all done, sync all model
	if (config.processID != ROOT) {
		int K = samplers[0].K;
		for (auto& sampler : samplers) {
			int s = sampler.partialM;
			int info[3] = { sampler.task_id, s, sampler.documentOffset };
			MPI_Send(&info, 3, MPI_INT, ROOT, datatag, MPI_COMM_WORLD); //SEND partial document count = M 
			MPI_Send(sampler.nd, s * K, MPI_INT, ROOT, datatag, MPI_COMM_WORLD); //SEND nd

		}
	}
	else {
		//receive all sampler result from other process
		
		for (int i = 0; i < config.processID; i++) {
			if (i != ROOT) {
				int info[3];
				MPI_Recv(&info, 3, MPI_INT, i, datatag, MPI_COMM_WORLD, MPI_STATUS_IGNORE); //SEND partial document count = M 
				int task_id = info[0];
				int partialM = info[1];
				int documentOffset = info[2];
				int *tmp = new int[partialM];
				MPI_Recv(tmp, partialM * model->K, MPI_INT, i, datatag, MPI_COMM_WORLD, MPI_STATUS_IGNORE); //SEND nd
				
			}
		}
		//root self
		for (auto& sampler : samplers) {
			int offset = sampler.documentOffset;
			memcpy(&model->nd[offset], sampler.nw, sampler.partialM);
		}

	}

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
			//executorSyncCollector.push_back(sampler.sample());

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
				//samplers[task_id].update(updateData);
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
				//samplers[task_id].update(updateData);
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