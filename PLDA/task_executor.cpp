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

using namespace ParallelHLDA;
inline void debug(int mpi_res) {
	if (mpi_res != MPI_SUCCESS) {
		cout << "Line:" << __LINE__ << ", Bad mpi result = " << mpi_res << endl;
		throw mpi_res;
	}
}

void TaskExecutor::receiveMasterTasks(vector<TaskPartition> & tasks, Model * model)
{
	//used only by ROOT
	this->samplers = vector<Sampler>(tasks.size());
	for (auto &task : tasks) {
		this->samplers[task.partition_id].siblingSize = tasks.size();
		this->samplers[task.partition_id].sampleMode = config.parallelType;
		this->samplers[task.partition_id].fromTask(task);
		if (config.inferencing) {
			this->samplers[task.partition_id].inferModel = this->inferModel;
		}
		//this->samplers[task.id].pid = config.processID;
	}


	this->model = model;

	//cout << "Proc ID = " << this->procNumber << " Received " << tasks.size() << " tasks" << endl;
}

void TaskExecutor::receiveRemoteTasks()
{

	using namespace MPIHelper;
	vector<TaskPartition> tasks;
	mpiReceive2(tasks, ROOT);

	this->samplers = vector<Sampler>(tasks.size());

	for (auto &task : tasks) {
		//cout << "Give task to sampler, PID=" << this->procNumber << ", Task id= " << task.id << endl;
		this->samplers[task.partition_id].siblingSize = tasks.size();
		this->samplers[task.partition_id].sampleMode = config.parallelType;
		this->samplers[task.partition_id].fromTask(task);
		if (config.inferencing) {
			this->samplers[task.partition_id].inferModel = this->inferModel;
		}
		//this->samplers[task.id].pid = config.processID;
	}

	//cout << "Proc ID = " << this->procNumber << " Received " << tasks.size() << " tasks" << endl;
}

using namespace fastVector2D;
inline void importND(Model * model, vecFast2D<int> nd, int partialM, int offset) {
	//cout << "import ND offset=" << offset << ", partialM=" << partialM << endl;
	int K = model->K;
	for (int m = 0; m < partialM; m++) {
		for (int k = 0; k < K; k++) {
			int rm = m + offset;
			model->nd[rm][k] = readvec2D<int>(nd, m, k, K);
		}
	}
}
inline void importNW(Model * model, vecFast2D<int> nw, int partialV, int offset) {
	//cout << "import NW offset=" << offset << ", partialV=" << partialV << endl;
	int K = model->K;
	for (int v = 0; v < partialV; v++) {
		for (int k = 0; k < K; k++) {
			int rv = v + offset;
			int tmp = readvec2D<int>(nw, v, k, K);
			//cout << "v=" << v << ",rv= " << rv << ", k=" << k << ",nw = " << tmp << endl;
			model->nw[rv][k] = tmp;

		}
	}
}

inline void import_Z(Model * model, vector<int> &z, vector<int> &wordSampling, int offsetV, int offsetM) {
	for (int i = 0; i < z.size(); i++) {
		int doc_id = wordSampling[i * 2] + offsetM;
		int w = wordSampling[i * 2 + 1] + offsetV;
		int topic = z[i];
		model->z[doc_id].push_back(topic);
		model->w[doc_id].push_back(w);
	}
}

