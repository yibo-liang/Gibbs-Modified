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

inline void debug(int mpi_res) {
	if (mpi_res != MPI_SUCCESS) {
		cout << "Bad mpi result = " << mpi_res << endl;
		throw mpi_res;
	}
}

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
		cout << "Give task to sampler, PID=" << this->procNumber << ", Task id= " << task.id << endl;
		this->samplers[task.id].fromTask(task);
		this->samplers[task.id].pid = config.processID;
	}

	cout << "Proc ID = " << this->procNumber << " Received " << tasks.size() << " tasks" << endl;
}

using namespace fastVector2D;
inline void importND(Model * model, vecFast2D<int> nd, int partialM, int offset) {
	cout << "import nd offset=" << offset << ", partialM=" << partialM << endl;
	int K = model->K;
	for (int m = 0; m < partialM; m++) {
		for (int k = 0; k < K; k++) {
			int rm = m + offset;
			model->nd[rm][k] = readvec2D<int>(nd, m, k, K);
		}
	}
}

void TaskExecutor::execMaster()
{
	using namespace MPIHelper;
	int totalTask = config.totalProcessCount * config.taskPerProcess;
	size_t nwSize = model->V*model->K;
	//int ** nwCollection;
	//nwCollection = new int *[totalTask];
	vector<vecFast2D<int>> nwCollection(totalTask);

	vecFast2D<int> temp_nw = newVec2D<int>(model->V, model->K);

	for (int i = samplers.size(); i < totalTask; i++) {
		nwCollection[i] = new int[nwSize];
	}

	Timer t;
	int iteration_i = 0;
	while (iteration_i < config.iterationNumber) {
		t.reset();
		int i = 0;
		for (auto &sampler : samplers) {
			sampler.sample();
			//after sampling each task, send nw to sync, master only need pointer
			nwCollection[i] = sampler.nw;
			++i;
		}

		size_t s = model->V*model->K;
		for (int j = 0; j < config.totalProcessCount; j++) {
			if (j != config.processID) {
				for (int it = 0; it < config.taskPerProcess; it++) {
					MPI_Recv(nwCollection[i], s, MPI_INT, j, datatag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
					++i;
				}
			}
		}
		// sync model with all sampler's data, using n[w,k] <- n[w,k] + sum (n[p][w,k] - n[w|k])
		// and also calculate new nwsum
		int K = model->K;
		int nwsumAll = 0;
		for (int k = 0; k < K; k++) {
			int nwsum = 0;
			for (int w = 0; w < model->V; w++) {
				int sum = 0;
				int old_nwk = model->nw[w][k];
				for (int j = 0; j < totalTask; j++) {
					vecFast2D<int> nw_j = nwCollection[j];
					sum += readvec2D(nw_j, w, k, K) - old_nwk;
				}
				model->nw[w][k] = old_nwk + sum;
				if (model->nw[w][k] < 0) {
					cout << "nw < 0, w=" << w << ", k=" << k << ", sum=" << sum << ", nw=" << model->nw[w][k] << ", old nw=" << old_nwk << endl;
					cout << "nwsum[k]=" << model->nwsum[k] << endl;
					for (int j = 0; j < totalTask; j++) {
						vecFast2D<int> nw_j = nwCollection[j];
						cout << "j=" << j << ", nw_j[w][k] = " << readvec2D(nw_j, w, k, K) <<  endl;
					}
					cout << endl;
					throw 20;
				}
				writevec2D<int>(model->nw[w][k], temp_nw, w, k, K);
				nwsum += model->nw[w][k];
			}
			model->nwsum[k] = nwsum;
			nwsumAll += nwsum;
		}
		//send back nw nwsum
		MPI_Bcast(temp_nw, s, MPI_INT, ROOT, MPI_COMM_WORLD);
		MPI_Bcast(&(model->nwsum[0]), K, MPI_INT, ROOT, MPI_COMM_WORLD);


		for (int si = 0; si < samplers.size(); si++) {
			auto& sampler = samplers[si];
			memcpy(sampler.nw, temp_nw, s * sizeof(int)); //direct memory copy for other sampler's nw
			//for (int it = 0; it < nwSize; it++) {
			//	sampler.nw[it] = temp_nw[it];
			//}
			sampler.nwsum = model->nwsum;
		}

		cout << "Iteration " << iteration_i;
		double t2 = t.elapsed();
		cout << ", elapsed" << t2 << ", nwsum all=" << nwsumAll << endl;

		iteration_i++;
	}
	free(temp_nw);
	//receive all sampler result from other process

	for (int i = 0; i < config.totalProcessCount; i++) {
		if (i != ROOT) {
			for (int j = 0; j < config.taskPerProcess; j++) {
				int info[3];
				cout << "master receiving partial nd from pid=" << i;
				MPI_Recv(&info, 3, MPI_INT, i, datatag, MPI_COMM_WORLD, MPI_STATUS_IGNORE); //SEND partial document count = M 
				int task_id = info[0];
				cout << "task i=" << task_id << endl;
				int partialM = info[1];
				int documentOffset = info[2];
				vecFast2D<int> tmp = newVec2D<int>(partialM, model->K);
				MPI_Recv(tmp, partialM * model->K, MPI_INT, i, datatag, MPI_COMM_WORLD, MPI_STATUS_IGNORE); //SEND nd
				importND(model, tmp, partialM, documentOffset);
				free(tmp);
			}
		}
	}
	//root self
	for (auto& sampler : samplers) {
		int offset = sampler.documentOffset;
		importND(model, sampler.nw, sampler.partialM, sampler.documentOffset);
	}

	for (int i = samplers.size(); i < totalTask; i++) {
		delete[] nwCollection[i];
	}
}

void TaskExecutor::execSlave()
{
	using namespace MPIHelper;
	int iteration_i = 0;
	auto& s0 = samplers[0];
	int nwSize = s0.V*s0.K;

	while (iteration_i < config.iterationNumber) {
		for (auto &sampler : samplers) {
			sampler.sample();
		}

		for (auto &sampler : samplers) {
			debug(MPI_Send(sampler.nw, nwSize, MPI_INT, ROOT, datatag, MPI_COMM_WORLD));
		};

		debug(MPI_Bcast(s0.nw, nwSize, MPI_INT, ROOT, MPI_COMM_WORLD));
		debug(MPI_Bcast(&(s0.nwsum[0]), s0.K, MPI_INT, ROOT, MPI_COMM_WORLD));

		for (auto& sampler : samplers) {
			if (sampler.task_id != s0.task_id) {

				memcpy(sampler.nw, s0.nw, nwSize * sizeof(int)); //direct memory copy for other sampler's nw
				sampler.nwsum = s0.nwsum;
			}
		}
		iteration_i++;

	}


	int K = samplers[0].K;
	for (int i = 0; i < samplers.size(); i++) {
		auto& sampler = samplers[i];
		cout << "sending partial nd pid=" << config.processID << ", sampler i=" << i << endl;
		int s = sampler.partialM;
		int info[3] = { sampler.task_id, s, sampler.documentOffset };
		MPI_Send(&info, 3, MPI_INT, ROOT, datatag, MPI_COMM_WORLD); //SEND partial document count = M 
		MPI_Send(sampler.nd, s * K, MPI_INT, ROOT, datatag, MPI_COMM_WORLD); //SEND nd
		cout << "Partial sent, pid="<< config.processID << endl;

	}
}


void TaskExecutor::execute()
{
	if (config.processID == MPIHelper::ROOT) {
		execMaster();
	}
	else {
		execSlave();
	}
}

TaskExecutor::TaskExecutor(JobConfig config)
{
	this->config = config;
	this->procNumber = config.processID;
	if (config.processID == MPIHelper::ROOT) isMaster = true;

}

TaskExecutor::~TaskExecutor()
{
}