void TaskExecutor::executePartition()
{
	int offset = config.processID;
	using namespace MPIHelper;
	/*
	each iteration, we will iterate each nw partition,
	and sync this with the process that uses it in the next iteration
	*/
	int totalProcessCount = config.totalProcessCount;
	int receiver_pid = (offset + totalProcessCount - 1) % totalProcessCount;
	int sender_pid = (offset + 1) % totalProcessCount;

	Timer timer;
	for (int iter_n = 0; iter_n < config.iterationNumber; iter_n++) {
		timer.reset();
		for (int iter_partition = 0; iter_partition < samplers.size(); iter_partition++) {

			int partiton_i = (iter_partition + offset) % totalProcessCount;
			Sampler& sampler = samplers[partiton_i];

			if (config.inferencing) {
				sampler.inference();
			}
			else {
				sampler.sample();
			}



			//after sampling /
			if (config.totalProcessCount > 1) {
				int next_partition_i = (partiton_i + 1) % totalProcessCount;

				Sampler& nextSampler = samplers[next_partition_i];
				int current_nw_size = sampler.partialV * sampler.K;
				int next_nw_size = nextSampler.partialV * nextSampler.K;

				memcpy(&nextSampler.nd[0], &sampler.nd[0], sampler.partialM*sampler.K);
				MPI_Sendrecv(
					&sampler.nw[0], current_nw_size, MPI_INT, receiver_pid, datatag,
					&nextSampler.nw[0], next_nw_size, MPI_INT, sender_pid, datatag,
					MPI_COMM_WORLD, MPI_STATUSES_IGNORE);

				MPI_Allreduce(MPI_IN_PLACE, &sampler.nwsumDiff[0], sampler.K, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

				for (int i = 0; i < nextSampler.K; i++) {
					nextSampler.nwsum[i] += sampler.nwsumDiff[i];
					for (auto& s : samplers) {
						s.nwsum[i] = nextSampler.nwsum[i];
					}
					sampler.nwsumDiff[i] = 0;
				}


				if (config.parallelType == P_GPU) {
					sampler.syncToDevice();
				}
			}
			else {
				if (config.parallelType == P_GPU)
					sampler.syncFinish();
			}
		}

		if (offset == 0)
			cout << "\rIteration " << iter_n << ", elapsed " << timer.elapsed() << std::flush;

	}

	if (config.parallelType == P_GPU) {
		for (auto & sampler : samplers) {
			sampler.syncFinish();
			sampler.syncFromDevice();
			sampler.release_GPU();
		}
	}
	//cout << endl;
}

void TaskExecutor::execMaster()
{
	using namespace MPIHelper;
	int totalTask = config.totalProcessCount * config.taskPerProcess;
	size_t nwSize = model->V*model->K;


	//receive all sampler result from other process
	for (int m = 0; m < model->M; m++) {
		model->w.at(m).clear();
		model->z.at(m).clear();
	}
	for (int i = 0; i < config.totalProcessCount; i++) {
		if (i != ROOT) {
			for (int j = 0; j < config.taskPerProcess; j++) {
				int info[5];
				//cout << "Master receiving partition from pid=" << i << endl;
				MPI_Recv(&info, 5, MPI_INT, i, datatag, MPI_COMM_WORLD, MPI_STATUS_IGNORE); //SEND partial document count = M 
				int task_id = info[0];
				int offsetM = info[1];
				int partialM = info[2];
				int offsetV = info[3];
				int partialV = info[4];
/*
				cout << "---- import partial info: i=" << i << ", j=" << j << endl;
				cout << "offset m = " << offsetM << ", offset v = " << offsetV << endl;
				cout << "partial M = " << partialM << ", partial V = " << partialV << endl;*/
				vecFast2D<int> recevied_nd = newVec2D<int>(partialM, model->K);

				vecFast2D<int> recevied_nw = newVec2D<int>(partialV, model->K);

				MPI_Recv(recevied_nw, partialV * model->K, MPI_INT, i, datatag, MPI_COMM_WORLD, MPI_STATUS_IGNORE); //nd from partition

				MPI_Recv(recevied_nd, partialM * model->K, MPI_INT, i, datatag, MPI_COMM_WORLD, MPI_STATUS_IGNORE); //nd from partition

				importND(model, recevied_nd, partialM, offsetM);

				importNW(model, recevied_nw, partialV, offsetV);


				int word_count;
				MPI_Recv(&word_count, 1, MPI_INT, i, datatag, MPI_COMM_WORLD, MPI_STATUS_IGNORE); //word count from the partition

				vector<int> received_z(word_count);
				vector<int> received_ws(word_count * 2);
				MPI_Recv(&received_z[0], word_count, MPI_INT, i, datatag, MPI_COMM_WORLD, MPI_STATUS_IGNORE); //word count from the partition
				MPI_Recv(&received_ws[0], word_count * 2, MPI_INT, i, datatag, MPI_COMM_WORLD, MPI_STATUS_IGNORE); //word count from the partition

				import_Z(model, received_z, received_ws, offsetV, offsetM);

				delete[](recevied_nd);

				delete[](recevied_nw);

			}
		}
	}
	//root self
	for (auto& sampler : samplers) {
		importND(model, &sampler.nd[0], sampler.partialM, sampler.offsetM);
		importNW(model, &sampler.nw[0], sampler.partialV, sampler.offsetV);
		import_Z(model, sampler.z, sampler.wordSampling, sampler.offsetV, sampler.offsetM);

	}
	model->update();
}

void TaskExecutor::execSlave()
{
	using namespace MPIHelper;

	int K = samplers[0].K;
	for (int i = 0; i < samplers.size(); i++) {
		auto& sampler = samplers[i];
		//cout << "sending partial nd pid=" << config.processID << ", sampler i=" << i << endl;
		int partialM = sampler.partialM;
		int partialV = sampler.partialV;
		int offsetM = sampler.offsetM;
		int offsetV = sampler.offsetV;

		int info[5] = { sampler.partition_id, offsetM, partialM, offsetV, partialV };
		MPI_Send(&info, 5, MPI_INT, ROOT, datatag, MPI_COMM_WORLD); //SEND partial document count = M 

		MPI_Send(&sampler.nw[0], partialV * K, MPI_INT, ROOT, datatag, MPI_COMM_WORLD); //SEND nd

		MPI_Send(&sampler.nd[0], partialM * K, MPI_INT, ROOT, datatag, MPI_COMM_WORLD); //SEND nd

		MPI_Send(&sampler.wordInsNum, 1, MPI_INT, ROOT, datatag, MPI_COMM_WORLD); //SEND nd
		MPI_Send(&sampler.z[0], sampler.z.size(), MPI_INT, ROOT, datatag, MPI_COMM_WORLD); //SEND nd
		MPI_Send(&sampler.wordSampling[0], sampler.wordSampling.size(), MPI_INT, ROOT, datatag, MPI_COMM_WORLD); //SEND nd


		//cout << "Partial sent, pid=" << config.processID << endl;

	}
}




void TaskExecutor::execute()
{
	//MPI_Barrier(MPI_COMM_WORLD);

	//cout << "Word load PID=" << this->procNumber;
	//for (auto& s : samplers) {
	//	cout << "\t" << s.wordInsNum;
	//}
	//cout << endl;

	executePartition();
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
	this->MODE = config.parallelType;
	if (config.processID == MPIHelper::ROOT) isMaster = true;

}

TaskExecutor::~TaskExecutor()
{
}
